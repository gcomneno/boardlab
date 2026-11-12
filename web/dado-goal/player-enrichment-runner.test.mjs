import assert from "node:assert/strict";
import test from "node:test";

import {
  runPlayerEnrichmentBatch,
} from "./player-enrichment-runner.mjs";

const catalogue = {
  teams: [
    { providerTeamId: "10" },
    { providerTeamId: "20" },
    { providerTeamId: "30" },
  ],
};

function adapter(calls) {
  return {
    async fetchPlayers({ teamId }) {
      calls.push(teamId);
      return [{
        id: Number(teamId) + 1,
        name: `Player ${teamId}`,
        position: "Defender",
        shirtNumber: 4,
        age: 25,
        photo: `player-${teamId}.png`,
      }];
    },
  };
}

test("runner creates a persistent checkpoint for a bounded batch", async () => {
  const calls = [];

  const result = await runPlayerEnrichmentBatch({
    catalogue,
    adapter: adapter(calls),
    requestBudget: 2,
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["10", "20"]);
  assert.equal(result.requests, 2);
  assert.equal(result.complete, false);
  assert.deepEqual(result.remainingTeamIds, ["30"]);
  assert.deepEqual(
    result.checkpoint.completedTeamIds,
    ["10", "20"],
  );
  assert.equal(result.checkpoint.snapshots.length, 2);
});

test("runner resumes checkpoint without reacquiring completed teams", async () => {
  const calls = [];

  const first = await runPlayerEnrichmentBatch({
    catalogue,
    adapter: adapter(calls),
    requestBudget: 1,
    requestIntervalMs: 0,
  });

  const second = await runPlayerEnrichmentBatch({
    catalogue,
    adapter: adapter(calls),
    requestBudget: 2,
    checkpoint: first.checkpoint,
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["10", "20", "30"]);
  assert.equal(second.requests, 2);
  assert.equal(second.complete, true);
  assert.deepEqual(second.remainingTeamIds, []);
  assert.deepEqual(
    second.checkpoint.completedTeamIds,
    ["10", "20", "30"],
  );
  assert.equal(second.checkpoint.snapshots.length, 3);
});

test("runner honors injected pacing", async () => {
  const waits = [];

  await runPlayerEnrichmentBatch({
    catalogue,
    adapter: adapter([]),
    requestBudget: 3,
    requestIntervalMs: 6500,
    wait: async (milliseconds) => waits.push(milliseconds),
  });

  assert.deepEqual(waits, [6500, 6500]);
});
