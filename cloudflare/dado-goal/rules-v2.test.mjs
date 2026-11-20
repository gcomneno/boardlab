import test from "node:test";
import assert from "node:assert/strict";

import {
  newV2Match,
  applyV2Event,
} from "../../web/dado-goal/engine-v2.mjs";
import {
  rollServerV2,
  replayServerV2,
} from "./rules-v2.mjs";

function bytes(sequence) {
  let index = 0;

  return () => {
    assert.ok(index < sequence.length, "Unexpected random draw");
    return sequence[index++];
  };
}

function finalState() {
  let state = newV2Match(1);

  for (const die of ["team", "pink"]) {
    const event = {
      type: "roll",
      rulesVersion: 2,
      seat: 0,
      die,
      faceIndex: 0,
    };

    state = applyV2Event(state, event);
  }

  return state;
}

test("M034: server V2 roll has canonical event identity", () => {
  const initial = newV2Match(1);
  const outcome = rollServerV2(initial, bytes([0]));

  assert.equal(outcome.event.rulesVersion, 2);
  assert.equal(outcome.event.seat, 0);
  assert.equal(outcome.event.die, "team");
  assert.equal(outcome.event.faceIndex, 0);
  assert.equal(outcome.event.kind, "attack");
  assert.deepEqual(
    replayServerV2(initial, outcome.event),
    outcome.state,
  );
});

test("M034: penalty award occupies exactly 1/40 slots", () => {
  const initial = finalState();

  for (let slot = 0; slot < 40; slot += 1) {
    const source = slot === 0 ? bytes([slot]) : bytes([slot, 0]);
    const outcome = rollServerV2(initial, source);

    if (slot === 0) {
      assert.equal(outcome.event.type, "penalty-awarded");
      assert.equal(outcome.event.kind, "penalty-awarded");
      assert.equal(outcome.state.phase, "penalty");
    } else {
      assert.equal(outcome.event.type, "roll");
      assert.equal(outcome.event.die, "result");
    }

    assert.deepEqual(
      replayServerV2(initial, outcome.event),
      outcome.state,
    );
  }
});

test("M034: rejection sampling eliminates modulo bias", () => {
  const start = newV2Match(1);
  const result = rollServerV2(start, bytes([252, 253, 251]));

  assert.equal(result.event.faceIndex, 5);
  assert.equal(result.event.kind, "successful-pass");

  const penalty = rollServerV2(finalState(), bytes([240, 0]));
  assert.equal(penalty.event.type, "penalty-awarded");
});

test("M034: server replay preserves missed penalty", () => {
  let state = newV2Match(1);
  let replay = structuredClone(state);

  const sources = [
    [0],
    [0],
    [0],
    [5],
  ];

  const events = [];

  for (const sequence of sources) {
    const outcome = rollServerV2(state, bytes(sequence));
    events.push(outcome.event);
    state = outcome.state;
    replay = replayServerV2(replay, outcome.event);
  }

  assert.equal(events[2].kind, "penalty-awarded");
  assert.equal(events[3].kind, "penalty-missed");
  assert.equal(events[3].face, "SBAGLIATO!");
  assert.deepEqual(state.goals, [0, 0]);
  assert.deepEqual(replay, state);
});

test("M034: invalid server random bytes fail closed", () => {
  const initial = newV2Match(1);

  for (const value of [-1, 256, 1.5, NaN]) {
    assert.throws(
      () => rollServerV2(initial, () => value),
      RangeError,
    );
  }
});
