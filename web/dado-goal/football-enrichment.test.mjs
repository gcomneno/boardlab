import assert from "node:assert/strict";
import test from "node:test";

import { importCatalogueJson } from "./catalogue-json.mjs";
import { enrichFootballCatalogue } from "./football-enrichment.mjs";

test("enriches teams without altering existing squads", async () => {
  const source = JSON.stringify({
    schemaVersion: 3,
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    importedAt: "2026-01-01T00:00:00Z",
    teams: [{
      provider: "api-football",
      providerTeamId: "487",
      name: "Lazio",
      competitionId: "135",
      season: "2024",
      colors: ["#87CEEB", "#FFFFFF"],
      logo: "https://example.test/lazio.png",
      players: [{
        provider: "api-football",
        providerPlayerId: "123",
        name: "Mario Rossi",
        position: "Midfielder",
        shirtNumber: 8
      }]
    }]
  });

  const catalogue = importCatalogueJson(source);
  const playersBefore = structuredClone(catalogue.teams[0].players);

  const adapter = {
    name: "api-football",
    async fetchTeams({ competitionId, season }) {
      assert.equal(competitionId, "135");
      assert.equal(season, "2024");

      return [{
        id: 487,
        name: "Lazio",
        logo: "https://provider.test/487.png",
        code: "LAZ",
        country: "Italy",
        founded: 1900,
        venue: {
          id: 910,
          name: "Stadio Olimpico",
          address: "Viale dei Gladiatori",
          city: "Roma",
          capacity: 68530,
          surface: "grass"
        }
      }];
    }
  };

  const enriched = await enrichFootballCatalogue({
    catalogue,
    adapter,
    retrievedAt: "2026-11-12T12:00:00Z"
  });

  const team = enriched.teams[0];

  assert.equal(enriched.schemaVersion, 4);
  assert.equal(team.name, "Lazio");
  assert.equal(team.logo, "https://example.test/lazio.png");
  assert.deepEqual(team.players, playersBefore);
  assert.equal(team.enrichment.code, "LAZ");
  assert.equal(team.enrichment.country, "Italy");
  assert.equal(team.enrichment.founded, 1900);
  assert.equal(team.enrichment.venue.name, "Stadio Olimpico");
  assert.equal(team.enrichment.provenance.provider, "api-football");
  assert.equal(
    team.enrichment.provenance.retrievedAt,
    "2026-11-12T12:00:00Z",
  );
});

test("preserves unmatched catalogue teams", async () => {
  const source = await import("./team-catalogue.mjs");

  const team = {
    ...source.createTeam({
      provider: "api-football",
      providerTeamId: "999",
      name: "Fixture FC",
      competitionId: "135",
      season: "2024",
      colors: ["#000000"],
      logo: "https://example.test/999.png"
    }),
    players: []
  };

  const catalogue = source.createCatalogue({
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    importedAt: "2026-01-01T00:00:00Z",
    teams: [team]
  });

  const enriched = await enrichFootballCatalogue({
    catalogue,
    adapter: {
      name: "api-football",
      async fetchTeams() {
        return [];
      }
    },
    retrievedAt: "2026-11-12T12:00:00Z"
  });

  assert.deepEqual(enriched.teams[0], team);
});
