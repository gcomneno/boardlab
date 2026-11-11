import assert from "node:assert/strict";
import test from "node:test";

import { createManagerClub } from "./manager-profile.mjs";
import {
  MANAGER_SCHEMA_VERSION,
  MANAGER_STORAGE_KEY,
  clearManager,
  createManagerDocument,
  exportManagerJson,
  importManagerJson,
  loadManager,
  saveManager,
} from "./manager-store.mjs";
import { createManagementDocument } from "./team-management-store.mjs";

function team() {
  return {
    provider: "test",
    providerTeamId: "club-1",
    name: "Test Club",
    players: Array.from({ length: 20 }, (_, index) => ({
      provider: "test",
      providerPlayerId: `p-${index + 1}`,
      name: `Player ${index + 1}`,
      position:
        index === 0 ? "Goalkeeper" :
        index <= 4 ? "Defender" :
        index <= 8 ? "Midfielder" :
        index <= 10 ? "Attacker" :
        "Midfielder",
      shirtNumber: index + 1,
    })),
  };
}

function memoryStorage() {
  const values = new Map();

  return {
    getItem(key) {
      return values.has(key) ? values.get(key) : null;
    },
    setItem(key, value) {
      values.set(key, String(value));
    },
    removeItem(key) {
      values.delete(key);
    },
  };
}

test("manager save is explicitly versioned and catalogue-backed", () => {
  const club = createManagerClub(team());
  const document = createManagerDocument({ club });

  assert.equal(document.schemaVersion, MANAGER_SCHEMA_VERSION);
  assert.equal(document.club.providerTeamId, "club-1");
  assert.equal(document.club.lineup.length, 11);
  assert.equal(document.club.bench.length, 9);
  assert.equal(document.management.schemaVersion, 1);
});

test("manager JSON round-trip preserves club selection and history", () => {
  const club = createManagerClub(team());
  const management = createManagementDocument({
    ratings: [{ providerPlayerId: "p-1" }],
    matches: [{ matchId: "match-1" }],
  });

  const source = exportManagerJson(
    createManagerDocument({ club, management }),
  );
  const restored = importManagerJson(source);

  assert.deepEqual(restored.club.lineup, club.lineup);
  assert.deepEqual(restored.club.bench, club.bench);
  assert.equal(restored.management.matches[0].matchId, "match-1");
});

test("manager local storage uses an isolated key", () => {
  const storage = memoryStorage();
  const document = createManagerDocument({
    club: createManagerClub(team()),
  });

  saveManager(storage, document);

  assert.equal(storage.getItem(MANAGER_STORAGE_KEY) !== null, true);
  assert.deepEqual(loadManager(storage), document);

  clearManager(storage);
  assert.equal(loadManager(storage), null);
});

test("invalid manager imports are rejected", () => {
  assert.throws(
    () => importManagerJson("{"),
    /invalid manager JSON/,
  );

  assert.throws(
    () => importManagerJson(JSON.stringify({
      schemaVersion: 99,
      club: {},
      management: {},
    })),
    /unsupported manager schema/,
  );
});

test("manager store contains no implicit career systems", () => {
  const document = createManagerDocument({
    club: createManagerClub(team()),
  });

  for (const field of [
    "season",
    "league",
    "finances",
    "contracts",
    "transfers",
    "fatigue",
    "morale",
    "injuries",
    "training",
  ]) {
    assert.equal(Object.hasOwn(document, field), false);
  }
});

test("manager save persists an optional prepared next match", () => {
  const club = createManagerClub(team());
  const document = createManagerDocument({
    club,
    nextMatch: {
      opponentProviderTeamId: "opponent-2",
    },
  });

  const restored = importManagerJson(exportManagerJson(document));

  assert.deepEqual(restored.nextMatch, {
    opponentProviderTeamId: "opponent-2",
  });
});

test("legacy Manager M1 saves load with no prepared next match", () => {
  const club = createManagerClub(team());
  const legacy = JSON.stringify({
    schemaVersion: MANAGER_SCHEMA_VERSION,
    club,
    management: createManagementDocument(),
  });

  const restored = importManagerJson(legacy);

  assert.equal(restored.nextMatch, null);
});

test("manager save rejects the managed club as next opponent", () => {
  const club = createManagerClub(team());

  assert.throws(() => createManagerDocument({
    club,
    nextMatch: {
      opponentProviderTeamId: club.providerTeamId,
    },
  }), /cannot play itself/);
});
