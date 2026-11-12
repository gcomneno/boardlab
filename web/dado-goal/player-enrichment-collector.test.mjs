import assert from "node:assert/strict";
import test from "node:test";

import {
  collectPlayerSnapshots,
} from "./player-enrichment-collector.mjs";

function catalogue() {
  return {
    teams: [
      { providerTeamId: "10" },
      { providerTeamId: "20" },
      { providerTeamId: "30" },
    ],
  };
}

function adapter(calls) {
  return {
    async fetchPlayers({ teamId }) {
      calls.push(teamId);
      return [{
        id: Number(teamId) + 1,
        name: `Player ${teamId}`,
        position: "Midfielder",
        shirtNumber: 8,
        age: 25,
        photo: `player-${teamId}.png`,
      }];
    },
  };
}

test("hard request budget stops acquisition before the next team", async () => {
  const calls = [];

  const result = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 2,
    requestIntervalMs: 0,
  });

  assert.deepEqual(calls, ["10", "20"]);
  assert.equal(result.requests, 2);
  assert.deepEqual(result.processedTeamIds, ["10", "20"]);
  assert.deepEqual(result.remainingTeamIds, ["30"]);
  assert.equal(result.complete, false);
  assert.equal(result.snapshots.length, 2);
  assert.equal(result.snapshots[0].providerPlayerId, 11);
  assert.equal(result.snapshots[0].providerTeamId, "10");
  assert.equal(result.snapshots[0].age, 25);
});

test("resume skips completed teams without spending request budget", async () => {
  const calls = [];

  const result = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 1,
    requestIntervalMs: 0,
    completedTeamIds: ["10", "20"],
  });

  assert.deepEqual(calls, ["30"]);
  assert.equal(result.requests, 1);
  assert.deepEqual(result.processedTeamIds, ["30"]);
  assert.deepEqual(result.remainingTeamIds, []);
  assert.equal(result.complete, true);
});

test("never calls the team-list endpoint", async () => {
  let fetchTeamsCalls = 0;
  const playerCalls = [];

  const result = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: {
      async fetchTeams() {
        fetchTeamsCalls += 1;
        throw new Error("must not be called");
      },
      async fetchPlayers({ teamId }) {
        playerCalls.push(teamId);
        return [];
      },
    },
    requestBudget: 3,
    requestIntervalMs: 0,
  });

  assert.equal(fetchTeamsCalls, 0);
  assert.deepEqual(playerCalls, ["10", "20", "30"]);
  assert.equal(result.requests, 3);
  assert.equal(result.complete, true);
});

test("rejects invalid request budgets before acquisition", async () => {
  let calls = 0;

  await assert.rejects(
    collectPlayerSnapshots({
      catalogue: catalogue(),
      adapter: {
        async fetchPlayers() {
          calls += 1;
          return [];
        },
      },
      requestBudget: 0,
    }),
    /requestBudget/,
  );

  assert.equal(calls, 0);
});

test("resume preserves previously collected snapshots", async () => {
  const calls = [];

  const first = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 2,
    requestIntervalMs: 0,
  });

  assert.equal(first.snapshots.length, 2);
  assert.deepEqual(first.processedTeamIds, ["10", "20"]);

  const resumed = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 1,
    requestIntervalMs: 0,
    completedTeamIds: first.processedTeamIds,
    existingSnapshots: first.snapshots,
  });

  assert.deepEqual(calls, ["10", "20", "30"]);
  assert.equal(resumed.requests, 1);
  assert.equal(resumed.snapshots.length, 3);
  assert.deepEqual(
    resumed.snapshots.map((snapshot) => snapshot.providerTeamId),
    ["10", "20", "30"],
  );
  assert.equal(resumed.complete, true);
});

test("paces requests without waiting after the final request", async () => {
  const calls = [];
  const waits = [];

  const result = await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 3,
    requestIntervalMs: 6500,
    wait: async (milliseconds) => waits.push(milliseconds),
  });

  assert.deepEqual(calls, ["10", "20", "30"]);
  assert.deepEqual(waits, [6500, 6500]);
  assert.equal(result.requests, 3);
  assert.equal(result.complete, true);
});

test("single-request batches do not incur a pacing wait", async () => {
  const calls = [];
  const waits = [];

  await collectPlayerSnapshots({
    catalogue: catalogue(),
    adapter: adapter(calls),
    requestBudget: 1,
    requestIntervalMs: 6500,
    wait: async (milliseconds) => waits.push(milliseconds),
  });

  assert.deepEqual(calls, ["10"]);
  assert.deepEqual(waits, []);
});
