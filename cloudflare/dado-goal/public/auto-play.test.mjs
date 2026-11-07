import test from "node:test";
import assert from "node:assert/strict";
import {
  autoAllowsTeam,
  autoQuotaReached,
  createAutoConfig,
  createInterruptibleDelay,
  remainingHands,
} from "./auto-play.mjs";

test("AUTO team selection and launch quota are independent", () => {
  assert.equal(autoAllowsTeam(createAutoConfig({ mode: "all" }), 0), true);
  assert.equal(autoAllowsTeam(createAutoConfig({ mode: "all" }), 1), true);

  const home = createAutoConfig({ mode: "team-0" });
  assert.equal(autoAllowsTeam(home, 0), true);
  assert.equal(autoAllowsTeam(home, 1), false);

  const away = createAutoConfig({ mode: "team-1" });
  assert.equal(autoAllowsTeam(away, 0), false);
  assert.equal(autoAllowsTeam(away, 1), true);

  const limitedAll = createAutoConfig({ mode: "all", handLimit: 3 });
  assert.equal(autoQuotaReached(limitedAll, 2), false);
  assert.equal(autoQuotaReached(limitedAll, 3), true);

  const limitedHome = createAutoConfig({
    mode: "team-0",
    handLimit: 2,
  });
  assert.equal(autoAllowsTeam(limitedHome, 0), true);
  assert.equal(autoAllowsTeam(limitedHome, 1), false);
  assert.equal(autoQuotaReached(limitedHome, 1), false);
  assert.equal(autoQuotaReached(limitedHome, 2), true);

  const unlimited = createAutoConfig({ mode: "all" });
  assert.equal(autoQuotaReached(unlimited, 100), false);
});

test("AUTO delays are independent and validated", () => {
  const config = createAutoConfig({ rollDelay: 125, handDelay: 900 });
  assert.equal(config.rollDelay, 125);
  assert.equal(config.handDelay, 900);

  assert.throws(
    () => createAutoConfig({ rollDelay: -1 }),
    RangeError
  );
  assert.throws(
    () => createAutoConfig({ handDelay: 10001 }),
    RangeError
  );
});

test("AUTO quota, when present, must be positive", () => {
  assert.throws(
    () => createAutoConfig({ handLimit: 0 }),
    RangeError
  );
  assert.throws(
    () => createAutoConfig({ handLimit: -1 }),
    RangeError
  );
  assert.throws(
    () => createAutoConfig({ handLimit: 1.5 }),
    RangeError
  );
  assert.throws(
    () => createAutoConfig({ mode: "unknown" }),
    RangeError
  );
});

test("AUTO delay can be interrupted without scheduling an action", async () => {
  let callback = null;
  let cleared = false;

  const delay = createInterruptibleDelay(
    fn => {
      callback = fn;
      return 7;
    },
    timer => {
      assert.equal(timer, 7);
      cleared = true;
    }
  );

  const pending = delay.wait(5000);
  assert.equal(delay.cancel(), true);
  assert.equal(await pending, false);
  assert.equal(cleared, true);

  callback = null;
  assert.equal(delay.cancel(), false);
});

test("remaining AUTO hands are bounded by current match state", () => {
  assert.equal(
    remainingHands({ handsPerTeam: 5, hands: [0, 0] }),
    10
  );
  assert.equal(
    remainingHands({ handsPerTeam: 5, hands: [3, 4] }),
    3
  );
  assert.equal(
    remainingHands({ handsPerTeam: 5, hands: [5, 5] }),
    0
  );

  assert.throws(
    () => remainingHands({ handsPerTeam: 5, hands: [6, 0] }),
    RangeError
  );
});
