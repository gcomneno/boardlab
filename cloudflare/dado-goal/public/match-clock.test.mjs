import test from "node:test";
import assert from "node:assert/strict";
import { matchClock } from "./match-clock.mjs";

test("kickoff, halftime and fulltime for different match lengths", () => {
  for (const n of [1, 3, 6, 1000]) {
    assert.equal(matchClock({ handsPerTeam: n, hands: [0, 0] }).label, "00:00");
    const halftime = matchClock({
      handsPerTeam: n,
      hands: [Math.ceil(n / 2), Math.floor(n / 2)],
    });
    assert.equal(halftime.label, "45:00");
    assert.equal(halftime.halftime, true);

    const fulltime = matchClock({ handsPerTeam: n, hands: [n, n] });
    assert.equal(fulltime.label, "90:00");
    assert.equal(fulltime.finished, true);
  }
});

test("clock advances monotonically and only with completed hands", () => {
  for (const n of [1, 3, 6, 1000]) {
    let previous = -1;
    for (let completed = 0; completed <= 2 * n; completed++) {
      const state = {
        handsPerTeam: n,
        hands: [Math.ceil(completed / 2), Math.floor(completed / 2)],
      };
      const current = matchClock(state);
      assert.ok(current.seconds >= previous);
      assert.equal(current.halftime, completed === n);
      previous = current.seconds;
    }
  }

  const state = { handsPerTeam: 6, hands: [1, 0] };
  assert.deepEqual(matchClock(state), matchClock(state));
});

test("invalid clock states are rejected", () => {
  assert.throws(() => matchClock({ handsPerTeam: 0, hands: [0, 0] }), RangeError);
  assert.throws(() => matchClock({ handsPerTeam: 6, hands: [7, 0] }), RangeError);
  assert.throws(() => matchClock({ handsPerTeam: 6, hands: [1.5, 0] }), RangeError);
});
