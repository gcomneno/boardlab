import assert from "node:assert/strict";
import test from "node:test";

import {
  MANAGEMENT_STORAGE_KEY,
  clearManagement,
  createManagementDocument,
  exportManagementJson,
  importManagementJson,
  loadManagement,
  saveManagement,
} from "./team-management-store.mjs";

function memoryStorage() {
  const values = new Map();

  return {
    getItem(key) {
      return values.has(key) ? values.get(key) : null;
    },
    setItem(key, value) {
      values.set(key, value);
    },
    removeItem(key) {
      values.delete(key);
    },
  };
}

test("management JSON round-trip preserves ratings and history", () => {
  const document = createManagementDocument({
    ratings: [{
      providerPlayerId: "123",
      ratings: { attack: "excellent" },
    }],
    matches: [{
      playedAt: "2026-11-02T12:00:00Z",
      score: { home: 2, away: 1 },
    }],
  });

  assert.deepEqual(
    importManagementJson(exportManagementJson(document)),
    document,
  );
});

test("local storage round-trip is deterministic", () => {
  const storage = memoryStorage();
  const document = createManagementDocument({
    ratings: [{ providerPlayerId: "7" }],
    matches: [{ playedAt: "2026-11-02T13:00:00Z" }],
  });

  saveManagement(storage, document);

  assert.deepEqual(loadManagement(storage), document);
});

test("missing local state produces an empty document", () => {
  const storage = memoryStorage();

  assert.deepEqual(loadManagement(storage), {
    schemaVersion: 1,
    ratings: [],
    matches: [],
  });
});

test("local state can be explicitly cleared", () => {
  const storage = memoryStorage();

  saveManagement(storage, createManagementDocument({
    ratings: [{ providerPlayerId: "9" }],
  }));

  clearManagement(storage);

  assert.equal(storage.getItem(MANAGEMENT_STORAGE_KEY), null);
});

test("rejects malformed or unsupported JSON", () => {
  assert.throws(
    () => importManagementJson("{"),
    /invalid team-management JSON/,
  );

  assert.throws(
    () => importManagementJson(
      '{"schemaVersion":999,"ratings":[],"matches":[]}',
    ),
    /unsupported team-management schema/,
  );
});
