import assert from "node:assert/strict";
import test from "node:test";

import {
  runCoachEnrichmentBatch,
} from "./coach-enrichment-runner.mjs";

const catalogue = {
  teams: [
    { providerTeamId: "38" },
    { providerTeamId: "39" },
  ],
};

test("coach runner creates resumable bounded checkpoint", async () => {
  const result = await runCoachEnrichmentBatch({
    catalogue,
    adapter: {
      async fetchCoaches({ teamId }) {
        return [{
          id: Number(teamId),
          name: `Coach ${teamId}`,
          career: [],
        }];
      },
    },
    requestBudget: 1,
    requestIntervalMs: 0,
  });

  assert.equal(result.requests, 1);
  assert.equal(result.complete, false);
  assert.deepEqual(result.checkpoint.completedTeamIds, ["38"]);
  assert.equal(result.checkpoint.snapshots.length, 1);
});

test("coach runner resumes without reacquiring completed teams", async () => {
  const calls = [];

  const result = await runCoachEnrichmentBatch({
    catalogue,
    adapter: {
      async fetchCoaches({ teamId }) {
        calls.push(teamId);
        return [];
      },
    },
    requestBudget: 1,
    checkpoint: {
      completedTeamIds: ["38"],
      snapshots: [],
    },
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["39"]);
  assert.equal(result.complete, true);
});
