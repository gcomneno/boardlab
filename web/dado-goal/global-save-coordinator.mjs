import {
  createGlobalSaveTransaction,
  GLOBAL_SAVE_TRANSACTION_KEYS as KEYS,
  GLOBAL_SAVE_TRANSACTION_KEY as JOURNAL,
  GLOBAL_SAVE_GENERATION_KEY as GENERATION,
} from "./global-save-transaction.mjs";
import { prepareCupArchiveTransition, revalidateCupArchiveTransition } from "./cup-archive-transition.mjs";
import { CUP_STORAGE_KEY as CUP, CUP_ARCHIVE_STORAGE_KEY as ARCHIVE,
  CUP_RECOVERY_STORAGE_KEY as CUP_RECOVERY } from "./cup-store.mjs";

export const SINGLE_PLAYER_PERSISTENCE_LOCK = "dado-goal.single-player.persistence";
export const CUP_CREATION_LOCK = "dado-goal.cup.create";

export class GlobalSaveCoordinatorError extends Error {
  constructor(code, cause) {
    super(code, { cause });
    this.name = "GlobalSaveCoordinatorError";
    this.code = code;
  }
}
const check = (condition, code) => {
  if (!condition) throw new GlobalSaveCoordinatorError(code);
};
const cupName = id => `dado-goal.cup:${id}`;
const cupKeys = [CUP, ARCHIVE, CUP_RECOVERY];
const isThenable = value => value !== null && ["object", "function"].includes(typeof value) &&
  typeof value.then === "function";

function dataProperties(value, keys, code, complete = false) {
  check(value && [Object.prototype, null].includes(Object.getPrototypeOf(value)), code);
  const names = Reflect.ownKeys(value);
  check(!complete || names.length === keys.length, code);
  const result = {};
  for (const key of names) {
    check(keys.includes(key), code);
    const descriptor = Object.getOwnPropertyDescriptor(value, key);
    check(descriptor.enumerable && Object.hasOwn(descriptor, "value"), code);
    result[key] = descriptor.value;
  }
  return result;
}

function optionsOf(value) {
  const result = dataProperties(value, ["signal", "ownership", "cupIds"], "INVALID_OPTIONS");
  if (result.cupIds !== undefined) {
    check(Array.isArray(result.cupIds), "INVALID_CUP_ID");
    result.cupIds = Object.freeze(Array.from(result.cupIds, id => {
      check(typeof id === "string" && id.trim().length > 0, "INVALID_CUP_ID");
      return id;
    }));
  }
  return Object.freeze(result);
}

// Only detached, already validated images enter this helper.
function cupIds(images) {
  const current = images[CUP] === null ? null : JSON.parse(images[CUP]);
  const archive = images[ARCHIVE] === null ? [] : JSON.parse(images[ARCHIVE]);
  return [...(current ? [current.cupId] : []), ...archive.map(cup => cup.cupId)];
}

/**
 * Cooperative origin-wide persistence boundary; no browser globals or UI wiring.
 * Inject {locks: navigator.locks, storage: localStorage, hasPendingAttempt}.
 * hasPendingAttempt is a required synchronous, read-only integration adapter and
 * must report the retained SP-01 failed-save candidate (true or false).
 *
 * read(project?, {signal, ownership}?) -> Promise<project(snapshot)>; without a
 * projector returns a frozen snapshot {generation, images, journal:null}. The
 * synchronous projector runs under a SHARED gate, after priority recovery and
 * pending-attempt checks; capture runtime, validate and serialize exports here.
 * It receives detached data, no live storage. Never retain live UI references.
 *
 * write(expectedSnapshot, command, options?) -> Promise<{value, snapshot}>.
 * Requires a snapshot issued by THIS instance. After recovery, compare generation
 * AND ALL exact predecessor bytes under an EXCLUSIVE gate, before calling the
 * synchronous command({storage, snapshot}). The revocable storage facade supports
 * existing synchronous public stores/shootout coordinator, only for the domain
 * allowlist; it cannot write generation/journal. Cup IDs newly introduced by a
 * command must be declared in options.cupIds. No new transaction per attempt.
 * Command exceptions do not roll back completed ordinary writes; keep existing
 * association-recovery protocols for those commands. Multi-key replacements
 * must use transact/archive, never a series of ordinary facade writes.
 * Pending attempts are allowed ONLY on this ordinary-command path, for SP-01
 * retry; the existing domain coordinator must reject other pending commands.
 * bindWriter(initialize) -> run(expected, command, options?). initialize(storage)
 * synchronously constructs a retained public component (e.g. SP-01 coordinator).
 * Its storage dispatcher is usable only during its own run's synchronous command,
 * which receives {component, snapshot}. This preserves a failed draw across runs
 * without retaining a released storage lease or recreating the domain component.
 *
 * transact(expectedSnapshot, request, options?) -> Promise<engine result> uses
 * SP-02D1's real engine; request is its internal serialized plan, never portable
 * input. archive(expectedSnapshot, {cupJson, transactionId}, options?) prepares,
 * revalidates and commits through SP-02D2/D1 without an application callback.
 * Both block pending attempts. They never hydrate UI or reprepare portable input.
 * recover(options?) -> Promise<snapshot> explicitly refreshes admitted persisted
 * state. read() is also an explicit refresh; writes NEVER refresh stale memory.
 *
 * withCupOwnership(lock, async token => {... await editorLifetime ...}) bridges
 * the EXISTING Cup lifetime lock. Call and AWAIT it INSIDE that Web Locks callback
 * with its actual exclusive Lock. Pass {ownership:token} to operations; no second
 * lifetime lock is acquired. Token validity ends with this scope. This bridge,
 * injected adapters and raw external stores are trusted integration boundaries:
 * no JS API can prove an externally supplied Lock is native/still held if its
 * owning callback violates this lifetime rule. Never supply booleans/imported
 * lock objects or end the outer lock callback early.
 *
 * Ordering: lifetime Cup -> global gate is allowed. Under global -> creation and
 * sorted Cup identities ONLY with ifAvailable:true. Busy releases all temporary
 * locks and the gate; never wait for foreign Cup ownership or reacquire our own.
 * Recovery validates the WHOLE journal on detached storage before acquiring its
 * before/after Cup identities and before the first live recovery write. Shared
 * readers release their gate before requesting exclusive recovery, then retry;
 * they never upgrade in place. Writer/recovery errors preserve engine evidence.
 *
 * Commands/projectors must be synchronous. Facades expire in finally, even on
 * exceptions/thenables; escaped async work cannot use them. No raw engine, gate
 * boolean or unchecked withGate callback is exposed. Independently held raw
 * storage and uncooperative writers remain outside this protocol's guarantee.
 * Cancellation uses Web Locks AbortSignal; unsupported/failed acquisition never
 * invokes commands. Once synchronous commit/recovery starts, let the engine finish
 * its persisted decision; cancellation never interrupts its journal protocol.
 * No timers, random sources or domain writes on clean admission.
 */
export function createGlobalSaveCoordinator({ locks, storage, hasPendingAttempt } = {}) {
  check(storage && ["getItem", "setItem", "removeItem"].every(k => typeof storage[k] === "function"),
    "STORAGE_UNAVAILABLE");
  check(typeof hasPendingAttempt === "function", "PENDING_ATTEMPT_ADAPTER_REQUIRED");
  const engine = createGlobalSaveTransaction(storage);
  const snapshots = new WeakSet();
  const ownerships = new WeakMap();
  const refresh = Symbol("explicit refresh");
  let inCallback = false;

  function get(key) {
    const raw = storage.getItem(key);
    check(raw === null || typeof raw === "string", "INVALID_STORAGE_READ");
    return raw;
  }
  function pending() {
    const value = hasPendingAttempt();
    check(typeof value === "boolean", "INVALID_PENDING_ATTEMPT_STATE");
    return value;
  }
  function synchronous(callback, argument) {
    check(typeof callback === "function" && callback.constructor?.name !== "AsyncFunction",
      "SYNCHRONOUS_CALLBACK_REQUIRED");
    const previous = inCallback;
    inCallback = true;
    try {
      const result = callback(argument);
      if (isThenable(result)) {
        // Observe rejection; the escaped task still has no valid storage facade.
        Promise.resolve(result).catch(() => {});
        throw new GlobalSaveCoordinatorError("SYNCHRONOUS_CALLBACK_REQUIRED");
      }
      return result;
    } finally { inCallback = previous; }
  }
  function snapshot(admitted) {
    const result = Object.freeze({ generation: admitted.generation, images: admitted.images, journal: null });
    snapshots.add(result);
    return result;
  }
  function fence(expected, current) {
    check(expected && snapshots.has(expected), "SNAPSHOT_REQUIRED");
    check(expected.generation === current.generation, "STALE_GENERATION");
    check(KEYS.every(key => expected.images[key] === current.images[key]), "SOURCE_MISMATCH");
  }
  function owned(token) {
    if (token === undefined) return null;
    const scope = ownerships.get(token);
    check(scope?.active, "CUP_OWNERSHIP_EXPIRED");
    return scope;
  }
  function cancelled(signal) {
    if (signal?.aborted) throw signal.reason ?? new GlobalSaveCoordinatorError("ACQUISITION_CANCELLED");
  }
  function request(name, options, action) {
    check(typeof locks?.request === "function", "WEB_LOCKS_UNAVAILABLE");
    return locks.request(name, options, lock => {
      check(lock && lock.name === name && lock.mode === options.mode, "LOCK_UNAVAILABLE");
      return action();
    });
  }
  async function gate(mode, options, action) {
    check(!inCallback, "REENTRANT_OPERATION");
    cancelled(options.signal);
    owned(options.ownership);
    return request(SINGLE_PLAYER_PERSISTENCE_LOCK,
      { mode, ...(options.signal ? { signal: options.signal } : {}) }, async () => {
        cancelled(options.signal);
        owned(options.ownership);
        return action();
      });
  }
  async function cups(ids, options, action) {
    const scope = owned(options.ownership);
    const names = [...new Set([CUP_CREATION_LOCK, ...ids.map(id => {
      check(typeof id === "string" && id.trim().length > 0, "INVALID_CUP_ID");
      return cupName(id);
    })])].sort().filter(name => name !== scope?.name);
    function acquire(index) {
      cancelled(options.signal);
      owned(options.ownership);
      if (index === names.length) return action(new Set(ids));
      return request(names[index], { mode: "exclusive", ifAvailable: true }, () => acquire(index + 1));
    }
    return acquire(0);
  }
  function detachedAdmission(images, journal = null) {
    const map = new Map(Object.entries(images));
    map.set(JOURNAL, journal);
    return createGlobalSaveTransaction({
      getItem: key => map.get(key) ?? null,
      setItem: (key, value) => { map.set(key, value); },
      removeItem: key => { map.delete(key); },
    }).recover();
  }
  function cleanAdmission() {
    check(get(JOURNAL) === null, "GLOBAL_RECOVERY_REQUIRED");
    return detachedAdmission(Object.fromEntries(KEYS.map(key => [key, get(key)])));
  }
  async function priority(options) {
    const raw = get(JOURNAL);
    if (raw === null) return cleanAdmission();
    // The engine validates both images and the journal grammar, including duplicate
    // keys, on a private map. Interrupted live keys are deliberately NOT parsed.
    detachedAdmission({}, raw);
    const journal = JSON.parse(raw);
    return cups([...cupIds(journal.before), ...cupIds(journal.after)], options, () => {
      check(get(JOURNAL) === raw, "JOURNAL_CHANGED");
      return engine.recover();
    });
  }
  async function exclusive(expected, options, action) {
    return gate("exclusive", options, async () => {
      const current = await priority(options);
      cancelled(options.signal);
      owned(options.ownership);
      if (expected !== refresh) fence(expected, current);
      return action(current);
    });
  }
  function fresh(expected) {
    check(get(JOURNAL) === null, "GLOBAL_RECOVERY_REQUIRED");
    const images = Object.fromEntries(KEYS.map(key => [key, get(key)]));
    check(images[GENERATION] === expected[GENERATION], "STALE_GENERATION");
    check(KEYS.every(key => images[key] === expected[key]), "SOURCE_MISMATCH");
  }

  const api = {
    bindWriter(initialize) {
      let lease = null;
      const use = () => {
        check(lease !== null, "GATE_SCOPE_EXPIRED");
        return lease;
      };
      const dispatcher = Object.freeze({
        getItem: key => use().getItem(key),
        setItem: (key, raw) => use().setItem(key, raw),
        removeItem: key => use().removeItem(key),
      });
      const component = synchronous(initialize, dispatcher);
      return (expected, command, options = {}) => api.write(expected, ({ storage: scoped, snapshot: current }) => {
        lease = scoped;
        try { return synchronous(command, Object.freeze({ component, snapshot: current })); }
        finally { lease = null; }
      }, options);
    },
    async withCupOwnership(lock, callback) {
      check(typeof locks?.request === "function", "WEB_LOCKS_UNAVAILABLE");
      check(lock?.mode === "exclusive" && typeof lock.name === "string" &&
        lock.name.startsWith("dado-goal.cup:") && lock.name.length > "dado-goal.cup:".length,
      "INVALID_CUP_OWNERSHIP");
      check(typeof callback === "function", "INVALID_CALLBACK");
      const token = Object.freeze({});
      const scope = { name: lock.name, active: true };
      ownerships.set(token, scope);
      try { return await callback(token); } finally { scope.active = false; }
    },
    async read(project = value => value, options = {}) {
      options = optionsOf(options);
      for (;;) {
        const result = await gate("shared", options, () => {
          if (get(JOURNAL) !== null) return null;
          const current = snapshot(cleanAdmission());
          check(!pending(), "PENDING_ATTEMPT");
          return { value: synchronous(project, current) };
        });
        if (result !== null) return result.value;
        // Release shared ownership before requesting an exclusive gate. A writer
        // may run between recovery and retry, hence the next journal recheck.
        await api.recover(options);
      }
    },
    async recover(options = {}) {
      options = optionsOf(options);
      return exclusive(refresh, options, current => snapshot(current));
    },
    async write(expected, command, options = {}) {
      options = optionsOf(options);
      return exclusive(expected, options, current =>
        cups([...cupIds(current.images), ...(options.cupIds ?? [])], options, held => {
          fresh(current.images);
          const predecessor = { ...current.images };
          let active = true;
          const live = () => {
            check(active, "GATE_SCOPE_EXPIRED");
            cancelled(options.signal);
            owned(options.ownership);
            fresh(predecessor);
          };
          function mutation(key, raw) {
            live();
            check(KEYS.includes(key) && key !== GENERATION, "STORAGE_KEY_FORBIDDEN");
            check(raw === null || typeof raw === "string", "INVALID_STORAGE_VALUE");
            if (cupKeys.includes(key)) {
              const proposed = { ...predecessor, [key]: raw };
              check(cupIds(proposed).every(id => held.has(id)), "CUP_OWNERSHIP_REQUIRED");
              if (key === CUP_RECOVERY && raw !== null) {
                check(held.has(JSON.parse(raw).cupId), "CUP_OWNERSHIP_REQUIRED");
              }
            }
            if (raw === null) storage.removeItem(key);
            else storage.setItem(key, raw);
            check(get(key) === raw, "WRITE_VERIFICATION_FAILED");
            predecessor[key] = raw;
          }
          const guardedStorage = Object.freeze({
            getItem(key) {
              live();
              check(KEYS.includes(key), "STORAGE_KEY_FORBIDDEN");
              return get(key);
            },
            setItem: (key, raw) => mutation(key, raw),
            removeItem: key => mutation(key, null),
          });
          let value;
          try {
            value = synchronous(command, Object.freeze({ storage: guardedStorage, snapshot: snapshot(current) }));
          } finally { active = false; }
          return Object.freeze({ value, snapshot: snapshot(cleanAdmission()) });
        }));
    },
    async transact(expected, requestPlan, options = {}) {
      options = optionsOf(options);
      // Copy/validate the request with the public engine on DETACHED storage.
      // This also serializes without invoking application callbacks under the gate.
      return exclusive(expected, options, current => {
        check(!pending(), "PENDING_ATTEMPT");
        const trial = new Map(Object.entries(current.images));
        const planEngine = createGlobalSaveTransaction({
          getItem: key => trial.get(key) ?? null,
          setItem: (key, value) => { trial.set(key, value); },
          removeItem: key => { trial.delete(key); },
        });
        const planned = planEngine.transact(requestPlan);
        // D1 rejects getters/hooks; retain a detached request across lock awaits.
        const plan = JSON.parse(JSON.stringify(requestPlan));
        return cups([...cupIds(current.images), ...cupIds(planned.images)], options, () => {
          fresh(current.images);
          check(!pending(), "PENDING_ATTEMPT");
          return engine.transact(plan);
        });
      });
    },
    async archive(expected, requestPlan, options = {}) {
      options = optionsOf(options);
      return exclusive(expected, options, current => {
        check(!pending(), "PENDING_ATTEMPT");
        const { cupJson, transactionId } = dataProperties(requestPlan,
          ["cupJson", "transactionId"], "INVALID_ARCHIVE_REQUEST", true);
        const captured = Object.freeze({ images: current.images, journal: null });
        const plan = prepareCupArchiveTransition({ snapshot: captured, cupJson, transactionId,
          expectedGeneration: current.generation });
        return cups([...cupIds(current.images), plan.cupId], options, () => {
          fresh(current.images);
          const requestPlan = revalidateCupArchiveTransition(plan, {
            snapshot: captured, ownsPersistenceGate: true, ownedCupId: plan.cupId,
            hasPendingAttempt: pending(),
          });
          return engine.transact(requestPlan);
        });
      });
    },
  };
  return Object.freeze(api);
}
