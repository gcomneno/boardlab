import test from "node:test";
import assert from "node:assert/strict";

import {
  FACES_V2, applyV2Event, newV2Match, rollV2,
} from "./engine-v2.mjs";
import { normalizeV2Event } from "./roll-event-v2.mjs";

test("M034: every V2 physical face has a stable kind", () => {
  let checked = 0;

  for (const [die, faces] of Object.entries(FACES_V2)) {
    for (const [faceIndex, face] of faces.entries()) {
      const event = normalizeV2Event({
        type: "roll", rulesVersion: 2,
        seat: 0, die, faceIndex, face,
      });
      assert.equal(typeof event.kind, "string");
      assert.ok(event.kind.length > 0);
      assert.equal(event.faceIndex, faceIndex);
      assert.equal(event.face, face);
      checked += 1;
    }
  }

  assert.equal(checked, 24);
});

test("M034: five green GOAL faces share semantic outcome", () => {
  const kinds = FACES_V2.penalty.map((face, faceIndex) =>
    normalizeV2Event({
      type: "roll", rulesVersion: 2, seat: 0,
      die: "penalty", faceIndex, face,
    }).kind,
  );
  assert.deepEqual(kinds, [
    "penalty-goal", "penalty-goal", "penalty-goal",
    "penalty-goal", "penalty-goal", "penalty-missed",
  ]);
});

test("M034: awarded penalty is an independent event", () => {
  const award = normalizeV2Event({
    type: "penalty-awarded", rulesVersion: 2, seat: 0,
  });
  assert.equal(award.kind, "penalty-awarded");
  assert.equal(Object.hasOwn(award, "die"), false);
});

test("M034: adapter rejects identity and face spoofing", () => {
  const valid = {
    type: "roll", rulesVersion: 2, seat: 0,
    die: "pink", faceIndex: 2, face: "Rovesciata",
  };
  for (const bad of [
    { ...valid, rulesVersion: 1 },
    { ...valid, seat: 2 },
    { ...valid, faceIndex: 3 },
    { ...valid, kind: "goal" },
    { ...valid, faceIndex: -1 },
    { ...valid, die: "unknown" },
    { type: "penalty-awarded", rulesVersion: 2,
      seat: 0, die: "result" },
  ]) {
    assert.throws(() => normalizeV2Event(bad), RangeError);
  }
});

test("M034: normalized events replay without changing gameplay", () => {
  let state = newV2Match(1);
  for (const value of [0, 0, 0, 0.999]) {
    const { event, state: next } = rollV2(state, () => value);
    const normalized = normalizeV2Event(event);
    assert.deepEqual(applyV2Event(state, normalized), next);
    state = next;
  }
  assert.deepEqual(state.goals, [0, 0]);
  assert.deepEqual(state.hands, [1, 0]);
});
