import assert from "node:assert/strict";
import test from "node:test";

import { simulateMatch } from "./match-simulator.mjs";

function sequenceRandom(values) {
  let index = 0;
  return () => values[index++ % values.length];
}

test("simulated match runs the canonical dice engine to completion", () => {
  const result = simulateMatch({
    handsPerTeam: 1,
    random: sequenceRandom([
      0.0,   // Tiro in porta!
      0.9,   // Colpo di testa
      0.2,   // GOAL!
      0.4,   // Passaggio intercettato!
    ]),
  });

  assert.equal(result.homeGoals, 1);
  assert.equal(result.awayGoals, 0);
  assert.deepEqual(result.rolls, [
    {
      seat: 0,
      die: "team",
      kind: "shot",
      face: "Tiro in porta!",
    },
    {
      seat: 0,
      die: "pink",
      kind: "header",
      face: "Colpo di testa",
    },
    {
      seat: 0,
      die: "result",
      kind: "goal",
      face: "GOAL!",
    },
    {
      seat: 1,
      die: "team",
      kind: "intercepted-pass",
      face: "Passaggio intercettato!",
    },
  ]);
});

test("simulated match preserves the canonical penalty die", () => {
  const result = simulateMatch({
    handsPerTeam: 1,
    random: sequenceRandom([
      0.0,   // Tiro in porta!
      0.9,   // Colpo di testa
      0.7,   // RIGORE
      0.0,   // penalty GOAL!
      0.4,   // Passaggio intercettato!
    ]),
  });

  assert.equal(result.homeGoals, 1);
  assert.equal(result.awayGoals, 0);
  assert.equal(result.rolls[3].die, "penalty");
  assert.equal(result.rolls[3].face, "GOAL!");
});

test("default simulation produces a valid completed score", () => {
  const result = simulateMatch();

  assert.ok(Number.isSafeInteger(result.homeGoals));
  assert.ok(Number.isSafeInteger(result.awayGoals));
  assert.ok(result.homeGoals >= 0);
  assert.ok(result.awayGoals >= 0);
  assert.ok(result.rolls.length >= 12);
});
