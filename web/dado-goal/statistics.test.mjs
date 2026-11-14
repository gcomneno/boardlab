import test from "node:test";
import assert from "node:assert/strict";
import { matchStatistics } from "./statistics.mjs";
import { FACES } from "./engine.mjs";

test("all physical face positions have a statistics mapping", () => {
  const events = Object.entries(FACES).flatMap(([die, faces]) =>
    faces.map(face => ({ seat: 0, die, face }))
  );
  const teams = matchStatistics(events);
  const mappedTotal = teams
    .flatMap(team => Object.entries(team))
    .filter(([key]) => !["possession", "possessionRolls"].includes(key))
    .reduce((sum, [, value]) => sum + value, 0);

  assert.equal(mappedTotal, 24);
  assert.equal(teams[0].possession, 100);
  assert.equal(teams[1].possession, 0);
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


test("extended set pieces and penalty die are counted canonically", () => {
  const [home, away] = matchStatistics([
    { seat: 0, die: "team", face: "Cross!" },
    { seat: 0, die: "pink", face: "Mezza rovesciata" },
    { seat: 0, die: "result", face: "RIGORE" },
    { seat: 0, die: "penalty", face: "GOAL!" },
    { seat: 1, die: "penalty", face: "PARATO/RESPINTO" },
  ]);

  assert.equal(home.corners, 1);
  assert.equal(home.freeKicks, 1);
  assert.equal(home.penalties, 1);
  assert.equal(home.penaltyGoals, 1);
  assert.equal(home.penaltySaved, 1);
  assert.equal(away.penaltySaved, 0);
});

test("possession is derived from valid roll counts and sums to 100", () => {
  const [home, away] = matchStatistics([
    { seat: 0, die: "team", face: "Passaggio" },
    { seat: 0, die: "team", face: "Passaggio" },
    { seat: 0, die: "team", face: "Tiro in porta!" },
    { seat: 1, die: "team", face: "Passaggio intercettato!" },
  ]);

  assert.equal(home.possessionRolls, 3);
  assert.equal(away.possessionRolls, 1);
  assert.equal(home.possession, 75);
  assert.equal(away.possession, 25);
  assert.equal(home.possession + away.possession, 100);
});
