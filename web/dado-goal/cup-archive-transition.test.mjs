import assert from "node:assert/strict";
import test from "node:test";
import { Worker, isMainThread, parentPort, workerData } from "node:worker_threads";
import {
  prepareCupArchiveTransition, revalidateCupArchiveTransition, CupArchiveTransitionError,
} from "./cup-archive-transition.mjs";
import {
  createGlobalSaveTransaction, GLOBAL_SAVE_TRANSACTION_KEYS as KEYS,
  GLOBAL_SAVE_TRANSACTION_KEY as JOURNAL, GLOBAL_SAVE_GENERATION_KEY as GENERATION,
} from "./global-save-transaction.mjs";
import {
  createCupDocument, verifyCupDocument, recordCupShootout,
  CUP_STORAGE_KEY as CUP, CUP_ARCHIVE_STORAGE_KEY as ARCHIVE,
  CUP_RECOVERY_STORAGE_KEY as RECOVERY,
} from "./cup-store.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";
import { createCupShootout, applyCupShootoutFace, replayCupShootout } from "./cup-shootout.mjs";
import { createManagementDocument, MANAGEMENT_STORAGE_KEY as MANAGEMENT } from "./team-management-store.mjs";
import { createMatchRecord } from "./team-history.mjs";

const unrelated = { "other-app": " \u0000 preserve 🥅 ", "dado-goal.multiplayer-seat.v1": '{"seat":1}',
  "dado-goal.locale": "en", "dado-goal.runtime.v1": "unrelated runtime" };
const entriesOf = images => Object.fromEntries(Object.entries(images).filter(([, raw]) => raw !== null));
const imagesOf = entries => Object.fromEntries(KEYS.map(key => [key, entries[key] ?? null]));
const code = expected => error => error instanceof CupArchiveTransitionError && error.code === expected;

class Storage {
  constructor(entries = {}, fault = null) {
    this.map = new Map(Object.entries(entries));
    this.calls = [];
    this.fault = fault;
    this.fired = false;
    this.offline = false;
  }
  boundary(call, side) {
    if (this.offline) throw new Error("offline");
    if (this.fired || this.fault?.at !== call.index || this.fault.side !== side) return;
    this.fired = true;
    if (this.fault.mode === "crash") {
      parentPort.postMessage(Object.fromEntries(this.map));
      process.exit(0); // Actual worker termination bypasses engine catch/finally.
    }
    if (this.fault.mode === "offline") this.offline = true;
    throw new Error("injected fault");
  }
  access(method, key, value, action) {
    const call = { index: this.calls.length, method, key, value };
    this.calls.push(call);
    this.boundary(call, "before");
    const result = action();
    this.boundary(call, "after");
    return result;
  }
  getItem(key) { return this.access("get", key, undefined, () => this.map.get(key) ?? null); }
  setItem(key, value) { return this.access("set", key, value, () => this.map.set(key, value)); }
  removeItem(key) { return this.access("remove", key, undefined, () => this.map.delete(key)); }
  get writes() { return this.calls.filter(call => call.method !== "get"); }
}

function completedCup({ cupId = 'archive-"🥅', size = 4, legs = 1, sessions = true } = {}) {
  const teams = Array.from({ length: size }, (_, i) => ({
    provider: "test", providerTeamId: `team-${i}`, name: `Team ${i}`,
    metadata: { original: [i, "preserve"] },
  }));
  let cup = createCupDocument({ cupId, participants: teams, legs, name: "Original Cup" });
  cup.metadata = JSON.parse('{"savedAt":"original-time","nested":{"__proto__":{"keep":true}},"list":[3,2,1]}');
  const matches = [];
  for (let round = 1; cup.championProviderTeamId === undefined; round += 1) {
    const fixtures = cup.fixtures.filter(f => f.round === round);
    for (const fixture of fixtures) {
      const tied = sessions && fixture.pair === 1;
      const match = createMatchRecord({
        matchId: `${cupId}-${fixture.fixtureId}`, playedAt: "original-time",
        home: teams.find(team => team.providerTeamId === fixture.homeProviderTeamId),
        away: teams.find(team => team.providerTeamId === fixture.awayProviderTeamId),
        homeGoals: tied ? 1 : fixture.leg === 1 ? 2 : 0, awayGoals: tied ? 1 : 0,
        events: [{ type: "goal", team: 0, minute: 23, number: null }],
        rollEvents: [{ seat: 0, die: "result", face: "GOAL!", kind: "goal" }], statistics: { penalties: 0 },
      });
      matches.push(match);
      cup = completeCupFixture(cup, fixture.fixtureId, match.matchId);
    }
    if (sessions) {
      const key = `r${round}-p1`;
      let session = createCupShootout(cup, matches, round, 1);
      const outcomes = round === Math.log2(size)
        ? [...Array(10).fill(true), true, true, false, false, true, false]
        : [true, false, true, false, true, false];
      for (const goal of outcomes) {
        session = applyCupShootoutFace(session, {
          attemptId: JSON.stringify([cupId, key, session.attempts.length + 1]),
          expectedRevision: session.attempts.length, face: goal ? "GOAL!" : "FUORI",
        });
      }
      cup = recordCupShootout(cup, matches, round, 1, replayCupShootout(session).goals);
      cup.shootoutSessions = { ...(cup.shootoutSessions ?? {}), [key]: session };
    }
    cup = advanceCupIfReady(cup, matches, cup.shootouts ?? {}).cup;
  }
  return { cup: verifyCupDocument(cup, matches), matches };
}

function fixture(options = {}) {
  const current = completedCup(options);
  const prior = completedCup({ cupId: "older-cup", sessions: false });
  const matches = [...prior.matches, ...current.matches];
  const first = current.cup.fixtures[0];
  const recovery = { cupId: current.cup.cupId, fixtureId: first.fixtureId, matchId: first.matchId };
  const images = imagesOf({
    [MANAGEMENT]: ` \n${JSON.stringify(createManagementDocument({ matches }), null, 2)}\t`,
    [CUP]: `\n${JSON.stringify(current.cup, null, 2)} `,
    [ARCHIVE]: ` \n${JSON.stringify([prior.cup], null, 2)}\t`,
    [RECOVERY]: `\n${JSON.stringify(recovery)} `, [GENERATION]: "4",
  });
  const input = { snapshot: { images, journal: null }, cupJson: JSON.stringify(current.cup),
    transactionId: "archive-transaction-1", expectedGeneration: 4 };
  return { ...current, prior: prior.cup, input, entries: { ...entriesOf(images), ...unrelated } };
}

function capture(storage) {
  return { images: Object.fromEntries(KEYS.map(key => [key, storage.getItem(key)])),
    journal: storage.getItem(JOURNAL) };
}

function preflight(plan, snapshot, overrides = {}) {
  return revalidateCupArchiveTransition(plan, { snapshot, ownsPersistenceGate: true,
    ownedCupId: plan.cupId, hasPendingAttempt: false, ...overrides });
}

function assertImage(storage, images) {
  assert.deepEqual(imagesOf(Object.fromEntries(storage.map)), images);
  for (const [key, value] of Object.entries(unrelated)) assert.equal(storage.map.get(key), value);
}

function assertRecovered(entries, target) {
  const storage = new Storage(entries);
  const api = createGlobalSaveTransaction(storage);
  assert.deepEqual(api.recover().images, target);
  assertImage(storage, target);
  assert.equal(storage.map.has(JOURNAL), false);
  const writes = storage.writes.length;
  assert.deepEqual(api.recover().images, target);
  assert.deepEqual(createGlobalSaveTransaction(storage).recover().images, target);
  assert.equal(storage.writes.length, writes);
  return storage;
}

function interrupted(plan, phase, count) {
  const mixed = { ...plan.before };
  for (const key of KEYS.slice(0, count)) mixed[key] = plan.after[key];
  return { ...entriesOf(mixed), ...unrelated, [JOURNAL]: JSON.stringify({
    schemaVersion: 1, transactionId: plan.transaction.transactionId, operation: "archive", phase,
    before: plan.before, after: plan.after,
  }) };
}

function crash(entries, request, at, side, task = "transact") {
  return new Promise((resolve, reject) => {
    const worker = new Worker(new URL(import.meta.url), {
      workerData: { entries, request, task, fault: { at, side, mode: "crash" } },
    });
    let result;
    worker.on("message", value => { result = value; });
    worker.on("error", reject);
    worker.on("exit", exit => exit === 0 && result ? resolve(result) : reject(new Error("worker did not crash at boundary")));
  });
}

if (!isMainThread) {
  const storage = new Storage(workerData.entries, workerData.fault);
  const api = createGlobalSaveTransaction(storage);
  if (workerData.task === "recover") api.recover();
  else api.transact(workerData.request);
  throw new Error("crash injection was not reached");
} else {
  test("prepares one immutable deterministic archive insertion with exact completed history", () => {
    const f = fixture();
    const original = structuredClone(f.input);
    const plan = prepareCupArchiveTransition(f.input);
    assert.equal(plan.disposition, "inserted");
    assert.equal(plan.cupId, f.cup.cupId);
    assert.deepEqual(plan.cup, f.cup);
    assert.deepEqual(plan.before, f.input.snapshot.images);
    assert.equal(plan.after[CUP], null);
    assert.equal(plan.after[RECOVERY], null);
    assert.equal(plan.after[GENERATION], "5");
    assert.deepEqual(JSON.parse(plan.after[ARCHIVE]), [f.prior, f.cup]);
    assert.equal(JSON.parse(plan.after[ARCHIVE]).filter(cup => cup.cupId === f.cup.cupId).length, 1);
    assert.deepEqual(plan.transaction, { transactionId: f.input.transactionId, operation: "archive",
      expectedGeneration: 4, changes: { [CUP]: null, [ARCHIVE]: plan.after[ARCHIVE], [RECOVERY]: null } });
    for (const key of KEYS.filter(key => ![CUP, ARCHIVE, RECOVERY, GENERATION].includes(key))) {
      assert.equal(plan.after[key], plan.before[key]);
    }
    assert.deepEqual(prepareCupArchiveTransition(f.input), plan);
    assert.deepEqual(f.input, original);
    assert.deepEqual(plan.requirements, { persistenceGate: "dado-goal.single-player.persistence",
      cupOwnership: [f.cup.cupId], noPendingAttempt: true });
    function assertFrozen(value) {
      if (!value || typeof value !== "object") return;
      assert.ok(Object.isFrozen(value));
      Object.values(value).forEach(assertFrozen);
    }
    assertFrozen(plan);
    assert.throws(() => { plan.transaction.changes[CUP] = "changed"; }, TypeError);
    f.input.snapshot.images[CUP] = "caller mutation";
    assert.equal(plan.before[CUP], original.snapshot.images[CUP]);
  });

  for (const legs of [1, 2]) {
    for (const size of [4, 8, 16, 32]) {
      test(`${size} teams / ${legs} legs preserves every fixture, session, attempt and canonical record`, () => {
        const f = fixture({ size, legs });
        const plan = prepareCupArchiveTransition(f.input);
        const archived = JSON.parse(plan.after[ARCHIVE]).at(-1);
        assert.deepEqual(archived, f.cup);
        assert.deepEqual(verifyCupDocument(archived, f.matches), f.cup);
        assert.equal(archived.fixtures.length, (size - 1) * legs);
        assert.equal(Object.keys(archived.shootoutSessions).length, Math.log2(size));
        assert.equal(Object.values(archived.shootoutSessions).at(-1).attempts.length, 16);
        assert.equal(plan.after[MANAGEMENT], plan.before[MANAGEMENT]);
      });
    }
  }

  test("legacy manual penalties stay manual without fabricated sessions", () => {
    const f = fixture();
    const cup = structuredClone(f.cup);
    delete cup.shootoutSessions;
    f.input.cupJson = JSON.stringify(cup);
    f.input.snapshot.images[CUP] = f.input.cupJson;
    const archived = JSON.parse(prepareCupArchiveTransition(f.input).after[ARCHIVE]).at(-1);
    assert.deepEqual(archived, cup);
    assert.equal(Object.hasOwn(archived, "shootoutSessions"), false);
  });

  test("absent archive/recovery/generation become one archive and epoch 1", () => {
    const f = fixture();
    for (const key of [ARCHIVE, RECOVERY, GENERATION]) f.input.snapshot.images[key] = null;
    f.input.expectedGeneration = 0;
    const plan = prepareCupArchiveTransition(f.input);
    assert.deepEqual(JSON.parse(plan.after[ARCHIVE]), [f.cup]);
    assert.equal(plan.after[GENERATION], "1");
  });

  test("successful public transaction then exact archival retry is a write-free no-op", () => {
    const f = fixture();
    const storage = new Storage(f.entries);
    const api = createGlobalSaveTransaction(storage);
    const plan = prepareCupArchiveTransition(f.input);
    const request = preflight(plan, capture(storage));
    assert.equal(storage.writes.length, 0);
    const result = api.transact(request);
    assert.equal(result.status, "committed");
    assert.equal(result.usable, true);
    assertImage(storage, plan.after);
    const writes = storage.writes.length;
    const retry = prepareCupArchiveTransition({ ...f.input, snapshot: capture(storage), expectedGeneration: 5 });
    assert.equal(retry.disposition, "already-archived");
    assert.deepEqual(retry.transaction.changes, {});
    assert.deepEqual(retry.before, retry.after);
    assert.equal(api.transact(preflight(retry, capture(storage))).status, "noop");
    assert.equal(storage.writes.length, writes);
    assertImage(storage, plan.after);
  });

  test("equivalence ignores object property order and whitespace but retains archive raw bytes on retry", () => {
    const f = fixture();
    const cup = Object.fromEntries(Object.entries(f.cup).reverse());
    f.input.snapshot.images[CUP] = null;
    f.input.snapshot.images[RECOVERY] = null;
    f.input.snapshot.images[ARCHIVE] = `\n${JSON.stringify([f.prior, cup], null, 3)}\t`;
    const plan = prepareCupArchiveTransition(f.input);
    assert.equal(plan.disposition, "already-archived");
    assert.equal(plan.after[ARCHIVE], f.input.snapshot.images[ARCHIVE]);
  });

  const badCups = [
    ["invalid JSON", () => "{"],
    ["null", () => "null"],
    ["unsupported schema", cup => { cup.schemaVersion = 2; }],
    ["missing champion", cup => { delete cup.championProviderTeamId; }],
    ["wrong champion", cup => { cup.championProviderTeamId = "team-1"; }],
    ["unplayed final", cup => { cup.fixtures.at(-1).matchId = null; }],
    ["fabricated bracket", cup => { cup.fixtures.at(-1).homeProviderTeamId = "team-1"; }],
    ["missing canonical match", cup => { cup.fixtures.at(-1).matchId = "missing"; }],
    ["reversed orientation", cup => {
      const fixture = cup.fixtures[0];
      [fixture.homeProviderTeamId, fixture.awayProviderTeamId] = [fixture.awayProviderTeamId, fixture.homeProviderTeamId];
    }],
    ["tampered shootout", cup => { cup.shootoutSessions["r1-p1"].attempts[0].face = "invalid"; }],
    ["changed shootout totals", cup => { cup.shootouts["r1-p1"].home += 1; }],
  ];
  for (const [label, corrupt] of badCups) {
    test(`rejects ${label} without input or storage changes`, () => {
      const f = fixture();
      const cup = structuredClone(f.cup);
      const raw = corrupt(cup) ?? JSON.stringify(cup);
      f.input.cupJson = raw;
      f.input.snapshot.images[CUP] = raw;
      const before = structuredClone(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), CupArchiveTransitionError);
      assert.deepEqual(f.input, before);
    });
  }

  for (const [label, raw] of [
    ["truncated", "["], ["object", "{}"], ["null", "null"], ["scalar", "1"],
    ["null entry", "[null]"], ["invalid document", '[{"schemaVersion":99}]'],
  ]) {
    test(`rejects ${label} archive`, () => {
      const f = fixture();
      f.input.snapshot.images[ARCHIVE] = raw;
      const before = structuredClone(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
      assert.deepEqual(f.input, before);
    });
  }

  test("rejects duplicate archive IDs, including identical entries, and unfinished archive", () => {
    for (const archive of [f => [f.prior, f.prior], f => [createCupDocument({
      cupId: "unfinished", participants: f.cup.participants,
    })]]) {
      const f = fixture();
      f.input.snapshot.images[ARCHIVE] = JSON.stringify(archive(f));
      assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
    }
  });

  test("current/archive identity collision remains invalid even when documents agree", () => {
    const f = fixture();
    f.input.snapshot.images[ARCHIVE] = JSON.stringify([f.prior, f.cup]);
    assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
  });

  for (const [label, change] of [
    ["metadata", cup => { cup.metadata.savedAt = "different"; }],
    ["name", cup => { cup.name = "different"; }],
    ["history", cup => { delete cup.shootoutSessions; }],
    ["optional field", cup => { cup.extra = true; }],
  ]) {
    test(`same-ID archived ${label} conflict fails closed`, () => {
      const f = fixture();
      const conflicting = structuredClone(f.cup);
      change(conflicting);
      f.input.snapshot.images[CUP] = null;
      f.input.snapshot.images[RECOVERY] = null;
      f.input.snapshot.images[ARCHIVE] = JSON.stringify([f.prior, conflicting]);
      const before = structuredClone(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("ARCHIVE_CONFLICT"));
      assert.deepEqual(f.input, before);
    });
    test(`same-ID current ${label} change invalidates requested document`, () => {
      const f = fixture();
      const changed = structuredClone(f.cup);
      change(changed);
      f.input.snapshot.images[CUP] = JSON.stringify(changed);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("CURRENT_CUP_MISMATCH"));
    });
  }

  test("missing/different current Cup cannot be archived from a cached request", () => {
    const f = fixture();
    f.input.snapshot.images[CUP] = null;
    f.input.snapshot.images[RECOVERY] = null;
    assert.throws(() => prepareCupArchiveTransition(f.input), code("CURRENT_CUP_MISSING"));
    f.input.snapshot.images[CUP] = JSON.stringify(f.prior);
    f.input.snapshot.images[ARCHIVE] = "[]";
    assert.throws(() => prepareCupArchiveTransition(f.input), code("CURRENT_CUP_MISMATCH"));
  });

  test("rejects duplicate/empty canonical IDs, bad scores, and shared match ownership", () => {
    for (const change of [
      management => { management.matches.push(management.matches[0]); },
      management => { management.matches[0].matchId = " "; },
      management => { management.matches.at(-1).score.home = -1; },
      management => { management.matches.at(-1).score.away = Number.MAX_SAFE_INTEGER + 1; },
    ]) {
      const f = fixture();
      const management = JSON.parse(f.input.snapshot.images[MANAGEMENT]);
      change(management);
      f.input.snapshot.images[MANAGEMENT] = JSON.stringify(management);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
    }
    const f = fixture();
    f.input.snapshot.images[ARCHIVE] = JSON.stringify([f.prior, { ...f.prior, cupId: "other-owner" }]);
    assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
  });

  for (const [label, raw] of [
    ["malformed", "{"], ["JSON null", "null"], ["missing fields", "{}"],
    ["foreign owner", JSON.stringify({ cupId: "foreign", fixtureId: "r1-p1-l1", matchId: "missing" })],
    ["missing fixture", JSON.stringify({ cupId: 'archive-"🥅', fixtureId: "r9-p1-l1", matchId: "missing" })],
    ["unresolved record", JSON.stringify({ cupId: 'archive-"🥅', fixtureId: "r1-p1-l1", matchId: "missing" })],
  ]) {
    test(`${label} Cup recovery is preserved and blocks preparation`, () => {
      const f = fixture();
      f.input.snapshot.images[RECOVERY] = raw;
      const before = structuredClone(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
      assert.deepEqual(f.input, before);
    });
  }

  test("conflicting exact canonical recovery and orphaned retry recovery are never discarded", () => {
    const f = fixture();
    f.input.snapshot.images[RECOVERY] = JSON.stringify({ cupId: f.cup.cupId,
      fixtureId: f.cup.fixtures[0].fixtureId, matchId: f.cup.fixtures.at(-1).matchId });
    assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
    f.input.snapshot.images[RECOVERY] = f.entries[RECOVERY];
    f.input.snapshot.images[CUP] = null;
    f.input.snapshot.images[ARCHIVE] = JSON.stringify([f.prior, f.cup]);
    assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
  });

  for (const raw of ["{}", "invalid", '{"phase":"prepared"}', '{"phase":"committed"}']) {
    test(`pending global journal ${raw} requires priority recovery without mutations`, () => {
      const f = fixture();
      f.input.snapshot.journal = raw;
      const original = structuredClone(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), code("GLOBAL_RECOVERY_REQUIRED"));
      assert.deepEqual(f.input, original);
    });
  }

  test("generation mismatch, malformed epoch and overflow fail closed; no-op can retain maximum epoch", () => {
    const f = fixture();
    assert.throws(() => prepareCupArchiveTransition({ ...f.input, expectedGeneration: 3 }), code("GENERATION_MISMATCH"));
    for (const raw of ["-1", "01", "garbage", "9007199254740992"]) {
      f.input.snapshot.images[GENERATION] = raw;
      assert.throws(() => prepareCupArchiveTransition(f.input), code("INVALID_ARCHIVE_STATE"));
    }
    f.input.snapshot.images[GENERATION] = String(Number.MAX_SAFE_INTEGER);
    f.input.expectedGeneration = Number.MAX_SAFE_INTEGER;
    assert.throws(() => prepareCupArchiveTransition(f.input), code("GENERATION_OVERFLOW"));
    f.input.snapshot.images[CUP] = null;
    f.input.snapshot.images[RECOVERY] = null;
    f.input.snapshot.images[ARCHIVE] = JSON.stringify([f.prior, f.cup]);
    assert.equal(prepareCupArchiveTransition(f.input).disposition, "already-archived");
  });

  test("strict raw snapshot boundary rejects missing/extra keys, invalid types and accessors", () => {
    for (const change of [
      input => { delete input.snapshot.images[CUP]; },
      input => { input.snapshot.images.arbitrary = "extra"; },
      input => { input.snapshot.images[CUP] = {}; },
      input => { delete input.snapshot.journal; },
      input => { input.snapshot.journal = undefined; },
      input => { input.expectedGeneration = -1; },
      input => { input.transactionId = " "; },
      input => { input.cupJson = {}; },
      input => { Object.defineProperty(input.snapshot.images, CUP, {
        enumerable: true, get() { assert.fail("getter invoked"); },
      }); },
    ]) {
      const f = fixture();
      change(f.input);
      assert.throws(() => prepareCupArchiveTransition(f.input), CupArchiveTransitionError);
    }
  });

  test("preflight requires actual gate, Cup ownership and explicit absence of a pending attempt", () => {
    const f = fixture();
    const plan = prepareCupArchiveTransition(f.input);
    for (const [override, expected] of [
      [{ ownsPersistenceGate: false }, "PERSISTENCE_GATE_REQUIRED"],
      [{ ownsPersistenceGate: undefined }, "PERSISTENCE_GATE_REQUIRED"],
      [{ ownedCupId: "other" }, "CUP_OWNERSHIP_REQUIRED"],
      [{ ownedCupId: null }, "CUP_OWNERSHIP_REQUIRED"],
      [{ hasPendingAttempt: true }, "PENDING_ATTEMPT"],
      [{ hasPendingAttempt: undefined }, "PENDING_ATTEMPT"],
    ]) assert.throws(() => preflight(plan, f.input.snapshot, override), code(expected));
    assert.equal(preflight(plan, f.input.snapshot), plan.transaction);
  });

  for (const key of KEYS) {
    test(`preflight rejects changed predecessor bytes at ${key}, even without generation advance`, () => {
      const f = fixture();
      const plan = prepareCupArchiveTransition(f.input);
      const snapshot = structuredClone(f.input.snapshot);
      snapshot.images[key] = snapshot.images[key] === null ? "null" : `${snapshot.images[key]} `;
      assert.throws(() => preflight(plan, snapshot), code("STALE_ARCHIVE_PLAN"));
    });
  }

  test("preflight rejects a newly pending global journal; transaction independently rejects stale generation", () => {
    const f = fixture();
    const plan = prepareCupArchiveTransition(f.input);
    assert.throws(() => preflight(plan, { ...f.input.snapshot, journal: "{}" }), code("GLOBAL_RECOVERY_REQUIRED"));
    const storage = new Storage(f.entries);
    storage.map.set(GENERATION, "5");
    assert.throws(() => createGlobalSaveTransaction(storage).transact(plan.transaction),
      error => error.code === "GENERATION_MISMATCH");
    assert.equal(storage.writes.length, 0);
  });

  test("preparation/preflight have no storage writes, browser access, clock IDs or random draws", () => {
    const f = fixture();
    const storage = new Storage(f.entries);
    const snapshot = capture(storage);
    const random = Math.random;
    const now = Date.now;
    const descriptor = Object.getOwnPropertyDescriptor(globalThis, "localStorage");
    Math.random = () => assert.fail("random draw");
    Date.now = () => assert.fail("clock access");
    Object.defineProperty(globalThis, "localStorage", { configurable: true,
      get() { assert.fail("real localStorage accessed"); } });
    try {
      const plan = prepareCupArchiveTransition({ ...f.input, snapshot });
      const request = preflight(plan, snapshot);
      assert.equal(storage.writes.length, 0);
      createGlobalSaveTransaction(storage).transact(request);
      assertImage(storage, plan.after);
      assert.deepEqual(createGlobalSaveTransaction(storage).recover().images, plan.after);
    } finally {
      Math.random = random;
      Date.now = now;
      if (descriptor) Object.defineProperty(globalThis, "localStorage", descriptor);
      else delete globalThis.localStorage;
    }
  });

  for (const phase of ["prepared", "committed"]) {
    for (let count = 0; count <= KEYS.length; count += 1) {
      test(`archive ${phase} interruption after ${count} keys recovers exact ${phase === "prepared" ? "before" : "after"}`, () => {
        const f = fixture();
        const plan = prepareCupArchiveTransition(f.input);
        const entries = interrupted(plan, phase, count);
        assert.throws(() => prepareCupArchiveTransition({ ...f.input, snapshot: {
          images: imagesOf(entries), journal: entries[JOURNAL],
        } }), code("GLOBAL_RECOVERY_REQUIRED"));
        const target = phase === "prepared" ? plan.before : plan.after;
        const storage = assertRecovered(entries, target);
        const next = prepareCupArchiveTransition({ ...f.input, snapshot: capture(storage),
          expectedGeneration: phase === "prepared" ? 4 : 5 });
        assert.equal(next.disposition, phase === "prepared" ? "inserted" : "already-archived");
      });
    }
  }

  const base = fixture();
  const prepared = prepareCupArchiveTransition(base.input);
  const traced = new Storage(base.entries);
  createGlobalSaveTransaction(traced).transact(prepared.transaction);
  const trace = traced.calls;
  for (const call of trace) {
    for (const side of ["before", "after"]) {
      test(`archive storage fault ${side} ${call.index}: ${call.method} ${call.key}`, () => {
        const storage = new Storage(base.entries, { at: call.index, side });
        let result;
        let error;
        try { result = createGlobalSaveTransaction(storage).transact(prepared.transaction); }
        catch (caught) { error = caught; }
        assert.ok(storage.fired);
        assert.ok(result || error);
        const raw = storage.map.get(JOURNAL);
        const committed = raw ? JSON.parse(raw).phase === "committed" : storage.map.get(GENERATION) === "5";
        const target = committed ? prepared.after : prepared.before;
        if (result?.usable) assert.deepEqual(result.images, target);
        assertRecovered(Object.fromEntries(storage.map), target);
      });
    }
  }

  for (const side of ["before", "after"]) {
    test(`archive real worker termination ${side} EVERY storage boundary recovers via public API`, async () => {
      for (const call of trace) {
        const entries = await crash(base.entries, prepared.transaction, call.index, side);
        const committed = entries[JOURNAL] ? JSON.parse(entries[JOURNAL]).phase === "committed" : entries[GENERATION] === "5";
        const storage = assertRecovered(entries, committed ? prepared.after : prepared.before);
        const retry = prepareCupArchiveTransition({ ...base.input, snapshot: capture(storage),
          expectedGeneration: committed ? 5 : 4 });
        assert.equal(retry.disposition, committed ? "already-archived" : "inserted");
      }
    });
  }

  test("persistent commit failure retains prepared evidence and blocks until fresh recovery", () => {
    const call = trace.find(call => call.method === "remove" && call.key === CUP);
    const storage = new Storage(base.entries, { at: call.index, side: "after", mode: "offline" });
    assert.throws(() => createGlobalSaveTransaction(storage).transact(prepared.transaction), error => error.blocked);
    assert.equal(JSON.parse(storage.map.get(JOURNAL)).phase, "prepared");
    const entries = Object.fromEntries(storage.map);
    assertRecovered(entries, prepared.before);
  });

  for (const phase of ["prepared", "committed"]) {
    const entries = interrupted(prepared, phase, 5);
    const tracedRecovery = new Storage(entries);
    createGlobalSaveTransaction(tracedRecovery).recover();
    for (const call of tracedRecovery.calls) {
      for (const side of ["before", "after"]) {
        test(`${phase} archive recovery fault ${side} ${call.index}: ${call.method} ${call.key}`, () => {
          const storage = new Storage(entries, { at: call.index, side });
          assert.throws(() => createGlobalSaveTransaction(storage).recover(), error => error.blocked);
          assert.ok(storage.fired);
          assertRecovered(Object.fromEntries(storage.map), phase === "prepared" ? prepared.before : prepared.after);
        });
      }
    }
    for (const side of ["before", "after"]) {
      test(`${phase} archive recovery real worker termination ${side} EVERY storage boundary`, async () => {
        for (const call of tracedRecovery.calls) {
          const stopped = await crash(entries, null, call.index, side, "recover");
          assertRecovered(stopped, phase === "prepared" ? prepared.before : prepared.after);
        }
      });
    }
  }
}
