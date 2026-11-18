import assert from "node:assert/strict";
import test from "node:test";

import {
  createCup,
  completeCupFixture,
  advanceCupIfReady,
  resolveCupTie,
} from "./cup.mjs";

const teams = Array.from({ length: 4 }, (_, index) => ({
  provider: "test",
  providerTeamId: String(index + 1),
  name: `Team ${index + 1}`,
}));

function completed(cup, scores) {
  const matches = [];
  for (const [index, fixture] of cup.fixtures.entries()) {
    const matchId = `match-${index + 1}`;
    matches.push({
      matchId,
      home: { providerTeamId: fixture.homeProviderTeamId },
      away: { providerTeamId: fixture.awayProviderTeamId },
      score: scores[index],
    });
    cup = completeCupFixture(cup, fixture.fixtureId, matchId);
  }
  return { cup, matches };
}

test("creates canonical single-leg and two-leg brackets", () => {
  const single = createCup({ cupId: "single", participants: teams });
  const double = createCup({
    cupId: "double", participants: teams, legs: 2,
  });

  assert.equal(single.fixtures.length, 2);
  assert.equal(double.fixtures.length, 4);
  assert.equal(double.fixtures[0].fixtureId, "r1-p1-l1");
  assert.equal(double.fixtures[1].fixtureId, "r1-p1-l2");
  assert.equal(
    double.fixtures[0].homeProviderTeamId,
    double.fixtures[1].awayProviderTeamId,
  );
});

test("rejects duplicate participants", () => {
  assert.throws(
    () => createCup({
      cupId: "invalid",
      participants: [teams[0], teams[0], teams[2], teams[3]],
    }),
    /duplicate cup participant/,
  );
});

test("waits for incomplete matches", () => {
  const cup = createCup({ cupId: "pending", participants: teams });
  assert.equal(
    advanceCupIfReady(cup, [], {}).status,
    "pending-matches",
  );
});

test("advances winners after both single-leg semifinals", () => {
  const initial = createCup({ cupId: "advance", participants: teams });
  const { cup, matches } = completed(initial, [
    { home: 2, away: 0 },
    { home: 0, away: 1 },
  ]);

  const result = advanceCupIfReady(cup, matches, {});
  assert.equal(result.status, "advanced");
  assert.equal(result.cup.fixtures.filter(f => f.round === 2).length, 1);
});

test("resolves two legs by aggregate goals", () => {
  const initial = createCup({
    cupId: "aggregate", participants: teams, legs: 2,
  });
  const { cup, matches } = completed(initial, [
    { home: 2, away: 0 },
    { home: 1, away: 0 },
    { home: 1, away: 0 },
    { home: 0, away: 0 },
  ]);

  const result = advanceCupIfReady(cup, matches, {});
  assert.equal(result.status, "advanced");
  assert.equal(result.cup.fixtures.filter(f => f.round === 2).length, 2);
});

test("requires decisive penalties for tied matches", () => {
  const initial = createCup({ cupId: "penalties", participants: teams });
  const { cup, matches } = completed(initial, [
    { home: 1, away: 1 },
    { home: 2, away: 0 },
  ]);

  const pending = advanceCupIfReady(cup, matches, {});
  assert.equal(pending.status, "pending-penalties");
  assert.equal(pending.pair, 1);

  assert.throws(
    () => resolveCupTie({
      fixtures: [cup.fixtures[0]],
      matches,
      penalties: { home: 5, away: 5 },
    }),
    /invalid penalty shootout/,
  );

  const decided = advanceCupIfReady(cup, matches, {
    "r1-p1": { home: 5, away: 4 },
  });
  assert.equal(decided.status, "advanced");
});
