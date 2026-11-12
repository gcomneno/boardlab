import assert from "node:assert/strict";
import test from "node:test";

import {
  createPlayerEnrichmentCheckpoint,
  exportPlayerEnrichmentCheckpoint,
  importPlayerEnrichmentCheckpoint,
  PLAYER_ENRICHMENT_CHECKPOINT_VERSION,
} from "./player-enrichment-checkpoint.mjs";

test("checkpoint round-trip preserves resume state", () => {
  const checkpoint = createPlayerEnrichmentCheckpoint({
    completedTeamIds: [10, "20"],
    snapshots: [{
      providerPlayerId: 101,
      providerTeamId: "10",
      age: 25,
      photo: "player.png",
    }],
  });

  const restored = importPlayerEnrichmentCheckpoint(
    exportPlayerEnrichmentCheckpoint(checkpoint),
  );

  assert.deepEqual(restored, {
    version: PLAYER_ENRICHMENT_CHECKPOINT_VERSION,
    completedTeamIds: ["10", "20"],
    snapshots: [{
      providerPlayerId: 101,
      providerTeamId: "10",
      age: 25,
      photo: "player.png",
    }],
  });
});

test("checkpoint creation clones snapshot data", () => {
  const snapshots = [{
    providerPlayerId: 101,
    nested: { value: "original" },
  }];

  const checkpoint = createPlayerEnrichmentCheckpoint({
    completedTeamIds: ["10"],
    snapshots,
  });

  snapshots[0].nested.value = "mutated";

  assert.equal(checkpoint.snapshots[0].nested.value, "original");
});

test("rejects malformed and unsupported checkpoints", () => {
  assert.throws(
    () => importPlayerEnrichmentCheckpoint("{"),
    /valid JSON/,
  );

  assert.throws(
    () => importPlayerEnrichmentCheckpoint(JSON.stringify({
      version: 999,
      completedTeamIds: [],
      snapshots: [],
    })),
    /unsupported checkpoint version/,
  );
});
