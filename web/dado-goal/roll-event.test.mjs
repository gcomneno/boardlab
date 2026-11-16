import test from "node:test";
import assert from "node:assert/strict";

import {
  createRollEvent,
  normalizeRollEvent,
  normalizeRollEvents,
  rollEventKind,
} from "./roll-event.mjs";

test("canonical roll events carry stable machine-readable semantics", () => {
  assert.deepEqual(
    createRollEvent({ seat: 0, die: "result", face: "RIGORE" }),
    {
      seat: 0,
      die: "result",
      kind: "penalty-awarded",
      face: "RIGORE",
    },
  );

  assert.equal(rollEventKind("penalty", "GOAL!"), "penalty-goal");
  assert.equal(rollEventKind("result", "GOAL!"), "goal");
});

test("legacy roll events are upgraded exactly once at the compatibility boundary", () => {
  const legacy = { seat: 1, die: "result", face: "Parato!" };
  const canonical = normalizeRollEvent(legacy);

  assert.equal(canonical.kind, "save");
  assert.equal(canonical.face, "Parato!");
  assert.deepEqual(normalizeRollEvents([legacy]), [canonical]);
});

test("canonical kind is authoritative for machine processing", () => {
  const event = normalizeRollEvent({
    seat: 0,
    die: "result",
    kind: "goal",
    face: "presentation text is not inspected",
  });

  assert.equal(event.kind, "goal");
});

test("unknown semantic kinds fail closed", () => {
  assert.throws(
    () => normalizeRollEvent({
      seat: 0,
      die: "result",
      kind: "translated-goal-label",
      face: "GOAL!",
    }),
    RangeError,
  );
});
