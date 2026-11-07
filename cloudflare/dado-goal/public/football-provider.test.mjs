import assert from "node:assert/strict";
import test from "node:test";

import { importFootballCatalogue } from "./football-provider.mjs";

test("normalizes provider teams and players", async () => {
  const adapter = {
    name: "fixture",
    async fetchTeams() {
      return [
        {
          id: 492,
          name: "SSC Napoli",
          logo: "https://example.test/492.png",
        },
        {
          id: 497,
          name: "AS Roma",
          logo: "https://example.test/497.png",
        },
      ];
    },
    async fetchPlayers({ teamId }) {
      return [{
        id: teamId * 100,
        name: teamId === 492 ? "José Ángel" : "Mario Rossi",
        position: teamId === 492 ? "Goalkeeper" : "Forward",
        shirtNumber: teamId === 492 ? 1 : 9,
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
  assert.equal(catalogue.teams[0].logo, "https://example.test/492.png");
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
