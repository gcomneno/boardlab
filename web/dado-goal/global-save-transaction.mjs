import { prepareGlobalSave } from "./global-save.mjs";
import { loadCupRecovery } from "./cup-store.mjs";
import { loadTournamentRecovery } from "./tournament-store.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";
import { completeFixture } from "./tournament.mjs";

export const GLOBAL_SAVE_TRANSACTION_KEY = "dado-goal.global-save-transaction.v1";
export const GLOBAL_SAVE_GENERATION_KEY = "dado-goal.global-save-generation.v1";
export const GLOBAL_SAVE_TRANSACTION_KEYS = Object.freeze([
  "dado-goal.team-management.v1",
  "dado-goal.manager.v1",
  "dado-goal.tournament.v1",
  "dado-goal.tournament-recovery.v1",
  "dado-goal.cup.v1",
  "dado-goal.cup-archive.v1",
  "dado-goal.cup-recovery.v1",
  GLOBAL_SAVE_GENERATION_KEY,
]);
const [MANAGEMENT, MANAGER, TOURNAMENT, TOURNAMENT_RECOVERY, CUP, ARCHIVE, CUP_RECOVERY] =
  GLOBAL_SAVE_TRANSACTION_KEYS;
const operations = ["import", "archive", "reset"];
const natural = value => Number.isSafeInteger(value) && value >= 0;
const text = value => typeof value === "string" && value.trim().length > 0;

/** `blocked` forbids admission/mutation until recover() succeeds. `decision`
 * describes persisted evidence (none/prepared/committed/unknown), never a guess from
 * a thrown setItem. A committed decision must not be retried as a new operation.
 */
export class GlobalSaveTransactionError extends Error {
  constructor(code, { cause, blocked = true, decision = "unknown" } = {}) {
    super(code, { cause });
    this.name = "GlobalSaveTransactionError";
    this.code = code;
    this.blocked = blocked;
    this.decision = decision;
  }
}

function check(condition, code = "INVALID_PLAN") {
  if (!condition) throw new GlobalSaveTransactionError(code);
}

// Copy data properties only: no getters, toJSON, callbacks, symbols or coercion.
function dataMap(value, allowed, complete = false) {
  check(value !== null && typeof value === "object" && !Array.isArray(value));
  check([Object.prototype, null].includes(Object.getPrototypeOf(value)));
  const keys = Reflect.ownKeys(value);
  check(keys.every(key => allowed.includes(key)) && (!complete || keys.length === allowed.length));
  return Object.fromEntries(keys.map(key => {
    const descriptor = Object.getOwnPropertyDescriptor(value, key);
    check(descriptor.enumerable && Object.hasOwn(descriptor, "value"));
    return [key, descriptor.value];
  }));
}

function images(value) {
  const result = dataMap(value, GLOBAL_SAVE_TRANSACTION_KEYS, true);
  check(Object.values(result).every(value => value === null || typeof value === "string"));
  return Object.freeze(result);
}

function generation(source) {
  if (source === null) return 0;
  check(typeof source === "string" && /^(0|[1-9]\d*)$/.test(source) && natural(Number(source)),
    "INVALID_GENERATION");
  return Number(source);
}

// Reuse SP-02C2's complete persistent graph authority without exporting a second
// graph validator or depending on UI callbacks. This fixed idle envelope is only
// a validation adapter; it is never persisted or used to hydrate application UI.
function validatePersistent(persistent) {
  const roster = () => ({ active: Array.from({ length: 11 }, (_, i) => i + 1),
    bench: [], departed: [], sentOff: [], substitutesIn: [], substitutions: 0 });
  prepareGlobalSave({
    schemaVersion: 1, savedAt: "transaction-validation", locale: "it", persistent,
    configuration: {
      competition: "135:2024", hands: "1", randomHands: false, teams: ["", ""],
      colors: [0, 1].map(() => ({ primary: "#000000", secondary: "#ffffff", secondaryEnabled: false })),
      commentaryEnabled: false,
      auto: { mode: "all", handLimit: "all", rollDelay: "0", handDelay: "0" },
    },
    runtime: {
      state: null, names: ["Home", "Away"], colors: ["#000000", "#ffffff"],
      teamPalettes: [null, null], selectedMatchTeams: [null, null], rosters: [roster(), roster()],
      refereeStates: [{ yellowed: [] }, { yellowed: [] }],
      extraordinaryState: { occurredFamilies: [], occurredTypes: [] },
      rollEvents: [], matchEvents: [], matchStarted: false, matchRecorded: false,
      lastCommentary: null, lastRefereeEvent: null, lastExtraordinaryEvent: null,
      lastGoalScorer: null, messageKey: "setupPrompt", messageValues: {},
      competitionRef: null, activeTournamentFixtureId: null, tournamentViewedMatchday: null,
    },
  });
}

function validateImages(image) {
  generation(image[GLOBAL_SAVE_GENERATION_KEY]);
  const parse = (key, absent) => {
    if (image[key] === null) return absent;
    const value = JSON.parse(image[key]);
    check(value !== null, "INVALID_DOMAIN");
    return value;
  };
  const persistent = {
    management: parse(MANAGEMENT, { schemaVersion: 1, matches: [], ratings: [] }),
    manager: parse(MANAGER, null), tournament: parse(TOURNAMENT, null),
    cup: parse(CUP, null), cupArchive: parse(ARCHIVE, []),
  };
  validatePersistent(persistent);
  const detachedStorage = { getItem: key => image[key] };
  // Historical association journals are acceptable only if exactly recoverable.
  // Validate their detached resolution; preserve all original image bytes.
  for (const [key, recovery, identity, complete] of [
    ["tournament", loadTournamentRecovery(detachedStorage), "championshipId", completeFixture],
    ["cup", loadCupRecovery(detachedStorage), "cupId", completeCupFixture],
  ]) {
    if (recovery === null) continue;
    const owner = persistent[key];
    const fixture = owner?.fixtures.find(f => f.fixtureId === recovery.fixtureId);
    const match = persistent.management.matches.find(m => m.matchId === recovery.matchId);
    check(owner?.[identity] === recovery[identity] && fixture && match &&
      (fixture.matchId === null || fixture.matchId === recovery.matchId) &&
      String(match.home.providerTeamId) === String(fixture.homeProviderTeamId) &&
      String(match.away.providerTeamId) === String(fixture.awayProviderTeamId), "INVALID_RECOVERY_IMAGE");
    if (fixture.matchId === null) {
      persistent[key] = complete(owner, recovery.fixtureId, recovery.matchId);
      if (key === "cup") persistent.cup = advanceCupIfReady(
        persistent.cup, persistent.management.matches, persistent.cup.shootouts ?? {},
      ).cup;
    }
  }
  validatePersistent(persistent);
}

function parseJournal(source) {
  try {
    const parsed = JSON.parse(source);
    // JSON.parse alone silently accepts duplicate phase/image keys. Reject that
    // ambiguity, including differently escaped spellings of the same key.
    const stack = [];
    for (const token of source.match(/"(?:[^"\\]|\\.)*"|[{}\[\]:,]|true|false|null|-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?/g)) {
      if (token === "{") stack.push({ keys: new Set(), key: true });
      else if (token === "[") stack.push(null);
      else if (token === "}" || token === "]") stack.pop();
      else if (token === "," && stack.at(-1)) stack.at(-1).key = true;
      else if (token.startsWith('"') && stack.at(-1)?.key) {
        const top = stack.at(-1);
        const key = JSON.parse(token);
        check(!top.keys.has(key));
        top.keys.add(key);
        top.key = false;
      }
    }
    const journal = dataMap(parsed, ["schemaVersion", "transactionId", "operation", "phase", "before", "after"], true);
    check(journal.schemaVersion === 1 && text(journal.transactionId) && operations.includes(journal.operation) &&
      ["prepared", "committed"].includes(journal.phase));
    journal.before = images(journal.before);
    journal.after = images(journal.after);
    const epoch = generation(journal.before[GLOBAL_SAVE_GENERATION_KEY]);
    check(epoch < Number.MAX_SAFE_INTEGER && journal.after[GLOBAL_SAVE_GENERATION_KEY] === String(epoch + 1));
    validateImages(journal.before);
    validateImages(journal.after);
    return Object.freeze(journal);
  } catch (cause) {
    throw new GlobalSaveTransactionError("INVALID_JOURNAL", { cause });
  }
}

/**
 * Synchronous storage-only engine; no DOM, browser globals, random IDs, clocks,
 * async work or application callbacks. Inject a Storage-compatible object with
 * getItem/setItem/removeItem (each key replacement must be atomic).
 *
 * recover(): priority recovery AND validated snapshot admission. Returns frozen
 * {status: clean|rolled-back|rolled-forward, transactionId, generation, images,
 * usable: true}. No-journal reads still validate all persisted domain data.
 *
 * transact({transactionId, operation, expectedGeneration, changes}): accept only
 * internally prepared, serialized raw string/null changes, never portable input.
 * operation is import|archive|reset; transactionId is a caller-supplied nonempty
 * deterministic ID. changes is a subset of the allowlist excluding generation,
 * which is engine-owned. Omitted keys keep EXACT bytes; null removes a key.
 * expectedGeneration is required, including on no-op; absence of metadata is 0.
 * Returns {status: noop|committed, transactionId, generation, usable,
 * cleanupPending, images?}. Exact no-op performs no writes or epoch advance.
 * A verified commit with failed cleanup returns committed, usable:false,
 * cleanupPending:true (no images). Recover before using state; do not resubmit.
 * A precommit failure is recovered immediately if possible, then throws
 * TRANSACTION_ABORTED with blocked:false. Uncertain/failed recovery throws with
 * blocked:true; a fresh recover() must succeed before ANY loader or mutation.
 *
 * CALLER PRECONDITIONS (SP-02D2/E): hold the shared exclusive persistence gate
 * and required Cup ownership for EVERY call, before ordinary loaders/writers;
 * verify pending draws, predecessor bytes and full portable/runtime preparation
 * under that gate. This module does not implement locks or guard raw store calls.
 * Epoch checks without that external gate are NOT a multi-tab CAS/lock. No mixed
 * physical state is admitted by this API; localStorage itself is NOT multi-key
 * atomic. Persisted images require intact storage and enough space for recovery.
 */
export function createGlobalSaveTransaction(storage) {
  check(storage && ["getItem", "setItem", "removeItem"].every(key => typeof storage[key] === "function"),
    "STORAGE_UNAVAILABLE");
  let busy = false;
  function run(action) {
    check(!busy, "REENTRANT_OPERATION");
    busy = true;
    try { return action(); } finally { busy = false; }
  }
  function get(key) {
    const value = storage.getItem(key);
    check(value === null || typeof value === "string", "INVALID_STORAGE_READ");
    return value;
  }
  const readImages = () => images(Object.fromEntries(GLOBAL_SAVE_TRANSACTION_KEYS.map(key => [key, get(key)])));
  function verify(image) {
    for (const key of GLOBAL_SAVE_TRANSACTION_KEYS) check(get(key) === image[key], "IMAGE_VERIFICATION_FAILED");
  }
  function apply(image) {
    for (const key of GLOBAL_SAVE_TRANSACTION_KEYS) {
      if (image[key] === null) storage.removeItem(key);
      else storage.setItem(key, image[key]);
    }
    verify(image); // Exact equality to the already semantically validated target.
  }
  function cleanup() {
    storage.removeItem(GLOBAL_SAVE_TRANSACTION_KEY);
    check(get(GLOBAL_SAVE_TRANSACTION_KEY) === null, "CLEANUP_FAILED");
  }
  function snapshot(status, image, transactionId = null) {
    return Object.freeze({ status, transactionId, generation: generation(image[GLOBAL_SAVE_GENERATION_KEY]),
      images: image, usable: true });
  }
  function recoverInternal(expected = null, domainStarted = false) {
    let decision = "unknown";
    try {
      const source = get(GLOBAL_SAVE_TRANSACTION_KEY);
      if (source === null) {
        check(!domainStarted, "JOURNAL_MISSING");
        const image = readImages();
        validateImages(image);
        return snapshot("clean", image);
      }
      const journal = parseJournal(source); // Validate BOTH images before writes.
      if (expected !== null) check(journal.transactionId === expected.transactionId &&
        journal.operation === expected.operation && GLOBAL_SAVE_TRANSACTION_KEYS.every(key =>
          journal.before[key] === expected.before[key] && journal.after[key] === expected.after[key]), "JOURNAL_CHANGED");
      decision = journal.phase;
      const target = decision === "prepared" ? journal.before : journal.after;
      apply(target);
      cleanup();
      return snapshot(decision === "prepared" ? "rolled-back" : "rolled-forward", target, journal.transactionId);
    } catch (cause) {
      throw new GlobalSaveTransactionError(cause.code === "INVALID_JOURNAL" ? "INVALID_JOURNAL" : "RECOVERY_BLOCKED",
        { cause, decision });
    }
  }
  function accepted(journal, cleanupPending = false) {
    return Object.freeze({ status: "committed", transactionId: journal.transactionId,
      generation: generation(journal.after[GLOBAL_SAVE_GENERATION_KEY]), usable: !cleanupPending,
      cleanupPending, ...(!cleanupPending ? { images: journal.after } : {}) });
  }
  return Object.freeze({
    recover: () => run(() => recoverInternal()),
    transact: request => run(() => {
      const current = recoverInternal(); // Priority recovery, even for stale work.
      const plan = dataMap(request, ["transactionId", "operation", "expectedGeneration", "changes"], true);
      check(text(plan.transactionId) && operations.includes(plan.operation) && natural(plan.expectedGeneration));
      check(plan.expectedGeneration === current.generation, "GENERATION_MISMATCH");
      const changes = dataMap(plan.changes, GLOBAL_SAVE_TRANSACTION_KEYS.filter(key => key !== GLOBAL_SAVE_GENERATION_KEY));
      check(Object.values(changes).every(value => value === null || typeof value === "string"));
      if (Object.keys(changes).every(key => changes[key] === current.images[key])) {
        return Object.freeze({ ...current, status: "noop", transactionId: null, cleanupPending: false });
      }
      check(current.generation < Number.MAX_SAFE_INTEGER, "GENERATION_OVERFLOW");
      const after = images({ ...current.images, ...changes, [GLOBAL_SAVE_GENERATION_KEY]: String(current.generation + 1) });
      validateImages(after);
      const journal = { schemaVersion: 1, transactionId: plan.transactionId, operation: plan.operation,
        phase: "prepared", before: current.images, after };
      // Serialize BOTH phases before the first write. No callbacks run in apply,
      // verification, marker replacement or cleanup, including on error paths.
      const prepared = JSON.stringify(journal);
      const committed = JSON.stringify({ ...journal, phase: "committed" });
      let domainStarted = false;
      try {
        storage.setItem(GLOBAL_SAVE_TRANSACTION_KEY, prepared);
        check(get(GLOBAL_SAVE_TRANSACTION_KEY) === prepared, "JOURNAL_VERIFICATION_FAILED");
        domainStarted = true;
        apply(after);
        storage.setItem(GLOBAL_SAVE_TRANSACTION_KEY, committed);
        check(get(GLOBAL_SAVE_TRANSACTION_KEY) === committed, "COMMIT_VERIFICATION_FAILED");
      } catch (cause) {
        // An exception is not a decision. Reread persisted evidence under the
        // caller's gate, including when marker storage succeeded but threw.
        const recovered = recoverInternal(journal, domainStarted);
        if (recovered.status === "rolled-forward") return accepted(journal);
        throw new GlobalSaveTransactionError("TRANSACTION_ABORTED", { cause, blocked: false,
          decision: recovered.status === "clean" ? "none" : "prepared" });
      }
      try { cleanup(); }
      catch { return accepted(journal, true); }
      return accepted(journal);
    }),
  });
}
