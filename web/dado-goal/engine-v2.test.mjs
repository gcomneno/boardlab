import test from "node:test";
import assert from "node:assert/strict";

import {
  FACES_V2,
  PENALTY_AWARD_RATE_V2,
  newV2Match,
  applyV2Event,
  rollV2,
} from "./engine-v2.mjs";

function face(state, faceIndex) {
  return applyV2Event(state, {
    type: "roll",
    rulesVersion: 2,
    seat: state.player,
    die: state.phase,
    faceIndex,
  });
}

test("M034: isolated dice preserve 6 faces and 5/6 green goals", () => {
  for (const faces of Object.values(FACES_V2)) {
    assert.equal(faces.length, 6);
  }
  assert.equal(
    FACES_V2.penalty.filter(face => face === "GOAL!").length,
    5,
  );
  assert.equal(FACES_V2.penalty[5], "SBAGLIATO!");
});

test("M034: team and pink transitions reach final resolution", () => {
  let state = newV2Match(1);
  state = face(state, 0);
  assert.equal(state.phase, "pink");
  state = face(state, 3);
  assert.equal(state.phase, "result");
  state = face(state, 0);
  assert.deepEqual(state.goals, [1, 0]);
  assert.equal(state.phase, "team");
});

test("M034: penalty award is explicit and replayable", () => {
  let state = newV2Match(1);
  state = face(state, 0);
  state = face(state, 0);

  const before = structuredClone(state);
  const award = rollV2(state, () => 0);
  assert.equal(award.event.type, "penalty-awarded");
  assert.equal(award.state.phase, "penalty");
  assert.deepEqual(
    applyV2Event(before, award.event),
    award.state,
  );

  const miss = rollV2(award.state, () => 0.999);
  assert.equal(miss.event.face, "SBAGLIATO!");
  assert.deepEqual(miss.state.goals, [0, 0]);
  assert.equal(miss.state.hands[0], 1);
});

test("M034: no penalty when final draw is above threshold", () => {
  let state = newV2Match(1);
  state = face(state, 0);
  state = face(state, 0);

  const values = [0.5, 0];
  let consumed = 0;
  const result = rollV2(state, () => values[consumed++]);
  assert.equal(consumed, 2);
  assert.equal(result.event.type, "roll");
  assert.equal(result.event.die, "result");
  assert.deepEqual(result.state.goals, [1, 0]);
});

test("M034: exact analytical balance stays within contract", () => {
  const finalReach = (3 / 5) * (4 / 6);
  const penaltyPerHand = finalReach * PENALTY_AWARD_RATE_V2;
  const goalsPerMatch = 20 * finalReach * (
    (1 - PENALTY_AWARD_RATE_V2) / 6 +
    PENALTY_AWARD_RATE_V2 * (5 / 6)
  );

  assert.ok(Math.abs(finalReach - 0.4) < 1e-12);
  assert.ok(Math.abs(penaltyPerHand - 0.01) < 1e-12);
  assert.ok(Math.abs(goalsPerMatch - 1.4666666667) < 1e-9);
});

test("M034: V2 rejects invalid award and completed rolls", () => {
  const start = newV2Match(1);
  assert.throws(
    () => applyV2Event(start, { type: "penalty-awarded" }),
    RangeError,
  );
  assert.throws(
    () => rollV2(start, () => 1),
    RangeError,
  );
});

test("M034: reject wrong seat, version and missing identity", () => {
  const state = newV2Match(1);
  const valid = {
    type: "roll",
    rulesVersion: 2,
    seat: 0,
    die: "team",
    faceIndex: 0,
  };

  assert.equal(applyV2Event(state, valid).phase, "pink");

  for (const invalid of [
    { ...valid, seat: 1 },
    { ...valid, rulesVersion: 1 },
    { ...valid, seat: undefined },
    { ...valid, rulesVersion: undefined },
  ]) {
    assert.throws(
      () => applyV2Event(state, invalid),
      RangeError,
    );
  }
});

test("M034: penalty award requires correct event identity", () => {
  let state = newV2Match(1);
  state = face(state, 0);
  state = face(state, 0);

  const award = {
    type: "penalty-awarded",
    rulesVersion: 2,
    seat: 0,
  };

  assert.equal(applyV2Event(state, award).phase, "penalty");

  for (const invalid of [
    { ...award, seat: 1 },
    { ...award, rulesVersion: 1 },
    { ...award, seat: undefined },
    { ...award, rulesVersion: undefined },
  ]) {
    assert.throws(
      () => applyV2Event(state, invalid),
      RangeError,
    );
  }
});

test("M034: full match replay is deterministic", () => {
  let seed = 0x0342026;
  const random = () => {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    return seed / 4294967296;
  };

  const initial = newV2Match(10);
  let live = initial;
  const events = [];

  while (live.phase !== "finished" && events.length < 1000) {
    const result = rollV2(live, random);
    events.push(result.event);
    live = result.state;
  }

  let replay = structuredClone(initial);
  for (const event of events) {
    replay = applyV2Event(replay, event);
  }

  assert.equal(live.phase, "finished");
  assert.deepEqual(live.hands, [10, 10]);
  assert.deepEqual(replay, live);
  assert.equal(events.length, 47);
  assert.deepEqual(live.goals, [2, 0]);
});

test("M034: awarded and missed penalty survives replay", () => {
  const initial = newV2Match(1);
  let live = initial;
  const events = [];

  // Team attack, pink conclusion, awarded penalty,
  // missed green kick, opposing team loses possession.
  const sequence = [0, 0, 0, 0.999, 0.55];
  let position = 0;

  while (live.phase !== "finished") {
    const result = rollV2(live, () => {
      assert.ok(position < sequence.length);
      return sequence[position++];
    });
    events.push(result.event);
    live = result.state;
  }

  let replay = structuredClone(initial);
  for (const event of events) {
    replay = applyV2Event(replay, event);
  }

  assert.equal(position, sequence.length);
  assert.equal(
    events.filter(event => event.type === "penalty-awarded").length,
    1,
  );
  assert.ok(events.some(event =>
    event.die === "penalty" && event.face === "SBAGLIATO!"
  ));
  assert.deepEqual(live.goals, [0, 0]);
  assert.deepEqual(replay, live);
});

test("M034: all physical face transitions match V2 contract", () => {
  const start = newV2Match(1);

  const pink = face(start, 0);
  const result = face(pink, 0);
  const penalty = applyV2Event(result, {
    type: "penalty-awarded",
    rulesVersion: 2,
    seat: result.player,
  });

  const cases = [
    {
      state: start,
      phases: ["pink", "pink", "team", "team", "pink", "team"],
      goals: [0, 0, 0, 0, 0, 0],
      completed: [false, false, true, true, false, false],
    },
    {
      state: pink,
      phases: ["result", "result", "result", "result", "team", "team"],
      goals: [0, 0, 0, 0, 0, 0],
      completed: [false, false, false, false, true, true],
    },
    {
      state: result,
      phases: Array(6).fill("team"),
      goals: [1, 0, 0, 0, 0, 0],
      completed: Array(6).fill(true),
    },
    {
      state: penalty,
      phases: Array(6).fill("team"),
      goals: [1, 1, 1, 1, 1, 0],
      completed: Array(6).fill(true),
    },
  ];

  let checked = 0;

  for (const group of cases) {
    for (let index = 0; index < 6; index += 1) {
      const next = face(group.state, index);

      assert.equal(next.phase, group.phases[index]);
      assert.equal(
        next.goals[0] - group.state.goals[0],
        group.goals[index],
      );
      assert.equal(
        next.hands[0] - group.state.hands[0],
        Number(group.completed[index]),
      );
      checked += 1;
    }
  }

  assert.equal(checked, 24);
});
