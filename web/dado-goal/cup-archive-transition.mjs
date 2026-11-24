import {
  CUP_STORAGE_KEY as CUP,
  CUP_ARCHIVE_STORAGE_KEY as ARCHIVE,
  CUP_RECOVERY_STORAGE_KEY as RECOVERY,
  importVerifiedCupJson,
  loadCupArchive,
  loadCupRecovery,
} from "./cup-store.mjs";
import { MANAGEMENT_STORAGE_KEY as MANAGEMENT } from "./team-management-store.mjs";
import {
  createGlobalSaveTransaction,
  GLOBAL_SAVE_TRANSACTION_KEYS as KEYS,
  GLOBAL_SAVE_TRANSACTION_KEY as JOURNAL,
  GLOBAL_SAVE_GENERATION_KEY as GENERATION,
} from "./global-save-transaction.mjs";

export class CupArchiveTransitionError extends TypeError {
  constructor(code, cause) {
    super(code, { cause });
    this.name = "CupArchiveTransitionError";
    this.code = code;
  }
}

function check(condition, code) {
  if (!condition) throw new CupArchiveTransitionError(code);
}

function freeze(value) {
  if (value && typeof value === "object") {
    Object.values(value).forEach(freeze);
    Object.freeze(value);
  }
  return value;
}

// Raw images only. Never call a caller's storage, getters or serialization hooks.
function dataObject(value, keys) {
  check(value !== null && typeof value === "object" && !Array.isArray(value) &&
    [Object.prototype, null].includes(Object.getPrototypeOf(value)), "INVALID_INPUT");
  check(Reflect.ownKeys(value).length === keys.length &&
    Reflect.ownKeys(value).every(key => keys.includes(key)), "INVALID_INPUT");
  return Object.fromEntries(keys.map(key => {
    const descriptor = Object.getOwnPropertyDescriptor(value, key);
    check(descriptor?.enumerable && Object.hasOwn(descriptor, "value"), "INVALID_INPUT");
    return [key, descriptor.value];
  }));
}

function snapshotOf(value) {
  const snapshot = dataObject(value, ["images", "journal"]);
  // Even a valid persisted global journal requires priority recovery outside
  // preparation. A pure preparer cannot decide from the mixed physical keys.
  check(snapshot.journal === null, "GLOBAL_RECOVERY_REQUIRED");
  const images = dataObject(snapshot.images, KEYS);
  check(Object.values(images).every(raw => raw === null || typeof raw === "string"), "INVALID_INPUT");
  return images;
}

function detachedStorage(images) {
  return {
    getItem: key => key === JOURNAL ? null : images[key],
    setItem() { throw new CupArchiveTransitionError("UNEXPECTED_PREPARATION_WRITE"); },
    removeItem() { throw new CupArchiveTransitionError("UNEXPECTED_PREPARATION_WRITE"); },
  };
}

function validateImages(images) {
  // Public SP-02D1 admission reuses the SP-02C2 canonical graph authority,
  // including cross-Cup/competition match ownership and association recovery.
  // With a null journal this call only reads detached raw images.
  return createGlobalSaveTransaction(detachedStorage(images)).recover();
}

// JSON object member order is immaterial; array order and every field/value
// (including optional metadata and shootout history) must agree exactly.
function equivalent(a, b) {
  if (a === b) return true;
  if (a === null || b === null || typeof a !== "object" || typeof b !== "object" ||
      Array.isArray(a) !== Array.isArray(b)) return false;
  const keys = Object.keys(a);
  return keys.length === Object.keys(b).length &&
    keys.every(key => Object.hasOwn(b, key) && equivalent(a[key], b[key]));
}

/**
 * Pure SP-02D2 preparation. Inputs are internal, detached data, never portable
 * key maps or a Storage object. snapshot = {images, journal}: images contains
 * exactly the SP-02D1 allowlist, with raw string/null values; journal is the
 * separately captured global journal raw value and MUST be null after priority
 * recovery. cupJson is the exact completed Cup requested by the caller, retained
 * for retry even when the current key is absent. No ID/clock/random generation.
 *
 * Returns a deeply frozen plan:
 * {cupId, cup, disposition: "inserted"|"already-archived", before, after,
 *  transaction: {transactionId, operation:"archive", expectedGeneration, changes},
 *  requirements: {persistenceGate, cupOwnership:[cupId], noPendingAttempt:true}}.
 * `before`/`after` are complete raw images. Generation is engine-owned and is
 * excluded from changes. A reconciliation has changes:{} and identical images.
 * A current/archive ID collision is invalid even if equivalent (§9): successful
 * retry requires current absence. Unresolved journals are never cleared.
 *
 * SP-02E must hold the global gate and Cup ownership, recover first, capture
 * authoritative images, and call revalidateCupArchiveTransition immediately
 * before engine.transact(returnedRequest). Keep the gate/ownership across both
 * calls; no callbacks, draws, timers or async work between validation and commit.
 * This module neither acquires locks nor implements browser storage lifecycle.
 */
export function prepareCupArchiveTransition(input) {
  const { snapshot, cupJson, transactionId, expectedGeneration } = dataObject(input,
    ["snapshot", "cupJson", "transactionId", "expectedGeneration"]);
  check(typeof cupJson === "string" && typeof transactionId === "string" &&
    transactionId.trim().length > 0 && Number.isSafeInteger(expectedGeneration) &&
    expectedGeneration >= 0, "INVALID_INPUT");
  const before = snapshotOf(snapshot);
  try {
    const admitted = validateImages(before);
    check(admitted.generation === expectedGeneration, "GENERATION_MISMATCH");
    const storage = detachedStorage(before);
    const matches = before[MANAGEMENT] === null ? [] : JSON.parse(before[MANAGEMENT]).matches;
    const cup = importVerifiedCupJson(cupJson, matches);
    check(cup.championProviderTeamId !== undefined, "CUP_INCOMPLETE");
    const archive = loadCupArchive(storage, matches);
    const saved = archive.find(entry => entry.cupId === cup.cupId);
    const current = before[CUP] === null ? null : importVerifiedCupJson(before[CUP], matches);
    let changes;
    let disposition;
    if (current === null) {
      check(saved !== undefined, "CURRENT_CUP_MISSING");
      check(equivalent(saved, cup), "ARCHIVE_CONFLICT");
      // D1 also rejects recovery without its current owner. Never silently drop
      // a journal pointing at an archive or a different/missing competition.
      check(before[RECOVERY] === null, "CUP_RECOVERY_UNRESOLVED");
      changes = {};
      disposition = "already-archived";
    } else {
      check(current.cupId === cup.cupId && equivalent(current, cup), "CURRENT_CUP_MISMATCH");
      check(saved === undefined, "ARCHIVE_CONFLICT");
      const recovery = loadCupRecovery(storage);
      if (recovery !== null) {
        const fixture = current.fixtures.find(entry => entry.fixtureId === recovery.fixtureId);
        // A completed Cup can only acknowledge an already linked exact record.
        // No recovery-driven match linking or bracket advancement in this tranche.
        check(recovery.cupId === cup.cupId && fixture?.matchId === recovery.matchId,
          "CUP_RECOVERY_UNRESOLVED");
      }
      check(expectedGeneration < Number.MAX_SAFE_INTEGER, "GENERATION_OVERFLOW");
      changes = { [CUP]: null, [ARCHIVE]: JSON.stringify([...archive, current]), [RECOVERY]: null };
      disposition = "inserted";
    }
    const after = disposition === "inserted"
      ? { ...before, ...changes, [GENERATION]: String(expectedGeneration + 1) }
      : { ...before };
    validateImages(after);
    return freeze({
      cupId: cup.cupId, cup, disposition, before, after,
      transaction: { transactionId, operation: "archive", expectedGeneration, changes },
      requirements: { persistenceGate: "dado-goal.single-player.persistence",
        cupOwnership: [cup.cupId], noPendingAttempt: true },
    });
  } catch (cause) {
    if (cause instanceof CupArchiveTransitionError) throw cause;
    throw new CupArchiveTransitionError("INVALID_ARCHIVE_STATE", cause);
  }
}

/** Read-only final preflight for an internally prepared plan. Ownership facts
 * must come from SP-02E's actual held locks/coordinator, never from imported JSON.
 * Check every predecessor byte: ordinary writers need not advance generation.
 * Returns the exact SP-02D1 transact request; does not execute it.
 */
export function revalidateCupArchiveTransition(plan, evidence) {
  const { snapshot, ownsPersistenceGate, ownedCupId, hasPendingAttempt } = dataObject(evidence,
    ["snapshot", "ownsPersistenceGate", "ownedCupId", "hasPendingAttempt"]);
  check(ownsPersistenceGate === true, "PERSISTENCE_GATE_REQUIRED");
  check(ownedCupId === plan.cupId, "CUP_OWNERSHIP_REQUIRED");
  check(hasPendingAttempt === false, "PENDING_ATTEMPT");
  const images = snapshotOf(snapshot);
  check(KEYS.every(key => images[key] === plan.before[key]), "STALE_ARCHIVE_PLAN");
  return plan.transaction;
}
