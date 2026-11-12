import assert from "node:assert/strict";
import test from "node:test";

import {
  finalizePlayerEnrichment,
} from "./player-enrichment-finalizer.mjs";
import {
  createCatalogue,
  createPlayer,
  createTeam,
} from "./team-catalogue.mjs";

function fixtureCatalogue() {
  return createCatalogue({
    provider: "api-football",
    competitionId: "39",
    season: 2024,
    importedAt: "2026-11-12T17:00:00.000Z",
    teams: [{
      ...createTeam({
        provider: "api-football",
        providerTeamId: "33",
        name: "Manchester United",
        competitionId: "39",
        season: 2024,
        colors: ["#DA291C", "#000000"],
        logo: "https://example.test/team-33.png",
      }),
      players: [
        createPlayer({
          provider: "api-football",
          providerPlayerId: "18885",
          name: "K. Darlow",
          position: "Goalkeeper",
          shirtNumber: 1,
        }),
      ],
    }],
  });
}

test("finalizes a complete checkpoint without rebuilding the roster", () => {
  const catalogue = fixtureCatalogue();

  const result = finalizePlayerEnrichment({
    catalogue,
    checkpoint: {
      version: 1,
      completedTeamIds: ["33"],
      snapshots: [{
        providerTeamId: "33",
        providerPlayerId: "18885",
        age: 36,
        photo: "https://example.test/18885.png",
      }],
    },
    provider: "api-football",
    retrievedAt: "2026-11-12T18:30:00.000Z",
  });

  assert.equal(result.stats.snapshots, 1);
  assert.equal(result.stats.matched, 1);
  assert.equal(result.stats.unmatched, 0);
  assert.equal(result.catalogue.teams.length, 1);
  assert.equal(result.catalogue.teams[0].players.length, 1);
  assert.equal(result.catalogue.teams[0].players[0].providerPlayerId, "18885");
  assert.equal(result.catalogue.teams[0].players[0].enrichment.age, 36);

  const exported = JSON.parse(result.json);
  assert.equal(exported.teams[0].players[0].enrichment.age, 36);
});

test("rejects an incomplete checkpoint before enrichment", () => {
  assert.throws(
    () => finalizePlayerEnrichment({
      catalogue: fixtureCatalogue(),
      checkpoint: {
        version: 1,
        completedTeamIds: [],
        snapshots: [],
      },
      provider: "api-football",
      retrievedAt: "2026-11-12T18:30:00.000Z",
    }),
    /incomplete player enrichment checkpoint: 33/,
  );
});

test("reports unmatched snapshots while preserving canonical players", () => {
  const result = finalizePlayerEnrichment({
    catalogue: fixtureCatalogue(),
    checkpoint: {
      version: 1,
      completedTeamIds: ["33"],
      snapshots: [{
        providerTeamId: "33",
        providerPlayerId: "999999",
        age: 21,
      }],
    },
    provider: "api-football",
    retrievedAt: "2026-11-12T18:30:00.000Z",
  });

  assert.deepEqual(result.stats, {
    snapshots: 1,
    matched: 0,
    unmatched: 1,
  });
  assert.equal(result.catalogue.teams[0].players.length, 1);
  assert.equal(
    result.catalogue.teams[0].players[0].providerPlayerId,
    "18885",
  );
  assert.equal(
    result.catalogue.teams[0].players[0].enrichment,
    undefined,
  );
});
