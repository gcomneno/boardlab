import assert from "node:assert/strict";
import test from "node:test";

import { completeFixture } from "./tournament.mjs";
import {
  createTournamentDocument,
  loadTournament,
  saveTournament,
  TOURNAMENT_STORAGE_KEY,
} from "./tournament-store.mjs";

const teams = ["A", "B", "C", "D"].map(id => ({
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

test("persists and reconstructs championship progression", () => {
  const storage = memoryStorage();
  let document = createTournamentDocument({
    championshipId: "champ-1",
    participants: teams,
  });

  document = {
    ...document,
    ...completeFixture(
      document,
      document.fixtures[0].fixtureId,
      "match-1",
    ),
  };

  saveTournament(storage, document);
  const restored = loadTournament(storage);

  assert.deepEqual(restored, document);
  assert.equal(restored.fixtures[0].matchId, "match-1");

  const persisted = JSON.parse(
    storage.values.get(TOURNAMENT_STORAGE_KEY),
  );
  assert.equal("standings" in persisted, false);
  assert.equal("statistics" in persisted, false);
});

test("rejects non-canonical persisted schedules", () => {
  const storage = memoryStorage();
  const document = createTournamentDocument({
    championshipId: "champ-1",
    participants: teams,
  });

  const corrupted = structuredClone(document);
  corrupted.fixtures[0].homeProviderTeamId = "CORRUPTED";
  storage.setItem(
    TOURNAMENT_STORAGE_KEY,
    JSON.stringify(corrupted),
  );

  assert.throws(
    () => loadTournament(storage),
    /schedule is not canonical/,
  );
});
