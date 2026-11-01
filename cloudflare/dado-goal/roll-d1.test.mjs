import test from "node:test";
import assert from "node:assert/strict";
import { rollAuthorized } from "./roll-d1.mjs";
import { initialState, applyRoll, FACES } from "./game.mjs";
import { matchClock } from "./public/match-clock.mjs";

function fakeD1(state, seat) {
  const row = {
    revision: 0,
    status: "active",
    state_json: JSON.stringify(state),
    updated_at: 100,
    expires_at: 10000,
  };
  const events = [];
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
    assert.deepEqual(d1.events[0].goal, {
      type: "goal",
      team: seat,
      minute: Math.floor(matchClock(first.state).seconds / 60),
    });
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
