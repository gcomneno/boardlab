import test from "node:test";
import assert from "node:assert/strict";
import { matchStatistics } from "./statistics.mjs";
import { FACES } from "./engine.mjs";

test("all 18 physical face positions have a statistics mapping", () => {
  const events = Object.entries(FACES).flatMap(([die, faces]) =>
    faces.map(face => ({ seat: 0, die, face }))
  );
  const teams = matchStatistics(events);
  assert.equal(teams.flatMap(Object.values).reduce((a, b) => a + b, 0), 18);
});

test("statistics separate teams and repeatable events", () => {
  const [home, away] = matchStatistics([
    { seat: 0, die: "team", face: "Passaggio" },
    { seat: 0, die: "team", face: "Passaggio" },
    { seat: 0, die: "result", face: "GOAL!" },
    { seat: 1, die: "result", face: "Parato!" },
    { seat: 0, die: "result", face: "Parato!" },
  ]);
  assert.equal(home.passes, 2);
  assert.equal(home.goals, 1);
  assert.equal(home.saves, 1);
  assert.equal(away.saves, 1);
  assert.equal(away.goals, 0);
});

test("invalid events fail closed", () => {
  assert.throws(() => matchStatistics([
    { seat: 3, die: "result", face: "GOAL!" },
  ]), RangeError);
  assert.throws(() => matchStatistics([
    { seat: 0, die: "pink", face: "GOAL!" },
  ]), RangeError);
});
