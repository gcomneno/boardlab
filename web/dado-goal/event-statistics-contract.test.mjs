import test from "node:test";
import assert from "node:assert/strict";

import { createRollEvent } from "./roll-event.mjs";
import { matchStatistics } from "./statistics.mjs";
import { playerStatistics } from "./player-statistics.mjs";
import {
  createMatchRecord,
  exportHistoryJson,
  importHistoryJson,
} from "./team-history.mjs";

const HOME = {
  provider: "fixture",
  providerTeamId: "home",
  name: "Home FC",
};
const AWAY = {
  provider: "fixture",
  providerTeamId: "away",
  name: "Away FC",
};

test("statistics derive from semantic kinds rather than rendered face text", () => {
  const [home] = matchStatistics([
    {
      seat: 0,
      die: "result",
      kind: "goal",
      face: "not-a-localized-contract",
    },
    {
      seat: 0,
      die: "result",
      kind: "penalty-awarded",
      face: "also-not-a-contract",
    },
    {
      seat: 0,
      die: "penalty",
      kind: "penalty-goal",
      face: "presentation-only",
    },
  ]);

  assert.equal(home.goals, 2);
  assert.equal(home.penalties, 1);
  assert.equal(home.penaltyGoals, 1);
});

test("completed history preserves structured rolls and player events", () => {
  const rollEvents = [
    createRollEvent({ seat: 0, die: "result", face: "RIGORE" }),
    createRollEvent({ seat: 0, die: "penalty", face: "GOAL!" }),
    createRollEvent({ seat: 1, die: "result", face: "Parato!" }),
  ];

  const events = [
    {
      type: "penalty",
      team: 0,
      minute: 41,
      number: 9,
      outcome: "GOAL!",
      player: { providerPlayerId: "p9", name: "Player Nine" },
    },
    {
      type: "yellow",
      team: 1,
      minute: 50,
      number: 5,
      reason: "reckless",
      player: { providerPlayerId: "p5", name: "Player Five" },
    },
    {
      type: "substitution",
      team: 0,
      minute: 63,
      out: 10,
      in: 18,
      reason: "injury",
      outPlayer: { providerPlayerId: "p10", name: "Player Ten" },
      inPlayer: { providerPlayerId: "p18", name: "Player Eighteen" },
    },
    {
      type: "light-knock",
      classification: "narrative",
      family: "injury",
      team: 1,
      minute: 72,
      number: 7,
    },
  ];

  const statistics = matchStatistics(rollEvents);
  const record = createMatchRecord({
    matchId: "qualification-match",
    playedAt: "2026-11-16T09:00:00.000Z",
    home: HOME,
    away: AWAY,
    homeGoals: 1,
    awayGoals: 0,
    events,
    rollEvents,
    statistics,
  });

  const [restored] = importHistoryJson(exportHistoryJson([record]));

  assert.deepEqual(restored.rollEvents, rollEvents);
  assert.deepEqual(restored.events, events);
  assert.deepEqual(matchStatistics(restored.rollEvents), statistics);

  const [homePlayers, awayPlayers] = playerStatistics(restored.events);
  assert.equal(homePlayers.find(player => player.number === 9).goals, 1);
  assert.equal(
    homePlayers.find(player => player.number === 10).subbedOff,
    1,
  );
  assert.equal(
    homePlayers.find(player => player.number === 18).subbedOn,
    1,
  );
  assert.equal(awayPlayers.find(player => player.number === 5).yellowCards, 1);

  const injurySubstitution = restored.events.find(
    event => event.type === "substitution",
  );
  assert.equal(injurySubstitution.reason, "injury");

  const extraordinary = restored.events.find(
    event => event.type === "light-knock",
  );
  assert.equal(extraordinary.classification, "narrative");
  assert.equal(extraordinary.family, "injury");
});

test("legacy history without rollEvents remains readable", () => {
  const legacy = JSON.stringify({
    schemaVersion: 1,
    matches: [{
      schemaVersion: 1,
      matchId: "legacy-match",
      playedAt: "2026-11-15T09:00:00.000Z",
      home: HOME,
      away: AWAY,
      score: { home: 0, away: 0 },
      events: [],
      statistics: {},
      ratings: [],
    }],
  });

  const [restored] = importHistoryJson(legacy);
  assert.deepEqual(restored.rollEvents, []);
});
