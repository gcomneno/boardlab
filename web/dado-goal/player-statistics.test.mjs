import test from "node:test";
import assert from "node:assert/strict";

import { playerStatistics } from "./player-statistics.mjs";

test("aggregates canonical player events by team and identity", () => {
  const [home, away] = playerStatistics([
    {
      type: "goal", team: 0, number: 9,
      player: { providerPlayerId: "900", name: "Mario Nove" },
    },
    {
      type: "penalty", team: 0, number: 9, outcome: "GOAL!",
      player: { providerPlayerId: "900", name: "Mario Nove" },
    },
    {
      type: "yellow", team: 0, number: 9,
      player: { providerPlayerId: "900", name: "Mario Nove" },
    },
    {
      type: "red", team: 1, number: 5,
      player: { providerPlayerId: "500", name: "Paolo Cinque" },
    },
  ]);

  assert.equal(home.length, 1);
  assert.equal(home[0].goals, 2);
  assert.equal(home[0].penalties, 1);
  assert.equal(home[0].yellowCards, 1);

  assert.equal(away.length, 1);
  assert.equal(away[0].redCards, 1);
});

test("tracks both sides of a substitution", () => {
  const [home] = playerStatistics([
    {
      type: "substitution",
      team: 0,
      out: 10,
      in: 21,
      outPlayer: {
        providerPlayerId: "1000",
        name: "Mario Rossi",
      },
      inPlayer: {
        providerPlayerId: "2100",
        name: "Luca Bianchi",
      },
    },
  ]);

  assert.equal(home.length, 2);
  assert.equal(home[0].number, 10);
  assert.equal(home[0].subbedOff, 1);
  assert.equal(home[1].number, 21);
  assert.equal(home[1].subbedOn, 1);
});

test("synthetic events fall back to shirt number identity", () => {
  const [, away] = playerStatistics([
    { type: "goal", team: 1, number: 7, player: null },
    { type: "yellow", team: 1, number: 7, player: null },
  ]);

  assert.deepEqual(
    {
      number: away[0].number,
      name: away[0].name,
      goals: away[0].goals,
      yellowCards: away[0].yellowCards,
    },
    {
      number: 7,
      name: null,
      goals: 1,
      yellowCards: 1,
    },
  );
});

test("irrelevant narrative events do not create player rows", () => {
  const teams = playerStatistics([
    { type: "var", team: 0, minute: 42 },
  ]);

  assert.deepEqual(teams, [[], []]);
});
