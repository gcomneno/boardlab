import assert from "node:assert/strict";
import test from "node:test";
import { createCup, completeCupFixture } from "./cup.mjs";
import {
  createCupShootout,
  replayCupShootout,
  applyCupShootoutFace,
} from "./cup-shootout.mjs";

function tiedCup(legs = 1) {
  let cup = { schemaVersion: 1, ...createCup({
    cupId: 'cup-"tuple', legs,
    participants: ["A", "B", "C", "D"].map(providerTeamId => ({ providerTeamId })),
  }) };
  const matches = [];
  for (const fixture of cup.fixtures) {
    const matchId = `match-${fixture.fixtureId}`;
    matches.push({
      matchId,
      home: { providerTeamId: fixture.homeProviderTeamId },
      away: { providerTeamId: fixture.awayProviderTeamId },
      score: fixture.pair === 1 ? { home: 2, away: legs === 2 ? 0 : 2 } :
        { home: fixture.leg === 1 ? 1 : 0, away: 0 },
    });
    cup = completeCupFixture(cup, fixture.fixtureId, matchId);
  }
  return { cup, matches };
}

function initial(legs) {
  const { cup, matches } = tiedCup(legs);
  return createCupShootout(cup, matches, 1, 1);
}

function kick(session, goal) {
  return applyCupShootoutFace(session, {
    attemptId: JSON.stringify([...JSON.parse(session.shootoutId), session.attempts.length + 1]),
    expectedRevision: session.attempts.length,
    face: goal ? "GOAL!" : "FUORI",
  });
}

function play(outcomes, session = initial()) {
  for (const goal of outcomes) session = kick(session, goal);
  return session;
}

test("initial session freezes first-leg identity, tuple IDs and alternating turns", () => {
  const { cup, matches } = tiedCup(2);
  const before = structuredClone({ cup, matches });
  const session = createCupShootout(cup, matches, 1, 1);
  assert.deepEqual(session.fixtureRefs, [
    { fixtureId: "r1-p1-l1", matchId: "match-r1-p1-l1" },
    { fixtureId: "r1-p1-l2", matchId: "match-r1-p1-l2" },
  ]);
  assert.equal(session.homeProviderTeamId, "A");
  assert.equal(session.awayProviderTeamId, "B");
  assert.deepEqual(replayCupShootout(session), {
    status: "initial", attemptsByTeam: { home: 0, away: 0 },
    goals: { home: 0, away: 0 }, nextTeam: "home", winner: null,
  });
  const first = kick(session, true);
  assert.equal(replayCupShootout(first).nextTeam, "away");
  const second = kick(first, false);
  assert.deepEqual(second.attempts.map(a => a.team), ["home", "away"]);
  assert.equal(session.attempts.length, 0);
  assert.equal(first.attempts.length, 1);
  assert.deepEqual({ cup, matches }, before);
  assert.deepEqual(replayCupShootout(JSON.parse(JSON.stringify(second))), replayCupShootout(second));
});

test("five rounds can end at the tenth attempt with an away winner", () => {
  const session = play([1, 1, 1, 1, 1, 1, 1, 1, 0, 1]);
  assert.deepEqual(replayCupShootout(session), {
    status: "complete", attemptsByTeam: { home: 5, away: 5 },
    goals: { home: 4, away: 5 }, nextTeam: null, winner: "away",
  });
});

test("early wins at six attempts work in either direction", () => {
  for (const homeWins of [true, false]) {
    const session = play(Array.from({ length: 6 }, (_, i) => (i % 2 === 0) === homeWins));
    assert.equal(session.status, "complete");
    assert.equal(replayCupShootout(session).winner, homeWins ? "home" : "away");
    assert.throws(() => kick(session, true), { code: "CUP_SHOOTOUT_DECIDED" });
  }
});

test("early termination permits seven attempts; recoverable equality still continues", () => {
  const six = play([1, 0, 1, 0, 0, 0]);
  assert.equal(six.status, "initial"); // 2-0: two remaining away shots can tie.
  const seven = kick(six, true);
  assert.equal(seven.status, "complete");
  assert.deepEqual(replayCupShootout(seven).attemptsByTeam, { home: 4, away: 3 });
  const away = play([0, 1, 0, 1, 0, 0, 0]);
  assert.equal(replayCupShootout(away).winner, "away");
});

test("0-0 and 5-5 initial ties enter sudden death with home next", () => {
  for (const goal of [false, true]) {
    const session = play(Array(10).fill(goal));
    assert.equal(session.status, "sudden-death");
    assert.equal(replayCupShootout(session).nextTeam, "home");
    assert.deepEqual(replayCupShootout(session).goals, { home: goal ? 5 : 0, away: goal ? 5 : 0 });
  }
});

test("sudden death resolves only after an equal pair, for either winner", () => {
  const level = play([...Array(10).fill(1), 1, 1, 0, 0]);
  for (const homeWins of [true, false]) {
    const half = kick(level, homeWins);
    assert.equal(half.status, "sudden-death");
    assert.equal(replayCupShootout(half).winner, null);
    assert.equal(replayCupShootout(half).nextTeam, "away");
    const end = kick(half, !homeWins);
    assert.equal(replayCupShootout(end).winner, homeWins ? "home" : "away");
  }
});

test("initial attempts are bounded by five each; sudden death has no fixed cap", () => {
  // Explore all binary initial sequences, stopping at the first terminal prefix.
  let shortest = 10;
  for (let bits = 0; bits < 1024; bits += 1) {
    let session = initial();
    for (let i = 0; i < 10 && session.status === "initial"; i += 1) {
      session = kick(session, (bits & (1 << i)) !== 0);
    }
    const state = replayCupShootout(session);
    assert.ok(state.attemptsByTeam.home <= 5 && state.attemptsByTeam.away <= 5);
    assert.notEqual(state.status, "initial");
    shortest = Math.min(shortest, session.attempts.length);
  }
  assert.equal(shortest, 6);
  const long = play(Array(210).fill(false));
  assert.equal(long.status, "sudden-death");
  assert.deepEqual(replayCupShootout(long).attemptsByTeam, { home: 105, away: 105 });
  assert.equal(play([1, 0], long).status, "complete");
});

test("rejects stale revisions, foreign IDs and noncanonical faces without mutation", () => {
  const session = initial();
  const before = structuredClone(session);
  for (const command of [
    { expectedRevision: 1, attemptId: "wrong", face: "GOAL!" },
    { expectedRevision: 0, attemptId: "wrong", face: "GOAL!" },
    { expectedRevision: Number.MAX_SAFE_INTEGER, attemptId: "wrong", face: "GOAL!" },
    { expectedRevision: 0, attemptId: JSON.stringify([...JSON.parse(session.shootoutId), 1]), face: "GOAL" },
  ]) assert.throws(() => applyCupShootoutFace(session, command));
  assert.deepEqual(session, before);
});

test("replay rejects tampered versions, projections, ordering, faces and identities", () => {
  const session = play([1, 0]);
  const corruptions = [
    s => { s.schemaVersion = 2; },
    s => { s.rulesVersion = "v2"; },
    s => { s.firstTeam = "away"; },
    s => { s.admitted = true; },
    s => { s.goals = { home: 99, away: 0 }; },
    s => { s.round = Number.MAX_SAFE_INTEGER + 1; },
    s => { s.pair = 0; },
    s => { s.shootoutId = '["other","r1-p2"]'; },
    s => { s.status = "complete"; },
    s => { s.attempts[0].team = "away"; },
    s => { s.attempts[0].score = 1; },
    s => { s.attempts[0].face = "GOAL"; },
    s => { s.attempts[0].kind = "penalty-wide"; },
    s => { s.attempts[0].index = Number.MAX_SAFE_INTEGER; },
    s => { s.attempts[1].attemptId = s.attempts[0].attemptId; },
    s => { s.attempts.reverse(); },
    s => { delete s.attempts[0]; },
    s => { s.fixtureRefs = []; },
    s => { s.fixtureRefs[0] = []; },
    s => { delete s.fixtureRefs[0]; },
    s => { s.homeProviderTeamId = s.awayProviderTeamId; },
  ];
  for (const corrupt of corruptions) {
    const changed = structuredClone(session);
    corrupt(changed);
    assert.throws(() => replayCupShootout(changed));
  }
  const terminal = play([1, 0, 1, 0, 1, 0]);
  terminal.attempts.push({
    attemptId: JSON.stringify([...JSON.parse(terminal.shootoutId), 7]),
    index: 7, team: "home", face: "FUORI", kind: "penalty-wide",
  });
  assert.throws(() => replayCupShootout(terminal), { code: "CUP_SHOOTOUT_INVALID_STATE" });
});

test("creation respects readiness order, both legs and aggregate eligibility", () => {
  const { cup, matches } = tiedCup(2);
  assert.throws(() => createCupShootout(cup, matches, 1, 2), { code: "CUP_SHOOTOUT_NOT_ELIGIBLE" });
  const incomplete = structuredClone(cup);
  incomplete.fixtures[1].matchId = null;
  assert.throws(() => createCupShootout(incomplete, matches, 1, 1));
  for (const score of [{ home: 1, away: 0 }, { home: 0, away: 0 }]) {
    const decisive = structuredClone(matches);
    decisive[1].score = score;
    assert.throws(() => createCupShootout(cup, decisive, 1, 1), { code: "CUP_SHOOTOUT_NOT_ELIGIBLE" });
  }
  const missing = matches.slice(1);
  assert.throws(() => createCupShootout(cup, missing, 1, 1));
});
