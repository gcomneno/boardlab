import { prepareGlobalSave } from "./global-save.mjs";
import { loadCupRecovery } from "./cup-store.mjs";
import { loadTournamentRecovery } from "./tournament-store.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";
import { completeFixture } from "./tournament.mjs";

const KEYS = Object.freeze({
  management: "dado-goal.team-management.v1",
  manager: "dado-goal.manager.v1",
  tournament: "dado-goal.tournament.v1",
  tournamentRecovery: "dado-goal.tournament-recovery.v1",
  cup: "dado-goal.cup.v1",
  cupArchive: "dado-goal.cup-archive.v1",
  cupRecovery: "dado-goal.cup-recovery.v1",
});

const emptyManagement = () => ({
  schemaVersion: 1,
  matches: [],
  ratings: [],
});

function readImage(images, key, fallback) {
  if (!Object.hasOwn(images, key)) {
    throw new TypeError(`Missing snapshot image: ${key}`);
  }
  const raw = images[key];
  if (raw === null) return fallback;
  if (typeof raw !== "string") {
    throw new TypeError(`Invalid snapshot image: ${key}`);
  }
  return JSON.parse(raw);
}

function resolveRecovery(owner, recovery, identity, matches, complete) {
  if (recovery === null) return owner;

  if (!owner || owner[identity] !== recovery[identity]) {
    throw new TypeError("Recovery owner mismatch");
  }

  const fixture = owner.fixtures.find(
    entry => entry.fixtureId === recovery.fixtureId,
  );
  const match = matches.find(
    entry => entry.matchId === recovery.matchId,
  );

  if (
    !fixture ||
    !match ||
    (fixture.matchId !== null && fixture.matchId !== recovery.matchId) ||
    String(fixture.homeProviderTeamId) !==
      String(match.home.providerTeamId) ||
    String(fixture.awayProviderTeamId) !==
      String(match.away.providerTeamId)
  ) {
    throw new TypeError("Unresolved canonical fixture recovery");
  }

  return fixture.matchId === recovery.matchId
    ? owner
    : complete(owner, recovery.fixtureId, recovery.matchId);
}

/**
 * Build an export from a coordinator-admitted snapshot and a detached
 * runtime/configuration draft captured within the same shared gate.
 *
 * Ordinary association journals are projected on copies only.
 * No source storage mutation or journal deletion is performed here.
 */
export function projectPortablePersistentSnapshot(snapshot) {
  if (!snapshot || !snapshot.images) {
    throw new TypeError("Coordinator snapshot is required");
  }

  const images = snapshot.images;
  const storage = {
    getItem(key) {
      if (!Object.hasOwn(images, key)) {
        throw new TypeError(`Unknown snapshot key: ${key}`);
      }
      return images[key];
    },
  };

  const management = readImage(
    images, KEYS.management, emptyManagement(),
  );
  const matches = management.matches;
  const manager = readImage(images, KEYS.manager, null);
  let tournament = readImage(images, KEYS.tournament, null);
  let cup = readImage(images, KEYS.cup, null);
  const cupArchive = readImage(images, KEYS.cupArchive, []);

  tournament = resolveRecovery(
    tournament,
    loadTournamentRecovery(storage),
    "championshipId",
    matches,
    completeFixture,
  );

  const cupRecovery = loadCupRecovery(storage);
  const previousCup = cup;
  cup = resolveRecovery(
    cup,
    cupRecovery,
    "cupId",
    matches,
    completeCupFixture,
  );

  if (cupRecovery !== null && cup !== previousCup) {
    cup = advanceCupIfReady(
      cup, matches, cup.shootouts ?? {},
    ).cup;
  }

  return { management, manager, tournament, cup, cupArchive };
}

export function projectPortableGlobalSave(snapshot, draft) {
  if (!draft) {
    throw new TypeError("Runtime draft is required");
  }

  const persistent = projectPortablePersistentSnapshot(snapshot);

  const portable = {
    ...structuredClone(draft),
    persistent,
  };

  return prepareGlobalSave(portable).document;
}
