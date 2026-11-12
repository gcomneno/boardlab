import assert from "node:assert/strict";
import test from "node:test";

import {
  createCoachEnrichmentCheckpoint,
  exportCoachEnrichmentCheckpoint,
  importCoachEnrichmentCheckpoint,
} from "./coach-enrichment-checkpoint.mjs";

test("coach checkpoint round-trip preserves resume state", () => {
  const checkpoint = createCoachEnrichmentCheckpoint({
    completedTeamIds: [38],
    snapshots: [{
      providerCoachId: 10,
      providerTeamId: "38",
      career: [],
    }],
  });

  assert.deepEqual(
    importCoachEnrichmentCheckpoint(
      exportCoachEnrichmentCheckpoint(checkpoint),
    ),
    checkpoint,
  );
});

test("coach checkpoint clones snapshot data", () => {
  const snapshots = [{ providerCoachId: 10, career: [] }];
  const checkpoint = createCoachEnrichmentCheckpoint({
    completedTeamIds: [],
    snapshots,
  });

  snapshots[0].providerCoachId = 99;
  assert.equal(checkpoint.snapshots[0].providerCoachId, 10);
});

test("rejects unsupported coach checkpoints", () => {
  assert.throws(
    () => importCoachEnrichmentCheckpoint('{"version":99}'),
    /unsupported checkpoint version/,
  );
});
