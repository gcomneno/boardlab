import assert from "node:assert/strict";
import test from "node:test";

import {
  createManagementDocument,
  loadManagement,
  saveManagement,
} from "./team-management-store.mjs";
import { createMatchRecord } from "./team-history.mjs";

test("completed managed match survives local persistence as a snapshot", () => {
  const values = new Map();
  const storage = {
    getItem: key => values.get(key) ?? null,
    setItem: (key, value) => values.set(key, value),
  };

  const team = {
    provider: "api-football",
    providerTeamId: "492",
    name: "SSC Napoli",
    players: [{
      providerPlayerId: "10",
      name: "Player A",
      position: "Attacker",
      shirtNumber: 10,
    }],
  };

  const record = createMatchRecord({
    matchId: "match-001",
    playedAt: "2026-11-02T14:00:00Z",
    home: team,
    away: {
      provider: "local",
      providerTeamId: "local-1",
      name: "Squadra 2",
      players: [],
    },
    homeGoals: 2,
    awayGoals: 0,
    events: [{ type: "goal", team: 0, number: 10 }],
    statistics: { rolls: 12 },
    ratings: [],
  });

  const document = createManagementDocument({ matches: [record] });
  saveManagement(storage, document);

  team.name = "CHANGED AFTER MATCH";

  const restored = loadManagement(storage);
  assert.equal(restored.matches[0].home.name, "SSC Napoli");
  assert.equal(restored.matches[0].home.players[0].name, "Player A");
  assert.equal(restored.matches[0].score.home, 2);
});
