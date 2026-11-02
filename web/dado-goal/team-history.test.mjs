import assert from "node:assert/strict";
import test from "node:test";

import {
  createMatchRecord,
  exportHistoryJson,
  importHistoryJson,
  teamStatistics,
} from "./team-history.mjs";

const napoli = {
  provider: "api-football",
  providerTeamId: "492",
  name: "SSC Napoli",
};

const roma = {
  provider: "api-football",
  providerTeamId: "497",
  name: "AS Roma",
};

test("records simulated match result with provider identities", () => {
  const match = createMatchRecord({
    matchId: "match-001",
    matchId: "match-003",
      matchId: "match-004",
      matchId: "match-invalid",
      playedAt: "2026-11-02T12:00:00Z",
    home: napoli,
    away: roma,
    homeGoals: 2,
    awayGoals: 1,
  });

  assert.equal(match.home.name, "SSC Napoli");
  assert.equal(match.away.name, "AS Roma");
  assert.deepEqual(match.score, { home: 2, away: 1 });
});

test("aggregates simulated team statistics", () => {
  const records = [
    createMatchRecord({
      matchId: "match-002",
      playedAt: "2026-11-01T12:00:00Z",
      home: napoli,
      away: roma,
      homeGoals: 2,
      awayGoals: 1,
    }),
    createMatchRecord({
      matchId: "match-003",
      playedAt: "2026-11-02T12:00:00Z",
      home: roma,
      away: napoli,
      homeGoals: 0,
      awayGoals: 0,
    }),
  ];

  assert.deepEqual(teamStatistics(records, "492"), {
    played: 2,
    won: 1,
    drawn: 1,
    lost: 0,
    goalsFor: 2,
    goalsAgainst: 1,
  });
});

test("history JSON round-trip preserves simulated results", () => {
  const records = [
    createMatchRecord({
      matchId: "match-004",
      playedAt: "2026-11-02T12:00:00Z",
      home: napoli,
      away: roma,
      homeGoals: 3,
      awayGoals: 2,
    }),
  ];

  assert.deepEqual(
    importHistoryJson(exportHistoryJson(records)),
    records,
  );
});

test("rejects malformed history data", () => {
  assert.throws(
    () => importHistoryJson('{"schemaVersion":999,"matches":[]}'),
    /unsupported history schema/,
  );

  assert.throws(
    () => createMatchRecord({
        matchId: "match-invalid",
      playedAt: "2026-11-02T12:00:00Z",
      home: napoli,
      away: roma,
      homeGoals: -1,
      awayGoals: 0,
    }),
    /goals/,
  );
});

test("completed match owns immutable descriptive snapshots", () => {
  const match = createMatchRecord({
    matchId: "match-snapshot",
    playedAt: "2026-11-02T14:00:00Z",
    home: { ...napoli, players: [{ providerPlayerId: "10", name: "Player A" }] },
    away: roma,
    homeGoals: 1,
    awayGoals: 0,
    events: [{ type: "goal", team: 0, number: 10 }],
    statistics: { possession: null, rolls: 12 },
    ratings: [{ providerPlayerId: "10", rating: "excellent" }],
  });

  assert.equal(match.matchId, "match-snapshot");
  assert.equal(match.home.players[0].name, "Player A");
  assert.deepEqual(match.events, [{ type: "goal", team: 0, number: 10 }]);
  assert.deepEqual(match.statistics, { possession: null, rolls: 12 });
  assert.deepEqual(match.ratings, [
    { providerPlayerId: "10", rating: "excellent" },
  ]);
});
