import test from "node:test";
import assert from "node:assert/strict";

import {
  dispatchRulesRoll,
  resolveRulesVersion,
} from "./rules-dispatch.mjs";
import { newV2Match } from "../../web/dado-goal/engine-v2.mjs";

test("M034: missing persisted version means legacy V1", () => {
  assert.equal(resolveRulesVersion(undefined), 1);
  assert.equal(resolveRulesVersion(null), 1);
  assert.equal(resolveRulesVersion(1), 1);
  assert.equal(resolveRulesVersion(2), 2);
});

test("M034: invalid versions fail closed", () => {
  for (const value of [0, 3, "2", -1, false, {}]) {
    assert.throws(
      () => resolveRulesVersion(value),
      RangeError,
    );
  }
});

test("M034: legacy dispatch remains isolated", () => {
  const state = {
    schemaVersion: 1,
    phase: "team",
  };
  let calls = 0;

  const outcome = dispatchRulesRoll({
    persistedRulesVersion: null,
    state,
    rollLegacy: received => {
      calls += 1;
      assert.equal(received, state);
      return { legacy: true };
    },
    byteSource: () => {
      throw new Error("V2 randomness must not run");
    },
  });

  assert.deepEqual(outcome, { legacy: true });
  assert.equal(calls, 1);
});

test("M034: V2 dispatch never invokes legacy handler", () => {
  const state = newV2Match(1);

  const outcome = dispatchRulesRoll({
    persistedRulesVersion: 2,
    state,
    rollLegacy: () => {
      throw new Error("Legacy handler must not run");
    },
    byteSource: () => 0,
  });

  assert.equal(outcome.event.rulesVersion, 2);
  assert.equal(outcome.event.kind, "attack");
  assert.equal(outcome.state.phase, "pink");
});

test("M034: cross-version state mismatches are rejected", () => {
  const v2 = newV2Match(1);

  assert.throws(() => dispatchRulesRoll({
    persistedRulesVersion: 1,
    state: v2,
    rollLegacy: () => ({}),
  }), RangeError);

  assert.throws(() => dispatchRulesRoll({
    persistedRulesVersion: 2,
    state: { schemaVersion: 1, phase: "team" },
    byteSource: () => 0,
  }), RangeError);

  assert.throws(() => dispatchRulesRoll({
    persistedRulesVersion: 3,
    state: v2,
  }), RangeError);
});
