import assert from "node:assert/strict";
import test from "node:test";

import { importFootballCatalogue } from "./football-provider.mjs";

test("normalizes provider teams and players", async () => {
  const adapter = {
    name: "fixture",
    async fetchTeams() {
      return [
        { id: 10, name: "SSC Napoli" },
        { id: 20, name: "AS Roma" },
      ];
    },
    async fetchPlayers({ teamId }) {
      return [{
        id: teamId * 100,
        name: teamId === 10 ? "José Ángel" : "Mario Rossi",
        position: teamId === 10 ? "Goalkeeper" : "Forward",
        shirtNumber: teamId === 10 ? 1 : 9,
      }];
    },
  };

  const catalogue = await importFootballCatalogue({
    adapter,
    competitionId: 135,
    season: 2026,
    importedAt: "2026-11-02T10:00:00Z",
  });

  assert.equal(catalogue.teams.length, 2);
  assert.equal(catalogue.teams[0].name, "SSC Napoli");
  assert.equal(catalogue.teams[0].players[0].name, "José Ángel");
  assert.equal(catalogue.teams[1].name, "AS Roma");
  assert.equal(catalogue.teams[1].players[0].shirtNumber, 9);
});

test("rejects invalid adapters", async () => {
  await assert.rejects(
    importFootballCatalogue({
      adapter: {},
      competitionId: 135,
      season: 2026,
    }),
    /invalid provider adapter/,
  );
});
