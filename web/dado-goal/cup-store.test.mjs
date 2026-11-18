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

// M009 ARCHIVE TESTS
import {
  archiveCompletedCup,
  loadCupArchive,
  CUP_ARCHIVE_STORAGE_KEY,
} from "./cup-store.mjs";
import {
  completeCupFixture,
  advanceCupIfReady,
} from "./cup.mjs";

function completedCupForArchive() {
  let cup = sampleCup();
  const matches = [];

  for (let round = 1; round <= 2; round += 1) {
    const fixtures = cup.fixtures.filter(f => f.round === round);
    for (const fixture of fixtures) {
      const matchId = `archive-match-${fixture.fixtureId}`;
      matches.push({
        matchId,
        home: { providerTeamId: fixture.homeProviderTeamId },
        away: { providerTeamId: fixture.awayProviderTeamId },
        score: { home: 2, away: 0 },
      });
      cup = completeCupFixture(cup, fixture.fixtureId, matchId);
    }
    const result = advanceCupIfReady(cup, matches, {});
    assert.equal(result.status, "advanced");
    cup = result.cup;
  }

  assert.ok(cup.championProviderTeamId);
  return { cup, matches };
}

test("M009 archives a completed cup and preserves its results", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();

  const archive = archiveCompletedCup(storage, cup, matches);

  assert.equal(archive.length, 1);
  assert.deepEqual(loadCupArchive(storage, matches), [cup]);
  assert.ok(storage.values.has(CUP_ARCHIVE_STORAGE_KEY));
});

test("M009 refuses incomplete cups and duplicate archives", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();

  assert.throws(
    () => archiveCompletedCup(storage, sampleCup(), matches),
    /cup is not completed/,
  );

  archiveCompletedCup(storage, cup, matches);

  assert.throws(
    () => archiveCompletedCup(storage, cup, matches),
    /cup already archived/,
  );

  assert.equal(loadCupArchive(storage, matches).length, 1);
});

test("M009 rejects malformed archive without modifying storage", () => {
  const storage = memoryStorage();
  const raw = '{"invalid":"archive"}';
  storage.setItem(CUP_ARCHIVE_STORAGE_KEY, raw);

  assert.throws(
    () => loadCupArchive(storage, []),
    /invalid cup archive/,
  );
  assert.equal(storage.getItem(CUP_ARCHIVE_STORAGE_KEY), raw);
});

test("M009 archive does not overwrite active cup or unrelated data", () => {
  const storage = memoryStorage();
  const { cup, matches } = completedCupForArchive();
  const active = sampleCup();

  saveCup(storage, active);
  storage.setItem("other-data", "preserve");

  archiveCompletedCup(storage, cup, matches);

  assert.deepEqual(loadCup(storage), active);
  assert.equal(storage.getItem("other-data"), "preserve");
});
