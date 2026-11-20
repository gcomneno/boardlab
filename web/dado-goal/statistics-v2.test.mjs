import test from "node:test";
import assert from "node:assert/strict";
import { FACES_V2 } from "./engine-v2.mjs";
import { matchStatisticsV2 } from "./statistics-v2.mjs";
import { matchStatistics } from "./statistics.mjs";

function roll(seat, die, faceIndex) {
  return {
    type: "roll",
    rulesVersion: 2,
    seat,
    die,
    faceIndex,
    face: FACES_V2[die][faceIndex],
  };
}

function award(seat) {
  return {
    type: "penalty-awarded",
    rulesVersion: 2,
    seat,
  };
}

test("M034: missed penalty is neither goal nor save", () => {
  const [home, away] = matchStatisticsV2([
    roll(0, "team", 0),
    roll(0, "pink", 0),
    award(0),
    roll(0, "penalty", 5),
  ]);

  assert.equal(home.penalties, 1);
  assert.equal(home.penaltyMissed, 1);
  assert.equal(home.penaltyGoals, 0);
  assert.equal(home.goals, 0);
  assert.equal(away.saves, 0);
  assert.equal(home.possessionRolls, 3);
});

test("M034: converted penalty counts exactly one goal", () => {
  const [home] = matchStatisticsV2([
    award(0),
    roll(0, "penalty", 3),
  ]);

  assert.equal(home.penalties, 1);
  assert.equal(home.penaltyGoals, 1);
  assert.equal(home.goals, 1);
  assert.equal(home.penaltyMissed, 0);
  assert.equal(home.possessionRolls, 1);
});

test("M034: ordinary save belongs to defending team", () => {
  const [home, away] = matchStatisticsV2([
    roll(0, "result", 1),
  ]);

  assert.equal(home.saves, 0);
  assert.equal(away.saves, 1);
  assert.equal(home.possession, 100);
});

test("M034: V2 counts shot variants and preserves source", () => {
  const input = [
    roll(0, "pink", 0),
    roll(0, "pink", 1),
    roll(0, "pink", 2),
    roll(0, "pink", 3),
    roll(0, "pink", 4),
  ];
  const original = structuredClone(input);
  const [home] = matchStatisticsV2(input);

  assert.equal(home.shots, 5);
  assert.equal(home.powerfulShots, 1);
  assert.equal(home.halfBicycleKicks, 1);
  assert.equal(home.bicycleKicks, 1);
  assert.equal(home.headers, 1);
  assert.equal(home.blockedShots, 1);
  assert.deepEqual(input, original);
});

test("M034: V1 statistics remain independent", () => {
  const legacy = [
    { seat: 0, die: "result", face: "GOAL!" },
  ];
  const before = matchStatistics(legacy);

  assert.equal(before[0].goals, 1);
  assert.throws(
    () => matchStatisticsV2(legacy),
    RangeError,
  );
  assert.deepEqual(matchStatistics(legacy), before);
});

test("M034: invalid events cannot enter V2 statistics", () => {
  assert.throws(() => matchStatisticsV2(null), TypeError);
  assert.throws(
    () => matchStatisticsV2([{
      ...roll(0, "penalty", 5),
      kind: "penalty-goal",
    }]),
    RangeError,
  );
});
