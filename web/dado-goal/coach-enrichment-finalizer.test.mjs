import assert from "node:assert/strict";
import test from "node:test";

import {
  finalizeCoachEnrichment,
} from "./coach-enrichment-finalizer.mjs";

const existingEnrichment = {
  code: "AAA",
  country: "Italy",
  founded: 1900,
  venue: { id: 1, name: "Stadio" },
  provenance: {
    provider: "api-football",
    retrievedAt: "2026-11-12T00:00:00.000Z",
  },
};

test("adds selected coach without clobbering existing team enrichment", () => {
  const catalogue = {
    schemaVersion: 4,
    teams: [{
      providerTeamId: 10,
      name: "Example",
      enrichment: existingEnrichment,
    }],
  };

  const snapshots = [
    {
      providerTeamId: 10,
      id: 100,
      name: "Old Coach",
      firstname: "Old",
      lastname: "Coach",
      age: 50,
      nationality: "Italy",
      photo: "old.jpg",
      career: [{
        team: { id: 10, name: "Example", logo: "logo.png" },
        start: "2024-01-01",
        end: null,
      }],
    },
    {
      providerTeamId: 10,
      id: 101,
      name: "New Coach",
      firstname: "New",
      lastname: "Coach",
      age: 51,
      nationality: "Italy",
      photo: "new.jpg",
      career: [{
        team: { id: 10, name: "Example", logo: "logo.png" },
        start: "2025-01-01",
        end: null,
      }],
    },
  ];

  const result = finalizeCoachEnrichment({
    catalogue,
    snapshots,
    retrievedAt: "2026-11-13T00:00:00.000Z",
  });

  assert.deepEqual(
    result.catalogue.teams[0].enrichment.code,
    existingEnrichment.code,
  );
  assert.deepEqual(
    result.catalogue.teams[0].enrichment.venue,
    existingEnrichment.venue,
  );
  assert.deepEqual(
    result.catalogue.teams[0].enrichment.provenance,
    existingEnrichment.provenance,
  );

  assert.equal(
    result.catalogue.teams[0].enrichment.coach.providerCoachId,
    101,
  );
  assert.equal(
    result.catalogue.teams[0].enrichment.coach.name,
    "New Coach",
  );
  assert.equal(result.stats.selected, 1);
  assert.equal(result.stats.unresolved, 0);
});

test("leaves team unchanged when provider returned no coach", () => {
  const team = {
    providerTeamId: 490,
    name: "Cagliari",
    enrichment: existingEnrichment,
  };

  const catalogue = {
    schemaVersion: 4,
    teams: [team],
  };

  const result = finalizeCoachEnrichment({
    catalogue,
    snapshots: [],
    retrievedAt: "2026-11-13T00:00:00.000Z",
  });

  assert.deepEqual(result.catalogue.teams[0], team);
  assert.equal(result.stats.selected, 0);
  assert.equal(result.stats.unresolved, 1);
});

test("does not mutate source catalogue", () => {
  const catalogue = {
    schemaVersion: 4,
    teams: [{
      providerTeamId: 10,
      name: "Example",
      enrichment: existingEnrichment,
    }],
  };

  const before = structuredClone(catalogue);

  finalizeCoachEnrichment({
    catalogue,
    snapshots: [{
      providerTeamId: 10,
      id: 101,
      name: "Coach",
      firstname: "Test",
      lastname: "Coach",
      age: 51,
      nationality: "Italy",
      photo: "coach.jpg",
      career: [{
        team: { id: 10, name: "Example", logo: null },
        start: "2025-01-01",
        end: null,
      }],
    }],
    retrievedAt: "2026-11-13T00:00:00.000Z",
  });

  assert.deepEqual(catalogue, before);
});
