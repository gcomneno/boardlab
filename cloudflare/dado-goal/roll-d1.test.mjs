import test from "node:test";
import assert from "node:assert/strict";
import { rollAuthorized } from "./roll-d1.mjs";
import { initialState, applyRoll, FACES } from "./game.mjs";
import { matchClock } from "./public/match-clock.mjs";
import { createRoster } from "../../web/dado-goal/match-roster.mjs";
import { replayMatchEvents } from "./match-events-replay.mjs";


function fakeD1(state, seat, initialRosters = null) {
  const row = {
    revision: 0,
    status: "active",
    state_json: JSON.stringify(state),
    updated_at: 100,
    expires_at: 10000,
  };
  const events = initialRosters
    ? [{ type: "joined", rosters: initialRosters }]
    : [];
  const receipts = new Map();
  let batches = 0;

  const db = {
    prepare(sql) {
      return {
        bind(...args) {
          return {
            sql,
            args,
            async first() {
              assert.match(sql, /FROM lifecycle_receipts/);
              return receipts.get(JSON.stringify(args)) ?? null;
            },
            async all() {
              assert.match(sql, /FROM lifecycle_events/);
              return {
                results: events.map((event, index) => ({
                  revision: index + 1,
                  event_json: JSON.stringify(event),
                })),
              };
            },
          };
        },
      };
    },
    async batch(statements) {
      batches++;
      assert.equal(statements.length, 3);

      const [update, eventInsert, receiptInsert] = statements;
      assert.match(update.sql, /UPDATE lifecycle_matches/);
      assert.match(eventInsert.sql, /INSERT INTO lifecycle_events/);
      assert.match(receiptInsert.sql, /INSERT INTO lifecycle_receipts/);

      const [nextRevision, status, stateJson, updatedAt,
        expiresAt, completedAt, matchId, expectedRevision] = update.args;

      if (row.revision !== expectedRevision || row.status !== "active") {
        return Array.from({ length: 3 }, () => ({
          success: true,
          meta: { changes: 0 },
        }));
      }

      const event = JSON.parse(eventInsert.args[2]);
      assert.equal(eventInsert.args[0], matchId);
      assert.equal(eventInsert.args[1], nextRevision);

      const [receiptMatch, receiptSeat, requestId,
        receiptRevision, responseJson] = receiptInsert.args;

      assert.equal(receiptMatch, matchId);
      assert.equal(receiptSeat, seat);
      assert.equal(receiptRevision, expectedRevision);

      row.revision = nextRevision;
      row.status = status;
      row.state_json = stateJson;
      row.updated_at = updatedAt;
      row.expires_at = expiresAt;
      row.completed_at = completedAt;

      events.push(event);
      receipts.set(JSON.stringify([matchId, seat, requestId]), {
        expected_revision: receiptRevision,
        response_json: responseJson,
      });

      return Array.from({ length: 3 }, () => ({
        success: true,
        meta: { changes: 1 },
      }));
    },
  };

  const authority = {
    db,
    activeTTL: 3600,
    completedTTL: 86400,
    now: () => 200,
    seat: async () => seat,
    match: async () => ({ ...row }),
    matchHistory: async () => events.map((event, index) => ({
      revision: index + 1,
      event_json: JSON.stringify(event),
    })),
  };

  return {
    authority,
    events,
    get batches() { return batches; },
    get revision() { return row.revision; },
  };
}

test("M2A: authoritative goal, durable event and idempotent replay", async () => {
  // Reach a result-die state without modifying production rules.
  let state = initialState(5);
  const sequence = [
    ["team", "Tiro in porta!"],
    ["pink", "Mezza rovesciata"],
  ];

  for (const [phase, face] of sequence) {
    if (state.phase !== phase) break;
    state = applyRoll(state, face);
  }

  assert.equal(state.phase, "result",
    "Fixture must reach the final result die");

  const seat = state.player;
  const beforeGoals = state.goals[seat];

  const originalCrypto = globalThis.crypto;

  try {
    // Result die index 1 corresponds to GOAL!.
    Object.defineProperty(globalThis, "crypto", {
      configurable: true,
      value: {
        getRandomValues(bytes) {
          bytes.fill(1);
          return bytes;
        },
      },
    });

    const d1 = fakeD1(state, seat);
    const first = await rollAuthorized(
      d1.authority, "match-1", "seat-token", "request-1", 0
    );

    assert.equal(first.face, FACES.result[1]);
    assert.equal(first.duplicate, false);
    assert.equal(first.state.goals[seat], beforeGoals + 1);
    assert.equal(d1.revision, 1);
    assert.equal(d1.events.length, 1);
    assert.equal(d1.events[0].goal.type, "goal");
    assert.equal(d1.events[0].goal.team, seat);
    assert.equal(
      d1.events[0].goal.minute,
      Math.floor(matchClock(first.state).seconds / 60),
    );
    assert.equal(d1.events[0].goal.number, undefined);
    assert.ok(Number.isInteger(d1.events[0].goal.minute));
    assert.ok(d1.events[0].goal.minute >= 0);

    const replay = await rollAuthorized(
      d1.authority, "match-1", "seat-token", "request-1", 0
    );
    assert.equal(replay.duplicate, true);
    assert.equal(replay.revision, first.revision);
    assert.equal(d1.events.length, 1);
    assert.equal(d1.batches, 1);

    await assert.rejects(
      rollAuthorized(d1.authority, "match-1", "seat-token", "request-2", 0),
      /Stale revision/
    );
    assert.equal(d1.events.length, 1);
    assert.equal(d1.batches, 1);
  } finally {
    if (originalCrypto === undefined) {
      delete globalThis.crypto;
    } else {
      Object.defineProperty(globalThis, "crypto", {
        configurable: true,
        value: originalCrypto,
      });
    }
  }
});

test("penalty goal keeps one durable roll with taker and authoritative goal", async () => {
  let state = initialState(5);
  state = applyRoll(state, "Tiro in porta!");
  state = applyRoll(state, "Mezza rovesciata");
  assert.equal(state.phase, "result");

  const seat = state.player;
  state = applyRoll(state, "RIGORE");
  assert.equal(state.phase, "result",
    "Penalty fixture must require a second result roll");

  const rosters = [createRoster(() => 0.5), createRoster(() => 0.5)];
  const d1 = fakeD1(state, seat, rosters);

  d1.events.push({
    type: "roll",
    seat,
    die: "result",
    face: "RIGORE",
  });

  const beforeEvents = d1.events.length;
  const beforeGoals = state.goals[seat];
  const originalCrypto = globalThis.crypto;
  const originalRandom = Math.random;

  try {
    Math.random = () => 0;

    Object.defineProperty(globalThis, "crypto", {
      configurable: true,
      value: {
        getRandomValues(bytes) {
          bytes.fill(1);
          return bytes;
        },
      },
    });

    const result = await rollAuthorized(
      d1.authority,
      "match-penalty-goal",
      "seat-token",
      "penalty-goal",
      0
    );

    assert.equal(result.face, "GOAL!");
    assert.equal(result.state.goals[seat], beforeGoals + 1);
    assert.equal(d1.events.length, beforeEvents + 1,
      "Penalty goal must persist as one new roll event");

    const event = d1.events.at(-1);

    assert.equal(event.type, "roll");
    assert.equal(event.face, "GOAL!");

    assert.equal(event.penalty.type, "penalty");
    assert.equal(event.penalty.team, seat);
    assert.equal(event.penalty.outcome, "GOAL!");
    assert.ok(Number.isInteger(event.penalty.number));
    assert.ok(rosters[seat].active.includes(event.penalty.number));

    assert.deepEqual(event.goal, {
      type: "goal",
      team: seat,
      minute: event.penalty.minute,
      number: event.penalty.number,
    });
  } finally {
    Math.random = originalRandom;

    if (originalCrypto === undefined) {
      delete globalThis.crypto;
    } else {
      Object.defineProperty(globalThis, "crypto", {
        configurable: true,
        value: originalCrypto,
      });
    }
  }
});

test("#69: extraordinary narrative event persists once and replays without roster mutation", async () => {
  let state = initialState(5);
  state = applyRoll(state, "Tiro in porta!");
  state = applyRoll(state, "Mezza rovesciata");
  assert.equal(state.phase, "result");

  const seat = state.player;
  const rosters = [createRoster(() => 0.5), createRoster(() => 0.5)];
  const originalRosters = structuredClone(rosters);
  const d1 = fakeD1(state, seat, rosters);
  const originalCrypto = globalThis.crypto;
  const originalRandom = Math.random;
  let draws = 0;

  try {
    Math.random = () => {
      draws++;
      return draws === 1 ? 0.99 : 0;
    };
    Object.defineProperty(globalThis, "crypto", {
      configurable: true,
      value: {
        getRandomValues(bytes) {
          bytes.fill(1);
          return bytes;
        },
      },
    });

    const first = await rollAuthorized(
      d1.authority, "match-69", "seat-token", "dg69-extraordinary", 0
    );

    assert.equal(first.duplicate, false);
    assert.equal(d1.events.length, 2);

    const event = d1.events[1].matchEvent;
    assert.equal(event.type, "light-knock");
    assert.equal(event.family, "injury");
    assert.equal(event.classification, "narrative");
    assert.equal(event.team, seat);

    const context = replayMatchEvents(
      d1.events.map(item => ({ event_json: JSON.stringify(item) }))
    );

    assert.deepEqual(context.rosters, originalRosters);
    assert.ok(context.extraordinaryState.occurredTypes.has("light-knock"));
    assert.equal(context.extraordinaryState.occurredFamilies.size, 0);

    const retry = await rollAuthorized(
      d1.authority, "match-69", "seat-token", "dg69-extraordinary", 0
    );
    assert.equal(retry.duplicate, true);
    assert.equal(d1.events.length, 2);
    assert.equal(d1.batches, 1);
  } finally {
    Math.random = originalRandom;
    if (originalCrypto === undefined) {
      delete globalThis.crypto;
    } else {
      Object.defineProperty(globalThis, "crypto", {
        configurable: true,
        value: originalCrypto,
      });
    }
  }
});

test("M2D: yellow card persists once and replays after retry", async () => {
  let state = initialState(5);
  state = applyRoll(state, "Tiro in porta!");
  state = applyRoll(state, "Mezza rovesciata");
  assert.equal(state.phase, "result");

  const seat = state.player;
  const rosters = [createRoster(() => 0.5), createRoster(() => 0.5)];
  const d1 = fakeD1(state, seat, rosters);
  const originalCrypto = globalThis.crypto;
  const originalRandom = Math.random;

  try {
    Math.random = () => 0;
    Object.defineProperty(globalThis, "crypto", {
      configurable: true,
      value: {
        getRandomValues(bytes) {
          bytes.fill(1);
          return bytes;
        },
      },
    });

    const first = await rollAuthorized(
      d1.authority, "match-1", "seat-token", "m2d-yellow", 0
    );

    assert.equal(first.duplicate, false);
    assert.equal(d1.events.length, 2);
    const event = d1.events[1].matchEvent;
    assert.equal(event.type, "yellow");
    assert.equal(event.team, seat);
    assert.ok(rosters[seat].active.includes(event.number));

    const context = replayMatchEvents(
      d1.events.map(event => ({ event_json: JSON.stringify(event) }))
    );
    assert.ok(context.refereeStates[seat].yellowed.has(event.number));

    const retry = await rollAuthorized(
      d1.authority, "match-1", "seat-token", "m2d-yellow", 0
    );
    assert.equal(retry.duplicate, true);
    assert.equal(d1.events.length, 2);
    assert.equal(d1.batches, 1);
  } finally {
    Math.random = originalRandom;
    if (originalCrypto === undefined) {
      delete globalThis.crypto;
    } else {
      Object.defineProperty(globalThis, "crypto", {
        configurable: true,
        value: originalCrypto,
      });
    }
  }
});
