import assert from "node:assert/strict";
import test from "node:test";

import { footballCatalogue } from "./football-catalogue.mjs";

function dbWithCatalogue(catalogue) {
  return {
    prepare() {
      return {
        bind() {
          return {
            async first() {
              return catalogue
                ? { catalogue_json: JSON.stringify(catalogue) }
                : null;
            },
          };
        },
      };
    },
  };
}

test("rejects malformed catalogue query", async () => {
  const response = await footballCatalogue(
    new Request(
      "https://example.test/api/football/catalogue?competition=x&season=now",
    ),
    { DB: dbWithCatalogue(null) },
  );

  assert.equal(response.status, 400);
  assert.deepEqual(await response.json(), {
    error: { code: "INVALID_CATALOGUE_QUERY" },
  });
});

test("returns 404 when catalogue has not been imported", async () => {
  const response = await footballCatalogue(
    new Request(
      "https://example.test/api/football/catalogue?competition=135&season=2024",
    ),
    { DB: dbWithCatalogue(null) },
  );

  assert.equal(response.status, 404);
  assert.deepEqual(await response.json(), {
    error: { code: "FOOTBALL_CATALOGUE_NOT_IMPORTED" },
  });
});

test("returns persisted normalized catalogue", async () => {
  const catalogue = {
    schemaVersion: 1,
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    importedAt: "2026-11-02T14:00:00.000Z",
    teams: [
      {
        provider: "api-football",
        providerTeamId: 529,
        name: "AS Roma",
        competitionId: "135",
        season: "2024",
        players: [],
      },
    ],
  };

  const response = await footballCatalogue(
    new Request(
      "https://example.test/api/football/catalogue?competition=135&season=2024",
    ),
    { DB: dbWithCatalogue(catalogue) },
  );

  assert.equal(response.status, 200);
  assert.deepEqual(await response.json(), catalogue);
});
