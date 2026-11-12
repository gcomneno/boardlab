import assert from "node:assert/strict";
import test from "node:test";

import {
  collectCoachSnapshots,
} from "./coach-enrichment-collector.mjs";

const catalogue = {
  teams: [
    { providerTeamId: "38" },
    { providerTeamId: "39" },
  ],
};

test("hard request budget bounds coach acquisition", async () => {
  const calls = [];

  const result = await collectCoachSnapshots({
    catalogue,
    adapter: {
      async fetchCoaches({ teamId }) {
        calls.push(teamId);
        return [{
          id: Number(teamId) + 1000,
          name: `Coach ${teamId}`,
          career: [],
        }];
      },
    },
    requestBudget: 1,
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["38"]);
  assert.equal(result.requests, 1);
  assert.equal(result.complete, false);
  assert.deepEqual(result.processedTeamIds, ["38"]);
  assert.deepEqual(result.remainingTeamIds, ["39"]);
  assert.equal(result.snapshots[0].providerTeamId, "38");
  assert.equal(result.snapshots[0].providerCoachId, 1038);
});

test("resume skips completed teams and preserves snapshots", async () => {
  const calls = [];
  const existing = [{
    providerCoachId: 1038,
    id: 1038,
    name: "Existing",
    providerTeamId: "38",
    career: [],
  }];

  const result = await collectCoachSnapshots({
    catalogue,
    adapter: {
      async fetchCoaches({ teamId }) {
        calls.push(teamId);
        return [{
          id: 1039,
          name: "New",
          career: [],
        }];
      },
    },
    requestBudget: 1,
    completedTeamIds: ["38"],
    existingSnapshots: existing,
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["39"]);
  assert.equal(result.complete, true);
  assert.equal(result.snapshots.length, 2);
  assert.deepEqual(existing, [{
    providerCoachId: 1038,
    id: 1038,
    name: "Existing",
    providerTeamId: "38",
    career: [],
  }]);
});

test("paces coach requests only between calls", async () => {
  const waits = [];

  await collectCoachSnapshots({
    catalogue,
    adapter: {
      async fetchCoaches() {
        return [];
      },
    },
    requestBudget: 2,
    requestIntervalMs: 6500,
    wait: async (milliseconds) => waits.push(milliseconds),
  });

  assert.deepEqual(waits, [6500]);
});
