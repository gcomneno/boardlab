import assert from "node:assert/strict";
import test from "node:test";

import {
  CUP_STORAGE_KEY,
  createCupDocument,
  exportCupJson,
  importCupJson,
  saveCup,
  loadCup,
} from "./cup-store.mjs";

const participants = ["A", "B", "C", "D"].map(id => ({
  provider: "test",
  providerTeamId: id,
  name: `Team ${id}`,
}));

function memoryStorage() {
  const values = new Map();
  return {
    getItem: key => values.get(key) ?? null,
    setItem: (key, value) => values.set(key, value),
    values,
  };
}

function sampleCup(legs = 1) {
  return createCupDocument({
    cupId: "cup-store-test",
    name: "Test Cup",
    participants,
    legs,
  });
}

test("persists and restores the canonical cup document", () => {
  const storage = memoryStorage();
  const document = sampleCup(2);

  saveCup(storage, document);
  const restored = loadCup(storage);

  assert.deepEqual(restored, document);
  assert.equal(restored.legs, 2);
  assert.equal(restored.fixtures.length, 4);
  assert.ok(storage.values.has(CUP_STORAGE_KEY));
});

test("round-trips exported cup JSON", () => {
  const document = sampleCup();
  const restored = importCupJson(exportCupJson(document));
  assert.deepEqual(restored, document);
});

test("rejects unsupported schema and malformed JSON", () => {
  const document = sampleCup();
  assert.throws(() => importCupJson("{invalid"), /invalid cup JSON/);
  assert.throws(
    () => importCupJson(JSON.stringify({
      ...document,
      schemaVersion: 999,
    })),
    /unsupported cup schema/,
  );
});

test("rejects non-canonical first-round fixtures", () => {
  const storage = memoryStorage();
  const corrupted = structuredClone(sampleCup());
  corrupted.fixtures[0].homeProviderTeamId = "CORRUPTED";

  storage.setItem(CUP_STORAGE_KEY, JSON.stringify(corrupted));

  assert.throws(
    () => loadCup(storage),
    /non-canonical first cup round/,
  );
});

test("does not overwrite unrelated storage entries", () => {
  const storage = memoryStorage();
  storage.setItem("other-data", "preserve");

  saveCup(storage, sampleCup());

  assert.equal(storage.getItem("other-data"), "preserve");
});

// M009 ARCHIVE TESTS
import {
  archiveCompletedCup,
  loadCupArchive,
  CUP_ARCHIVE_STORAGE_KEY,
} from "./cup-store.mjs";
import {
  completeCupFixture,
  advanceCupIfReady,
} from "./cup.mjs";

function completedCupForArchive() {
  let cup = sampleCup();
  const matches = [];

  for (let round = 1; round <= 2; round += 1) {
    const fixtures = cup.fixtures.filter(f => f.round === round);
    for (const fixture of fixtures) {
      const matchId = `archive-match-${fixture.fixtureId}`;
      matches.push({
        matchId,
        home: { providerTeamId: fixture.homeProviderTeamId },
        away: { providerTeamId: fixture.awayProviderTeamId },
        score: { home: 2, away: 0 },
      });
      cup = completeCupFixture(cup, fixture.fixtureId, matchId);
    }
    const result = advanceCupIfReady(cup, matches, {});
    assert.equal(result.status, "advanced");
    cup = result.cup;
  }

  assert.ok(cup.championProviderTeamId);
  return { cup, matches };
}

test("M009 archives a completed cup and preserves its results", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();

  const archive = archiveCompletedCup(storage, cup, matches);

  assert.equal(archive.length, 1);
  assert.deepEqual(loadCupArchive(storage, matches), [cup]);
  assert.ok(storage.values.has(CUP_ARCHIVE_STORAGE_KEY));
});

test("M009 refuses incomplete cups and duplicate archives", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();

  assert.throws(
    () => archiveCompletedCup(storage, sampleCup(), matches),
    /cup is not completed/,
  );

  archiveCompletedCup(storage, cup, matches);

  assert.throws(
    () => archiveCompletedCup(storage, cup, matches),
    /cup already archived/,
  );

  assert.equal(loadCupArchive(storage, matches).length, 1);
});

test("M009 rejects malformed archive without modifying storage", () => {
  const storage = memoryStorage();
  const raw = '{"invalid":"archive"}';
  storage.setItem(CUP_ARCHIVE_STORAGE_KEY, raw);

  assert.throws(
    () => loadCupArchive(storage, []),
    /invalid cup archive/,
  );
  assert.equal(storage.getItem(CUP_ARCHIVE_STORAGE_KEY), raw);
});

test("M009 archive does not overwrite active cup or unrelated data", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();
  const active = sampleCup();

  saveCup(storage, active);
  storage.setItem("other-data", "preserve");

  archiveCompletedCup(storage, cup, matches);

  assert.deepEqual(loadCup(storage), active);
  assert.equal(storage.getItem("other-data"), "preserve");
});

import {
  createCupShootoutCoordinator,
  verifyCupDocument,
  importVerifiedCupJson,
  saveVerifiedCup,
  loadVerifiedCup,
  recordCupShootout,
  saveCupRecovery,
  loadCupRecovery,
} from "./cup-store.mjs";
import { replayCupShootout } from "./cup-shootout.mjs";

function shootoutStorage() {
  const storage = memoryStorage();
  return {
    values: storage.values,
    writes: [],
    failRead: false,
    failWrite: false,
    throwAfterWrite: false,
    getItem(key) {
      if (this.failRead) throw new Error("read denied");
      return storage.getItem(key);
    },
    setItem(key, value) {
      if (this.failWrite) throw new Error("quota exceeded");
      storage.setItem(key, value);
      this.writes.push({ key, value });
      if (this.throwAfterWrite) throw new Error("interrupted after write");
    },
  };
}

function shootoutFixture({ legs = 1, twoTies = false } = {}) {
  let cup = sampleCup(legs);
  cup.metadata = { preserve: ["unknown", "Cup v1"] };
  const matches = [];
  for (const fixture of cup.fixtures) {
    const matchId = `shootout-${fixture.fixtureId}`;
    matches.push({
      matchId,
      home: { providerTeamId: fixture.homeProviderTeamId },
      away: { providerTeamId: fixture.awayProviderTeamId },
      score: fixture.pair === 1 || twoTies ?
        { home: legs === 2 ? 2 : 1, away: legs === 2 ? 0 : 1 } :
        { home: fixture.leg === 1 ? 1 : 0, away: 0 },
      events: [{ type: "regular-goal" }], rollEvents: [{ face: "GOAL!" }],
      statistics: { penalties: 0 }, goalscorers: ["regular-scorer"],
    });
    cup = completeCupFixture(cup, fixture.fixtureId, matchId);
  }
  return { cup, matches };
}

function coordinator(storage, matches, options = { ownsLock: () => true }) {
  return createCupShootoutCoordinator(storage, matches, options);
}

function openShootout(options) {
  const { cup, matches } = shootoutFixture(options);
  const storage = shootoutStorage();
  saveVerifiedCup(storage, cup, matches);
  const editor = coordinator(storage, matches);
  const started = editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 });
  return { cup: started, matches, storage, editor };
}

function shotCommand(cup, key = "r1-p1") {
  const session = cup.shootoutSessions[key];
  const expectedRevision = session.attempts.length;
  return {
    cupId: cup.cupId, shootoutId: session.shootoutId,
    attemptId: JSON.stringify([cup.cupId, key, expectedRevision + 1]),
    expectedRevision,
  };
}

function shoot(editor, cup, goal, key) {
  return editor.confirmCupShootoutAttempt(shotCommand(cup, key), { random: () => goal ? 0 : 0.99 }).cup;
}

test("SP-01 start persists an empty session once; manual entry then locks", () => {
  const { cup, matches, storage, editor } = openShootout();
  assert.equal(cup.schemaVersion, 1);
  assert.equal(cup.shootouts, undefined);
  assert.equal(cup.shootoutSessions["r1-p1"].attempts.length, 0);
  const writes = storage.writes.length;
  assert.deepEqual(editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 }), cup);
  assert.equal(storage.writes.length, writes);
  assert.throws(() => editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 2 }));
  assert.throws(() => recordCupShootout(cup, matches, 1, 1, { home: 4, away: 3 }),
    { code: "CUP_SHOOTOUT_MANUAL_LOCKED" });
  assert.deepEqual(importVerifiedCupJson(exportCupJson(cup), matches), cup);
});

test("SP-01 each attempt commits once and reload/duplicate commands never reroll", () => {
  for (const legs of [1, 2]) {
    const context = openShootout({ legs });
    const { storage, matches } = context;
    let cup = context.cup;
    const untouched = structuredClone(matches);
    const journal = { cupId: cup.cupId, fixtureId: "r1-p1-l1", matchId: "existing-journal" };
    saveCupRecovery(storage, journal);
    storage.setItem("unrelated", "preserve");
    const keys = [...storage.values.keys()];
    let draws = 0;
    for (const goal of [...Array(10).fill(true), true, true, false, false, true, false]) {
      const editor = coordinator(storage, matches);
      const command = shotCommand(cup);
      const writes = storage.writes.length;
      const result = editor.confirmCupShootoutAttempt(command, { random: () => {
        draws += 1;
        return goal ? 0.1 : 0.9;
      } });
      assert.equal(result.disposition, "committed");
      assert.equal(storage.writes.length, writes + 1);
      assert.equal(storage.writes.at(-1).key, CUP_STORAGE_KEY);
      cup = loadVerifiedCup(storage, matches);
      assert.deepEqual(cup, result.cup);
      assert.equal(cup.shootoutSessions["r1-p1"].attempts.length, draws);
      const duplicate = coordinator(storage, matches).confirmCupShootoutAttempt(command, {
        random: () => { throw new Error("duplicate rerolled"); },
      });
      assert.equal(duplicate.disposition, "already-committed");
      assert.deepEqual(duplicate.attempt, result.attempt);
      assert.equal(storage.writes.length, writes + 1);
      if (draws === 11 || draws === 15) {
        assert.equal(replayCupShootout(cup.shootoutSessions["r1-p1"]).winner, null);
        assert.equal(cup.shootouts, undefined);
      }
    }
    assert.equal(draws, 16);
    assert.deepEqual(cup.shootouts, { "r1-p1": { home: 7, away: 6 } });
    assert.equal(cup.fixtures.filter(f => f.round === 2).length, legs);
    assert.equal(cup.fixtures.find(f => f.round === 2).homeProviderTeamId, "A");
    assert.deepEqual(matches, untouched);
    assert.deepEqual(loadCupRecovery(storage), journal);
    assert.deepEqual([...storage.values.keys()], keys);
    assert.equal(storage.getItem("unrelated"), "preserve");
    assert.deepEqual(cup.metadata, { preserve: ["unknown", "Cup v1"] });
    assert.deepEqual(importCupJson(exportCupJson(cup)), cup);
  }
});

test("SP-01 canonical Extended v1 slots and boundaries consume one draw each", () => {
  const cases = [
    [0, "GOAL!", "penalty-goal"],
    [1 / 12, "GOAL!", "penalty-goal"],
    [3 / 12, "GOAL!", "penalty-goal"],
    [5 / 12, "GOAL!", "penalty-goal"],
    [0.5 - Number.EPSILON, "GOAL!", "penalty-goal"],
    [0.5, "PALO/TRAVERSA", "penalty-woodwork"],
    [7 / 12, "PALO/TRAVERSA", "penalty-woodwork"],
    [4 / 6, "PARATO/RESPINTO", "penalty-saved"],
    [9 / 12, "PARATO/RESPINTO", "penalty-saved"],
    [5 / 6, "FUORI", "penalty-wide"],
    [11 / 12, "FUORI", "penalty-wide"],
    [1 - Number.EPSILON, "FUORI", "penalty-wide"],
  ];
  for (const [value, face, kind] of cases) {
    const { cup, editor } = openShootout();
    let draws = 0;
    const result = editor.confirmCupShootoutAttempt(shotCommand(cup), { random: () => {
      draws += 1;
      return value;
    } });
    assert.equal(draws, 1);
    assert.equal(result.attempt.face, face);
    assert.equal(result.attempt.kind, kind);
  }
});

test("SP-01 invalid randomness appends nothing and never draws a replacement", () => {
  for (const value of [-1, 1, NaN, Infinity, -Infinity, "0.1", undefined]) {
    const { cup, editor, storage } = openShootout();
    const bytes = storage.getItem(CUP_STORAGE_KEY);
    let draws = 0;
    assert.throws(() => editor.confirmCupShootoutAttempt(shotCommand(cup), { random: () => {
      draws += 1;
      return value;
    } }), { code: "CUP_SHOOTOUT_DRAW_INVALID" });
    assert.equal(draws, 1);
    assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
  }
});

test("SP-01 rejects stale/conflicting commands before drawing", () => {
  const { cup, editor, storage } = openShootout();
  const command = shotCommand(cup);
  const bytes = storage.getItem(CUP_STORAGE_KEY);
  for (const change of [
    { cupId: "other" }, { shootoutId: "other" }, { attemptId: "other" },
    { expectedRevision: -1 }, { expectedRevision: 1 },
    { expectedRevision: 0.5 }, { expectedRevision: Number.MAX_SAFE_INTEGER },
  ]) {
    assert.throws(() => editor.confirmCupShootoutAttempt({ ...command, ...change }, {
      random: () => { throw new Error("invalid command drew"); },
    }), { code: "CUP_SHOOTOUT_STALE" });
  }
  assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
});

test("SP-01 missing ownership fails closed; ownership can transfer without drawing", () => {
  const { cup, matches, storage } = openShootout();
  assert.throws(() => coordinator(storage, matches, {}).confirmCupShootoutAttempt(shotCommand(cup)),
    { code: "CUP_SHOOTOUT_LOCK_UNAVAILABLE" });
  let owner = false;
  const editor = coordinator(storage, matches, { ownsLock: () => owner });
  assert.throws(() => editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 }),
    { code: "CUP_SHOOTOUT_OTHER_TAB" });
  owner = true;
  const resumed = editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 });
  assert.deepEqual(resumed, cup);
});

test("SP-01 start/write/read failures retain stored bytes and draw nothing before resolution", () => {
  const { cup, matches } = shootoutFixture();
  const storage = shootoutStorage();
  saveVerifiedCup(storage, cup, matches);
  const bytes = storage.getItem(CUP_STORAGE_KEY);
  const editor = coordinator(storage, matches);
  storage.failWrite = true;
  assert.throws(() => editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 }),
    { code: "CUP_SHOOTOUT_SAVE_FAILED" });
  assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
  storage.failWrite = false;
  const started = editor.startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 });
  storage.failRead = true;
  assert.throws(() => editor.confirmCupShootoutAttempt(shotCommand(started), {
    random: () => { throw new Error("read failure drew"); },
  }), { code: "CUP_SHOOTOUT_STORAGE_UNAVAILABLE" });
});

test("SP-01 failed nonterminal and terminal writes retry the prepared shot without reroll", () => {
  for (const terminal of [false, true]) {
    const context = openShootout();
    const { editor, storage, matches } = context;
    let cup = context.cup;
    if (terminal) for (const goal of [1, 0, 1, 0, 1]) cup = shoot(editor, cup, goal);
    const bytes = storage.getItem(CUP_STORAGE_KEY);
    const command = shotCommand(cup);
    const writes = storage.writes.length;
    let draws = 0;
    storage.failWrite = true;
    assert.throws(() => editor.confirmCupShootoutAttempt(command, { random: () => {
      draws += 1;
      return 0.99;
    } }), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
    assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
    assert.deepEqual(loadVerifiedCup(storage, matches), cup);
    assert.throws(() => editor.confirmCupShootoutAttempt(command, {
      random: () => { draws += 1; return 0; },
    }), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
    assert.throws(() => editor.retryCupShootoutSave(), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
    storage.failWrite = false;
    const result = editor.retryCupShootoutSave();
    assert.equal(draws, 1);
    assert.equal(storage.writes.length, writes + 1);
    assert.equal(result.attempt.face, "FUORI");
    assert.deepEqual(loadVerifiedCup(storage, matches), result.cup);
    assert.equal(result.cup.shootoutSessions["r1-p1"].status, terminal ? "complete" : "initial");
    if (terminal) {
      const committed = JSON.parse(storage.writes.at(-1).value);
      assert.deepEqual(committed.shootouts, { "r1-p1": { home: 3, away: 0 } });
      assert.equal(committed.fixtures.filter(f => f.round === 2).length, 1);
    }
    assert.throws(() => editor.retryCupShootoutSave(), { code: "CUP_SHOOTOUT_STALE" });
  }
});

test("SP-01 retry detects an already written candidate, including terminal advancement", () => {
  const { editor, storage, matches, cup: started } = openShootout();
  let cup = started;
  for (const goal of [1, 0, 1, 0, 1]) cup = shoot(editor, cup, goal);
  const command = shotCommand(cup);
  let draws = 0;
  storage.throwAfterWrite = true;
  assert.throws(() => editor.confirmCupShootoutAttempt(command, { random: () => {
    draws += 1;
    return 0.99;
  } }), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
  const writes = storage.writes.length;
  storage.throwAfterWrite = false;
  const result = editor.retryCupShootoutSave();
  assert.equal(result.disposition, "already-committed");
  assert.equal(draws, 1);
  assert.equal(storage.writes.length, writes);
  assert.equal(result.cup.fixtures.length, 3);
  assert.deepEqual(loadVerifiedCup(storage, matches), result.cup);
});

test("SP-01 retry refuses to overwrite a changed predecessor", () => {
  const { editor, storage, matches, cup } = openShootout();
  storage.failWrite = true;
  assert.throws(() => shoot(editor, cup, true), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
  storage.failWrite = false;
  const newer = { ...cup, name: "Changed by another writer" };
  saveVerifiedCup(storage, newer, matches);
  const bytes = storage.getItem(CUP_STORAGE_KEY);
  assert.throws(() => editor.retryCupShootoutSave(), { code: "CUP_SHOOTOUT_STALE" });
  assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
});

test("SP-01 reload before failed write resumes only the previous committed revision", () => {
  const { editor, storage, matches, cup } = openShootout();
  storage.failWrite = true;
  assert.throws(() => shoot(editor, cup, true));
  storage.failWrite = false;
  const restored = loadVerifiedCup(storage, matches);
  assert.equal(restored.shootoutSessions["r1-p1"].attempts.length, 0);
  const committed = shoot(coordinator(storage, matches), restored, false);
  assert.equal(committed.shootoutSessions["r1-p1"].attempts[0].face, "FUORI");
});

test("SP-01 reentrant confirmation cannot produce an extra draw", () => {
  const { editor, cup } = openShootout();
  let draws = 0;
  const command = shotCommand(cup);
  editor.confirmCupShootoutAttempt(command, { random: () => {
    draws += 1;
    assert.throws(() => editor.confirmCupShootoutAttempt(command), { code: "CUP_SHOOTOUT_STALE" });
    return 0;
  } });
  assert.equal(draws, 1);
});

test("SP-01 two tied pairs and final admit each winner and champion exactly once", () => {
  const { storage, editor, matches, cup: started } = openShootout({ twoTies: true });
  let cup = started;
  for (const pair of [1, 2]) {
    if (pair === 2) cup = editor.startCupShootout({ cupId: cup.cupId, round: 1, pair });
    for (const goal of [1, 0, 1, 0, 1]) cup = shoot(editor, cup, goal, `r1-p${pair}`);
    const command = shotCommand(cup, `r1-p${pair}`);
    const terminalCommand = { ...command, expectedRevision: 5,
      attemptId: JSON.stringify([cup.cupId, `r1-p${pair}`, 6]) };
    cup = editor.confirmCupShootoutAttempt(terminalCommand, { random: () => 0.99 }).cup;
    const bytes = storage.getItem(CUP_STORAGE_KEY);
    for (let i = 0; i < 3; i += 1) {
      const result = coordinator(storage, matches).confirmCupShootoutAttempt(terminalCommand, {
        random: () => { throw new Error("terminal replay drew"); },
      });
      assert.equal(result.disposition, "already-committed");
    }
    assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
    assert.equal(cup.fixtures.length, pair === 1 ? 2 : 3);
  }
  const final = cup.fixtures.at(-1);
  matches.push({ matchId: "final-match", home: { providerTeamId: final.homeProviderTeamId },
    away: { providerTeamId: final.awayProviderTeamId }, score: { home: 0, away: 0 } });
  cup = completeCupFixture(cup, final.fixtureId, "final-match");
  saveVerifiedCup(storage, cup, matches);
  cup = editor.startCupShootout({ cupId: cup.cupId, round: 2, pair: 1 });
  for (const goal of [0, 1, 0, 1, 0]) cup = shoot(editor, cup, goal, "r2-p1");
  const command = shotCommand(cup, "r2-p1");
  cup = editor.confirmCupShootoutAttempt(command, { random: () => 0 }).cup;
  assert.equal(cup.championProviderTeamId, "C");
  assert.equal(Object.keys(cup.shootouts).length, 3);
  const writes = storage.writes.length;
  assert.equal(editor.confirmCupShootoutAttempt(command).disposition, "already-committed");
  assert.equal(storage.writes.length, writes);
  assert.throws(() => editor.confirmCupShootoutAttempt(shotCommand(cup, "r2-p1")),
    { code: "CUP_SHOOTOUT_DECIDED" });
  archiveCompletedCup(storage, cup, matches);
  assert.deepEqual(loadCupArchive(storage, matches), [cup]);
  assert.throws(() => archiveCompletedCup(storage, cup, matches), /already archived/);
});

test("SP-01 legacy manual totals remain authoritative without invented history", () => {
  const { cup, matches } = shootoutFixture();
  assert.deepEqual(importVerifiedCupJson(exportCupJson(cup), matches), cup);
  const manual = recordCupShootout(cup, matches, 1, 1,
    { home: Number.MAX_SAFE_INTEGER, away: Number.MAX_SAFE_INTEGER - 1 });
  assert.equal(manual.shootoutSessions, undefined);
  assert.deepEqual(verifyCupDocument(manual, matches), manual);
  assert.deepEqual(importVerifiedCupJson(exportCupJson(manual), matches), manual);
  assert.deepEqual(verifyCupDocument(manual, matches, { ...manual.shootouts }), manual);
  assert.throws(() => verifyCupDocument(manual, matches, {}));
  assert.throws(() => importVerifiedCupJson(exportCupJson(manual), matches,
    { "r1-p1": { home: 0, away: 1 } }));
  assert.throws(() => recordCupShootout(manual, matches, 1, 1, { home: 4, away: 3 }));
  const storage = shootoutStorage();
  saveVerifiedCup(storage, manual, matches);
  assert.throws(() => coordinator(storage, matches).startCupShootout({ cupId: cup.cupId, round: 1, pair: 1 }),
    { code: "CUP_SHOOTOUT_DECIDED" });
  const advanced = advanceCupIfReady(manual, matches, manual.shootouts).cup;
  assert.deepEqual(importVerifiedCupJson(exportCupJson(advanced), matches), advanced);
  for (const penalties of [
    { home: -1, away: 0 }, { home: 1.5, away: 0 }, { home: 0, away: 0 },
    { home: Number.MAX_SAFE_INTEGER + 1, away: 0 }, { home: "4", away: 3 },
  ]) assert.throws(() => recordCupShootout(cup, matches, 1, 1, penalties));
});

test("SP-01 malformed sessions/results fail every JSON boundary and preserve source bytes", () => {
  const { cup, matches, storage } = openShootout();
  const mutations = [
    c => { c.shootoutSessions = []; },
    c => { c.shootoutSessions = null; },
    c => { c.shootoutSessions["r1-p1"] = []; },
    c => { c.shootoutSessions["r1-p1"].schemaVersion = 9; },
    c => { c.shootoutSessions["r1-p1"].rulesVersion = "v2"; },
    c => { c.shootoutSessions["r1-p1"].shootoutId = '["other","r1-p1"]'; },
    c => { c.shootoutSessions["r1-p1"].homeProviderTeamId = "B"; },
    c => { c.shootoutSessions["r1-p1"].fixtureRefs[0].matchId = "other"; },
    c => { c.shootoutSessions["r1-p2"] = c.shootoutSessions["r1-p1"]; },
    c => { c.shootoutSessions["r01-p1"] = c.shootoutSessions["r1-p1"]; },
    c => { c.shootoutSessions.__proto__ = { injected: {} }; },
    c => { c.shootouts = { "r1-p1": { home: 4, away: 3 } }; },
    c => { c.shootouts = { "r99-p1": { home: 4, away: 3 } }; },
    c => { c.shootouts = { "r1-p2": { home: 4, away: 4 } }; },
    c => { c.shootouts = JSON.parse('{"__proto__":{"home":4,"away":3}}'); },
  ];
  for (const mutate of mutations) {
    const corrupted = structuredClone(cup);
    mutate(corrupted);
    // A custom prototype is rejected before serialization (JSON drops it).
    assert.throws(() => exportCupJson(corrupted));
    if (corrupted.shootoutSessions !== null &&
        !Array.isArray(corrupted.shootoutSessions) &&
        Object.getPrototypeOf(corrupted.shootoutSessions) !== Object.prototype) continue;
    const bytes = JSON.stringify(corrupted);
    storage.setItem(CUP_STORAGE_KEY, bytes);
    assert.throws(() => loadCup(storage));
    assert.throws(() => loadVerifiedCup(storage, matches));
    assert.throws(() => importCupJson(bytes));
    assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
  }
});

test("SP-01 complete history requires exact totals; final maps require real tied matches", () => {
  const { editor, storage, matches, cup: started } = openShootout();
  let cup = started;
  for (const goal of [1, 0, 1, 0, 1, 0]) cup = shoot(editor, cup, goal);
  for (const mutate of [
    c => { delete c.shootouts; },
    c => { c.shootouts["r1-p1"].home = 4; },
    c => { c.shootoutSessions["r1-p1"].status = "initial"; },
    c => { c.shootouts["r1-p2"] = { home: 3, away: 2 }; },
  ]) {
    const corrupted = structuredClone(cup);
    mutate(corrupted);
    const bytes = JSON.stringify(corrupted);
    storage.setItem(CUP_STORAGE_KEY, bytes);
    assert.throws(() => loadVerifiedCup(storage, matches));
    assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
  }
  assert.throws(() => verifyCupDocument(cup, matches.slice(1)));
  const swapped = structuredClone(matches);
  [swapped[0].home, swapped[0].away] = [swapped[0].away, swapped[0].home];
  assert.throws(() => verifyCupDocument(cup, swapped));
});

test("SP-01 verified admission rejects an unfinished session outside readiness order", () => {
  const { cup, matches } = openShootout({ twoTies: true });
  const session = cup.shootoutSessions["r1-p1"];
  const second = structuredClone(session);
  Object.assign(second, { pair: 2, shootoutId: JSON.stringify([cup.cupId, "r1-p2"]),
    homeProviderTeamId: "C", awayProviderTeamId: "D",
    fixtureRefs: [{ fixtureId: "r1-p2-l1", matchId: "shootout-r1-p2-l1" }] });
  const wrongOrder = { ...cup, shootoutSessions: { "r1-p2": second } };
  assert.doesNotThrow(() => importCupJson(JSON.stringify(wrongOrder)));
  assert.throws(() => verifyCupDocument(wrongOrder, matches));
  assert.throws(() => exportCupJson({ ...cup,
    shootoutSessions: { "r1-p1": session, "r1-p2": second } }));
  const notTied = structuredClone(matches);
  notTied[0].score = { home: 2, away: 1 };
  assert.throws(() => verifyCupDocument(cup, notTied));
});

// Execute the app's actual adapter with the real coordinator and a minimal DOM.
// These are Node integration tests; they do not claim browser Web Lock/a11y coverage.
import { readFileSync } from "node:fs";
import { createContext, runInContext } from "node:vm";
import { CupShootoutError } from "./cup-shootout.mjs";
import { FACES } from "./engine.mjs";
import { PENALTY_DICE_ART } from "./dice-art.mjs";
import { cupFixturePresentation } from "./cup-view.mjs";
import { clearCupRecovery } from "./cup-store.mjs";

const appSource = readFileSync(new URL("./app.mjs", import.meta.url), "utf8");
const adapterSource = appSource.slice(appSource.indexOf("// SP-01 app adapter:"),
  appSource.indexOf("// SP-01 app adapter end."));
const manualSource = appSource.slice(appSource.indexOf("function confirmCupShootout()"),
  appSource.indexOf('byId("cup-shootout-confirm").addEventListener'));
const recoverySource = appSource.slice(appSource.indexOf("function reconcileCupRecovery()"),
  appSource.indexOf("try {\n  const recovery = loadTournamentRecovery"));

function appNode() {
  return {
    textContent: "", value: "", hidden: false, disabled: false, dataset: {}, children: [],
    attributes: {}, focused: false, onclick: null,
    setAttribute(key, value) { this.attributes[key] = value; },
    replaceChildren(...children) { this.children = children; },
    append(...children) { this.children.push(...children); },
    focus() { this.focused = true; },
    classList: { add() {} },
  };
}

function fakeWebLocks() {
  const held = new Set();
  const requests = [];
  return {
    requests,
    async request(name, options, callback) {
      requests.push({ name, ...options });
      if (held.has(name)) return callback(null);
      held.add(name);
      try { return await callback({ name, mode: "exclusive" }); }
      finally { held.delete(name); }
    },
  };
}

function appAdapter({ cup, matches, storage, locks = fakeWebLocks(), owned = true } = openShootout()) {
  const nodes = new Map();
  const byId = id => {
    if (!nodes.has(id)) nodes.set(id, appNode());
    return nodes.get(id);
  };
  let locale = "it";
  let draws = 0;
  let randomValue = 0;
  let management = { matches };
  const context = createContext({
    console, structuredClone, CupShootoutError, CUP_STORAGE_KEY, FACES, PENALTY_DICE_ART,
    loadVerifiedCup, saveVerifiedCup, recordCupShootout, advanceCupIfReady,
    replayCupShootout, cupFixturePresentation, loadCupRecovery, clearCupRecovery,
    completeCupFixture,
    createCupShootoutCoordinator(store, records, options) {
      const editor = createCupShootoutCoordinator(store, records, options);
      return { ...editor, confirmCupShootoutAttempt(command) {
        return editor.confirmCupShootoutAttempt(command, { random: () => { draws += 1; return randomValue; } });
      } };
    },
    loadManagement: () => management,
    getLocale: () => locale,
    byId, document: { createElement: appNode },
    window: { localStorage: storage, navigator: { locks }, requestAnimationFrame: callback => setImmediate(callback) },
    cupState: cup, cupLoadError: null, cupLock: owned ? { cupId: cup.cupId, release() {} } : null,
    cupLockRequest: null, cupCoordinator: null, cupShootoutBusy: false,
    cupShootoutPendingSave: false, cupShootoutError: null, cupShootoutNotice: null,
    tournamentState: null, tournamentViewedMatchday: null, managementState: management,
    matchStarted: false, state: { phase: "finished" }, activeTournamentFixtureId: null,
  });
  runInContext(adapterSource + recoverySource + manualSource, context);
  context.renderTournament = () => {
    context.renderCupStorageStatus();
    if (context.cupState) context.renderCupShootout(context.cupState, id => `Team ${id}`);
  };
  context.cupCoordinator = context.createCupShootoutCoordinator(storage, matches, { ownsLock: context.ownsCupLock });
  context.renderTournament();
  return { context, nodes, byId, storage, locks, matches,
    draws: () => draws, random: value => { randomValue = value; },
    locale: value => { locale = value; },
    management: value => { management = value; },
  };
}

const flushApp = () => new Promise(resolve => setImmediate(resolve));

test("SP-01C app shows committed sequence, turns and localized accessible green-die controls", async () => {
  const app = appAdapter();
  const { context: ui, byId } = app;
  assert.match(byId("cup-shootout-turn").textContent, /Team A.*1 di 5/);
  assert.match(byId("cup-shootout-rules").textContent, /3 facce Goal su 6/);
  assert.equal(byId("cup-shootout-manual").hidden, true);
  const oldClick = byId("cup-shootout-kick").onclick;
  oldClick({ detail: 1 });
  oldClick({ detail: 1 }); // Reentrant/queued activation during the save is ignored.
  assert.equal(byId("cup-shootout-kick").disabled, true);
  await flushApp();
  assert.equal(app.draws(), 1);
  assert.match(byId("cup-shootout-turn").textContent, /salvato.*Team B/);
  assert.match(byId("cup-shootout-score").textContent, /Team A 1 – 0 Team B/);
  assert.match(byId("cup-shootout-sequence").children[0].textContent, /Team A.*Goal/);
  assert.equal(byId("cup-shootout-kick").focused, true);
  assert.match(byId("cup-shootout-die").alt, /Dado verde: Goal/);
  assert.equal(decodeURIComponent(byId("cup-shootout-die").src.split(",")[1]), PENALTY_DICE_ART[0]);
  oldClick({ detail: 1 }); // A delayed old command returns its committed attempt.
  await flushApp();
  assert.equal(app.draws(), 1);
  app.locale("en");
  ui.renderTournament();
  assert.match(byId("cup-shootout-turn").textContent, /Penalty saved.*Team B/);
  assert.equal(byId("cup-shootout-sequence").attributes["aria-label"], "Penalty sequence");
  assert.equal(app.draws(), 1);
  assert.equal(byId("cup-shootout").attributes["aria-busy"], "false");
});

test("SP-01C app retains failed candidates and retry never rerolls or publishes early", async () => {
  for (const terminal of [false, true]) {
    const fixture = openShootout();
    if (terminal) for (const goal of [1, 0, 1, 0, 1]) fixture.cup = shoot(fixture.editor, fixture.cup, goal);
    const app = appAdapter(fixture);
    const { context: ui, byId, storage } = app;
    app.random(0.99);
    const before = JSON.stringify(ui.cupState);
    const command = shotCommand(ui.cupState);
    storage.failWrite = true;
    await ui.runCupShootoutAction("kick", command);
    assert.equal(app.draws(), 1);
    assert.equal(JSON.stringify(ui.cupState), before);
    assert.equal(ui.cupShootoutPendingSave, true);
    assert.equal(byId("cup-shootout-retry").hidden, false);
    assert.equal(byId("cup-shootout-kick").disabled, true);
    assert.equal(byId("cup-storage-retry").disabled, true);
    const retained = ui.cupCoordinator;
    ui.refreshOwnedCup();
    assert.equal(ui.cupCoordinator, retained);
    await ui.runCupShootoutAction("kick", command);
    await ui.runCupShootoutAction("start", { cupId: ui.cupState.cupId, round: 1, pair: 1 });
    assert.equal(app.draws(), 1);
    await ui.runCupShootoutAction("retry");
    assert.equal(ui.cupShootoutPendingSave, true);
    storage.failWrite = false;
    await ui.runCupShootoutAction("retry");
    assert.equal(app.draws(), 1);
    assert.equal(ui.cupShootoutPendingSave, false);
    assert.equal(ui.cupState.shootoutSessions["r1-p1"].status, terminal ? "complete" : "initial");
    if (terminal) {
      assert.match(byId("cup-shootout-turn").textContent, /Team A vince ai rigori 3–0/);
      assert.equal(byId("cup-shootout-kick").hidden, true);
      assert.equal(ui.cupState.fixtures.filter(f => f.round === 2).length, 1);
    }
    assert.deepEqual(loadVerifiedCup(storage, app.matches), ui.cupState);
  }
});

test("SP-01C app resumes half a sudden-death pair and leaves regular match records untouched", async () => {
  const fixture = openShootout({ legs: 2 });
  for (const goal of [...Array(10).fill(1), 1]) fixture.cup = shoot(fixture.editor, fixture.cup, goal);
  const app = appAdapter(fixture);
  const before = structuredClone(app.matches);
  assert.equal(app.draws(), 0);
  assert.match(app.byId("cup-shootout-turn").textContent, /Oltranza.*Team B.*Nessun vincitore/);
  app.random(0.99);
  await app.context.runCupShootoutAction("kick", shotCommand(app.context.cupState));
  assert.equal(app.draws(), 1);
  assert.equal(app.context.cupState.fixtures.find(f => f.round === 2).homeProviderTeamId, "A");
  assert.deepEqual(app.matches, before);
  assert.deepEqual(app.context.cupState.shootouts, { "r1-p1": { home: 6, away: 5 } });
});

test("SP-01C app owns an exclusive request lifecycle and revalidates on handover", async () => {
  const fixture = openShootout();
  const locks = fakeWebLocks();
  const first = appAdapter({ ...fixture, locks, owned: false });
  const second = appAdapter({ ...fixture, locks, owned: false });
  first.context.requestCupOwnership();
  await flushApp();
  second.context.requestCupOwnership();
  await flushApp();
  assert.equal(first.context.ownsCupLock(), true);
  assert.equal(second.context.ownsCupLock(), false);
  assert.equal(second.byId("cup-shootout-kick").disabled, true);
  assert.match(second.byId("cup-storage-status").textContent, /sola lettura/);
  assert.deepEqual(locks.requests[0], { name: `dado-goal.cup:${fixture.cup.cupId}`, mode: "exclusive", ifAvailable: true });
  await first.context.runCupShootoutAction("kick", shotCommand(first.context.cupState));
  first.context.releaseCupOwnership();
  await flushApp();
  second.context.requestCupOwnership();
  await flushApp();
  assert.equal(second.context.ownsCupLock(), true);
  assert.equal(second.context.cupState.shootoutSessions["r1-p1"].attempts.length, 1);
  assert.match(second.byId("cup-shootout-turn").textContent, /Team B/);
  assert.equal(second.draws(), 0);
  second.context.releaseCupOwnership();
  await flushApp();
});

test("SP-01C app fails closed without locks and preserves malformed/unknown Cup bytes", async () => {
  const fixture = openShootout();
  const unavailable = appAdapter({ ...fixture, owned: false, locks: undefined });
  // Explicitly remove the feature; an omitted option normally builds fake locks.
  unavailable.context.window.navigator.locks = undefined;
  unavailable.context.requestCupOwnership();
  unavailable.context.renderTournament();
  assert.equal(unavailable.byId("cup-shootout-kick").disabled, true);
  assert.match(unavailable.byId("cup-storage-status").textContent, /non supporta/);
  await unavailable.context.runCupShootoutAction("kick", shotCommand(fixture.cup));
  assert.equal(unavailable.draws(), 0);
  for (const bytes of ["{broken", JSON.stringify({ ...fixture.cup,
    shootoutSessions: { "r1-p1": { ...fixture.cup.shootoutSessions["r1-p1"], rulesVersion: "v99" } } })]) {
    fixture.storage.setItem(CUP_STORAGE_KEY, bytes);
    unavailable.context.cupState = null;
    unavailable.context.reloadCupForOwnership();
    assert.ok(unavailable.context.cupLoadError);
    assert.equal(unavailable.context.cupGlobalImportBlocked(), true);
    assert.equal(fixture.storage.getItem(CUP_STORAGE_KEY), bytes);
  }
});

test("SP-01C manual numeric validation accepts decimal totals and rejects invalid input", () => {
  for (const [home, away, valid] of [
    ["4", "3", true], ["0", "1", true], [" 12 ", "11", true],
    ["9007199254740991", "0", true], ["-1", "0", false], ["2.5", "1", false],
    ["", "1", false], ["1e2", "1", false], ["4", "4", false],
    ["9007199254740992", "1", false], ["\\d", "1", false],
  ]) {
    const { cup, matches } = shootoutFixture();
    const storage = shootoutStorage();
    saveVerifiedCup(storage, cup, matches);
    const app = appAdapter({ cup, matches, storage });
    app.byId("cup-penalties-home").value = home;
    app.byId("cup-penalties-away").value = away;
    app.context.confirmCupShootout();
    assert.equal(Boolean(app.context.cupState.shootouts), valid, `${home}/${away}`);
    if (valid) {
      assert.deepEqual(app.context.cupState.shootouts["r1-p1"], { home: Number(home), away: Number(away) });
      assert.equal(app.context.cupState.shootoutSessions, undefined);
    } else assert.equal(app.context.cupShootoutError, "CUP_SHOOTOUT_INVALID_SCORE");
    assert.equal(app.draws(), 0);
  }
});

test("SP-01C manual handler and recovery writes cannot bypass ownership", () => {
  const { cup, matches } = shootoutFixture();
  const storage = shootoutStorage();
  saveVerifiedCup(storage, cup, matches);
  const app = appAdapter({ cup, matches, storage, owned: false });
  app.byId("cup-penalties-home").value = "4";
  app.byId("cup-penalties-away").value = "3";
  const before = storage.getItem(CUP_STORAGE_KEY);
  app.context.confirmCupShootout();
  assert.equal(app.context.cupShootoutError, "CUP_SHOOTOUT_OTHER_TAB");
  assert.throws(() => app.context.reconcileCupRecovery(), { code: "CUP_SHOOTOUT_OTHER_TAB" });
  assert.equal(storage.getItem(CUP_STORAGE_KEY), before);
});

test("SP-01C global import checks storage again after asynchronous file reading", async () => {
  const { cup, matches } = shootoutFixture();
  const storage = shootoutStorage();
  const app = appAdapter({ cup, matches, storage });
  const ui = app.context;
  ui.cupState = null;
  ui.isRolling = false;
  ui.validateGlobalSave = text => text;
  ui.restoreGlobalRuntime = () => assert.fail("must not restore over a Cup");
  runInContext(appSource.slice(appSource.indexOf("async function importGlobalSave(file)"),
    appSource.indexOf("function initializeGlobalSaveUi()")), ui);
  await assert.rejects(ui.importGlobalSave({ async text() {
    saveVerifiedCup(storage, cup, matches);
    return "{}";
  } }), { code: "CUP_GLOBAL_IMPORT_BLOCKED" });
  const bytes = storage.getItem(CUP_STORAGE_KEY);
  await assert.rejects(ui.importGlobalSave({ text: () => assert.fail("must block before reading") }),
    { code: "CUP_GLOBAL_IMPORT_BLOCKED" });
  assert.equal(storage.getItem(CUP_STORAGE_KEY), bytes);
  storage.failRead = true;
  assert.equal(ui.cupGlobalImportBlocked(), true);
});

test("SP-01C read failure disables attempts until refresh, and postcommit render failure recovers", async () => {
  const app = appAdapter();
  const { context: ui, storage, byId } = app;
  const command = shotCommand(ui.cupState);
  storage.failRead = true;
  await ui.runCupShootoutAction("kick", command);
  assert.equal(app.draws(), 0);
  assert.equal(byId("cup-shootout-kick").disabled, true);
  assert.equal(byId("cup-storage-retry").hidden, false);
  storage.failRead = false;
  ui.refreshOwnedCup();
  ui.renderTournament();
  assert.equal(byId("cup-shootout-kick").disabled, false);
  const render = ui.renderTournament;
  let interrupted = false;
  ui.renderTournament = () => {
    if (!interrupted && ui.cupState.shootoutSessions["r1-p1"].attempts.length) {
      interrupted = true;
      throw new Error("presentation interrupted after commit");
    }
    render();
  };
  await ui.runCupShootoutAction("kick", command);
  assert.equal(interrupted, true);
  assert.equal(app.draws(), 1);
  assert.equal(ui.cupShootoutPendingSave, false);
  assert.equal(ui.cupState.shootoutSessions["r1-p1"].attempts.length, 1);
  assert.match(byId("cup-shootout-turn").textContent, /Team B/);
});

test("SP-01C session start retries without drawing and locks manual entry", async () => {
  const { cup, matches } = shootoutFixture();
  const storage = shootoutStorage();
  saveVerifiedCup(storage, cup, matches);
  const app = appAdapter({ cup, matches, storage });
  const ui = app.context;
  storage.failWrite = true;
  const start = { cupId: cup.cupId, round: 1, pair: 1 };
  await ui.runCupShootoutAction("start", start);
  assert.equal(ui.cupState.shootoutSessions, undefined);
  assert.equal(ui.cupShootoutPendingSave, false);
  assert.equal(app.byId("cup-shootout-start").disabled, false);
  storage.failWrite = false;
  await ui.runCupShootoutAction("start", start);
  assert.equal(app.draws(), 0);
  assert.equal(ui.cupState.shootoutSessions["r1-p1"].attempts.length, 0);
  assert.equal(app.byId("cup-shootout-manual").hidden, true);
  app.byId("cup-penalties-home").value = "4";
  app.byId("cup-penalties-away").value = "3";
  ui.confirmCupShootout();
  assert.equal(ui.cupShootoutError, "CUP_SHOOTOUT_MANUAL_LOCKED");
  assert.equal(ui.cupState.shootouts, undefined);
});

test("SP-01C bracket keeps first-leg penalty orientation and labels legacy manual results", () => {
  const fixture = shootoutFixture({ legs: 2 });
  fixture.cup = recordCupShootout(fixture.cup, fixture.matches, 1, 1, { home: 4, away: 3 });
  const storage = shootoutStorage();
  saveVerifiedCup(storage, fixture.cup, fixture.matches);
  const app = appAdapter({ ...fixture, storage });
  runInContext(appSource.slice(appSource.indexOf("function renderCupBracket()"),
    appSource.indexOf("function confirmCupShootout()")), app.context);
  app.locale("en");
  app.context.renderCupBracket();
  const ties = app.byId("tournament-results").children.slice(0, 2);
  for (const item of ties) {
    assert.match(item.children[3].textContent, /Aggregate 2–2.*Penalties: Team A 4–3 Team B \(manual result\)/);
    assert.equal(item.children[1].textContent, "2–0");
  }
  assert.equal(app.context.cupState.shootoutSessions, undefined);
  assert.equal(app.draws(), 0);
});

test("SP-01C lock callback failure clears ownership before another editor can acquire", async () => {
  const fixture = openShootout();
  const locks = fakeWebLocks();
  const app = appAdapter({ ...fixture, locks, owned: false });
  const render = app.context.renderTournament;
  let interrupted = false;
  app.context.renderTournament = () => {
    if (!interrupted && app.context.ownsCupLock()) {
      interrupted = true;
      throw new Error("lock acquisition render failed");
    }
    render();
  };
  app.context.requestCupOwnership();
  await flushApp();
  assert.equal(interrupted, true);
  assert.equal(app.context.ownsCupLock(), false);
  const second = appAdapter({ ...fixture, locks, owned: false });
  second.context.requestCupOwnership();
  await flushApp();
  assert.equal(second.context.ownsCupLock(), true);
  second.context.releaseCupOwnership();
  await flushApp();
});
