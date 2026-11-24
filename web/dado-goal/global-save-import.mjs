import { prepareGlobalSave } from "./global-save.mjs";
import { projectPortablePersistentSnapshot } from "./global-save-export.mjs";
import {
  GLOBAL_SAVE_TRANSACTION_KEYS,
  GLOBAL_SAVE_GENERATION_KEY,
} from "./global-save-transaction.mjs";

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

function image(images, key, fallback) {
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

function serialize(value) {
  return value === null ? null : JSON.stringify(value);
}

/**
 * Pure import planner.
 *
 * The caller supplies a coordinator-admitted snapshot.
 * This function never accesses live storage and performs no writes.
 *
 * Ordinary recovery journals are deliberately blocking until the
 * coordinated reconciliation path is integrated.
 */
export function preparePortableGlobalSaveImport(
  snapshot,
  source,
  transactionId,
  options = {},
) {
  if (
    !snapshot ||
    !snapshot.images ||
    !Number.isSafeInteger(snapshot.generation) ||
    snapshot.generation < 0 ||
    typeof transactionId !== "string" ||
    transactionId.trim() === ""
  ) {
    throw new TypeError("Invalid import preparation inputs");
  }

  const images = snapshot.images;

  for (const key of GLOBAL_SAVE_TRANSACTION_KEYS) {
    if (!Object.hasOwn(images, key)) {
      throw new TypeError(`Incomplete import snapshot: ${key}`);
    }
  }

  if (snapshot.journal !== null) {
    throw new TypeError("Global transaction recovery is required");
  }

  // Reconcile ordinary journals on detached copies. Their source bytes
  // remain untouched until the journaled import transaction commits.
  const localState = {
    ...projectPortablePersistentSnapshot(snapshot),
    tournamentRecovery: null,
    cupRecovery: null,
    globalTransaction: null,
  };

  const prepared = prepareGlobalSave(source, {
    ...options,
    localState,
  });

  const changes = {
    [KEYS.management]: serialize(prepared.management),
    [KEYS.manager]: serialize(prepared.manager),
    [KEYS.tournament]: serialize(prepared.tournament),
    [KEYS.tournamentRecovery]: null,
    [KEYS.cup]: serialize(prepared.cup),
    [KEYS.cupArchive]: serialize(prepared.cupArchive),
    [KEYS.cupRecovery]: null,
  };

  if (Object.hasOwn(changes, GLOBAL_SAVE_GENERATION_KEY)) {
    throw new TypeError("Generation is engine-owned");
  }

  return Object.freeze({
    prepared,
    request: Object.freeze({
      transactionId,
      operation: "import",
      expectedGeneration: snapshot.generation,
      changes: Object.freeze(changes),
    }),
  });
}


/**
 * The caller must guard live runtime activity and supply any held Cup
 * ownership token. The coordinator fences the admitted predecessor.
 * No UI hydration is performed here.
 */
export async function executePortableGlobalSaveImport(
  coordinator, source, transactionId, options = {}, plannerOptions = {},
) {
  const expected = await coordinator.read(undefined, options);
  const plan = preparePortableGlobalSaveImport(
    expected, source, transactionId, plannerOptions,
  );

  let receipt;
  try {
    receipt = await coordinator.transact(expected, plan.request, options);
  } catch (cause) {
    if (cause?.decision === "committed") {
      throw new CommittedImportNeedsReload(cause);
    }
    throw cause;
  }

  if (!receipt?.usable) {
    if (receipt?.status !== "committed") {
      throw new CommittedImportNeedsReload();
    }

    let recovered;
    try {
      recovered = await coordinator.recover(options);
    } catch (cause) {
      throw new CommittedImportNeedsReload(cause);
    }

    const expectedGeneration = expected.generation + 1;
    const changes = plan.request.changes;
    const matchesCommittedImage =
      recovered?.generation === expectedGeneration &&
      recovered?.journal === null &&
      recovered?.images &&
      Object.entries(changes).every(
        ([key, value]) => recovered.images[key] === value,
      );

    if (!matchesCommittedImage) {
      throw new CommittedImportNeedsReload();
    }

    return Object.freeze({
      prepared: plan.prepared,
      receipt: Object.freeze({
        ...receipt,
        usable: true,
        cleanupPending: false,
        recovered: true,
      }),
    });
  }

  return Object.freeze({ prepared: plan.prepared, receipt });
}

export class CommittedImportNeedsReload extends Error {
  constructor(cause) {
    super("Import committed; reload required before continuing", { cause });
    this.name = "CommittedImportNeedsReload";
    this.committed = true;
  }
}
