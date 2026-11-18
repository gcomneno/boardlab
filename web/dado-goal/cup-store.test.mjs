import assert from "node:assert/strict";
import test from "node:test";

import {
  CUP_STORAGE_KEY,
  createCupDocument,
  exportCupJson,
  importCupJson,
  saveCup,
  loadCup,
} from "./cup-store.mjs";

const participants = ["A", "B", "C", "D"].map(id => ({
  provider: "test",
  providerTeamId: id,
  name: `Team ${id}`,
}));

function memoryStorage() {
  const values = new Map();
  return {
    getItem: key => values.get(key) ?? null,
    setItem: (key, value) => values.set(key, value),
    values,
  };
}

function sampleCup(legs = 1) {
  return createCupDocument({
    cupId: "cup-store-test",
    name: "Test Cup",
    participants,
    legs,
  });
}

test("persists and restores the canonical cup document", () => {
  const storage = memoryStorage();
  const document = sampleCup(2);

  saveCup(storage, document);
  const restored = loadCup(storage);

  assert.deepEqual(restored, document);
  assert.equal(restored.legs, 2);
  assert.equal(restored.fixtures.length, 4);
  assert.ok(storage.values.has(CUP_STORAGE_KEY));
});

test("round-trips exported cup JSON", () => {
  const document = sampleCup();
  const restored = importCupJson(exportCupJson(document));
  assert.deepEqual(restored, document);
});

test("rejects unsupported schema and malformed JSON", () => {
  const document = sampleCup();
  assert.throws(() => importCupJson("{invalid"), /invalid cup JSON/);
  assert.throws(
    () => importCupJson(JSON.stringify({
      ...document,
      schemaVersion: 999,
    })),
    /unsupported cup schema/,
  );
});

test("rejects non-canonical first-round fixtures", () => {
  const storage = memoryStorage();
  const corrupted = structuredClone(sampleCup());
  corrupted.fixtures[0].homeProviderTeamId = "CORRUPTED";

  storage.setItem(CUP_STORAGE_KEY, JSON.stringify(corrupted));

  assert.throws(
    () => loadCup(storage),
    /non-canonical first cup round/,
  );
});

test("does not overwrite unrelated storage entries", () => {
  const storage = memoryStorage();
  storage.setItem("other-data", "preserve");

  saveCup(storage, sampleCup());

  assert.equal(storage.getItem("other-data"), "preserve");
});
