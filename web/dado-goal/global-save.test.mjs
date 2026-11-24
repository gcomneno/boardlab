import assert from "node:assert/strict";
import test from "node:test";
import { Worker } from "node:worker_threads";
import { prepareGlobalSave, GlobalSaveError, GLOBAL_SAVE_SCHEMA_VERSION } from "./global-save.mjs";
import { preparePortableGlobalSaveImport, executePortableGlobalSaveImport, CommittedImportNeedsReload } from "./global-save-import.mjs";
import { newMatch, applyRoll } from "./engine.mjs";
import { createRollEvent } from "./roll-event.mjs";
import { createManagementDocument } from "./team-management-store.mjs";
import { createManagerDocument } from "./manager-store.mjs";
import { createTournamentDocument } from "./tournament-store.mjs";
import { createCupDocument } from "./cup-store.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";
import { completeFixture } from "./tournament.mjs";
import { createCupShootout, applyCupShootoutFace, replayCupShootout } from "./cup-shootout.mjs";
import { createMatchRecord } from "./team-history.mjs";
import { createCatalogueRoster } from "./catalogue-roster.mjs";
import { createManagerClub } from "./manager-profile.mjs";

const clone = value => structuredClone(value);
const teams = Array.from({ length: 32 }, (_, i) => ({
  provider: "fixture-provider", providerTeamId: `team-${i}`, name: `Team ${i}`,
  competitionId: "135", season: "2024", colors: ["#112233"], logo: "./team.svg",
  players: Array.from({ length: 20 }, (_, p) => ({
    provider: "fixture-provider", providerPlayerId: `player-${i}-${p}`, name: `Player ${p}`,
    position: p === 0 ? "Goalkeeper" : p < 5 ? "Defender" : p < 9 ? "Midfielder" : "Attacker",
    shirtNumber: p + 1,
  })),
}));
const teamFor = id => teams.find(t => t.providerTeamId === id);

function backup(legacy = false) {
  return {
    schemaVersion: 1, savedAt: "2026-11-22T12:00:00Z", locale: "it",
    configuration: {
      competition: "135:2024", hands: "1", randomHands: false, teams: ["", ""],
      colors: [0, 1].map(() => ({ primary: "#112233", secondaryEnabled: false, secondary: "#ffffff" })),
      commentaryEnabled: true,
      auto: { mode: "all", handLimit: "all", rollDelay: "0.7", handDelay: "0.4" },
    },
    persistent: {
      management: createManagementDocument(), manager: null, tournament: null,
      ...(legacy ? {} : { cup: null, cupArchive: [] }),
    },
    runtime: {
      state: null, names: ["Home", "Away"], colors: ["#112233", "#445566"],
      teamPalettes: [null, ["#445566", "#ffffff"]], rollEvents: [], matchEvents: [],
      rosters: teams.slice(0, 2).map(createCatalogueRoster), refereeStates: [{ yellowed: [] }, { yellowed: [] }],
      extraordinaryState: { occurredFamilies: [], occurredTypes: [] },
      lastCommentary: null, lastRefereeEvent: null, lastExtraordinaryEvent: null,
      lastGoalScorer: null, messageKey: "setupPrompt", messageValues: {}, matchStarted: false,
      selectedMatchTeams: [null, null], activeTournamentFixtureId: null,
      tournamentViewedMatchday: null, matchRecorded: false,
      ...(legacy ? {} : { competitionRef: null }),
    },
  };
}
function cup(id = "cup-1", size = 4, legs = 1) {
  return createCupDocument({ cupId: id, participants: teams.slice(0, size), legs, name: "Saved Cup" });
}
function match(fixture, matchId, home = 1, away = 0) {
  return clone(createMatchRecord({
    matchId, playedAt: "2026-11-22T12:00:00Z",
    home: teamFor(fixture.homeProviderTeamId), away: teamFor(fixture.awayProviderTeamId),
    homeGoals: home, awayGoals: away,
  }));
}
function linkRound(document, matches, tieFirst = false) {
  const round = Math.max(...document.fixtures.map(f => f.round));
  let next = document;
  for (const f of document.fixtures.filter(f => f.round === round && f.matchId === null)) {
    const id = `${document.cupId}:${f.fixtureId}`;
    // A reversed second leg is won by its away team, so the first-leg home wins.
    matches.push(match(f, id, tieFirst && f.pair === 1 ? 0 : f.leg === 1 ? 1 : 0,
      tieFirst && f.pair === 1 ? 0 : f.leg === 2 ? 1 : 0));
    next = completeCupFixture(next, f.fixtureId, id);
  }
  return next;
}
function completed(id = "completed", size = 4, legs = 1) {
  let document = cup(id, size, legs);
  const matches = [];
  while (!document.championProviderTeamId) {
    document = linkRound(document, matches);
    document = advanceCupIfReady(document, matches).cup;
  }
  return { document, matches };
}
function sessionBackup(outcomes = [], legs = 1) {
  const save = backup();
  const matches = save.persistent.management.matches;
  let document = linkRound(cup('cup-"tuple', 4, legs), matches, true);
  let session = createCupShootout(document, matches, 1, 1);
  for (const goal of outcomes) {
    session = applyCupShootoutFace(session, {
      attemptId: JSON.stringify([document.cupId, "r1-p1", session.attempts.length + 1]),
      expectedRevision: session.attempts.length, face: goal ? "GOAL!" : "FUORI",
    });
  }
  document.shootoutSessions = { "r1-p1": session };
  if (session.status === "complete") {
    document.shootouts = { "r1-p1": clone(replayCupShootout(session).goals) };
    document = advanceCupIfReady(document, matches, document.shootouts).cup;
  }
  save.persistent.cup = document;
  return save;
}
function fixtureRuntime(save, fixture, owner = save.persistent.cup ?? save.persistent.tournament) {
  const selected = [teamFor(fixture.homeProviderTeamId), teamFor(fixture.awayProviderTeamId)];
  save.configuration.teams = selected.map(t => t.providerTeamId);
  Object.assign(save.runtime, {
    state: newMatch(1), matchStarted: true, selectedMatchTeams: clone(selected),
    rosters: selected.map(createCatalogueRoster), activeTournamentFixtureId: fixture.fixtureId,
    competitionRef: owner.cupId ? { kind: "cup", cupId: owner.cupId, fixtureId: fixture.fixtureId } :
      { kind: "tournament", championshipId: owner.championshipId, fixtureId: fixture.fixtureId },
  });
}
function playRuntime(runtime, faces) {
  for (const face of faces) {
    runtime.rollEvents.push(createRollEvent({ seat: runtime.state.player, die: runtime.state.phase, face }));
    runtime.state = applyRoll(runtime.state, face);
  }
}
function rejectsMutation(name, mutate, base = backup) {
  test(name, () => {
    const save = base();
    mutate(save);
    const before = clone(save);
    assert.throws(() => prepareGlobalSave(save));
    assert.deepEqual(save, before);
  });
}

test("legacy V1 defaults, null idle state, and complete immutable destination", () => {
  const save = backup(true);
  delete save.configuration.randomHands;
  const plan = prepareGlobalSave(JSON.stringify(save));
  assert.equal(GLOBAL_SAVE_SCHEMA_VERSION, 1);
  assert.equal(plan.legacy, true);
  assert.equal(plan.document.schemaVersion, 1);
  assert.equal(plan.configuration.randomHands, false);
  assert.deepEqual(plan.runtime.state, newMatch(1));
  assert.equal(plan.runtime.competitionRef, null);
  assert.equal(plan.cup, null);
  assert.deepEqual(plan.cupArchive, []);
  assert.equal(plan.management, plan.document.persistent.management);
  assert.ok(Object.isFrozen(plan.runtime.rosters[0].identities["1"]));
  assert.equal(save.runtime.state, null);
  assert.ok(!Object.hasOwn(save.persistent, "cup"));
});

test("empty complete V1 remains complete on repeated preparation", () => {
  const plan = prepareGlobalSave(backup());
  assert.equal(plan.legacy, false);
  assert.deepEqual(prepareGlobalSave(plan.document).document, plan.document);
  assert.throws(() => { plan.runtime.refereeStates[0].yellowed.push(9); }, TypeError);
});

test("legacy ordinary runtime normalizes old rolls and preserves authoritative semantic kinds", () => {
  const save = backup(true);
  save.runtime.state = newMatch(1);
  save.runtime.matchStarted = true;
  playRuntime(save.runtime, ["Passaggio"]);
  save.runtime.rollEvents = [{ seat: 0, die: "team", face: "Passaggio" }];
  const plan = prepareGlobalSave(save);
  assert.equal(plan.runtime.rollEvents[0].kind, "pass");
  save.runtime.rollEvents[0] = { seat: 0, die: "team", face: "translated shot", kind: "pass", metadata: { retained: true } };
  const translated = prepareGlobalSave(save);
  assert.equal(translated.runtime.rollEvents[0].face, "translated shot");
  assert.equal(translated.runtime.rollEvents[0].kind, "pass");
  assert.ok(Object.isFrozen(translated.runtime.rollEvents[0].metadata));
});

test("legacy Manager nextMatch default, career-only history, and canonical mirror", () => {
  const save = backup(true);
  const record = match(cup().fixtures[0], "career");
  save.persistent.manager = createManagerDocument({ club: createManagerClub(teams[0]),
    management: createManagementDocument({ matches: [record] }) });
  delete save.persistent.manager.nextMatch;
  assert.equal(prepareGlobalSave(save).manager.nextMatch, null);
  save.persistent.management.matches.push(clone(record));
  const plan = prepareGlobalSave(save);
  assert.equal(plan.management.matches.length, 1);
  assert.equal(plan.manager.management.matches.length, 1);
  save.persistent.manager.management.matches[0].score.home++;
  assert.throws(() => prepareGlobalSave(save), /mirror/);
});

for (const size of [4, 8, 16, 32]) for (const legs of [1, 2]) {
  test(`preserves active, completed and archived ${size}-team ${legs}-leg Cup snapshots`, () => {
    const save = backup();
    save.persistent.cup = cup(`active-${size}-${legs}`, size, legs);
    save.persistent.cup.metadata = { label: "historical optional metadata", revision: 7 };
    assert.deepEqual(prepareGlobalSave(save).cup, save.persistent.cup);
    const done = completed(`archive-${size}-${legs}`, size, legs);
    save.persistent.management.matches = done.matches;
    save.persistent.cup = done.document;
    assert.deepEqual(prepareGlobalSave(save).cup, done.document);
    save.persistent.cup = null;
    save.persistent.cupArchive = [done.document];
    const plan = prepareGlobalSave(save);
    assert.deepEqual(plan.cupArchive, [done.document]);
    assert.deepEqual(plan.management.matches, done.matches);
    assert.deepEqual(prepareGlobalSave(plan.document).document, plan.document);
    assert.ok(!Object.hasOwn(plan.cupArchive[0], "shootoutSessions"));
  });
}

test("preserves archive order and independent active Cup", () => {
  const save = backup();
  const a = completed("z-first"), b = completed("a-second", 4, 2);
  save.persistent.cup = cup("current");
  save.persistent.cupArchive = [a.document, b.document];
  save.persistent.management.matches = [...a.matches, ...b.matches];
  assert.deepEqual(prepareGlobalSave(save).cupArchive.map(c => c.cupId), ["z-first", "a-second"]);
});

for (const [label, outcomes] of [
  ["zero", []], ["five", [1, 0, 1, 0, 1]], ["ten level", Array(10).fill(1)],
  ["odd sudden death", [...Array(10).fill(0), 1]],
  ["even sudden death", [...Array(10).fill(0), 1, 1]],
  ["terminal sudden death", [...Array(10).fill(1), 1, 0]],
  ["terminal tenth", [1, 1, 1, 1, 1, 1, 1, 1, 0, 1]],
  ["early clinch", [1, 0, 1, 0, 1, 0]],
  ["unequal terminal counts", [1, 0, 1, 0, 0, 0, 1]],
]) for (const legs of [1, 2]) {
  test(`preserves exact ${label} SP-01 session with ${legs} legs`, () => {
    const save = sessionBackup(outcomes, legs);
    const plan = prepareGlobalSave(save);
    assert.deepEqual(plan.cup, save.persistent.cup);
    assert.deepEqual(plan.management, save.persistent.management);
    assert.deepEqual(prepareGlobalSave(plan.document).document, plan.document);
    assert.deepEqual(plan.runtime.rollEvents, []);
    assert.deepEqual(plan.runtime.matchEvents, []);
  });
}

test("completed earlier-round session survives Cup completion and archive", () => {
  const save = sessionBackup([1, 0, 1, 0, 1, 0], 2);
  const original = clone(save.persistent.cup.shootoutSessions);
  save.persistent.cup = linkRound(save.persistent.cup, save.persistent.management.matches);
  save.persistent.cup = advanceCupIfReady(save.persistent.cup, save.persistent.management.matches,
    save.persistent.cup.shootouts).cup;
  const done = save.persistent.cup;
  save.persistent.cup = null;
  save.persistent.cupArchive = [done];
  const plan = prepareGlobalSave(save);
  assert.deepEqual(plan.cupArchive[0], done);
  assert.deepEqual(plan.cupArchive[0].shootoutSessions, original);
});

test("manual legacy shootout totals remain manual", () => {
  const save = sessionBackup();
  delete save.persistent.cup.shootoutSessions;
  save.persistent.cup.shootouts = { "r1-p1": { home: 4, away: 3 } };
  const plan = prepareGlobalSave(save);
  assert.deepEqual(plan.cup.shootouts, { "r1-p1": { home: 4, away: 3 } });
  assert.ok(!Object.hasOwn(plan.cup, "shootoutSessions"));
});

for (const [name, mutate] of [
  ["cup only", s => { delete s.persistent.cupArchive; }],
  ["archive only", s => { delete s.persistent.cup; }],
  ["null archive", s => { s.persistent.cupArchive = null; }],
  ["object archive", s => { s.persistent.cupArchive = {}; }],
  ["array current", s => { s.persistent.cup = []; }],
  ["false current", s => { s.persistent.cup = false; }],
  ["empty current", s => { s.persistent.cup = {}; }],
  ["future Cup version", s => { s.persistent.cup = { ...cup(), schemaVersion: 2 }; }],
  ["unfinished archive", s => { s.persistent.cupArchive = [cup()]; }],
  ["malformed name", s => { s.persistent.cup = { ...cup(), name: null }; }],
  ["malformed sessions", s => { s.persistent.cup = { ...cup(), shootoutSessions: null }; }],
  ["malformed results", s => { s.persistent.cup = { ...cup(), shootouts: [] }; }],
]) rejectsMutation(`rejects malformed Cup presence/content: ${name}`, mutate);

for (const [name, mutate] of [
  ["duplicate canonical IDs", s => { s.persistent.management.matches.push(clone(s.persistent.management.matches[0])); }],
  ["empty canonical ID", s => { s.persistent.management.matches[0].matchId = " "; }],
  ["missing match", s => { s.persistent.management.matches.shift(); }],
  ["wrong team", s => { s.persistent.management.matches[0].home.providerTeamId = "wrong"; }],
  ["swapped orientation", s => { const m = s.persistent.management.matches[0]; [m.home, m.away] = [m.away, m.home]; }],
  ["negative score", s => { s.persistent.management.matches[0].score.home = -1; }],
  ["unsafe score", s => { s.persistent.management.matches[0].score.home = Number.MAX_SAFE_INTEGER + 1; }],
  ["fractional score", s => { s.persistent.management.matches[0].score.home = 1.5; }],
  ["speculative match", s => { s.persistent.cup.fixtures[0].matchId = "future"; }],
  ["duplicate fixture match", s => { s.persistent.cup.fixtures[1].matchId = s.persistent.cup.fixtures[0].matchId; }],
  ["wrong fixture order", s => { s.persistent.cup.fixtures.reverse(); }],
  ["unknown bracket teams", s => { s.persistent.cup.fixtures[0].homeProviderTeamId = "wrong"; }],
]) rejectsMutation(`rejects canonical graph: ${name}`, mutate, () => sessionBackup());

test("rejects aggregate overflow even in a saved, unadvanced last round", () => {
  const save = sessionBackup([], 2);
  delete save.persistent.cup.shootoutSessions;
  for (const m of save.persistent.management.matches) m.score = { home: Number.MAX_SAFE_INTEGER, away: Number.MAX_SAFE_INTEGER };
  assert.throws(() => prepareGlobalSave(save), /overflow/);
});

test("rejects duplicate Cup identities and ownership across current/archive/tournament", () => {
  const done = completed();
  const save = backup();
  save.persistent.management.matches = done.matches;
  save.persistent.cupArchive = [done.document, clone(done.document)];
  assert.throws(() => prepareGlobalSave(save), /duplicate Cup/);
  save.persistent.cupArchive = [done.document];
  save.persistent.cup = clone(done.document);
  assert.throws(() => prepareGlobalSave(save), /duplicate Cup/);
  save.persistent.cup.cupId = "other";
  assert.throws(() => prepareGlobalSave(save), /multiply owned/);
  save.persistent.cup = null;
  save.persistent.tournament = createTournamentDocument({ championshipId: "league", participants: teams.slice(0, 4) });
  const f = save.persistent.tournament.fixtures.find(f => done.matches.some(m =>
    m.home.providerTeamId === f.homeProviderTeamId && m.away.providerTeamId === f.awayProviderTeamId));
  f.matchId = done.matches.find(m => m.home.providerTeamId === f.homeProviderTeamId && m.away.providerTeamId === f.awayProviderTeamId).matchId;
  assert.throws(() => prepareGlobalSave(save), /multiply owned/);
});

rejectsMutation("rejects simultaneous current competitions", s => {
  s.persistent.cup = cup();
  s.persistent.tournament = createTournamentDocument({ championshipId: "league", participants: teams.slice(0, 4) });
});

for (const [name, mutate] of [
  ["face", s => { s.attempts[0].face = "invalid"; }],
  ["kind", s => { s.attempts[0].kind = "penalty-wide"; }],
  ["attempt identity", s => { s.attempts[0].attemptId = "other"; }],
  ["future attempt", s => { s.attempts[0].index = 2; }],
  ["team order", s => { s.attempts[0].team = "away"; }],
  ["revision field", s => { s.revision = 1; }],
  ["rules", s => { s.rulesVersion = "future"; }],
  ["version", s => { s.schemaVersion = 2; }],
  ["fixture reference", s => { s.fixtureRefs[0].matchId = "other"; }],
  ["owner", s => { s.shootoutId = JSON.stringify(["other", "r1-p1"]); }],
  ["status", s => { s.status = "complete"; }],
  ["home identity", s => { s.homeProviderTeamId = "wrong"; }],
]) rejectsMutation(`rejects corrupt session ${name}`, save => mutate(save.persistent.cup.shootoutSessions["r1-p1"]),
  () => sessionBackup([1]));

test("rejects terminal history after clinch and inconsistent terminal totals, also in archive", () => {
  const save = sessionBackup([1, 0, 1, 0, 1, 0]);
  const session = save.persistent.cup.shootoutSessions["r1-p1"];
  session.attempts.push({ ...session.attempts[0], index: 7,
    attemptId: JSON.stringify([save.persistent.cup.cupId, "r1-p1", 7]) });
  assert.throws(() => prepareGlobalSave(save));
  session.attempts.pop();
  save.persistent.cup = advanceCupIfReady(linkRound(save.persistent.cup, save.persistent.management.matches),
    save.persistent.management.matches, save.persistent.cup.shootouts).cup;
  save.persistent.cupArchive = [save.persistent.cup];
  save.persistent.cup = null;
  save.persistent.cupArchive[0].shootouts["r1-p1"].home++;
  assert.throws(() => prepareGlobalSave(save));
});

test("rejects two unfinished sessions and sessions on ineligible regular scores", () => {
  const save = sessionBackup();
  const f = save.persistent.cup.fixtures[1];
  save.persistent.management.matches[1].score = { home: 0, away: 0 };
  const first = save.persistent.cup.shootoutSessions;
  delete save.persistent.cup.shootoutSessions;
  save.persistent.cup.shootouts = { "r1-p1": { home: 1, away: 0 } };
  const second = createCupShootout(save.persistent.cup, save.persistent.management.matches, 1, 2);
  delete save.persistent.cup.shootouts;
  save.persistent.cup.shootoutSessions = { ...first, "r1-p2": second };
  assert.equal(second.fixtureRefs[0].fixtureId, f.fixtureId);
  assert.throws(() => prepareGlobalSave(save));
  delete save.persistent.cup.shootoutSessions["r1-p2"];
  save.persistent.management.matches[0].score.home = 1;
  assert.throws(() => prepareGlobalSave(save));
});

for (const kind of ["cup", "tournament"]) {
  test(`validates explicit and inferred ${kind} runtime ownership`, () => {
    const save = backup(kind === "tournament");
    const owner = kind === "cup" ? cup() : createTournamentDocument({ championshipId: "league", participants: teams.slice(0, 4) });
    save.persistent[kind] = owner;
    fixtureRuntime(save, owner.fixtures[0], owner);
    const expected = clone(save.runtime.competitionRef);
    assert.deepEqual(prepareGlobalSave(save).runtime.competitionRef, expected);
    delete save.runtime.competitionRef;
    assert.deepEqual(prepareGlobalSave(save).runtime.competitionRef, expected);
    save.runtime.competitionRef = null;
    assert.throws(() => prepareGlobalSave(save), /contradictory/);
  });
}

for (const [name, mutate] of [
  ["wrong Cup", s => { s.runtime.competitionRef.cupId = "other"; }],
  ["wrong kind", s => { s.runtime.competitionRef.kind = "other"; }],
  ["empty owner", s => { s.runtime.competitionRef.cupId = ""; }],
  ["missing identity", s => { delete s.runtime.competitionRef.cupId; }],
  ["extra identity", s => { s.runtime.competitionRef.championshipId = "other"; }],
  ["missing fixture", s => { s.runtime.competitionRef.fixtureId = "missing"; }],
  ["projection mismatch", s => { s.runtime.activeTournamentFixtureId = "other"; }],
  ["runtime teams", s => { s.runtime.selectedMatchTeams.reverse(); }],
  ["configuration teams", s => { s.configuration.teams.reverse(); }],
  ["idle fixture", s => { s.runtime.matchStarted = false; }],
]) rejectsMutation(`rejects runtime competition ownership: ${name}`, mutate, () => {
  const s = backup(); s.persistent.cup = cup(); fixtureRuntime(s, s.persistent.cup.fixtures[0]); return s;
});

test("runtime cannot claim an archive or infer a Cup from a legacy backup", () => {
  const done = completed();
  const save = backup();
  save.persistent.cupArchive = [done.document];
  save.persistent.management.matches = done.matches;
  fixtureRuntime(save, done.document.fixtures[0], done.document);
  assert.throws(() => prepareGlobalSave(save));
  const old = backup(true);
  old.runtime.activeTournamentFixtureId = "r1-p1-l1";
  assert.throws(() => prepareGlobalSave(old), /unique current owner/);
});

test("recorded runtime agrees with exact fixture record; unrecorded runtime cannot claim it", () => {
  const save = backup(); save.persistent.cup = cup();
  const f = save.persistent.cup.fixtures[0];
  fixtureRuntime(save, f);
  playRuntime(save.runtime, ["Passaggio intercettato!", "Passaggio intercettato!"]);
  f.matchId = "runtime-match";
  save.persistent.management.matches = [match(f, f.matchId, 0, 0)];
  assert.throws(() => prepareGlobalSave(save), /already recorded/);
  save.runtime.matchRecorded = true;
  assert.equal(prepareGlobalSave(save).runtime.matchRecorded, true);
  save.persistent.management.matches[0].score.home = 1;
  assert.throws(() => prepareGlobalSave(save), /canonical finished match/);
});

test("championship canonical associations cannot use career-only records or mismatched teams", () => {
  const save = backup(true);
  const t = createTournamentDocument({ championshipId: "league", participants: teams.slice(0, 4) });
  const m = match(t.fixtures[0], "league-match");
  save.persistent.tournament = { schemaVersion: 1, ...completeFixture(t, t.fixtures[0].fixtureId, m.matchId) };
  save.persistent.manager = createManagerDocument({ club: createManagerClub(teams[0]),
    management: createManagementDocument({ matches: [m] }) });
  assert.throws(() => prepareGlobalSave(save), /missing/);
  save.persistent.management.matches = [clone(m)];
  assert.deepEqual(prepareGlobalSave(save).tournament, save.persistent.tournament);
  save.persistent.management.matches[0].away = teams[1];
  assert.throws(() => prepareGlobalSave(save));
});

for (const [name, mutate] of [
  ["schema", s => { s.schemaVersion = 2; }], ["locale", s => { s.locale = "xx"; }],
  ["hands", s => { s.configuration.hands = "NaN"; }],
  ["randomHands", s => { s.configuration.randomHands = null; }],
  ["commentary flag", s => { s.configuration.commentaryEnabled = 1; }],
  ["auto mode", s => { s.configuration.auto.mode = "arbitrary"; }],
  ["auto delay", s => { s.configuration.auto.rollDelay = "-1"; }],
  ["unknown competition", s => { s.configuration.competition = "unknown"; }],
  ["unresolved catalogue", s => { s.configuration.teams[0] = "missing"; }],
  ["seat array", s => { s.runtime.names.pop(); }],
  ["color", s => { s.runtime.colors[0] = "bad"; }],
  ["palette", s => { s.runtime.teamPalettes[0] = []; }],
  ["roster shape", s => { s.runtime.rosters[0].active = null; }],
  ["roster overlap", s => { s.runtime.rosters[0].bench.push(s.runtime.rosters[0].active[0]); }],
  ["roster identity", s => { delete s.runtime.rosters[0].identities["1"]; }],
  ["referee shape", s => { s.runtime.refereeStates[0].yellowed = {}; }],
  ["referee duplicates", s => { s.runtime.refereeStates[0].yellowed = [1, 1]; }],
  ["extraordinary shape", s => { s.runtime.extraordinaryState.occurredTypes = null; }],
  ["extraordinary family", s => { s.runtime.extraordinaryState.occurredFamilies = ["bogus"]; }],
  ["extraordinary inconsistency", s => { s.runtime.extraordinaryState.occurredTypes = ["heavy-rain"]; }],
  ["phase", s => { s.runtime.state = { ...newMatch(1), phase: "future" }; }],
  ["unsafe number", s => { s.runtime.state = { ...newMatch(1), handsPerTeam: Number.MAX_SAFE_INTEGER }; }],
  ["negative goals", s => { s.runtime.state = { ...newMatch(1), goals: [-1, 0] }; }],
  ["boolean", s => { s.runtime.matchRecorded = "false"; }],
  ["idle recorded", s => { s.runtime.matchRecorded = true; }],
  ["match event", s => { s.runtime.matchEvents = [{ type: "goal", team: 3, minute: -1 }]; }],
  ["last event", s => { s.runtime.lastRefereeEvent = {}; }],
  ["commentary", s => { s.runtime.lastCommentary = { index: 0 }; }],
  ["message values", s => { s.runtime.messageValues = []; }],
  ["goal scorer", s => { s.runtime.lastGoalScorer = -1; }],
  ["viewed matchday", s => { s.runtime.tournamentViewedMatchday = 99; }],
  ["storage map", s => { s.persistent.storage = {}; }],
]) rejectsMutation(`rejects malformed envelope/hydration: ${name}`, mutate);

test("validates engine history without an independent match engine", () => {
  const save = backup(); save.runtime.state = newMatch(1); save.runtime.matchStarted = true;
  playRuntime(save.runtime, ["Tiro in porta!", "Colpo di testa", "RIGORE", "GOAL!", "Passaggio intercettato!"]);
  assert.deepEqual(prepareGlobalSave(save).runtime.state, save.runtime.state);
  save.runtime.rollEvents[1] = { ...save.runtime.rollEvents[1], seat: 1 };
  assert.throws(() => prepareGlobalSave(save), /engine prefix/);
  save.runtime.rollEvents[1] = { ...save.runtime.rollEvents[1], seat: 0 };
  save.runtime.state.goals[0] = 0;
  assert.throws(() => prepareGlobalSave(save), /engine history/);
});

test("configuration resolves portable snapshots or supplied detached catalogues without fetching", () => {
  const save = backup(); save.configuration.teams = [teams[0].providerTeamId, teams[1].providerTeamId];
  const plan = prepareGlobalSave(save, { catalogueTeams: teams });
  assert.deepEqual(plan.configurationTeams, teams.slice(0, 2));
  assert.ok(Object.isFrozen(plan.configurationTeams[0].players[0]));
  assert.ok(!Object.isFrozen(teams[0]));
  save.runtime.selectedMatchTeams = clone(teams.slice(0, 2));
  assert.deepEqual(prepareGlobalSave(save).configurationTeams, teams.slice(0, 2));
});

test("legacy local archive retention verifies both source and incoming canonical history", () => {
  const done = completed();
  const localState = { cup: null, cupArchive: [done.document], management: createManagementDocument({ matches: done.matches }) };
  const save = backup(true); save.persistent.management.matches = clone(done.matches);
  const before = clone(localState);
  const plan = prepareGlobalSave(save, { localState });
  assert.deepEqual(plan.cupArchive, [done.document]);
  assert.deepEqual(localState, before);
  save.persistent.management.matches.pop();
  assert.throws(() => prepareGlobalSave(save, { localState }));
});

test("legacy current Cup blocks, including a completed unarchived Cup", () => {
  for (const local of [{ document: cup(), matches: [] }, completed()]) {
    const localState = { cup: local.document, cupArchive: [], management: createManagementDocument({ matches: local.matches }) };
    assert.throws(() => prepareGlobalSave(backup(true), { localState }), /blocked by a current Cup/);
  }
});

test("complete import replaces valid local Cup/archive instead of merging", () => {
  const done = completed();
  const localState = { cup: cup("local"), cupArchive: [done.document], management: createManagementDocument({ matches: done.matches }) };
  const plan = prepareGlobalSave(backup(), { localState });
  assert.equal(plan.cup, null);
  assert.deepEqual(plan.cupArchive, []);
  assert.equal(localState.cup.cupId, "local");
  assert.equal(localState.cupArchive.length, 1);
});

for (const [name, localState] of [
  ["missing explicit state", {}], ["null current", { cup: false, cupArchive: [] }],
  ["malformed archive", { cup: null, cupArchive: {} }],
  ["invalid retained history", { cup: null, cupArchive: [completed().document] }],
  ["null management", { cup: null, cupArchive: [], management: null }],
  ...["readError", "pendingAttempt", "cupRecovery", "tournamentRecovery", "globalTransaction"].map(key =>
    [key, { cup: null, cupArchive: [], [key]: { blocked: true } }]),
]) test(`blocks invalid/unresolved local state for legacy and complete: ${name}`, () => {
  for (const legacy of [true, false]) assert.throws(() => prepareGlobalSave(backup(legacy), { localState }));
});

test("preparation is detached, deterministic and uses no storage, randomness, clock, timers or network", { timeout: 10000 }, async () => {
  const save = sessionBackup([...Array(10).fill(1), 1, 0], 2);
  const options = { localState: { cup: null, cupArchive: [] }, catalogueTeams: teams.slice(0, 2) };
  // Isolate capability traps from node:test's own timer/Date machinery.
  const script = `
  "use strict";
  const { workerData } = require("node:worker_threads");
  (async () => {
  const assert = (await import("node:assert/strict")).default;
  const { prepareGlobalSave } = await import(workerData.moduleUrl);
  const { save, options } = workerData;
  const before = structuredClone({ save, options });
  const descriptors = new Map();
  const calls = [];
  const blocked = name => () => { calls.push(name); throw new Error("unexpected " + name); };
  for (const key of ["localStorage", "fetch", "setTimeout", "setInterval", "Date", "crypto"]) {
    descriptors.set(key, Object.getOwnPropertyDescriptor(globalThis, key));
    const replacement = key === "localStorage" ?
      { getItem: blocked(key), setItem: blocked(key), removeItem: blocked(key) } :
      key === "Date" ? new Proxy(Date, { apply: blocked(key), construct: blocked(key),
        get: (target, property) => property === "now" ? blocked("Date.now") : Reflect.get(target, property) }) : blocked(key);
    Object.defineProperty(globalThis, key, { configurable: true, value: replacement });
  }
  const originalRandom = Math.random;
  Math.random = blocked("Math.random");
  try {
    const a = prepareGlobalSave(save, options);
    const b = prepareGlobalSave(JSON.stringify(save), options);
    assert.deepEqual(a, b);
    assert.throws(() => prepareGlobalSave({ ...save, schemaVersion: 9 }, options));
    assert.deepEqual(a.cup.shootoutSessions, save.persistent.cup.shootoutSessions);
    assert.deepEqual(calls, []);
    assert.deepEqual({ save, options }, before);
    assert.notEqual(a.cup, save.persistent.cup);
    assert.throws(() => { a.cup.fixtures[0].matchId = "changed"; }, TypeError);
  } finally {
    Math.random = originalRandom;
    for (const [key, descriptor] of descriptors) {
      if (descriptor) Object.defineProperty(globalThis, key, descriptor);
      else delete globalThis[key];
    }
  }
  })();`;
  const worker = new Worker(script, {
    eval: true, workerData: { save, options, moduleUrl: new URL("./global-save.mjs", import.meta.url).href },
    env: Object.fromEntries(Object.entries(process.env).filter(([key]) => key !== "NODE_TEST_CONTEXT")),
  });
  await new Promise((resolve, reject) => {
    worker.once("error", reject);
    worker.once("exit", code => code === 0 ? resolve() : reject(new Error(`capability proof exited ${code}`)));
  });
});

test("plain object input rejects non-JSON values without invoking getters or coercion", () => {
  for (const value of [undefined, NaN, Infinity, new Set(), new Date(), () => {}]) {
    const save = backup(); save.persistent.cup = value;
    assert.throws(() => prepareGlobalSave(save));
  }
  const save = backup();
  Object.defineProperty(save, "dangerous", { enumerable: true, get() { assert.fail("getter invoked"); } });
  assert.throws(() => prepareGlobalSave(save), GlobalSaveError);
  const cyclic = backup(); cyclic.loop = cyclic;
  assert.throws(() => prepareGlobalSave(cyclic));
  assert.throws(() => prepareGlobalSave("{broken"));
});

test("Manager coexists with a Cup or tournament without claiming fixture ownership", () => {
  for (const kind of ["cup", "tournament"]) {
    const save = backup();
    save.persistent[kind] = kind === "cup" ? cup() :
      createTournamentDocument({ championshipId: "league", participants: teams.slice(0, 4) });
    save.persistent.manager = createManagerDocument({ club: createManagerClub(teams[0]) });
    assert.deepEqual(prepareGlobalSave(save).manager, save.persistent.manager);
    save.persistent.manager = createManagerDocument({ club: createManagerClub(teams[6]) });
    assert.deepEqual(prepareGlobalSave(save).manager, save.persistent.manager);
  }
});

for (const [name, mutate] of [
  ["unknown schema", m => { m.schemaVersion = 2; }],
  ["null next match", m => { m.nextMatch = false; }],
  ["unknown player", m => { m.club.lineup[0] = "missing"; }],
  ["duplicate roster identity", m => { m.club.players[1].providerPlayerId = m.club.players[0].providerPlayerId; }],
  ["invalid player", m => { m.club.players[0].name = null; }],
  ["invalid ratings", m => { m.management.ratings = [{ attack: null }]; }],
  ["null nested management", m => { m.management = null; }],
]) rejectsMutation(`rejects malformed Manager: ${name}`, s => mutate(s.persistent.manager), () => {
  const s = backup(); s.persistent.manager = createManagerDocument({ club: createManagerClub(teams[0]) }); return s;
});

for (const [name, mutate] of [
  ["face", session => { session.attempts[0].face = "wrong"; }],
  ["identity", session => { session.attempts[0].attemptId = "wrong"; }],
  ["rules", session => { session.rulesVersion = "future"; }],
  ["fixture refs", session => { session.fixtureRefs[0].fixtureId = "wrong"; }],
  ["status", session => { session.status = "initial"; }],
]) rejectsMutation(`rejects archived session corruption: ${name}`, s =>
  mutate(s.persistent.cupArchive[0].shootoutSessions["r1-p1"]), () => {
  const s = sessionBackup([1, 0, 1, 0, 1, 0]);
  const linked = linkRound(s.persistent.cup, s.persistent.management.matches);
  s.persistent.cupArchive = [advanceCupIfReady(linked, s.persistent.management.matches, linked.shootouts).cup];
  s.persistent.cup = null;
  return s;
});

test("valid full hydration inputs preserve roster identities, auxiliary events and commentary", () => {
  const save = backup();
  save.runtime.state = newMatch(1); save.runtime.matchStarted = true;
  save.runtime.selectedMatchTeams = clone(teams.slice(0, 2));
  save.configuration.teams = teams.slice(0, 2).map(t => t.providerTeamId);
  const before = clone(save.runtime.state);
  playRuntime(save.runtime, ["Passaggio intercettato!"]);
  const yellow = { type: "yellow", team: 0, minute: 45, number: 1, reason: "dissent",
    player: clone(save.runtime.rosters[0].identities[1]) };
  save.runtime.matchEvents.push(yellow);
  save.runtime.lastRefereeEvent = yellow;
  save.runtime.refereeStates[0].yellowed = [1];
  save.runtime.extraordinaryState = { occurredFamilies: ["weather"], occurredTypes: ["heavy-rain"] };
  save.runtime.lastExtraordinaryEvent = {
    type: "heavy-rain", family: "weather", classification: "narrative", team: 0, minute: 45,
  };
  save.runtime.lastCommentary = { event: { seat: 0, die: "team", face: "Passaggio intercettato!" },
    before, after: clone(save.runtime.state), names: save.runtime.names, index: 0 };
  assert.deepEqual(prepareGlobalSave(save).runtime, save.runtime);
  save.runtime.lastCommentary.after = newMatch(1);
  assert.throws(() => prepareGlobalSave(save), /commentary states/);
});

test("ordinary real-team legacy matches and optional historical record fields remain intact", () => {
  const save = backup(true);
  const historical = match(cup().fixtures[0], "historical");
  for (const key of ["schemaVersion", "rollEvents", "participation", "statistics", "ratings"]) delete historical[key];
  historical.note = { imported: "older V1" };
  save.persistent.management.matches.push(historical);
  save.runtime.selectedMatchTeams = clone(teams.slice(0, 2));
  save.configuration.teams = teams.slice(0, 2).map(t => t.providerTeamId);
  assert.deepEqual(prepareGlobalSave(save).management.matches[0], historical);
});

test("fallback catalogue resolves controls whose historical match snapshots omit players", () => {
  const save = backup();
  const historical = match(cup().fixtures[0], "historical");
  delete historical.home.players;
  delete historical.away.players;
  save.persistent.management.matches.push(historical);
  save.configuration.teams = [historical.home.providerTeamId, historical.away.providerTeamId];
  assert.throws(() => prepareGlobalSave(save), /unresolved team snapshot/);
  assert.deepEqual(prepareGlobalSave(save, { catalogueTeams: teams }).configurationTeams, teams.slice(0, 2));
});

for (const [name, mutate] of [
  ["too few selected players", s => { s.runtime.selectedMatchTeams[0].players = []; }],
  ["foreign roster identity", s => { s.runtime.rosters[0].identities[1].providerPlayerId = "foreign"; }],
  ["duplicate roster identity", s => { s.runtime.rosters[0].identities[2] = clone(s.runtime.rosters[0].identities[1]); }],
  ["unknown yellowed player", s => { s.runtime.refereeStates[0].yellowed = [999]; }],
  ["unknown roll kind", s => { s.runtime.rollEvents[0] = { seat: 0, die: "team", face: "Passaggio", kind: "unknown" }; }],
  ["empty roll face", s => { s.runtime.rollEvents[0] = { seat: 0, die: "team", face: "", kind: "pass" }; }],
  ["hidden active match", s => { s.runtime.matchStarted = false; }],
]) rejectsMutation(`rejects corrupt active hydration: ${name}`, mutate, () => {
  const s = backup(); s.runtime.state = newMatch(1); s.runtime.matchStarted = true;
  s.runtime.selectedMatchTeams = clone(teams.slice(0, 2));
  s.configuration.teams = teams.slice(0, 2).map(t => t.providerTeamId);
  playRuntime(s.runtime, ["Passaggio"]);
  return s;
});

for (const [name, mutate] of [
  ["wrong champion", c => { c.championProviderTeamId = "team-1"; }],
  ["missing fixture", c => { c.fixtures.pop(); }],
  ["future round teams", c => { c.fixtures.at(-1).homeProviderTeamId = "team-3"; }],
]) rejectsMutation(`rejects corrupt completed bracket: ${name}`, s => mutate(s.persistent.cup), () => {
  const done = completed(); const s = backup();
  s.persistent.cup = done.document; s.persistent.management.matches = done.matches;
  return s;
});

test("retains historical minimal competition participant snapshots accepted by domain validators", () => {
  const save = backup(true);
  save.persistent.tournament = createTournamentDocument({ championshipId: "legacy-league",
    participants: teams.slice(0, 4).map(t => ({ providerTeamId: t.providerTeamId })) });
  assert.deepEqual(prepareGlobalSave(save).tournament, save.persistent.tournament);
  const complete = backup();
  complete.persistent.cup = createCupDocument({ cupId: "minimal",
    participants: teams.slice(0, 4).map(t => ({ providerTeamId: t.providerTeamId })) });
  assert.deepEqual(prepareGlobalSave(complete).cup, complete.persistent.cup);
});


// SP-02E2B — pure transactional import preparation.
const importSnapshot = (persistent = backup().persistent, overrides = {}) => {
  const values = {
    "dado-goal.team-management.v1": JSON.stringify(persistent.management),
    "dado-goal.manager.v1": persistent.manager === null
      ? null : JSON.stringify(persistent.manager),
    "dado-goal.tournament.v1": persistent.tournament === null
      ? null : JSON.stringify(persistent.tournament),
    "dado-goal.tournament-recovery.v1": null,
    "dado-goal.cup.v1": persistent.cup === null
      ? null : JSON.stringify(persistent.cup),
    "dado-goal.cup-archive.v1": JSON.stringify(persistent.cupArchive),
    "dado-goal.cup-recovery.v1": null,
    "dado-goal.global-save-generation.v1": "7",
    ...overrides,
  };
  return { generation: 7, journal: null, images: values };
};

test("SP-02 import planner replaces current Cup without merging", () => {
  const local = backup();
  local.persistent.cup = cup("old-cup");
  const incoming = backup();
  incoming.persistent.cup = cup("new-cup");

  const snapshot = importSnapshot(local.persistent);
  const before = clone(snapshot);
  const inputBefore = clone(incoming);
  const planned = preparePortableGlobalSaveImport(
    snapshot, incoming, "replace-cup",
  );

  assert.equal(planned.request.operation, "import");
  assert.equal(planned.request.expectedGeneration, 7);
  assert.equal(planned.request.transactionId, "replace-cup");
  assert.equal(
    JSON.parse(planned.request.changes["dado-goal.cup.v1"]).cupId,
    "new-cup",
  );
  assert.equal(
    planned.request.changes["dado-goal.cup-recovery.v1"], null,
  );
  assert.deepEqual(planned.prepared.cup, incoming.persistent.cup);
  assert.deepEqual(snapshot, before);
  assert.deepEqual(incoming, inputBefore);
});

test("SP-02 legacy import retains compatible local archive", () => {
  const archived = completed("legacy-archive");
  const local = backup();
  local.persistent.management.matches = clone(archived.matches);
  local.persistent.cupArchive = [archived.document];

  const incoming = backup(true);
  incoming.persistent.management.matches = clone(archived.matches);

  const planned = preparePortableGlobalSaveImport(
    importSnapshot(local.persistent),
    incoming,
    "legacy-archive-retention",
  );

  assert.equal(planned.prepared.legacy, true);
  assert.deepEqual(planned.prepared.cupArchive, [archived.document]);
  assert.deepEqual(
    JSON.parse(planned.request.changes["dado-goal.cup-archive.v1"]),
    [archived.document],
  );
});

test("SP-02 import planner blocks unresolved recovery journals", () => {
  const snapshot = importSnapshot(backup().persistent, {
    "dado-goal.cup-recovery.v1": '{"pending":true}',
  });
  const before = clone(snapshot);

  assert.throws(() =>
    preparePortableGlobalSaveImport(snapshot, backup(), "blocked"),
  );
  assert.deepEqual(snapshot, before);
});

test("SP-02 invalid import preserves original inputs", () => {
  const incoming = backup();
  incoming.persistent.cup = cup("invalid-source");
  incoming.persistent.cupArchive = [
    clone(incoming.persistent.cup),
  ];

  const snapshot = importSnapshot();
  const previousSnapshot = clone(snapshot);
  const previousInput = clone(incoming);

  assert.throws(() =>
    preparePortableGlobalSaveImport(snapshot, incoming, "invalid"),
  );
  assert.deepEqual(snapshot, previousSnapshot);
  assert.deepEqual(incoming, previousInput);
});


test("SP-02 import planner reconciles canonical Cup journal on copies", () => {
  const local = backup();
  const current = cup("recoverable-cup");
  const fixture = current.fixtures[0];
  const canonical = match(fixture, "recoverable-match");

  local.persistent.cup = current;
  local.persistent.management.matches = [canonical];

  const journal = {
    cupId: current.cupId,
    fixtureId: fixture.fixtureId,
    matchId: canonical.matchId,
  };
  const snapshot = importSnapshot(local.persistent, {
    "dado-goal.cup-recovery.v1": JSON.stringify(journal),
  });
  const before = clone(snapshot);

  const incoming = backup();
  incoming.persistent.cup = cup("replacement-cup");

  const planned = preparePortableGlobalSaveImport(
    snapshot, incoming, "resolved-recovery",
  );

  assert.equal(planned.prepared.cup.cupId, "replacement-cup");
  assert.equal(planned.request.changes["dado-goal.cup-recovery.v1"], null);
  assert.deepEqual(snapshot, before);
});


test("SP-02 recovery already-linked Cup journal is idempotent", () => {
  const local = backup();
  const current = cup("already-linked");
  const fixture = current.fixtures[0];
  const record = match(fixture, "linked-match");

  local.persistent.management.matches = [record];
  local.persistent.cup = completeCupFixture(
    current, fixture.fixtureId, record.matchId,
  );

  const snapshot = importSnapshot(local.persistent, {
    "dado-goal.cup-recovery.v1": JSON.stringify({
      cupId: current.cupId,
      fixtureId: fixture.fixtureId,
      matchId: record.matchId,
    }),
  });
  const before = clone(snapshot);

  const planned = preparePortableGlobalSaveImport(
    snapshot, backup(), "already-linked-import",
  );

  assert.equal(planned.request.operation, "import");
  assert.deepEqual(snapshot, before);
});

test("SP-02 recovery rejects a missing canonical match without mutation", () => {
  const local = backup();
  const current = cup("missing-match");
  local.persistent.cup = current;

  const snapshot = importSnapshot(local.persistent, {
    "dado-goal.cup-recovery.v1": JSON.stringify({
      cupId: current.cupId,
      fixtureId: current.fixtures[0].fixtureId,
      matchId: "nonexistent",
    }),
  });
  const before = clone(snapshot);

  assert.throws(() =>
    preparePortableGlobalSaveImport(snapshot, backup(), "missing-recovery"),
  );
  assert.deepEqual(snapshot, before);
});

test("SP-02 recovery rejects incompatible Cup fixture ownership", () => {
  const local = backup();
  const current = cup("wrong-fixture");
  const first = current.fixtures[0];
  const second = current.fixtures[1];
  const record = match(first, "wrong-fixture-match");

  local.persistent.cup = current;
  local.persistent.management.matches = [record];

  const snapshot = importSnapshot(local.persistent, {
    "dado-goal.cup-recovery.v1": JSON.stringify({
      cupId: current.cupId,
      fixtureId: second.fixtureId,
      matchId: record.matchId,
    }),
  });
  const before = clone(snapshot);

  assert.throws(() =>
    preparePortableGlobalSaveImport(snapshot, backup(), "wrong-owner"),
  );
  assert.deepEqual(snapshot, before);
});

test("SP-02 recovery resolves a canonical championship fixture on copies", () => {
  const local = backup();
  const championship = createTournamentDocument({
    championshipId: "recovery-championship",
    participants: teams.slice(0, 4),
    name: "Recovery Championship",
  });
  const fixture = championship.fixtures[0];
  const record = match(fixture, "championship-recovery-match");

  local.persistent.tournament = championship;
  local.persistent.management.matches = [record];

  const snapshot = importSnapshot(local.persistent, {
    "dado-goal.tournament-recovery.v1": JSON.stringify({
      championshipId: championship.championshipId,
      fixtureId: fixture.fixtureId,
      matchId: record.matchId,
    }),
  });
  const before = clone(snapshot);

  const planned = preparePortableGlobalSaveImport(
    snapshot, backup(), "championship-recovery-import",
  );

  assert.equal(planned.request.operation, "import");
  assert.equal(
    planned.request.changes["dado-goal.tournament-recovery.v1"],
    null,
  );
  assert.deepEqual(snapshot, before);
});


test("SP-02 import executor commits through the coordinator", async () => {
  const snapshot = importSnapshot();
  const calls = [];
  const ownership = Object.freeze({});
  const options = { ownership };
  const receipt = { status: "committed", usable: true, generation: 8 };
  const coordinator = {
    async read(project, opts) {
      calls.push(["read", project, opts]);
      return snapshot;
    },
    async transact(expected, request, opts) {
      calls.push(["transact", expected, request, opts]);
      return receipt;
    },
  };

  const result = await executePortableGlobalSaveImport(
    coordinator, backup(), "coordinated-import", options,
  );

  assert.equal(result.receipt, receipt);
  assert.equal(result.prepared.cup, null);
  assert.equal(calls.length, 2);
  assert.equal(calls[0][0], "read");
  assert.equal(calls[0][2], options);
  assert.equal(calls[1][1], snapshot);
  assert.equal(calls[1][2].transactionId, "coordinated-import");
  assert.equal(calls[1][3], options);
});

test("SP-02 committed import with pending cleanup requires reload", async () => {
  const snapshot = importSnapshot();
  const calls = [];
  const coordinator = {
    async read() { return snapshot; },
    async transact() {
      calls.push("transact");
      return { status: "committed", usable: false, cleanupPending: true };
    },
    async recover() {
      calls.push("recover");
      return snapshot;
    },
  };

  await assert.rejects(
    executePortableGlobalSaveImport(
      coordinator, backup(), "cleanup-pending",
    ),
    error => error instanceof CommittedImportNeedsReload &&
      error.committed === true,
  );
  assert.deepEqual(calls, ["transact", "recover"]);
});


test("SP-02 verified committed import permits hydration", async () => {
  const snapshot = importSnapshot();
  let committedRequest;
  let recovered = false;
  const coordinator = {
    async read() { return snapshot; },
    async transact(expected, request) {
      assert.equal(expected, snapshot);
      committedRequest = request;
      return {
        status: "committed",
        generation: 8,
        usable: false,
        cleanupPending: true,
      };
    },
    async recover() {
      recovered = true;
      return {
        generation: 8,
        journal: null,
        images: {
          ...snapshot.images,
          ...committedRequest.changes,
          "dado-goal.global-save-generation.v1": "8",
        },
      };
    },
  };

  const result = await executePortableGlobalSaveImport(
    coordinator, backup(), "verified-recovery",
  );

  assert.equal(recovered, true);
  assert.equal(result.receipt.status, "committed");
  assert.equal(result.receipt.usable, true);
  assert.equal(result.receipt.cleanupPending, false);
  assert.equal(result.receipt.recovered, true);
  assert.deepEqual(result.prepared.document.persistent,
    prepareGlobalSave(backup()).document.persistent);
});

test("SP-02 divergent recovered generation prohibits hydration", async () => {
  const snapshot = importSnapshot();
  let committedRequest;
  const coordinator = {
    async read() { return snapshot; },
    async transact(expected, request) {
      committedRequest = request;
      return {
        status: "committed",
        generation: 8,
        usable: false,
        cleanupPending: true,
      };
    },
    async recover() {
      return {
        generation: 9,
        journal: null,
        images: {
          ...snapshot.images,
          ...committedRequest.changes,
          "dado-goal.global-save-generation.v1": "9",
        },
      };
    },
  };

  await assert.rejects(
    executePortableGlobalSaveImport(
      coordinator, backup(), "divergent-recovery",
    ),
    error => error instanceof CommittedImportNeedsReload &&
      error.committed === true,
  );
});
