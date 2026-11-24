import assert from "node:assert/strict";
import test from "node:test";
import { setImmediate as nextTurn } from "node:timers/promises";
import {
  createGlobalSaveCoordinator, GlobalSaveCoordinatorError,
  SINGLE_PLAYER_PERSISTENCE_LOCK as GATE, CUP_CREATION_LOCK as CREATE,
} from "./global-save-coordinator.mjs";
import {
  GLOBAL_SAVE_TRANSACTION_KEYS as KEYS, GLOBAL_SAVE_TRANSACTION_KEY as JOURNAL,
  GLOBAL_SAVE_GENERATION_KEY as GENERATION,
} from "./global-save-transaction.mjs";
import { createManagementDocument, saveManagement, MANAGEMENT_STORAGE_KEY as MANAGEMENT } from "./team-management-store.mjs";
import { executePortableGlobalSaveImport, CommittedImportNeedsReload } from "./global-save-import.mjs";
import { createCupDocument, createCupShootoutCoordinator, CUP_STORAGE_KEY as CUP,
  CUP_ARCHIVE_STORAGE_KEY as ARCHIVE } from "./cup-store.mjs";
import { createMatchRecord } from "./team-history.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";

const deferred = () => {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return { promise, resolve, reject };
};
const nameOf = id => `dado-goal.cup:${id}`;
const code = value => error => error instanceof GlobalSaveCoordinatorError && error.code === value;
const images = entries => Object.fromEntries(KEYS.map(key => [key, entries[key] ?? null]));
const entries = image => Object.fromEntries(Object.entries(image).filter(([, raw]) => raw !== null));
const teams = Array.from({ length: 4 }, (_, i) => ({
  provider: "test", providerTeamId: `team-${i}`, name: `Team ${i}`,
}));

class Storage {
  constructor(initial = {}) {
    this.map = new Map(Object.entries({ "foreign-key": "preserve", "dado-goal.multiplayer-seat.v1": "seat", ...initial }));
    this.writes = [];
    this.fault = null;
  }
  getItem(key) { this.fault?.("get", key); return this.map.get(key) ?? null; }
  setItem(key, raw) { this.fault?.("set", key); this.writes.push(["set", key, raw]); this.map.set(key, raw); }
  removeItem(key) { this.fault?.("remove", key); this.writes.push(["remove", key]); this.map.delete(key); }
}

// A deterministic origin-shared lock manager: fair shared/exclusive queues,
// nonblocking requests, cancellation while queued, and callback-lifetime release.
// Test barriers hold actual callbacks open; no wall-clock sleeps or fake leases.
class Locks {
  constructor() { this.states = new Map(); this.calls = []; this.events = []; }
  request(name, options, callback) {
    this.calls.push({ name, ...options });
    if (options.signal?.aborted) return Promise.reject(options.signal.reason);
    const state = this.states.get(name) ?? { held: [], queue: [] };
    this.states.set(name, state);
    const result = deferred();
    const item = { name, options, callback, result, abort: null };
    if (options.ifAvailable && (state.queue.length || !this.compatible(state, options.mode))) {
      return Promise.resolve().then(() => callback(null));
    }
    if (options.signal) {
      item.abort = () => {
        const index = state.queue.indexOf(item);
        if (index !== -1) {
          state.queue.splice(index, 1);
          result.reject(options.signal.reason);
          this.pump(state);
        }
      };
      options.signal.addEventListener("abort", item.abort, { once: true });
    }
    state.queue.push(item);
    this.pump(state);
    return result.promise;
  }
  compatible(state, mode) {
    return state.held.length === 0 || (mode === "shared" && state.held.every(i => i.options.mode === "shared"));
  }
  pump(state) {
    while (state.queue.length && this.compatible(state, state.queue[0].options.mode)) {
      const item = state.queue.shift();
      item.options.signal?.removeEventListener("abort", item.abort);
      state.held.push(item);
      this.events.push(["grant", item.name, item.options.mode, state.held.length]);
      Promise.resolve().then(() => item.callback(Object.freeze({ name: item.name, mode: item.options.mode })))
        .then(item.result.resolve, item.result.reject).finally(() => {
          state.held.splice(state.held.indexOf(item), 1);
          this.events.push(["release", item.name, item.options.mode]);
          this.pump(state);
        });
    }
  }
  held(name) { return this.states.get(name)?.held.length ?? 0; }
}

async function hold(locks, name = GATE, mode = "exclusive") {
  const entered = deferred();
  const release = deferred();
  const done = locks.request(name, { mode }, async lock => {
    entered.resolve(lock);
    await release.promise;
  });
  const lock = await entered.promise;
  return { lock, release: async () => { release.resolve(); await done; await nextTurn(); } };
}
function setup(initial = {}, locks = new Locks()) {
  const storage = new Storage(initial);
  let pending = false;
  const make = () => createGlobalSaveCoordinator({ locks, storage, hasPendingAttempt: () => pending });
  return { storage, locks, make, api: make(), setPending: value => { pending = value; } };
}
function cup(id = "cup-1") { return createCupDocument({ cupId: id, participants: teams }); }
function replacement(snapshot, changes = {}, operation = "import") {
  return { transactionId: "deterministic-transaction", operation,
    expectedGeneration: snapshot.generation, changes };
}
function journalState(phase = "prepared", withCup = false) {
  const before = images({ [MANAGEMENT]: ` \n${JSON.stringify(createManagementDocument())}\t`,
    ...(withCup ? { [CUP]: JSON.stringify(cup("old")) } : {}) });
  const after = { ...before, [GENERATION]: "1", [MANAGEMENT]: JSON.stringify(createManagementDocument()),
    ...(withCup ? { [CUP]: JSON.stringify(cup("new")) } : {}) };
  const journal = JSON.stringify({ schemaVersion: 1, transactionId: "interrupted", operation: "import", phase, before, after });
  // Mixed/corrupt physical state must not be validated instead of the journal.
  return { before, after, journal, initial: { ...entries(after), [CUP]: "interrupted bytes", [JOURNAL]: journal } };
}
function completedCup(id = "complete") {
  let document = cup(id);
  const matches = [];
  while (document.championProviderTeamId === undefined) {
    for (const fixture of document.fixtures.filter(f => f.matchId === null)) {
      const match = createMatchRecord({ matchId: `${id}:${fixture.fixtureId}`, playedAt: "original",
        home: teams.find(t => t.providerTeamId === fixture.homeProviderTeamId),
        away: teams.find(t => t.providerTeamId === fixture.awayProviderTeamId), homeGoals: 1, awayGoals: 0 });
      matches.push(match);
      document = completeCupFixture(document, fixture.fixtureId, match.matchId);
    }
    document = advanceCupIfReady(document, matches).cup;
  }
  return { document, matches };
}

test("clean acquisition validates detached data and performs zero live writes", async () => {
  const { api, storage } = setup({ [MANAGEMENT]: " \n" + JSON.stringify(createManagementDocument()) });
  const first = await api.read();
  const second = await api.recover();
  assert.equal(first.generation, 0);
  assert.deepEqual(first, second);
  assert.ok(Object.isFrozen(first) && Object.isFrozen(first.images));
  assert.equal(first.journal, null);
  assert.deepEqual(storage.writes, []);
});

test("independent instances use the identical origin-wide exclusive gate", async () => {
  const { api, make, locks, storage } = setup();
  const other = make();
  const [a, b] = await Promise.all([api.read(), other.read()]);
  const order = [];
  const blocker = await hold(locks);
  const one = api.write(a, ({ storage: scoped }) => {
    order.push("one"); saveManagement(scoped, createManagementDocument());
  });
  const two = other.write(b, () => { order.push("two"); });
  const rejected = assert.rejects(two, code("SOURCE_MISMATCH"));
  await nextTurn();
  assert.deepEqual(order, []);
  await blocker.release();
  await one; await rejected;
  assert.deepEqual(order, ["one"]);
  assert.equal(storage.writes.length, 1);
  assert.ok(locks.events.filter(e => e[0] === "grant" && e[1] === GATE && e[2] === "exclusive").every(e => e[3] === 1));
});

test("shared readers coexist; a writer waits for all shared holders", async () => {
  const { api, make, locks } = setup();
  const baseline = await api.read();
  const reader = await hold(locks, GATE, "shared");
  const readerTwo = await hold(locks, GATE, "shared");
  let projected = false, wrote = false;
  await api.read(snapshot => { projected = true; return snapshot; });
  const other = make();
  const writer = other.write(await other.read(), () => {});
  const real = api.write(baseline, () => { wrote = true; });
  await nextTurn();
  assert.equal(projected, true);
  assert.equal(wrote, false);
  assert.ok(locks.events.some(e => e[0] === "grant" && e[1] === GATE && e[2] === "shared" && e[3] > 1));
  await reader.release();
  assert.equal(wrote, false);
  await readerTwo.release();
  await writer; await real;
  assert.equal(wrote, true);
});

test("shared snapshot waits for an exclusive writer", async () => {
  const { api, locks } = setup();
  const writer = await hold(locks);
  let read = false;
  const promise = api.read(snapshot => { read = true; return snapshot; });
  await nextTurn(); assert.equal(read, false);
  await writer.release(); await promise;
  assert.equal(read, true);
});

test("simultaneous shared calls from separate instances capture coherent bytes", async () => {
  const { api, make, locks } = setup();
  const values = await Promise.all([api.read(), make().read(), make().read()]);
  assert.deepEqual(values[0], values[1]); assert.deepEqual(values[1], values[2]);
  assert.ok(locks.events.some(e => e[0] === "grant" && e[1] === GATE && e[3] === 3));
});

for (const operation of ["write", "transact", "archive"]) {
  for (const invalid of [null, undefined, {}, { generation: 0, images: images({}), journal: null }]) {
    test(`${operation} refuses an absent/forged snapshot (${String(invalid)})`, async () => {
      const { api, storage } = setup();
      let invoked = false;
      const promise = operation === "write" ? api.write(invalid, () => { invoked = true; })
        : operation === "transact" ? api.transact(invalid, {}) : api.archive(invalid, {});
      await assert.rejects(promise, code("SNAPSHOT_REQUIRED"));
      assert.equal(invoked, false); assert.deepEqual(storage.writes, []);
    });
  }
}

test("replacement invalidates old and queued memory even without storage events", async () => {
  const { api, make } = setup();
  const other = make();
  const [old, second] = await Promise.all([api.read(), other.read()]);
  await other.transact(second, replacement(second, { [MANAGEMENT]: JSON.stringify(createManagementDocument()) }));
  await assert.rejects(api.write(old, () => assert.fail("stale command")), code("STALE_GENERATION"));
  const refreshed = await api.read();
  assert.equal(refreshed.generation, 1);
  await api.write(refreshed, () => "explicit refresh accepted");
});

for (const change of ["whitespace", "absence-to-zero"]) {
  test(`source fence detects ${change} even when numeric generation is unchanged`, async () => {
    const { api, storage } = setup({ [MANAGEMENT]: JSON.stringify(createManagementDocument()) });
    const old = await api.read();
    if (change === "whitespace") storage.map.set(MANAGEMENT, " " + storage.map.get(MANAGEMENT));
    else storage.map.set(GENERATION, "0");
    await assert.rejects(api.write(old, () => assert.fail("stale bytes")), code("SOURCE_MISMATCH"));
    assert.deepEqual(storage.writes, []);
  });
}

test("final fence runs after temporary lock acquisition, before the command/draw", async () => {
  const { api, locks, storage } = setup();
  const old = await api.read();
  const request = locks.request.bind(locks);
  locks.request = (name, options, callback) => request(name, options, lock => {
    if (name === CREATE) storage.map.set(MANAGEMENT, JSON.stringify(createManagementDocument()));
    return callback(lock);
  });
  await assert.rejects(api.write(old, () => assert.fail("must not draw")), code("SOURCE_MISMATCH"));
  assert.deepEqual(storage.writes, []);
});

for (const phase of ["prepared", "committed"]) {
  for (const admission of ["read", "write", "recover"]) {
    test(`${admission} prioritizes ${phase} journal over mixed physical keys`, async () => {
      const state = journalState(phase);
      const { api, storage, locks } = setup(entries(state.before));
      const expected = await api.read();
      for (const [key, raw] of Object.entries(state.initial)) storage.map.set(key, raw);
      if (admission === "write" && phase === "committed") {
        await assert.rejects(api.write(expected, () => assert.fail("stale")), code("STALE_GENERATION"));
      } else {
        let observed;
        if (admission === "read") observed = await api.read();
        else if (admission === "recover") observed = await api.recover();
        else {
          // Recovery precedes this writer and restores its exact expected bytes.
          observed = (await api.write(expected, ({ snapshot }) => snapshot)).value;
        }
        assert.deepEqual(observed.images, phase === "prepared" ? state.before : state.after);
      }
      assert.equal(storage.getItem(JOURNAL), null);
      assert.equal(storage.getItem(CUP), null);
      assert.ok(locks.calls.some(c => c.name === GATE && c.mode === "exclusive"));
      assert.equal(storage.getItem("foreign-key"), "preserve");
      assert.equal(storage.getItem("dado-goal.multiplayer-seat.v1"), "seat");
    });
  }
}

test("read releases shared gate before exclusive recovery, then recaptures", async () => {
  const state = journalState();
  const { api, locks } = setup(state.initial);
  await api.read();
  assert.deepEqual(locks.events.filter(e => e[1] === GATE).map(e => e.slice(0, 3)), [
    ["grant", GATE, "shared"], ["release", GATE, "shared"],
    ["grant", GATE, "exclusive"], ["release", GATE, "exclusive"],
    ["grant", GATE, "shared"], ["release", GATE, "shared"],
  ]);
});

for (const raw of ["{", "null", '{"schemaVersion":999}',
  (() => { const s = journalState(); return s.journal.replace('"phase":"prepared"', '"phase":"prepared","phase":"committed"'); })()]) {
  test(`invalid journal fails closed without writes: ${raw.slice(0, 30)}`, async () => {
    const { api, storage } = setup({ [JOURNAL]: raw });
    await assert.rejects(api.read(), { code: "INVALID_JOURNAL" });
    assert.equal(storage.getItem(JOURNAL), raw);
    assert.deepEqual(storage.writes, []);
  });
}

for (const boundary of ["get", "set", "remove"]) {
  test(`recovery ${boundary} failure blocks exports and commands, retains journal, permits retry`, async () => {
    const state = journalState("committed");
    const { api, storage } = setup(entries(state.before));
    const before = await api.read();
    for (const [key, raw] of Object.entries(state.initial)) storage.map.set(key, raw);
    storage.fault = (method, key) => {
      if (method === boundary && (boundary !== "remove" || key === JOURNAL)) throw new Error("offline");
    };
    await assert.rejects(api.read(() => assert.fail("export blocked")));
    await assert.rejects(api.write(before, () => assert.fail("command blocked")));
    assert.equal(storage.map.get(JOURNAL), state.journal);
    storage.fault = null;
    assert.deepEqual((await api.recover()).images, state.after);
    assert.equal(storage.getItem(JOURNAL), null);
  });
}

test("journal recovery precedes pending-attempt export rejection", async () => {
  const state = journalState("committed");
  const { api, storage, setPending } = setup(state.initial);
  setPending(true);
  await assert.rejects(api.read(), code("PENDING_ATTEMPT"));
  assert.equal(storage.getItem(JOURNAL), null);
  assert.equal(storage.getItem(GENERATION), "1");
});

for (const operation of ["read", "transact", "archive"]) {
  test(`pending attempt blocks ${operation} without deleting the candidate`, async () => {
    const { api, storage, setPending } = setup();
    const old = await api.read();
    setPending(true);
    const run = operation === "read" ? api.read() : operation === "transact"
      ? api.transact(old, replacement(old)) : api.archive(old, {});
    await assert.rejects(run, code("PENDING_ATTEMPT"));
    assert.deepEqual(storage.writes, []);
    await assert.rejects(api.read(), code("PENDING_ATTEMPT"));
  });
}

test("pending-attempt adapter is required and must return an exact boolean", async () => {
  const { storage, locks } = setup();
  assert.throws(() => createGlobalSaveCoordinator({ storage, locks }), code("PENDING_ATTEMPT_ADAPTER_REQUIRED"));
  const api = createGlobalSaveCoordinator({ storage, locks, hasPendingAttempt: () => undefined });
  await assert.rejects(api.read(), code("INVALID_PENDING_ATTEMPT_STATE"));
});

for (const foreign of ["old", "new"]) {
  test(`import nonblockingly rejects foreign ${foreign} Cup ownership and releases all locks`, async () => {
    const { api, locks, storage } = setup({ [CUP]: JSON.stringify(cup("old")) });
    const baseline = await api.read();
    const editor = await hold(locks, nameOf(foreign));
    const original = new Map(storage.map);
    await assert.rejects(api.transact(baseline, replacement(baseline, { [CUP]: JSON.stringify(cup("new")) })),
      code("LOCK_UNAVAILABLE"));
    assert.deepEqual(storage.map, original);
    assert.equal(locks.held(GATE), 0); assert.equal(locks.held(CREATE), 0);
    assert.ok(locks.calls.filter(c => c.name.startsWith("dado-goal.cup") && c.ifAvailable !== undefined)
      .every(c => c.ifAvailable === true));
    await editor.release();
  });
}

test("current Cup owner can await the global gate while competing import reports busy", async () => {
  const { api, make, locks } = setup({ [CUP]: JSON.stringify(cup("owned")) });
  const importer = make();
  const [a, b] = await Promise.all([api.read(), importer.read()]);
  const editor = await hold(locks, nameOf("owned"));
  const blocker = await hold(locks);
  await api.withCupOwnership(editor.lock, async ownership => {
    const importing = importer.transact(b, replacement(b, { [CUP]: null }));
    const rejected = assert.rejects(importing, code("LOCK_UNAVAILABLE"));
    let wrote = false;
    const writing = api.write(a, () => { wrote = true; }, { ownership });
    await nextTurn(); assert.equal(wrote, false);
    await blocker.release();
    await rejected; await writing;
    assert.equal(wrote, true);
  });
  // Only the import tried the owned lock: the owner never reacquired itself.
  assert.equal(locks.calls.filter(c => c.name === nameOf("owned")).length, 2);
  await editor.release();
});

test("multiple Cup IDs use unique sorted nonblocking locks", async () => {
  const { api, locks } = setup();
  const baseline = await api.read();
  const index = locks.calls.length;
  await api.write(baseline, () => {}, { cupIds: ["z", "a", "z", "b"] });
  const calls = locks.calls.slice(index);
  assert.equal(calls[0].name, GATE);
  assert.deepEqual(calls.slice(1).map(c => c.name), [CREATE, nameOf("a"), nameOf("b"), nameOf("z")]);
  assert.ok(calls.slice(1).every(c => c.mode === "exclusive" && c.ifAvailable === true));
});

test("busy Cup creation lock does not block while holding the gate", async () => {
  const { api, locks } = setup();
  const baseline = await api.read();
  const creator = await hold(locks, CREATE);
  await assert.rejects(api.write(baseline, () => assert.fail("busy")), code("LOCK_UNAVAILABLE"));
  assert.equal(locks.held(GATE), 0);
  await creator.release();
  await api.write(baseline, () => {});
});

for (const phase of ["prepared", "committed"]) {
  test(`${phase} recovery respects ownership of BOTH journal images before live writes`, async () => {
    const state = journalState(phase, true);
    const { api, locks, storage } = setup(state.initial);
    const editor = await hold(locks, nameOf(phase === "prepared" ? "new" : "old"));
    await assert.rejects(api.recover(), code("LOCK_UNAVAILABLE"));
    assert.deepEqual(storage.writes, []);
    assert.equal(storage.getItem(JOURNAL), state.journal);
    assert.equal(locks.held(GATE), 0);
    await editor.release();
    assert.deepEqual((await api.recover()).images, phase === "prepared" ? state.before : state.after);
  });
}

test("ownership capability expires with its scope and cannot be forged or borrowed from another instance", async () => {
  const { api, make, locks } = setup();
  const baseline = await api.read();
  const editor = await hold(locks, nameOf("editor"));
  let token;
  await api.withCupOwnership(editor.lock, async ownership => {
    token = ownership;
    await assert.rejects(make().read(undefined, { ownership }), code("CUP_OWNERSHIP_EXPIRED"));
  });
  for (const ownership of [token, {}, { cupId: "editor", active: true }]) {
    await assert.rejects(api.write(baseline, () => assert.fail("expired"), { ownership }), code("CUP_OWNERSHIP_EXPIRED"));
  }
  await editor.release();
});

test("expired ownership while queued fails before executing commands", async () => {
  const { api, locks } = setup();
  const baseline = await api.read();
  const editor = await hold(locks, nameOf("editor"));
  const blocker = await hold(locks);
  let queued, rejected;
  await api.withCupOwnership(editor.lock, ownership => {
    queued = api.write(baseline, () => assert.fail("expired scope"), { ownership });
    rejected = assert.rejects(queued, code("CUP_OWNERSHIP_EXPIRED"));
  });
  await blocker.release(); await rejected; await editor.release();
});

for (const lock of [null, { name: CREATE, mode: "exclusive" }, { name: nameOf("x"), mode: "shared" }]) {
  test("ownership bridge rejects a missing/wrong kind of Lock", async () => {
    const { api } = setup();
    await assert.rejects(api.withCupOwnership(lock, () => assert.fail("invalid")), code("INVALID_CUP_OWNERSHIP"));
  });
}

test("normal Cup creation requires declared ownership and creation gate", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  await assert.rejects(api.write(baseline, ({ storage: scoped }) => scoped.setItem(CUP, JSON.stringify(cup("new")))),
    code("CUP_OWNERSHIP_REQUIRED"));
  assert.deepEqual(storage.writes, []);
  await api.write(baseline, ({ storage: scoped }) => scoped.setItem(CUP, JSON.stringify(cup("new"))), { cupIds: ["new"] });
  assert.equal(JSON.parse(storage.getItem(CUP)).cupId, "new");
});

test("archive uses real D2 preparation/revalidation and D1 engine; idempotent retry does not append", async () => {
  const { document, matches } = completedCup();
  const { api, storage } = setup({ [CUP]: JSON.stringify(document),
    [MANAGEMENT]: JSON.stringify(createManagementDocument({ matches })) });
  const before = await api.read();
  const result = await api.archive(before, { cupJson: JSON.stringify(document), transactionId: "archive" });
  assert.equal(result.status, "committed"); assert.equal(result.generation, 1);
  assert.equal(storage.getItem(CUP), null);
  assert.deepEqual(JSON.parse(storage.getItem(ARCHIVE)), [document]);
  const count = storage.writes.length;
  assert.equal((await api.archive(await api.read(), { cupJson: JSON.stringify(document), transactionId: "retry" })).status, "noop");
  assert.equal(storage.writes.length, count);
});

test("ordinary callbacks cannot write generation, journal or unrelated/multiplayer keys", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  for (const key of [GENERATION, JOURNAL, "foreign-key", "dado-goal.multiplayer-seat.v1"]) {
    await assert.rejects(api.write(baseline, ({ storage: scoped }) => scoped.setItem(key, "0")), code("STORAGE_KEY_FORBIDDEN"));
  }
  assert.deepEqual(storage.writes, []);
});

for (const kind of ["writer", "projector"]) {
  test(`${kind} exceptions release locks without manufacturing domain writes`, async () => {
    const { api, locks, storage } = setup();
    const baseline = await api.read();
    const failure = new Error("application failed");
    await assert.rejects(kind === "writer" ? api.write(baseline, () => { throw failure; })
      : api.read(() => { throw failure; }), error => error === failure);
    assert.equal(locks.held(GATE), 0); assert.equal(locks.held(CREATE), 0);
    assert.deepEqual(storage.writes, []);
    await api.write(baseline, () => {});
  });
}

test("escaped storage facade is revoked after both successful and failed commands", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  let escaped;
  await api.write(baseline, ({ storage: scoped }) => { escaped = scoped; });
  assert.throws(() => escaped.getItem(MANAGEMENT), code("GATE_SCOPE_EXPIRED"));
  assert.throws(() => escaped.setItem(MANAGEMENT, "{}"), code("GATE_SCOPE_EXPIRED"));
  await assert.rejects(api.write(baseline, ({ storage: scoped }) => { escaped = scoped; throw new Error("failed"); }));
  assert.throws(() => escaped.removeItem(MANAGEMENT), code("GATE_SCOPE_EXPIRED"));
  assert.deepEqual(storage.writes, []);
});

test("async callbacks are rejected before invocation; escaped thenable work has revoked storage", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  await assert.rejects(api.write(baseline, async () => assert.fail("async body")), code("SYNCHRONOUS_CALLBACK_REQUIRED"));
  const proceed = deferred();
  let late;
  await assert.rejects(api.write(baseline, ({ storage: scoped }) => {
    late = proceed.promise.then(() => scoped.setItem(MANAGEMENT, "{}"));
    return late;
  }), code("SYNCHRONOUS_CALLBACK_REQUIRED"));
  proceed.resolve();
  await assert.rejects(late, code("GATE_SCOPE_EXPIRED"));
  assert.deepEqual(storage.writes, []);
});

test("nested coordinator acquisition rejects instead of deadlocking", async () => {
  const { api } = setup();
  const baseline = await api.read();
  let nested;
  await api.write(baseline, () => { nested = api.recover(); });
  await assert.rejects(nested, code("REENTRANT_OPERATION"));
});

for (const value of [undefined, {}, { request: null }]) {
  test("unsupported Web Locks fail closed on every persistence path", async () => {
    const storage = new Storage({ [JOURNAL]: "do not touch" });
    const api = createGlobalSaveCoordinator({ storage, locks: value, hasPendingAttempt: () => false });
    for (const action of [() => api.read(), () => api.recover(), () => api.write(null, () => assert.fail("unlocked")),
      () => api.transact(null, {}), () => api.archive(null, {})]) {
      await assert.rejects(action(), code("WEB_LOCKS_UNAVAILABLE"));
    }
    assert.deepEqual(storage.writes, []);
  });
}

test("rejected acquisition leaves state unchanged and never invokes commands", async () => {
  const failure = new Error("security policy");
  const { storage } = setup();
  const api = createGlobalSaveCoordinator({ storage, locks: { request: () => Promise.reject(failure) }, hasPendingAttempt: () => false });
  await assert.rejects(api.read(() => assert.fail("unlocked")), error => error === failure);
  assert.deepEqual(storage.writes, []);
});

for (const timing of ["before", "queued", "after-global"]) {
  test(`cancellation ${timing} prevents commands and releases acquired locks`, async () => {
    const { api, locks, storage } = setup();
    const baseline = await api.read();
    const controller = new AbortController();
    const reason = new Error("cancelled");
    let blocker;
    if (timing === "before") controller.abort(reason);
    if (timing === "queued") blocker = await hold(locks);
    if (timing === "after-global") {
      const original = locks.request.bind(locks);
      locks.request = (name, options, callback) => original(name, options, lock => {
        if (name === CREATE) controller.abort(reason);
        return callback(lock);
      });
    }
    const result = api.write(baseline, () => assert.fail("cancelled command"), { signal: controller.signal });
    const rejected = assert.rejects(result, error => error === reason);
    if (timing === "queued") { await nextTurn(); controller.abort(reason); }
    await rejected;
    if (blocker) await blocker.release();
    assert.equal(locks.held(GATE), 0); assert.equal(locks.held(CREATE), 0);
    assert.deepEqual(storage.writes, []);
  });
}

test("bindWriter preserves real SP-01 failed-save candidate across revoked leases and retries without redraw", async () => {
  let document = cup("shootout");
  const matches = [];
  for (const fixture of document.fixtures) {
    const match = createMatchRecord({ matchId: fixture.fixtureId, playedAt: "original",
      home: teams.find(t => t.providerTeamId === fixture.homeProviderTeamId),
      away: teams.find(t => t.providerTeamId === fixture.awayProviderTeamId), homeGoals: 0, awayGoals: 0 });
    matches.push(match);
    document = completeCupFixture(document, fixture.fixtureId, match.matchId);
  }
  const { api, storage, locks, setPending } = setup({ [CUP]: JSON.stringify(document),
    [MANAGEMENT]: JSON.stringify(createManagementDocument({ matches })) });
  const baseline = await api.read();
  const editor = await hold(locks, nameOf(document.cupId));
  await api.withCupOwnership(editor.lock, async ownership => {
    let escapedDispatcher;
    const run = api.bindWriter(scoped => {
      escapedDispatcher = scoped;
      return createCupShootoutCoordinator(scoped, matches, { ownsLock: () => true });
    });
    assert.throws(() => escapedDispatcher.getItem(CUP), code("GATE_SCOPE_EXPIRED"));
    const started = await run(baseline, ({ component }) => component.startCupShootout({ cupId: document.cupId, round: 1, pair: 1 }), { ownership });
    const session = Object.values(started.value.shootoutSessions)[0];
    const command = { cupId: document.cupId, shootoutId: session.shootoutId,
      attemptId: JSON.stringify([document.cupId, "r1-p1", 1]), expectedRevision: 0 };
    let draws = 0;
    storage.fault = (method, key) => { if (method === "set" && key === CUP) throw new Error("quota"); };
    await assert.rejects(run(started.snapshot, ({ component }) => component.confirmCupShootoutAttempt(command,
      { random: () => { draws += 1; return 0; } }), { ownership }), { code: "CUP_SHOOTOUT_SAVE_FAILED" });
    setPending(true);
    await assert.rejects(api.read(), code("PENDING_ATTEMPT"));
    storage.fault = null;
    const retried = await run(started.snapshot, ({ component }) => component.retryCupShootoutSave(), { ownership });
    setPending(false);
    assert.equal(draws, 1);
    assert.equal(retried.value.disposition, "committed");
    assert.equal(Object.values(retried.value.cup.shootoutSessions)[0].attempts.length, 1);
    assert.throws(() => escapedDispatcher.getItem(CUP), code("GATE_SCOPE_EXPIRED"));
  });
  await editor.release();
});

test("zero randomness for lock acquisition, snapshot, recovery, import and archive", async () => {
  const { document, matches } = completedCup("deterministic");
  const { api } = setup({ [CUP]: JSON.stringify(document),
    [MANAGEMENT]: JSON.stringify(createManagementDocument({ matches })) });
  const interrupted = ["prepared", "committed"].map(phase => {
    const state = journalState(phase, true);
    return { ...setup(state.initial), state, phase };
  });
  const original = Math.random;
  let draws = 0;
  Math.random = () => { draws += 1; throw new Error("unexpected randomness"); };
  try {
    const baseline = await api.read();
    await api.recover();
    await api.write(baseline, () => {});
    await api.transact(baseline, replacement(baseline));
    await api.archive(baseline, { cupJson: JSON.stringify(document), transactionId: "zero-randomness" });
    for (const { api: recovering, state, phase } of interrupted) {
      assert.deepEqual((await recovering.recover()).images, phase === "prepared" ? state.before : state.after);
    }
    assert.equal(draws, 0);
  } finally { Math.random = original; }
});

test("two queued replacements from independent instances accept only the first generation", async () => {
  const { api, make, locks } = setup();
  const other = make();
  const [a, b] = await Promise.all([api.read(), other.read()]);
  const blocker = await hold(locks);
  const first = api.transact(a, replacement(a, { [MANAGEMENT]: JSON.stringify(createManagementDocument()) }));
  const second = other.transact(b, replacement(b, { [ARCHIVE]: "[]" }));
  const rejected = assert.rejects(second, code("STALE_GENERATION"));
  await blocker.release();
  assert.equal((await first).generation, 1);
  await rejected;
});

test("caller mutations cannot remove queued ownership checks or alter the frozen options", async () => {
  const { api, locks } = setup();
  const baseline = await api.read();
  const editor = await hold(locks, nameOf("editor"));
  const blocker = await hold(locks);
  let rejected;
  await api.withCupOwnership(editor.lock, ownership => {
    const options = { ownership, cupIds: ["z"] };
    const queued = api.write(baseline, () => assert.fail("released ownership"), options);
    rejected = assert.rejects(queued, code("CUP_OWNERSHIP_EXPIRED"));
    delete options.ownership;
    options.cupIds.splice(0);
  });
  await blocker.release(); await rejected; await editor.release();
});

test("import locks and commits the validated copied request despite caller changes during acquisition", async () => {
  const { api, storage, locks } = setup();
  const baseline = await api.read();
  const request = replacement(baseline, { [CUP]: JSON.stringify(cup("intended")) });
  const original = locks.request.bind(locks);
  locks.request = (name, options, callback) => original(name, options, lock => {
    if (name === CREATE) request.changes[CUP] = JSON.stringify(cup("unowned"));
    return callback(lock);
  });
  const result = await api.transact(baseline, request);
  assert.equal(result.status, "committed");
  assert.equal(JSON.parse(storage.getItem(CUP)).cupId, "intended");
  assert.ok(locks.calls.some(c => c.name === nameOf("intended")));
  assert.ok(!locks.calls.some(c => c.name === nameOf("unowned")));
});

test("committed cleanup failure preserves engine result and forces recovery before the next export", async () => {
  const { api, storage, locks } = setup();
  const baseline = await api.read();
  storage.fault = (method, key) => { if (method === "remove" && key === JOURNAL) throw new Error("cleanup offline"); };
  const result = await api.transact(baseline, replacement(baseline, { [MANAGEMENT]: JSON.stringify(createManagementDocument()) }));
  assert.equal(result.status, "committed");
  assert.equal(result.cleanupPending, true); assert.equal(result.usable, false);
  assert.equal(JSON.parse(storage.getItem(JOURNAL)).phase, "committed");
  assert.equal(locks.held(GATE), 0);
  await assert.rejects(api.read(() => assert.fail("cleanup still blocked")), { code: "RECOVERY_BLOCKED" });
  storage.fault = null;
  assert.equal((await api.read()).generation, 1);
  assert.equal(storage.getItem(JOURNAL), null);
});

test("failed live transaction leaves the engine's recovered prepared decision authoritative", async () => {
  const { api, storage, locks } = setup();
  const baseline = await api.read();
  let fired = false;
  storage.fault = (method, key) => {
    if (!fired && method === "set" && key === MANAGEMENT) { fired = true; throw new Error("quota"); }
  };
  await assert.rejects(api.transact(baseline, replacement(baseline, { [MANAGEMENT]: JSON.stringify(createManagementDocument()) })),
    { code: "TRANSACTION_ABORTED", blocked: false, decision: "prepared" });
  assert.equal(locks.held(GATE), 0);
  assert.deepEqual((await api.read()).images, baseline.images);
  assert.equal(storage.getItem(JOURNAL), null);
});

test("ordinary command exception after a verified write releases its lease without pretending rollback", async () => {
  const { api, storage, locks } = setup();
  const baseline = await api.read();
  let escaped;
  await assert.rejects(api.write(baseline, ({ storage: scoped }) => {
    escaped = scoped;
    saveManagement(scoped, createManagementDocument());
    throw new Error("render failed");
  }), /render failed/);
  assert.equal(locks.held(GATE), 0);
  assert.ok(storage.getItem(MANAGEMENT));
  assert.throws(() => escaped.getItem(MANAGEMENT), code("GATE_SCOPE_EXPIRED"));
  await assert.rejects(api.write(baseline, () => assert.fail("old memory")), code("SOURCE_MISMATCH"));
});

for (const key of [CUP, ARCHIVE, GENERATION, MANAGEMENT]) {
  test(`malformed local ${key} blocks writers and snapshot projection without repair`, async () => {
    const { api, storage } = setup();
    const baseline = await api.read();
    storage.map.set(key, "invalid stored evidence");
    await assert.rejects(api.read(() => assert.fail("invalid export")));
    await assert.rejects(api.write(baseline, () => assert.fail("invalid predecessor")));
    assert.equal(storage.getItem(key), "invalid stored evidence");
    assert.deepEqual(storage.writes, []);
  });
}

test("snapshot projector cannot mutate source maps and runs while the shared gate is held", async () => {
  const { api, locks, storage } = setup();
  const value = await api.read(current => {
    assert.equal(locks.held(GATE), 1);
    assert.throws(() => { current.images[MANAGEMENT] = "bad"; }, TypeError);
    return JSON.stringify({ epoch: current.generation });
  });
  assert.equal(value, '{"epoch":0}');
  assert.deepEqual(storage.writes, []);
});

test("getters in options or transaction plans never become application callbacks under the gate", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  const options = Object.defineProperty({}, "ownership", { get: () => assert.fail("options getter"), enumerable: true });
  await assert.rejects(api.write(baseline, () => assert.fail("command"), options), code("INVALID_OPTIONS"));
  const plan = replacement(baseline);
  Object.defineProperty(plan, "changes", { get: () => assert.fail("plan getter"), enumerable: true });
  await assert.rejects(api.transact(baseline, plan), { code: "INVALID_PLAN" });
  const archivePlan = { transactionId: "id" };
  Object.defineProperty(archivePlan, "cupJson", { get: () => assert.fail("archive getter"), enumerable: true });
  await assert.rejects(api.archive(baseline, archivePlan), code("INVALID_ARCHIVE_REQUEST"));
  assert.deepEqual(storage.writes, []);
});

test("fresh facade checks predecessor bytes again before each actual write", async () => {
  const { api, storage } = setup();
  const baseline = await api.read();
  await assert.rejects(api.write(baseline, ({ storage: scoped }) => {
    storage.map.set(MANAGEMENT, JSON.stringify(createManagementDocument()));
    scoped.removeItem(MANAGEMENT);
  }), code("SOURCE_MISMATCH"));
  assert.deepEqual(storage.writes, []);
});

test("bound component cannot use its dispatcher during another writer's command", async () => {
  const { api } = setup();
  const baseline = await api.read();
  let dispatcher;
  const run = api.bindWriter(storage => { dispatcher = storage; return {}; });
  await run(baseline, () => {});
  await api.write(baseline, () => assert.throws(() => dispatcher.getItem(CUP), code("GATE_SCOPE_EXPIRED")));
});

test("preexisting current Cup ownership does not block a read-only shared export", async () => {
  const { api, locks, storage } = setup({ [CUP]: JSON.stringify(cup("open-editor")) });
  const editor = await hold(locks, nameOf("open-editor"));
  const current = await api.read();
  assert.equal(JSON.parse(current.images[CUP]).cupId, "open-editor");
  assert.deepEqual(storage.writes, []);
  await editor.release();
});

test("a snapshot from another coordinator instance cannot authorize a command", async () => {
  const { api, make } = setup();
  await assert.rejects(
    api.write(await make().read(), () => assert.fail("borrowed snapshot")),
    code("SNAPSHOT_REQUIRED"),
  );
});

function portableImportFixture() {
  const emptyRoster = () => ({
    active: Array.from({ length: 11 }, (_, i) => i + 1),
    bench: [],
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
  });

  const portable = {
    schemaVersion: 1,
    savedAt: "2026-11-22T12:00:00Z",
    locale: "it",
    configuration: {
      competition: "135:2024",
      hands: "1",
      randomHands: false,
      teams: ["", ""],
      colors: [0, 1].map(() => ({
        primary: "#112233",
        secondaryEnabled: false,
        secondary: "#ffffff",
      })),
      commentaryEnabled: false,
      auto: {
        mode: "all",
        handLimit: "all",
        rollDelay: "0",
        handDelay: "0",
      },
    },
    persistent: {
      management: createManagementDocument(),
      manager: null,
      tournament: null,
      cup: null,
      cupArchive: [],
    },
    runtime: {
      state: null,
      names: ["Home", "Away"],
      colors: ["#112233", "#445566"],
      teamPalettes: [null, null],
      rollEvents: [],
      matchEvents: [],
      rosters: [emptyRoster(), emptyRoster()],
      refereeStates: [{ yellowed: [] }, { yellowed: [] }],
      extraordinaryState: {
        occurredFamilies: [],
        occurredTypes: [],
      },
      lastCommentary: null,
      lastRefereeEvent: null,
      lastExtraordinaryEvent: null,
      lastGoalScorer: null,
      messageKey: "setupPrompt",
      messageValues: {},
      matchStarted: false,
      selectedMatchTeams: [null, null],
      activeTournamentFixtureId: null,
      tournamentViewedMatchday: null,
      matchRecorded: false,
      competitionRef: null,
    },
  };

  return portable;
}

test("SP-02 real coordinator executes complete portable import", async () => {
  const originalCup = cup("replace-this-cup");
  const { api, storage, locks } = setup({
    [CUP]: JSON.stringify(originalCup),
  });
  const portable = portableImportFixture();

  const before = structuredClone(portable);
  const result = await executePortableGlobalSaveImport(
    api, portable, "sp02-real-import",
  );

  assert.equal(result.receipt.status, "committed");
  assert.equal(result.receipt.usable, true);
  assert.equal(result.receipt.generation, 1);
  assert.deepEqual(portable, before);

  assert.equal(storage.getItem(CUP), null);
  assert.deepEqual(
    JSON.parse(storage.getItem(MANAGEMENT)),
    portable.persistent.management,
  );
  assert.deepEqual(
    JSON.parse(storage.getItem(ARCHIVE)),
    [],
  );
  assert.equal(storage.getItem(GENERATION), "1");
  assert.equal(storage.getItem(JOURNAL), null);
  assert.equal(storage.getItem("foreign-key"), "preserve");
  assert.equal(
    storage.getItem("dado-goal.multiplayer-seat.v1"),
    "seat",
  );

  const admitted = await api.read();
  assert.equal(admitted.generation, 1);
  assert.equal(admitted.images[CUP], null);
  assert.equal(locks.held(GATE), 0);
  assert.equal(locks.held(CREATE), 0);
  assert.equal(locks.held(nameOf(originalCup.cupId)), 0);
});


test("SP-02 real import aborts and rolls back before commit", async () => {
  const original = cup("rollback-cup");
  const { api, storage, locks } = setup({
    [CUP]: JSON.stringify(original),
  });

  let fired = false;
  storage.fault = (method, key) => {
    if (!fired && method === "set" && key === MANAGEMENT) {
      fired = true;
      throw new Error("simulated storage write failure");
    }
  };

  await assert.rejects(
    executePortableGlobalSaveImport(
      api, portableImportFixture(), "sp02-precommit-failure",
    ),
    { code: "TRANSACTION_ABORTED", blocked: false, decision: "prepared" },
  );

  assert.equal(fired, true);
  assert.deepEqual(JSON.parse(storage.getItem(CUP)), original);
  assert.equal(storage.getItem(GENERATION), null);
  assert.equal(storage.getItem(JOURNAL), null);
  assert.equal(locks.held(GATE), 0);
  assert.equal(locks.held(nameOf(original.cupId)), 0);
  assert.equal((await api.read()).generation, 0);
});

test("SP-02 real import retains committed decision when cleanup fails", async () => {
  const original = cup("cleanup-cup");
  const { api, storage, locks } = setup({
    [CUP]: JSON.stringify(original),
  });

  storage.fault = (method, key) => {
    if (method === "remove" && key === JOURNAL) {
      throw new Error("simulated cleanup failure");
    }
  };

  await assert.rejects(
    executePortableGlobalSaveImport(
      api, portableImportFixture(), "sp02-committed-cleanup",
    ),
    error => error instanceof CommittedImportNeedsReload &&
      error.committed === true,
  );

  assert.equal(storage.getItem(CUP), null);
  assert.equal(storage.getItem(GENERATION), "1");
  assert.equal(JSON.parse(storage.getItem(JOURNAL)).phase, "committed");
  assert.equal(locks.held(GATE), 0);

  await assert.rejects(api.read(), { code: "RECOVERY_BLOCKED" });

  storage.fault = null;
  const recovered = await api.recover();

  assert.equal(recovered.generation, 1);
  assert.equal(storage.getItem(JOURNAL), null);
  assert.equal(storage.getItem(CUP), null);
  assert.equal(storage.getItem("foreign-key"), "preserve");
  assert.equal(locks.held(nameOf(original.cupId)), 0);
});
