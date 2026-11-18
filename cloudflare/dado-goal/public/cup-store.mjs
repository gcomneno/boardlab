import { createCup, resolveCupTie, validateCupProgression } from "./cup.mjs";

export const CUP_SCHEMA_VERSION = 1;
export const CUP_STORAGE_KEY = "dado-goal.cup.v1";

export const CUP_RECOVERY_STORAGE_KEY = "dado-goal.cup-recovery.v1";

function validateCupRecovery(data) {
  if (!data || typeof data !== "object" ||
      typeof data.cupId !== "string" || !data.cupId.trim() ||
      typeof data.fixtureId !== "string" ||
      !/^r[1-9]\d*-p[1-9]\d*-l[12]$/.test(data.fixtureId) ||
      typeof data.matchId !== "string" || !data.matchId.trim()) {
    throw new TypeError("invalid cup recovery");
  }

  return {
    cupId: data.cupId,
    fixtureId: data.fixtureId,
    matchId: data.matchId,
  };
}

export function saveCupRecovery(storage, recovery) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  storage.setItem(
    CUP_RECOVERY_STORAGE_KEY,
    JSON.stringify(validateCupRecovery(recovery)),
  );
}

export function loadCupRecovery(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  const source = storage.getItem(CUP_RECOVERY_STORAGE_KEY);
  if (source === null) return null;

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid cup recovery JSON");
  }
  return validateCupRecovery(data);
}

export function clearCupRecovery(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  storage.removeItem(CUP_RECOVERY_STORAGE_KEY);
}


function validateCupDocument(data) {
  if (!data || data.schemaVersion !== CUP_SCHEMA_VERSION) {
    throw new TypeError("unsupported cup schema");
  }

  if (data.name !== undefined &&
      (typeof data.name !== "string" ||
       !data.name.trim() || data.name.length > 80)) {
    throw new TypeError("invalid cup name");
  }

  const canonical = createCup({
    cupId: data.cupId,
    participants: data.participants,
    legs: data.legs,
  });

  if (!Array.isArray(data.fixtures) ||
      data.fixtures.length < canonical.fixtures.length ||
      data.fixtures.length >
        (data.participants.length - 1) * data.legs) {
    throw new TypeError("invalid cup fixture count");
  }

  const ids = new Set();
  const matchIds = new Set();

  for (let i = 0; i < data.fixtures.length; i += 1) {
    const fixture = data.fixtures[i];

    if (!fixture || typeof fixture !== "object" ||
        typeof fixture.fixtureId !== "string" ||
        !Number.isSafeInteger(fixture.round) ||
        !Number.isSafeInteger(fixture.pair) ||
        !Number.isSafeInteger(fixture.leg) ||
        fixture.round < 1 || fixture.pair < 1 ||
        fixture.leg < 1 || fixture.leg > data.legs ||
        fixture.fixtureId !==
          `r${fixture.round}-p${fixture.pair}-l${fixture.leg}` ||
        ids.has(fixture.fixtureId) ||
        typeof fixture.homeProviderTeamId !== "string" ||
        typeof fixture.awayProviderTeamId !== "string" ||
        fixture.homeProviderTeamId === fixture.awayProviderTeamId ||
        (fixture.matchId !== null &&
         (typeof fixture.matchId !== "string" ||
          !fixture.matchId.trim() ||
          matchIds.has(fixture.matchId)))) {
      throw new TypeError("invalid cup fixture");
    }

    ids.add(fixture.fixtureId);
    if (fixture.matchId !== null) matchIds.add(fixture.matchId);

    if (i < canonical.fixtures.length) {
      const expected = canonical.fixtures[i];
      if (fixture.fixtureId !== expected.fixtureId ||
          fixture.homeProviderTeamId !== expected.homeProviderTeamId ||
          fixture.awayProviderTeamId !== expected.awayProviderTeamId) {
        throw new TypeError("non-canonical first cup round");
      }
    }
  }

  if (data.championProviderTeamId !== undefined &&
      (typeof data.championProviderTeamId !== "string" ||
       !data.participants.some(team =>
         String(team.providerTeamId) === data.championProviderTeamId))) {
    throw new TypeError("invalid cup champion");
  }

  if (data.shootouts !== undefined &&
      (!data.shootouts ||
       typeof data.shootouts !== "object" ||
       Array.isArray(data.shootouts))) {
    throw new TypeError("invalid cup shootouts");
  }

  return structuredClone(data);
}

export function createCupDocument({
  cupId,
  participants,
  legs = 1,
  name,
} = {}) {
  const cup = createCup({ cupId, participants, legs });
  return validateCupDocument({
    schemaVersion: CUP_SCHEMA_VERSION,
    ...cup,
    ...(name === undefined ? {} : { name }),
  });
}

export function exportCupJson(document) {
  return JSON.stringify(validateCupDocument(document), null, 2);
}

export function importCupJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("cup JSON must be a string");
  }

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid cup JSON");
  }

  return validateCupDocument(data);
}

export function saveCup(storage, document) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  storage.setItem(CUP_STORAGE_KEY, exportCupJson(document));
}

export function loadCup(storage) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  const source = storage.getItem(CUP_STORAGE_KEY);
  return source === null ? null : importCupJson(source);
}

export function clearCup(storage) {
  if (!storage || typeof storage.removeItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  storage.removeItem(CUP_STORAGE_KEY);
}

export function verifyCupDocument(document, matches, shootouts = {}) {
  const canonical = validateCupDocument(document);
  validateCupProgression(canonical, matches, shootouts);
  return canonical;
}

export function importVerifiedCupJson(source, matches, shootouts = {}) {
  return verifyCupDocument(importCupJson(source), matches, shootouts);
}

export function saveVerifiedCup(storage, document, matches) {
  if (!storage || typeof storage.setItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  const verified = verifyCupDocument(
    document, matches, document?.shootouts ?? {},
  );
  storage.setItem(CUP_STORAGE_KEY, JSON.stringify(verified, null, 2));
}

export function loadVerifiedCup(storage, matches) {
  if (!storage || typeof storage.getItem !== "function") {
    throw new TypeError("storage is unavailable");
  }
  const source = storage.getItem(CUP_STORAGE_KEY);
  if (source === null) return null;
  const document = importCupJson(source);
  return verifyCupDocument(document, matches, document.shootouts ?? {});
}

export function recordCupShootout(document, matches, round, pair, penalties) {
  const cup = verifyCupDocument(
    document, matches, document?.shootouts ?? {},
  );

  if (cup.championProviderTeamId !== undefined ||
      !Number.isSafeInteger(round) || round < 1 ||
      !Number.isSafeInteger(pair) || pair < 1) {
    throw new TypeError("invalid cup shootout target");
  }

  const currentRound = Math.max(...cup.fixtures.map(f => f.round));
  const key = `r${round}-p${pair}`;

  if (round !== currentRound ||
      Object.hasOwn(cup.shootouts ?? {}, key)) {
    throw new TypeError("shootout already decided or round inactive");
  }

  const fixtures = cup.fixtures
    .filter(f => f.round === round && f.pair === pair)
    .sort((a, b) => a.leg - b.leg);

  if (fixtures.length !== cup.legs ||
      fixtures.some((f, i) => f.leg !== i + 1)) {
    throw new TypeError("invalid cup shootout pairing");
  }

  const pending = resolveCupTie({ fixtures, matches });
  if (pending.decidedBy !== "pending-penalties") {
    throw new TypeError("shootout requires an aggregate tie");
  }

  const decided = resolveCupTie({ fixtures, matches, penalties });
  if (decided.decidedBy !== "penalties") {
    throw new TypeError("invalid cup shootout decision");
  }

  const next = structuredClone(cup);
  next.shootouts = {
    ...(next.shootouts ?? {}),
    [key]: decided.penalties,
  };

  return verifyCupDocument(next, matches, next.shootouts);
}
