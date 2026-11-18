import assert from "node:assert/strict";
import test from "node:test";
import { createCup, completeCupFixture } from "./cup.mjs";
import { cupFixturePresentation } from "./cup-view.mjs";

const participants = ["A", "B", "C", "D"].map(id => ({
  providerTeamId: id, name: id,
}));

test("pending fixtures have no score or winner", () => {
  const cup = createCup({ cupId: "view", participants });
  const view = cupFixturePresentation(cup, cup.fixtures[0], []);
  assert.equal(view.status, "pending");
  assert.equal(view.winnerProviderTeamId, null);
});

test("completed single-leg tie identifies winner", () => {
  let cup = createCup({ cupId: "single", participants });
  const fixture = cup.fixtures[0];
  cup = completeCupFixture(cup, fixture.fixtureId, "m1");
  const matches = [{
    matchId: "m1",
    home: { providerTeamId: fixture.homeProviderTeamId },
    away: { providerTeamId: fixture.awayProviderTeamId },
    score: { home: 2, away: 1 },
  }];
  const view = cupFixturePresentation(cup, cup.fixtures[0], matches);
  assert.equal(view.status, "completed");
  assert.equal(view.winnerProviderTeamId, "A");
  assert.equal(view.score, "2–1");
});

test("aggregate tie requests penalties", () => {
  let cup = createCup({ cupId: "double", participants, legs: 2 });
  const fixtures = cup.fixtures.slice(0, 2);
  const matches = fixtures.map((fixture, index) => ({
    matchId: `m${index}`,
    home: { providerTeamId: fixture.homeProviderTeamId },
    away: { providerTeamId: fixture.awayProviderTeamId },
    score: { home: 1, away: 0 },
  }));
  for (const [index, fixture] of fixtures.entries()) {
    cup = completeCupFixture(cup, fixture.fixtureId, `m${index}`);
  }
  const view = cupFixturePresentation(cup, cup.fixtures[0], matches);
  assert.equal(view.status, "penalties");
  assert.equal(view.aggregate, "1–1");
  assert.equal(view.winnerProviderTeamId, null);
});
