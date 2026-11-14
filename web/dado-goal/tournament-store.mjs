import {
  createChampionship,
  completeFixture,
} from "./tournament.mjs";

export const TOURNAMENT_SCHEMA_VERSION = 1;
export const TOURNAMENT_STORAGE_KEY = "dado-goal.tournament.v1";
export const TOURNAMENT_RECOVERY_STORAGE_KEY =
  "dado-goal.tournament-recovery.v1";

function validateDocument(data) {
  if (!data || data.schemaVersion !== TOURNAMENT_SCHEMA_VERSION) {
    throw new TypeError("unsupported tournament schema");
  }

  const championship = createChampionship({
    championshipId: data.championshipId,
    participants: data.participants,
  });

  if (!Array.isArray(data.fixtures) ||
      data.fixtures.length !== championship.fixtures.length) {
    throw new TypeError("invalid tournament fixtures");
  }

  let restored = championship;

  for (let index = 0; index < data.fixtures.length; index += 1) {
    const expected = championship.fixtures[index];
    const fixture = data.fixtures[index];

    if (fixture.fixtureId !== expected.fixtureId ||
        fixture.matchday !== expected.matchday ||
        String(fixture.homeProviderTeamId) !== expected.homeProviderTeamId ||
        String(fixture.awayProviderTeamId) !== expected.awayProviderTeamId) {
      throw new TypeError("tournament fixture schedule is not canonical");
    }

    if (fixture.matchId !== null) {
      restored = completeFixture(
        restored,
        fixture.fixtureId,
        fixture.matchId,
      );
    }
  }

  return restored;
}

export function createTournamentDocument({
  championshipId,
  participants,
  fixtures = null,
} = {}) {
  const championship = createChampionship({
    championshipId,
    participants,
  });

  if (fixtures === null) {
    return {
      schemaVersion: TOURNAMENT_SCHEMA_VERSION,
      ...structuredClone(championship),
    };
  }

  return {
    schemaVersion: TOURNAMENT_SCHEMA_VERSION,
    ...structuredClone(validateDocument({
      schemaVersion: TOURNAMENT_SCHEMA_VERSION,
      championshipId,
      participants,
      fixtures,
    })),
  };
}

export function exportTournamentJson(document) {
  const canonical = validateDocument(document);
  return JSON.stringify({
    schemaVersion: TOURNAMENT_SCHEMA_VERSION,
    ...canonical,
  }, null, 2);
}

export function importTournamentJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("tournament JSON must be a string");
  }

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid tournament JSON");
  }

  const canonical = validateDocument(data);
  return {
    schemaVersion: TOURNAMENT_SCHEMA_VERSION,
    ...canonical,
  };
}

export function saveTournament(storage, document) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.setItem(
    TOURNAMENT_STORAGE_KEY,
    exportTournamentJson(document),
  );
}

export function loadTournament(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  const source = storage.getItem(TOURNAMENT_STORAGE_KEY);
  return source === null ? null : importTournamentJson(source);
}

export function clearTournament(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.removeItem(TOURNAMENT_STORAGE_KEY);
}

function validateRecovery(data) {
  if (!data ||
      typeof data.championshipId !== "string" ||
      data.championshipId.length === 0 ||
      typeof data.fixtureId !== "string" ||
      data.fixtureId.length === 0 ||
      typeof data.matchId !== "string" ||
      data.matchId.length === 0) {
    throw new TypeError("invalid tournament recovery journal");
  }

  return {
    championshipId: data.championshipId,
    fixtureId: data.fixtureId,
    matchId: data.matchId,
  };
}

export function saveTournamentRecovery(storage, recovery) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.setItem(
    TOURNAMENT_RECOVERY_STORAGE_KEY,
    JSON.stringify(validateRecovery(recovery)),
  );
}

export function loadTournamentRecovery(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  const source = storage.getItem(TOURNAMENT_RECOVERY_STORAGE_KEY);
  if (source === null) return null;

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid tournament recovery JSON");
  }

  return validateRecovery(data);
}

export function clearTournamentRecovery(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }

  storage.removeItem(TOURNAMENT_RECOVERY_STORAGE_KEY);
}
