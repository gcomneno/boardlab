import assert from "node:assert/strict";
import {
  MultiplayerClient,
  MultiplayerAPIError,
} from "./multiplayer-client.mjs";

const matchId = "A".repeat(32);
const calls = [];

const fetchImpl = async (path, options) => {
  calls.push({ path, options });

  let status = 200;
  let payload;

  if (path === "/api/v1/matches") {
    status = 201;
    payload = {
      matchId,
      hostToken: "host-secret",
      inviteToken: "invite-secret",
      snapshot: { revision: 0, seat: 0 },
    };
  } else if (path.endsWith("/join")) {
    payload = {
      guestToken: "guest-secret",
      snapshot: { revision: 1, seat: 1 },
    };
  } else if (path.endsWith("/roll")) {
    payload = {
      revision: 2,
      face: "Passaggio",
      duplicate: false,
    };
  } else {
    payload = { revision: 1, seat: 0 };
  }

  return {
    ok: true,
    status,
    json: async () => payload,
  };
};

const host = new MultiplayerClient({ fetchImpl });

assert.throws(() => host.requireSession());

const created = await host.create(10);
assert.equal(created.matchId, matchId);
assert.equal(created.inviteToken, "invite-secret");
assert.equal(host.seat, 0);

const snapshot = await host.snapshot();
assert.equal(snapshot.revision, 1);

const roll = await host.roll(1, "request-1");
assert.equal(roll.revision, 2);
assert.equal(roll.face, "Passaggio");

assert.equal(
  calls.at(-1).options.headers.Authorization,
  "Bearer host-secret"
);

assert.deepEqual(
  JSON.parse(calls.at(-1).options.body),
  { requestId: "request-1", expectedRevision: 1 }
);

assert.ok(calls.every(
  call => call.options.cache === "no-store"
));

assert.ok(calls.every(
  call => call.options.credentials === "omit"
));

assert.ok(calls.every(
  call => !call.path.includes("secret")
));

const guest = new MultiplayerClient({ fetchImpl });
const joined = await guest.join(matchId, "invite-secret");

assert.equal(joined.seat, 1);
assert.equal(guest.seat, 1);
assert.equal(guest.seatToken, "guest-secret");

guest.leave();
assert.equal(guest.seatToken, null);
assert.throws(() => guest.requireSession());

await assert.rejects(
  () => host.create(0),
  RangeError
);

await assert.rejects(
  () => host.roll(-1, "bad"),
  RangeError
);

await assert.rejects(
  () => host.roll(1, ""),
  TypeError
);

const denied = new MultiplayerClient({
  fetchImpl: async () => ({
    ok: false,
    status: 403,
    json: async () => ({
      error: { code: "NOT_YOUR_TURN" }
    }),
  }),
});

await assert.rejects(
  () => denied.create(1),
  error =>
    error instanceof MultiplayerAPIError &&
    error.status === 403 &&
    error.code === "NOT_YOUR_TURN"
);


// Reproduce browsers that require fetch to retain its global receiver.
const originalFetch = globalThis.fetch;
const receiver = globalThis;

try {
  globalThis.fetch = function (path, options) {
    assert.equal(this, receiver, "Fetch lost its browser receiver");
    return fetchImpl(path, options);
  };

  const browserClient = new MultiplayerClient();
  const browserCreated = await browserClient.create(1);

  assert.equal(browserCreated.matchId, matchId);
} finally {
  globalThis.fetch = originalFetch;
}

console.log("BROWSER_FETCH_RECEIVER_GATE=PASS");

console.log("MULTIPLAYER_CLIENT_API_GATE=PASS");
console.log("SEAT_TOKEN_ISOLATION_GATE=PASS");
console.log("REQUEST_VALIDATION_GATE=PASS");
console.log("NO_CREDENTIALS_IN_URL_GATE=PASS");
console.log("MULTIPLAYER_CLIENT_TEST_GATE=PASS");

// Optional browser recovery qualification.
import { RECOVERY_KEY } from "./multiplayer-client.mjs";

function memoryStorage() {
  const records = new Map();
  return {
    getItem: key => records.has(key) ? records.get(key) : null,
    setItem: (key, value) => records.set(key, value),
    removeItem: key => records.delete(key),
  };
}

const retained = memoryStorage();
const recoveryFetch = async (path, options) => ({
  ok: true,
  status: 200,
  json: async () => ({
    revision: 5,
    status: "active",
    seat: 0,
    state: { player: 0 },
  }),
});

const originalClient = new MultiplayerClient({
  fetchImpl: recoveryFetch,
  storage: retained,
});
originalClient.matchId = matchId;
originalClient.seatToken = "host-secret";
originalClient.seat = 0;

// Explicit consent only.
assert.equal(retained.getItem(RECOVERY_KEY), null);
originalClient.remember();
const stored = JSON.parse(retained.getItem(RECOVERY_KEY));
assert.deepEqual(Object.keys(stored).sort(), [
  "formatVersion", "matchId", "seat", "seatToken"
]);

const recovered = new MultiplayerClient({
  fetchImpl: recoveryFetch,
  storage: retained,
});
assert.equal((await recovered.restore()).revision, 5);
assert.equal(recovered.seat, 0);
assert.equal(recovered.seatToken, "host-secret");

const wrongSeat = new MultiplayerClient({
  storage: retained,
  fetchImpl: async () => ({
    ok: true,
    status: 200,
    json: async () => ({
      revision: 5, status: "active",
      seat: 1, state: { player: 0 },
    }),
  }),
});
assert.equal(await wrongSeat.restore(), null);
assert.equal(retained.getItem(RECOVERY_KEY), null);

originalClient.remember();
const networkError = new MultiplayerClient({
  storage: retained,
  fetchImpl: async () => { throw new Error("Network unavailable"); },
});
await assert.rejects(() => networkError.restore(), /Network unavailable/);
assert.notEqual(retained.getItem(RECOVERY_KEY), null);

const rejected = new MultiplayerClient({
  storage: retained,
  fetchImpl: async () => ({
    ok: false,
    status: 401,
    json: async () => ({ error: { code: "UNAUTHORIZED" } }),
  }),
});
assert.equal(await rejected.restore(), null);
assert.equal(retained.getItem(RECOVERY_KEY), null);

retained.setItem(RECOVERY_KEY, "{invalid");
assert.equal(await recovered.restore(), null);
assert.equal(retained.getItem(RECOVERY_KEY), null);

originalClient.remember();
originalClient.leave();
assert.equal(retained.getItem(RECOVERY_KEY), null);
assert.equal(originalClient.seatToken, null);

const noStorage = new MultiplayerClient({ fetchImpl: recoveryFetch });
assert.equal(await noStorage.restore(), null);
assert.throws(() => noStorage.remember());

console.log("OPTIONAL_RECOVERY_CONSENT_GATE=PASS");
console.log("AUTHENTICATED_RECOVERY_GATE=PASS");
console.log("WRONG_SEAT_REJECTION_GATE=PASS");
console.log("NETWORK_FAILURE_RETENTION_GATE=PASS");
console.log("INVALID_CREDENTIAL_CLEANUP_GATE=PASS");
console.log("FORGET_SESSION_GATE=PASS");

// Storage denial must not trigger network requests.
const blockedRecovery = new MultiplayerClient({
  storage: {
    getItem: () => { throw new Error("Storage blocked"); },
    setItem: () => { throw new Error("Storage blocked"); },
    removeItem: () => { throw new Error("Storage blocked"); },
  },
  fetchImpl: async () => {
    throw new Error("Unexpected fetch");
  },
});
assert.equal(await blockedRecovery.restore(), null);

// Leaving during an asynchronous restore must not resurrect credentials.
const raceStore = memoryStorage();
raceStore.setItem(RECOVERY_KEY, JSON.stringify({
  formatVersion: 1,
  matchId,
  seat: 0,
  seatToken: "host-secret",
}));

let unblock;
const pendingResponse = new Promise(resolve => { unblock = resolve; });

const racingClient = new MultiplayerClient({
  storage: raceStore,
  fetchImpl: async () => {
    await pendingResponse;
    return {
      ok: true,
      status: 200,
      json: async () => ({
        revision: 5,
        status: "active",
        seat: 0,
        state: { player: 0 },
      }),
    };
  },
});

const recoveryPending = racingClient.restore();
racingClient.leave();
unblock();

assert.equal(await recoveryPending, null);
assert.equal(racingClient.seatToken, null);
assert.equal(raceStore.getItem(RECOVERY_KEY), null);

console.log("STORAGE_DENIAL_GATE=PASS");
console.log("LEAVE_RECOVERY_RACE_GATE=PASS");


// A stale authentication failure must never delete newer credentials.
const staleStore = memoryStorage();
staleStore.setItem(RECOVERY_KEY, JSON.stringify({
  formatVersion: 1,
  matchId,
  seat: 0,
  seatToken: "old-token",
}));

let releaseStale;
const stalePending = new Promise(resolve => {
  releaseStale = resolve;
});

const staleClient = new MultiplayerClient({
  storage: staleStore,
  fetchImpl: async () => {
    await stalePending;
    return {
      ok: false,
      status: 401,
      json: async () => ({
        error: { code: "UNAUTHORIZED" },
      }),
    };
  },
});

const staleRecovery = staleClient.restore();

staleClient.leave();
staleClient.matchId = "B".repeat(32);
staleClient.seatToken = "new-token";
staleClient.seat = 1;
staleClient.remember();

releaseStale();

assert.equal(await staleRecovery, null);

const preserved = JSON.parse(
  staleStore.getItem(RECOVERY_KEY)
);

assert.equal(preserved.matchId, "B".repeat(32));
assert.equal(preserved.seatToken, "new-token");
assert.equal(staleClient.seatToken, "new-token");

console.log("STALE_AUTH_FAILURE_ISOLATION_GATE=PASS");

console.log("M0B_BROWSER_RECOVERY_TEST_GATE=PASS");

