import assert from "node:assert/strict";
import test from "node:test";

import {
  exportCatalogueJson,
  importCatalogueJson,
} from "./catalogue-json.mjs";
import { createCatalogue } from "./team-catalogue.mjs";

test("catalogue JSON round-trip preserves data", () => {
  const catalogue = createCatalogue({
    provider: "api-football",
    competitionId: 135,
    season: 2026,
    importedAt: "2026-11-02T10:00:00Z",
    teams: [{
      provider: "api-football",
      providerTeamId: "492",
      name: "SSC Napoli",
      competitionId: "135",
      season: "2026",
      players: [],
    }],
  });

  const restored = importCatalogueJson(
    exportCatalogueJson(catalogue),
  );

  assert.deepEqual(restored, catalogue);
});

test("rejects unknown schema versions", () => {
  assert.throws(
    () => importCatalogueJson(
      JSON.stringify({ schemaVersion: 999, teams: [] }),
    ),
    /unsupported catalogue schema/,
  );
});
