import assert from "node:assert/strict";
import test from "node:test";
import { Worker, isMainThread, parentPort, workerData } from "node:worker_threads";
import {
  createGlobalSaveTransaction, GlobalSaveTransactionError,
  GLOBAL_SAVE_TRANSACTION_KEY as JOURNAL, GLOBAL_SAVE_GENERATION_KEY as GENERATION,
  GLOBAL_SAVE_TRANSACTION_KEYS as KEYS,
} from "./global-save-transaction.mjs";
import { createManagementDocument } from "./team-management-store.mjs";
import { createManagerDocument } from "./manager-store.mjs";
import { createManagerClub } from "./manager-profile.mjs";
import { createTournamentDocument } from "./tournament-store.mjs";
import { createCupDocument } from "./cup-store.mjs";
import { completeCupFixture, advanceCupIfReady } from "./cup.mjs";
import { completeFixture } from "./tournament.mjs";
import { createMatchRecord } from "./team-history.mjs";

const [MANAGEMENT, MANAGER, TOURNAMENT, TOURNAMENT_RECOVERY, CUP, ARCHIVE, CUP_RECOVERY] = KEYS;
const unrelated = { "other-app": " \u0000 untouched 🥅 ", "dado-goal.multiplayer-seat.v1": '{"seat":1}',
  "dado-goal.locale": "en", "dado-goal.runtime.v1": "foreign ordinary runtime" };
const teams = Array.from({ length: 4 }, (_, i) => ({
  provider: "test", providerTeamId: `team-${i}`, name: `Team ${i}`,
  players: Array.from({ length: 11 }, (_, p) => ({
    provider: "test", providerPlayerId: `player-${i}-${p}`, name: `Player ${p}`,
    position: p === 0 ? "Goalkeeper" : p < 5 ? "Defender" : p < 9 ? "Midfielder" : "Attacker",
    shirtNumber: p + 1,
  })),
}));
const imageOf = entries => Object.fromEntries(KEYS.map(key => [key, Object.hasOwn(entries, key) ? entries[key] : null]));
const entriesOf = image => Object.fromEntries(Object.entries(image).filter(([, value]) => value !== null));
const clone = value => structuredClone(value);

// Every get/set/remove has independently injectable before/after boundaries.
// "throw" permits ordinary recovery; "offline" models persistent failure;
// "crash" exits the worker immediately, so engine catches/finally cannot run.
class FaultStorage {
  constructor(entries = {}, fault = null) {
    this.map = new Map(Object.entries(entries));
    this.fault = fault;
    this.calls = [];
    this.fired = false;
    this.offline = false;
  }
  boundary(call, side) {
    if (this.offline) throw new Error("storage offline");
    if (this.fired || this.fault?.at !== call.index || this.fault.side !== side) return;
    this.fired = true;
    if (this.fault.mode === "crash") {
      parentPort.postMessage({ entries: Object.fromEntries(this.map), call, side });
      process.exit(0);
    }
    if (this.fault.mode === "offline") this.offline = true;
    throw new Error(`injected ${side} ${call.method} ${call.key}`);
  }
  access(method, key, value, action) {
    const call = { index: this.calls.length, method, key, value };
    this.calls.push(call);
    this.boundary(call, "before");
    const result = action();
    this.boundary(call, "after");
    return result;
  }
  getItem(key) { return this.access("get", key, undefined, () => this.map.get(key) ?? null); }
  setItem(key, value) { return this.access("set", key, value, () => this.map.set(key, value)); }
  removeItem(key) { return this.access("remove", key, undefined, () => this.map.delete(key)); }
  get writes() { return this.calls.filter(call => call.method !== "get"); }
}

function fixture() {
  const before = imageOf({
    [MANAGEMENT]: ` \n${JSON.stringify(createManagementDocument(), null, 2)}\t`,
    [MANAGER]: `\n${JSON.stringify(createManagerDocument({ club: createManagerClub(teams[0]) }))} `,
    [ARCHIVE]: " [ ] \n", [GENERATION]: "4",
  });
  const changes = {
    [MANAGEMENT]: JSON.stringify(createManagementDocument()), [MANAGER]: null,
    [CUP]: JSON.stringify(createCupDocument({ cupId: 'cup-"🥅', participants: teams })),
    [ARCHIVE]: "[]",
  };
  const after = { ...before, ...changes, [GENERATION]: "5" };
  const request = { transactionId: "transaction-1", operation: "import", expectedGeneration: 4, changes };
  return { before, after, request, entries: { ...entriesOf(before), ...unrelated } };
}
function journal(f, phase = "prepared") {
  return { schemaVersion: 1, transactionId: f.request.transactionId, operation: f.request.operation,
    phase, before: f.before, after: f.after };
}
function interrupted(f, phase, count = 3) {
  const image = { ...f.before };
  for (const key of KEYS.slice(0, count)) image[key] = f.after[key];
  return { ...entriesOf(image), ...unrelated, [JOURNAL]: JSON.stringify(journal(f, phase)) };
}
function assertImage(storage, image) {
  assert.deepEqual(imageOf(Object.fromEntries(storage.map)), image);
  for (const [key, value] of Object.entries(unrelated)) assert.equal(storage.map.get(key), value);
}
function assertAdmitted(result, image) {
  assert.equal(result.usable, true);
  assert.deepEqual(result.images, image);
  assert.ok(Object.isFrozen(result) && Object.isFrozen(result.images));
}
function assertSelectedThenIdempotent(storage, f, target) {
  const api = createGlobalSaveTransaction(storage);
  assertAdmitted(api.recover(), target);
  assertImage(storage, target);
  assert.equal(storage.map.has(JOURNAL), false);
  const writes = storage.writes.length;
  assertAdmitted(api.recover(), target);
  assertAdmitted(createGlobalSaveTransaction(storage).recover(), target);
  assert.equal(storage.writes.length, writes);
}
function errorCode(code, blocked = undefined) {
  return error => error instanceof GlobalSaveTransactionError && error.code === code &&
    (blocked === undefined || error.blocked === blocked);
}
function traceTransaction() {
  const f = fixture();
  const storage = new FaultStorage(f.entries);
  createGlobalSaveTransaction(storage).transact(f.request);
  return storage.calls;
}
function traceRecovery(phase) {
  const f = fixture();
  const storage = new FaultStorage(interrupted(f, phase));
  createGlobalSaveTransaction(storage).recover();
  return storage.calls;
}
function regularRecoveryFixture(kind, linked = false) {
  let document = kind === "cup"
    ? createCupDocument({ cupId: "regular-cup", participants: teams })
    : createTournamentDocument({ championshipId: "regular-tournament", participants: teams });
  const fixture = document.fixtures[0];
  const match = createMatchRecord({ matchId: "canonical-1", playedAt: "saved-time",
    home: teams.find(t => t.providerTeamId === fixture.homeProviderTeamId),
    away: teams.find(t => t.providerTeamId === fixture.awayProviderTeamId), homeGoals: 1, awayGoals: 0 });
  if (linked) document = (kind === "cup" ? completeCupFixture : completeFixture)(document, fixture.fixtureId, match.matchId);
  const identity = kind === "cup" ? "cupId" : "championshipId";
  const ownerKey = kind === "cup" ? CUP : TOURNAMENT;
  const recoveryKey = kind === "cup" ? CUP_RECOVERY : TOURNAMENT_RECOVERY;
  const recovery = { [identity]: document[identity], fixtureId: fixture.fixtureId, matchId: match.matchId };
  const before = imageOf({
    [MANAGEMENT]: JSON.stringify(createManagementDocument({ matches: [match] })),
    [ownerKey]: JSON.stringify(document), [recoveryKey]: `\n${JSON.stringify(recovery)} `,
  });
  const changes = { [recoveryKey]: null };
  if (!linked) {
    const resolved = clone(document);
    resolved.fixtures[0].matchId = match.matchId;
    changes[ownerKey] = JSON.stringify(resolved);
  }
  return { before, after: { ...before, ...changes, [GENERATION]: "1" },
    request: { transactionId: "regular-recovery", operation: "import", expectedGeneration: 0, changes },
    entries: { ...entriesOf(before), ...unrelated }, recoveryKey, ownerKey, identity, recovery };
}

async function hardStop(task, entries, request, at, side) {
  const worker = new Worker(new URL(import.meta.url), {
    workerData: { task, entries, request, fault: { at, side, mode: "crash" } },
  });
  let stopped;
  worker.on("message", message => { stopped = message; });
  await new Promise((resolve, reject) => {
    worker.on("error", reject);
    worker.on("exit", code => code === 0 ? resolve() : reject(new Error(`worker exit ${code}`)));
  });
  assert.ok(stopped, `hard stop reached ${at} ${side}`);
  return stopped;
}

if (!isMainThread) {
  const storage = new FaultStorage(workerData.entries, workerData.fault);
  const engine = createGlobalSaveTransaction(storage);
  if (workerData.task === "recover") engine.recover();
  else engine.transact(workerData.request);
  throw new Error("hard-stop boundary was not reached");
} else {
  test("successful commit: exact complete journal, ordered writes, verified marker, cleanup", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const requestBefore = clone(f.request);
    const result = createGlobalSaveTransaction(storage).transact(f.request);
    assert.equal(result.status, "committed");
    assert.equal(result.generation, 5);
    assert.equal(result.cleanupPending, false);
    assertAdmitted(result, f.after);
    assertImage(storage, f.after);
    assert.equal(storage.map.has(JOURNAL), false);
    assert.deepEqual(f.request, requestBefore);
    const writes = storage.writes;
    assert.deepEqual(writes.map(call => call.key), [JOURNAL, ...KEYS, JOURNAL, JOURNAL]);
    assert.deepEqual(JSON.parse(writes[0].value), journal(f));
    assert.deepEqual(JSON.parse(writes.at(-2).value), journal(f, "committed"));
    for (const call of writes.slice(1, -2)) {
      assert.equal(call.method, f.after[call.key] === null ? "remove" : "set");
      if (call.method === "set") assert.equal(call.value, f.after[call.key]);
    }
  });

  test("exact no-op validates generation but writes nothing and does not increment it", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const api = createGlobalSaveTransaction(storage);
    for (const changes of [{}, { [MANAGEMENT]: f.before[MANAGEMENT], [CUP]: null }]) {
      const result = api.transact({ ...f.request, changes });
      assert.equal(result.status, "noop");
      assert.equal(result.generation, 4);
      assert.equal(result.transactionId, null);
      assertAdmitted(result, f.before);
    }
    assert.equal(storage.writes.length, 0);
  });

  test("null means deletion, JSON null is rejected, whitespace/Unicode are exact, absence stays absent", () => {
    const storage = new FaultStorage(unrelated);
    const api = createGlobalSaveTransaction(storage);
    assert.equal(api.recover().generation, 0);
    const raw = '\n {"schemaVersion":1,"matches":[],"ratings":[],"note":"🥅\\u0000"} \t';
    assert.equal(api.transact({ transactionId: "create", operation: "import", expectedGeneration: 0,
      changes: { [MANAGEMENT]: raw } }).generation, 1);
    assert.equal(storage.map.get(MANAGEMENT), raw);
    const result = api.transact({ transactionId: "delete", operation: "reset", expectedGeneration: 1,
      changes: { [MANAGEMENT]: null } });
    assert.equal(result.generation, 2);
    assert.equal(storage.map.has(MANAGEMENT), false);
    assert.equal(storage.map.has(CUP), false);
    assert.throws(() => api.transact({ transactionId: "bad", operation: "import", expectedGeneration: 2,
      changes: { [MANAGEMENT]: "null" } }));
  });

  for (const phase of ["prepared", "committed"]) {
    for (let count = 0; count <= KEYS.length; count += 1) {
      test(`${phase} interruption after ${count} destination writes chooses exact ${phase === "prepared" ? "before" : "after"}`, () => {
        const f = fixture();
        const storage = new FaultStorage(interrupted(f, phase, count));
        assertSelectedThenIdempotent(storage, f, phase === "prepared" ? f.before : f.after);
      });
    }
    test(`${phase} recovery ignores physically corrupt/missing live documents`, () => {
      const f = fixture();
      const storage = new FaultStorage({ ...interrupted(f, phase), [MANAGEMENT]: "truncated JSON", [GENERATION]: "garbage" });
      assertSelectedThenIdempotent(storage, f, phase === "prepared" ? f.before : f.after);
    });
  }

  // Exhaustive ordinary exceptions include reads, individual set/remove calls,
  // marker verification and cleanup verification, on both sides of each call.
  const transactionTrace = traceTransaction();
  for (const call of transactionTrace) {
    for (const side of ["before", "after"]) {
      test(`transaction exception ${side} boundary ${call.index}: ${call.method} ${call.key}`, () => {
        const f = fixture();
        const storage = new FaultStorage(f.entries, { at: call.index, side });
        let result;
        let error;
        try { result = createGlobalSaveTransaction(storage).transact(f.request); }
        catch (caught) { error = caught; }
        assert.ok(storage.fired);
        assert.ok(error || result);
        const stored = storage.map.get(JOURNAL);
        const decision = stored ? JSON.parse(stored).phase : null;
        const committed = result?.status === "committed" || decision === "committed" ||
          storage.map.get(GENERATION) === "5";
        if (result?.usable) assertAdmitted(result, committed ? f.after : f.before);
        if (result?.cleanupPending) {
          assert.equal(result.status, "committed");
          assert.equal(result.usable, false);
          assert.equal(Object.hasOwn(result, "images"), false);
        }
        assertSelectedThenIdempotent(storage, f, committed ? f.after : f.before);
      });
    }
  }

  for (const phase of ["prepared", "committed"]) {
    for (const call of traceRecovery(phase)) {
      for (const side of ["before", "after"]) {
        test(`${phase} recovery failure/retry ${side} boundary ${call.index}: ${call.method} ${call.key}`, () => {
          const f = fixture();
          const storage = new FaultStorage(interrupted(f, phase), { at: call.index, side });
          assert.throws(() => createGlobalSaveTransaction(storage).recover(), error =>
            error instanceof GlobalSaveTransactionError && error.blocked);
          assert.ok(storage.fired);
          // Before removal, evidence is unchanged. A remove that succeeds then
          // throws cannot retain a journal, but all selected images verified first.
          if (storage.map.has(JOURNAL)) assert.equal(storage.map.get(JOURNAL), JSON.stringify(journal(f, phase)));
          else assertImage(storage, phase === "prepared" ? f.before : f.after);
          assertSelectedThenIdempotent(storage, f, phase === "prepared" ? f.before : f.after);
        });
      }
    }
  }

  for (const side of ["before", "after"]) {
    test(`real process termination ${side} EVERY transaction storage boundary, then fresh-engine admission`, async () => {
      const f = fixture();
      for (const call of transactionTrace) {
        const stopped = await hardStop("transact", f.entries, f.request, call.index, side);
        const storage = new FaultStorage(stopped.entries);
        const raw = storage.map.get(JOURNAL);
        const target = raw ? (JSON.parse(raw).phase === "committed" ? f.after : f.before) :
          storage.map.get(GENERATION) === "5" ? f.after : f.before;
        assertSelectedThenIdempotent(storage, f, target);
      }
    });
    for (const phase of ["prepared", "committed"]) {
      test(`real process termination ${side} EVERY ${phase} recovery boundary, then repeated recovery`, async () => {
        const f = fixture();
        for (const call of traceRecovery(phase)) {
          const stopped = await hardStop("recover", interrupted(f, phase), null, call.index, side);
          assertSelectedThenIdempotent(new FaultStorage(stopped.entries), f, phase === "prepared" ? f.before : f.after);
        }
      });
    }
  }

  test("failed initial journal persistence leaves all domain keys unchanged, with no domain write", () => {
    const f = fixture();
    const firstWrite = transactionTrace.find(call => call.method === "set");
    const storage = new FaultStorage(f.entries, { at: firstWrite.index, side: "before" });
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("TRANSACTION_ABORTED", false));
    assertImage(storage, f.before);
    assert.deepEqual(storage.writes.map(call => call.key), [JOURNAL]);
  });

  test("failed commit marker rolls back; marker persisted then throwing rolls forward", () => {
    const f = fixture();
    const marker = transactionTrace.find(call => call.key === JOURNAL && call.value?.includes('"phase":"committed"'));
    for (const side of ["before", "after"]) {
      const storage = new FaultStorage(f.entries, { at: marker.index, side });
      const api = createGlobalSaveTransaction(storage);
      if (side === "before") assert.throws(() => api.transact(f.request), errorCode("TRANSACTION_ABORTED", false));
      else assert.equal(api.transact(f.request).status, "committed");
      assertImage(storage, side === "before" ? f.before : f.after);
    }
  });

  test("cleanup failure reports committed but inadmissible state; recovery never runs operation twice", () => {
    const f = fixture();
    const cleanup = transactionTrace.find(call => call.method === "remove" && call.key === JOURNAL);
    const storage = new FaultStorage(f.entries, { at: cleanup.index, side: "before" });
    const api = createGlobalSaveTransaction(storage);
    const result = api.transact(f.request);
    assert.equal(result.status, "committed");
    assert.equal(result.usable, false);
    assert.equal(result.cleanupPending, true);
    assert.equal(JSON.parse(storage.map.get(JOURNAL)).phase, "committed");
    assert.throws(() => api.transact(f.request), errorCode("GENERATION_MISMATCH"));
    assertImage(storage, f.after);
    assert.equal(storage.map.has(JOURNAL), false);
  });

  test("silent initial journal failure cannot reach any domain write", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    storage.setItem = () => {}; // Storage adapter lies about success.
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("TRANSACTION_ABORTED", false));
    assert.equal(storage.writes.length, 0);
    assertImage(storage, f.before);
  });

  test("malformed initial journal readback is retained and blocks before domain writes", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const write = storage.setItem.bind(storage);
    storage.setItem = (key, value) => write(key, key === JOURNAL ? "truncated" : value);
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("INVALID_JOURNAL", true));
    assert.equal(storage.map.get(JOURNAL), "truncated");
    assert.deepEqual(storage.writes.map(call => call.key), [JOURNAL]);
    assertImage(storage, f.before);
  });

  test("silent destination write failure is detected by exact readback and rolled back", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const write = storage.setItem.bind(storage);
    let dropped = false;
    storage.setItem = (key, value) => {
      if (key === CUP && !dropped) { dropped = true; return; }
      write(key, value);
    };
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("TRANSACTION_ABORTED", false));
    assertImage(storage, f.before);
    assert.equal(storage.map.has(JOURNAL), false);
  });

  test("silently lost commit marker selects prepared; corrupt marker is retained and blocks", () => {
    const f = fixture();
    for (const corrupt of [false, true]) {
      const storage = new FaultStorage(f.entries);
      const write = storage.setItem.bind(storage);
      storage.setItem = (key, value) => {
        if (key === JOURNAL && JSON.parse(value).phase === "committed") {
          if (corrupt) write(key, "malformed marker");
          return;
        }
        write(key, value);
      };
      assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request),
        errorCode(corrupt ? "INVALID_JOURNAL" : "TRANSACTION_ABORTED", corrupt));
      if (corrupt) {
        assert.equal(storage.map.get(JOURNAL), "malformed marker");
        assert.throws(() => createGlobalSaveTransaction(storage).recover(), errorCode("INVALID_JOURNAL", true));
        assertImage(storage, f.after);
      } else assertImage(storage, f.before);
    }
  });

  test("unreadable decision after a persisted commit blocks without choosing before, then retries forward", () => {
    const f = fixture();
    const marker = transactionTrace.find(call => call.key === JOURNAL && call.value?.includes('"phase":"committed"'));
    const storage = new FaultStorage(f.entries, { at: marker.index, side: "after", mode: "offline" });
    const api = createGlobalSaveTransaction(storage);
    assert.throws(() => api.transact(f.request), error => errorCode("RECOVERY_BLOCKED", true)(error) && error.decision === "unknown");
    assert.equal(JSON.parse(storage.map.get(JOURNAL)).phase, "committed");
    assertImage(storage, f.after);
    storage.offline = false;
    assertSelectedThenIdempotent(storage, f, f.after);
  });

  for (const phase of ["prepared", "committed"]) {
    test(`${phase} silent recovery write failure retains evidence, blocks new writes and retries`, () => {
      const f = fixture();
      const storage = new FaultStorage(interrupted(f, phase));
      const target = phase === "prepared" ? f.before : f.after;
      // Force a detectable differing value at an otherwise valid target key.
      storage.map.set(MANAGEMENT, "broken live management");
      const write = storage.setItem.bind(storage);
      storage.setItem = (key, value) => { if (key !== MANAGEMENT) write(key, value); };
      const api = createGlobalSaveTransaction(storage);
      for (const action of [() => api.recover(), () => api.transact(f.request)]) {
        assert.throws(action, error => errorCode("RECOVERY_BLOCKED", true)(error) && error.decision === phase);
        assert.equal(storage.map.get(JOURNAL), JSON.stringify(journal(f, phase)));
        assert.equal(storage.writes.some(call => call.key === JOURNAL), false);
      }
      storage.setItem = write;
      assertSelectedThenIdempotent(storage, f, target);
    });
    test(`${phase} cleanup verification failure cannot admit state until a later cleanup succeeds`, () => {
      const f = fixture();
      const storage = new FaultStorage(interrupted(f, phase));
      const remove = storage.removeItem.bind(storage);
      storage.removeItem = key => { if (key !== JOURNAL) remove(key); };
      assert.throws(() => createGlobalSaveTransaction(storage).recover(), errorCode("RECOVERY_BLOCKED", true));
      assert.equal(storage.map.get(JOURNAL), JSON.stringify(journal(f, phase)));
      storage.removeItem = remove;
      assertSelectedThenIdempotent(storage, f, phase === "prepared" ? f.before : f.after);
    });
  }

  test("initial prepared readback failure is settled from durable phase before allowing another transaction", () => {
    const f = fixture();
    const first = transactionTrace.find(call => call.key === JOURNAL && call.method === "set");
    const storage = new FaultStorage(f.entries, { at: first.index + 1, side: "before", mode: "offline" });
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("RECOVERY_BLOCKED", true));
    assertImage(storage, f.before);
    assert.equal(JSON.parse(storage.map.get(JOURNAL)).phase, "prepared");
    assert.deepEqual(storage.writes.map(call => call.key), [JOURNAL]);
    storage.offline = false;
    assertSelectedThenIdempotent(storage, f, f.before);
  });

  test("persistent outage blocks this and fresh engines; intact evidence enables later retry", () => {
    const f = fixture();
    const domain = transactionTrace.filter(call => KEYS.includes(call.key) && call.method !== "get")[2];
    const storage = new FaultStorage(f.entries, { at: domain.index, side: "after", mode: "offline" });
    const api = createGlobalSaveTransaction(storage);
    assert.throws(() => api.transact(f.request), errorCode("RECOVERY_BLOCKED", true));
    const evidence = storage.map.get(JOURNAL);
    assert.equal(JSON.parse(evidence).phase, "prepared");
    assert.notDeepEqual(imageOf(Object.fromEntries(storage.map)), f.before); // Physical mixture exists.
    for (const engine of [api, createGlobalSaveTransaction(storage)]) {
      assert.throws(() => engine.recover(), errorCode("RECOVERY_BLOCKED", true));
      assert.throws(() => engine.transact(f.request), errorCode("RECOVERY_BLOCKED", true));
      assert.equal(storage.map.get(JOURNAL), evidence);
    }
    storage.offline = false;
    assertSelectedThenIdempotent(storage, f, f.before);
  });

  const invalidJournals = [
    ["truncated", () => '{"schemaVersion":1'], ["null", () => "null"],
    ["array", () => "[]"], ["unknown version", j => ({ ...j, schemaVersion: 2 })],
    ["missing schema", j => { delete j.schemaVersion; return j; }],
    ["unknown operation", j => ({ ...j, operation: "shootout" })],
    ["unknown phase", j => ({ ...j, phase: "writing" })],
    ["empty ID", j => ({ ...j, transactionId: "  " })],
    ["missing ID", j => { delete j.transactionId; return j; }],
    ["extra root field", j => ({ ...j, extra: true })],
    ["missing before key", j => { delete j.before[MANAGER]; return j; }],
    ["missing after key", j => { delete j.after[CUP_RECOVERY]; return j; }],
    ["extra before key", j => { j.before["unrelated"] = null; return j; }],
    ["extra after key", j => { j.after["dado-goal.multiplayer-seat.v1"] = "overwrite"; return j; }],
    ["self destination", j => { j.after[JOURNAL] = null; return j; }],
    ["numeric image", j => { j.after[CUP] = 1; return j; }],
    ["object image", j => { j.before[MANAGER] = {}; return j; }],
    ["invalid before generation", j => { j.before[GENERATION] = "4.0"; return j; }],
    ["impossible epoch jump", j => { j.after[GENERATION] = "6"; return j; }],
    ["missing epoch increment", j => { j.after[GENERATION] = "4"; return j; }],
    ["absent after epoch", j => { j.after[GENERATION] = null; return j; }],
    ["epoch overflow", j => { j.before[GENERATION] = String(Number.MAX_SAFE_INTEGER);
      j.after[GENERATION] = String(Number.MAX_SAFE_INTEGER + 1); return j; }],
    ["corrupt before domain", j => { j.before[MANAGEMENT] = "{}"; return j; }],
    ["corrupt after domain", j => { j.after[CUP] = "{}"; return j; }],
    ["unresolved recovery", j => { j.before[CUP_RECOVERY] = JSON.stringify({ cupId: "missing", fixtureId: "r1-p1-l1", matchId: "missing" }); return j; }],
    ["duplicate phase", j => JSON.stringify(j).replace('"phase":"prepared"', '"phase":"committed","phase":"prepared"')],
    ["escaped duplicate phase", j => JSON.stringify(j).replace('"phase":"prepared"', '"ph\\u0061se":"committed","phase":"prepared"')],
    ["duplicate image", j => JSON.stringify(j).replace('"before":{', `"before":{${JSON.stringify(MANAGEMENT)}:null,`)],
  ];
  for (const [name, corrupt] of invalidJournals) {
    test(`malformed/unsupported/ambiguous journal retained and blocks new transactions: ${name}`, () => {
      const f = fixture();
      const invalid = corrupt(clone(journal(f)));
      const source = typeof invalid === "string" ? invalid : JSON.stringify(invalid);
      const storage = new FaultStorage({ ...f.entries, [JOURNAL]: source });
      const before = Object.fromEntries(storage.map);
      const api = createGlobalSaveTransaction(storage);
      assert.throws(() => api.recover(), errorCode("INVALID_JOURNAL", true));
      assert.throws(() => api.transact(f.request), errorCode("INVALID_JOURNAL", true));
      assert.deepEqual(Object.fromEntries(storage.map), before);
      assert.equal(storage.writes.length, 0);
    });
  }

  for (const key of ["other-app", "dado-goal.multiplayer-seat.v1", "dado-goal.locale", JOURNAL, GENERATION, "__proto__"]) {
    test(`unauthorized destination rejected before any write: ${key}`, () => {
      const f = fixture();
      const storage = new FaultStorage(f.entries);
      assert.throws(() => createGlobalSaveTransaction(storage).transact({ ...f.request,
        changes: Object.fromEntries([[key, null]]) }), errorCode("INVALID_PLAN"));
      assert.equal(storage.writes.length, 0);
      assertImage(storage, f.before);
    });
  }
  for (const changes of [null, [], { [CUP]: undefined }, { [CUP]: {} }, { [CUP]: 0 }, { [CUP]: false }]) {
    test(`non-string/null plan values rejected: ${JSON.stringify(changes)}`, () => {
      const f = fixture();
      const storage = new FaultStorage(f.entries);
      assert.throws(() => createGlobalSaveTransaction(storage).transact({ ...f.request, changes }));
      assert.equal(storage.writes.length, 0);
    });
  }

  test("no application callback/accessor/toJSON execution in transaction plans", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    let calls = 0;
    const getter = Object.defineProperty({}, CUP, { enumerable: true, get() { calls += 1; return null; } });
    const toJSON = { toJSON() { calls += 1; return {}; } };
    const api = createGlobalSaveTransaction(storage);
    for (const changes of [getter, toJSON]) assert.throws(() => api.transact({ ...f.request, changes }));
    assert.throws(() => api.transact({ ...f.request, afterWrite() { calls += 1; } }));
    assert.throws(() => api.transact({ ...f.request, changes: { [CUP]: toJSON } }));
    assert.equal(calls, 0);
    assert.equal(storage.writes.length, 0);
  });

  for (const expectedGeneration of [0, 3, 5, "4", null, undefined, -1, 1.5, Number.MAX_SAFE_INTEGER + 1]) {
    test(`generation fence rejects stale/invalid request ${String(expectedGeneration)}`, () => {
      const f = fixture();
      const storage = new FaultStorage(f.entries);
      assert.throws(() => createGlobalSaveTransaction(storage).transact({ ...f.request, expectedGeneration }));
      assert.equal(storage.writes.length, 0);
      assertImage(storage, f.before);
    });
  }
  for (const raw of ["", " 0", "00", "01", "-1", "-0", "1.0", "1e0", "null", '"1"', "9007199254740992"]) {
    test(`malformed persisted generation blocks admission and mutation: ${JSON.stringify(raw)}`, () => {
      const f = fixture();
      const storage = new FaultStorage({ ...f.entries, [GENERATION]: raw });
      const api = createGlobalSaveTransaction(storage);
      assert.throws(() => api.recover(), errorCode("RECOVERY_BLOCKED", true));
      assert.throws(() => api.transact(f.request), errorCode("RECOVERY_BLOCKED", true));
      assert.equal(storage.map.get(GENERATION), raw);
      assert.equal(storage.writes.length, 0);
    });
  }
  test("safe-integer generation overflow blocks mutation; exact no-op remains read-only", () => {
    const f = fixture();
    const storage = new FaultStorage({ ...f.entries, [GENERATION]: String(Number.MAX_SAFE_INTEGER) });
    const api = createGlobalSaveTransaction(storage);
    const request = { ...f.request, expectedGeneration: Number.MAX_SAFE_INTEGER };
    assert.throws(() => api.transact(request), errorCode("GENERATION_OVERFLOW"));
    assert.equal(api.transact({ ...request, changes: {} }).status, "noop");
    assert.equal(storage.writes.length, 0);
  });

  for (const key of [MANAGEMENT, MANAGER, TOURNAMENT, TOURNAMENT_RECOVERY, CUP, ARCHIVE, CUP_RECOVERY]) {
    test(`malformed local ${key} cannot be erased by replacement/reset`, () => {
      const f = fixture();
      const storage = new FaultStorage({ ...f.entries, [key]: "{" });
      assert.throws(() => createGlobalSaveTransaction(storage).transact({ ...f.request, operation: "reset",
        changes: { ...f.request.changes, [key]: null } }), errorCode("RECOVERY_BLOCKED", true));
      assert.equal(storage.map.get(key), "{");
      assert.equal(storage.writes.length, 0);
    });
  }

  test("priority recovery precedes stale-generation checks and a later valid transaction", () => {
    const f = fixture();
    const storage = new FaultStorage(interrupted(f, "committed"));
    const api = createGlobalSaveTransaction(storage);
    assert.throws(() => api.transact(f.request), errorCode("GENERATION_MISMATCH"));
    assertImage(storage, f.after);
    const result = api.transact({ ...f.request, transactionId: "second", expectedGeneration: 5,
      changes: { [CUP]: null } });
    assert.equal(result.generation, 6);
    assert.equal(storage.map.has(CUP), false);
  });

  for (const kind of ["cup", "tournament"]) {
    for (const linked of [false, true]) {
      test(`valid ${kind} association image (${linked ? "already linked" : "pending exact link"}) is validated on copies without source writes`, () => {
        const f = regularRecoveryFixture(kind, linked);
        const storage = new FaultStorage(f.entries);
        const api = createGlobalSaveTransaction(storage);
        assertAdmitted(api.recover(), f.before);
        assert.equal(storage.writes.length, 0);
        assertImage(storage, f.before);
        assertAdmitted(api.transact(f.request), f.after);
        assert.equal(storage.map.has(f.recoveryKey), false);
      });
      for (const phase of ["prepared", "committed"]) {
        test(`${phase} recovery preserves exact historical ${kind} association images (${linked ? "linked" : "pending"})`, () => {
          const f = regularRecoveryFixture(kind, linked);
          const storage = new FaultStorage(interrupted(f, phase, 7));
          assertSelectedThenIdempotent(storage, f, phase === "prepared" ? f.before : f.after);
          if (phase === "prepared") assert.equal(storage.map.get(f.recoveryKey), f.before[f.recoveryKey]);
        });
      }
    }
    for (const [name, corrupt] of [
      ["wrong owner", (f, recovery) => { recovery[f.identity] = "other-owner"; }],
      ["missing match", (f, recovery) => { recovery.matchId = "nonexistent"; }],
      ["wrong fixture", (f, recovery) => { recovery.fixtureId = "r99-p1-l1"; }],
      ["wrong teams", (f, recovery) => { recovery.fixtureId = JSON.parse(f.before[f.ownerKey]).fixtures[1].fixtureId; }],
    ]) {
      test(`unresolved ${kind} recovery ${name} blocks replacement without clearing evidence`, () => {
        const f = regularRecoveryFixture(kind);
        const recovery = clone(f.recovery);
        corrupt(f, recovery);
        const source = JSON.stringify(recovery);
        const storage = new FaultStorage({ ...f.entries, [f.recoveryKey]: source });
        const api = createGlobalSaveTransaction(storage);
        assert.throws(() => api.recover(), errorCode("RECOVERY_BLOCKED", true));
        assert.throws(() => api.transact(f.request), errorCode("RECOVERY_BLOCKED", true));
        assert.equal(storage.map.get(f.recoveryKey), source);
        assert.equal(storage.writes.length, 0);
      });
    }
  }

  for (const originalEpoch of [null, "0"]) {
    for (const phase of ["prepared", "committed"]) {
      test(`${phase} recovery preserves ${originalEpoch === null ? "absent" : "explicit zero"} generation before-image`, () => {
        const f = fixture();
        f.before[GENERATION] = originalEpoch;
        f.after[GENERATION] = "1";
        assertSelectedThenIdempotent(new FaultStorage(interrupted(f, phase)), f,
          phase === "prepared" ? f.before : f.after);
      });
    }
  }

  test("no-op following priority rollback does not inherit the previous transaction identity", () => {
    const f = fixture();
    const storage = new FaultStorage(interrupted(f, "prepared"));
    const result = createGlobalSaveTransaction(storage).transact({ ...f.request, changes: {} });
    assert.equal(result.status, "noop");
    assert.equal(result.transactionId, null);
    assertAdmitted(result, f.before);
  });

  test("invalid intended reference graph is rejected before creating a journal", () => {
    const f = fixture();
    const cup = JSON.parse(f.request.changes[CUP]);
    const dangling = clone(cup);
    dangling.fixtures[0].matchId = "missing-canonical-match";
    const tournament = createTournamentDocument({ championshipId: "incompatible", participants: teams });
    for (const changes of [
      { [CUP]: JSON.stringify(dangling) },
      { [CUP]: JSON.stringify(cup), [TOURNAMENT]: JSON.stringify(tournament) },
      { [ARCHIVE]: JSON.stringify([cup]) }, // Unfinished Cup cannot be archived.
      { [MANAGEMENT]: JSON.stringify({ schemaVersion: 1, matches: [], ratings: [null] }) },
    ]) {
      const storage = new FaultStorage(f.entries);
      assert.throws(() => createGlobalSaveTransaction(storage).transact({ ...f.request, changes }));
      assert.equal(storage.writes.length, 0);
      assertImage(storage, f.before);
    }
  });

  test("engine commits a prepared archive destination with coherent current removal and exact history preservation", () => {
    let cup = createCupDocument({ cupId: "completed", participants: teams });
    const matches = [];
    while (!cup.championProviderTeamId) {
      for (const fixture of cup.fixtures.filter(f => f.matchId === null)) {
        const match = createMatchRecord({ matchId: `completed:${fixture.fixtureId}`, playedAt: "saved-time",
          home: teams.find(t => t.providerTeamId === fixture.homeProviderTeamId),
          away: teams.find(t => t.providerTeamId === fixture.awayProviderTeamId), homeGoals: 1, awayGoals: 0 });
        matches.push(match);
        cup = completeCupFixture(cup, fixture.fixtureId, match.matchId);
      }
      cup = advanceCupIfReady(cup, matches).cup;
    }
    const before = imageOf({ [MANAGEMENT]: JSON.stringify(createManagementDocument({ matches })),
      [CUP]: `\n${JSON.stringify(cup)} `, [ARCHIVE]: " [] ", [GENERATION]: "8" });
    const changes = { [CUP]: null, [ARCHIVE]: JSON.stringify([cup]), [CUP_RECOVERY]: null };
    const f = { before, after: { ...before, ...changes, [GENERATION]: "9" },
      request: { transactionId: "archive-completed", operation: "archive", expectedGeneration: 8, changes } };
    for (const phase of ["prepared", "committed"]) {
      for (let count = 0; count <= KEYS.length; count += 1) {
        const storage = new FaultStorage(interrupted(f, phase, count));
        assertSelectedThenIdempotent(storage, f, phase === "prepared" ? before : f.after);
      }
    }
    const storage = new FaultStorage({ ...entriesOf(before), ...unrelated });
    const api = createGlobalSaveTransaction(storage);
    assertAdmitted(api.transact(f.request), f.after);
    assert.equal(storage.map.has(CUP), false);
    assert.deepEqual(JSON.parse(storage.map.get(ARCHIVE)), [cup]);
    assert.equal(api.transact({ ...f.request, expectedGeneration: 9 }).status, "noop");
    assert.deepEqual(JSON.parse(storage.map.get(ARCHIVE)), [cup]);
  });

  test("a different journal observed on an error path is retained without speculative recovery writes", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const write = storage.setItem.bind(storage);
    const foreign = JSON.stringify({ ...journal(f), transactionId: "foreign-operation" });
    storage.setItem = (key, value) => {
      if (key === MANAGEMENT) {
        storage.map.set(JOURNAL, foreign);
        throw new Error("foreign writer bypassed the external gate");
      }
      write(key, value);
    };
    assert.throws(() => createGlobalSaveTransaction(storage).transact(f.request), errorCode("RECOVERY_BLOCKED", true));
    assert.equal(storage.map.get(JOURNAL), foreign);
    assert.deepEqual(storage.writes.map(call => call.key), [JOURNAL]);
  });

  test("same-instance reentry is rejected before any nested storage access", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const api = createGlobalSaveTransaction(storage);
    const write = storage.setItem.bind(storage);
    let attempts = 0;
    storage.setItem = (key, value) => {
      attempts += 1;
      assert.throws(() => api.recover(), errorCode("REENTRANT_OPERATION"));
      assert.throws(() => api.transact(f.request), errorCode("REENTRANT_OPERATION"));
      return write(key, value);
    };
    assertAdmitted(api.transact(f.request), f.after);
    assert.ok(attempts > 1);
  });

  test("archive/reset labels use the same engine; duplicate raw snapshot is a no-op", () => {
    const f = fixture();
    const storage = new FaultStorage(f.entries);
    const api = createGlobalSaveTransaction(storage);
    assert.equal(api.transact({ ...f.request, operation: "archive" }).status, "committed");
    const writes = storage.writes.length;
    assert.equal(api.transact({ ...f.request, operation: "archive", expectedGeneration: 5 }).status, "noop");
    assert.equal(storage.writes.length, writes);
    const result = api.transact({ ...f.request, transactionId: "reset", operation: "reset", expectedGeneration: 5,
      changes: Object.fromEntries(KEYS.filter(key => key !== GENERATION).map(key => [key, null])) });
    assert.equal(result.generation, 6);
    assertAdmitted(api.recover(), { ...imageOf({}), [GENERATION]: "6" });
  });

  test("deterministic operations and recovery consume zero random draws and need no browser globals", () => {
    const original = Math.random;
    Math.random = () => { throw new Error("unexpected random draw"); };
    try {
      const f = fixture();
      const first = new FaultStorage(f.entries);
      const second = new FaultStorage(f.entries);
      createGlobalSaveTransaction(first).transact(f.request);
      createGlobalSaveTransaction(second).transact(f.request);
      assert.deepEqual(first.calls, second.calls);
      assert.deepEqual(Object.fromEntries(first.map), Object.fromEntries(second.map));
      assertSelectedThenIdempotent(new FaultStorage(interrupted(f, "prepared")), f, f.before);
      assertSelectedThenIdempotent(new FaultStorage(interrupted(f, "committed")), f, f.after);
    } finally { Math.random = original; }
  });

  test("storage unavailable and invalid read types fail closed", () => {
    for (const storage of [null, {}, { getItem() {} }, { getItem() {}, setItem() {} }]) {
      assert.throws(() => createGlobalSaveTransaction(storage), errorCode("STORAGE_UNAVAILABLE"));
    }
    const storage = new FaultStorage();
    storage.getItem = () => undefined;
    assert.throws(() => createGlobalSaveTransaction(storage).recover(), errorCode("RECOVERY_BLOCKED", true));
  });

  test("all exported allowlisted keys are explicit, unique, frozen and exclude the journal", () => {
    assert.equal(KEYS.length, 8);
    assert.equal(new Set(KEYS).size, KEYS.length);
    assert.ok(Object.isFrozen(KEYS));
    assert.equal(KEYS.includes(JOURNAL), false);
    assert.equal(KEYS.at(-1), GENERATION);
  });
}
