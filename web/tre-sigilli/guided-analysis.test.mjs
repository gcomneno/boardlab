import assert from "node:assert/strict";

import {
  initial,
  legalMoves,
  applyMove,
  territoryScore,
  influence,
  terminal
} from "./engine.mjs";

import {
  analyzeMove,
  explainMoveItalian
} from "./guided-analysis.mjs";

let checks = 0;
let placementChecks = 0;
let movementChecks = 0;
let changedCellChecks = 0;

function checkState(state) {
  if (terminal(state)) return;

  for (const move of legalMoves(state)) {
    const original = structuredClone(state);
    const next = applyMove(state, move);
    const result = analyzeMove(state, move);

    assert.deepEqual(state, original);
    checks++;

    assert.deepEqual(result.scoreBefore, territoryScore(state));
    checks++;

    assert.deepEqual(result.scoreAfter, territoryScore(next));
    checks++;

    assert.equal(
      result.scoreDelta.north,
      result.scoreAfter.north - result.scoreBefore.north
    );
    checks++;

    assert.equal(
      result.scoreDelta.south,
      result.scoreAfter.south - result.scoreBefore.south
    );
    checks++;

    assert.equal(result.resultingTurn, state.turn + 1);
    checks++;

    const changed = [];

    for (let cell = 0; cell < 16; cell++) {
      const old = influence(state, cell);
      const now = influence(next, cell);

      const owner = value =>
        value.north > value.south ? "north" :
        value.south > value.north ? "south" : null;

      if (owner(old) !== owner(now)) changed.push(cell);
    }

    assert.deepEqual(
      result.changedCells.map(entry => entry.cell),
      changed
    );
    checks++;
    changedCellChecks += changed.length;

    const text = explainMoveItalian(result);

    assert(text.description.length > 0);
    assert(text.summary.includes("Avorio"));
    assert(text.summary.includes("Terracotta"));
    assert(text.disclaimer.includes("non una garanzia"));
    checks += 4;

    if (move.type === "place") {
      assert.equal(result.phase, "placement");
      placementChecks++;
    } else {
      assert.equal(result.phase, "movement");
      movementChecks++;
    }
    checks++;
  }
}

let state = initial();

for (let turn = 0; turn < 18; turn++) {
  if (terminal(state)) break;

  checkState(state);

  const moves = legalMoves(state);
  const selected = moves[(turn * 7 + 3) % moves.length];
  state = applyMove(state, selected);
}

assert(placementChecks > 0);
assert(movementChecks > 0);
checks += 2;

assert.throws(
  () => analyzeMove(initial(), {
    type: "place",
    cell: 99
  }),
  /Illegal suggested move/
);
checks++;

assert.throws(
  () => analyzeMove(state, {
    type: "slide",
    origin: -1,
    destination: 99
  }),
  /Illegal suggested move|Invalid game state/
);
checks++;

console.log(`GUIDED_ANALYSIS_ASSERTIONS=${checks}`);
console.log(`PLACEMENT_MOVES_ANALYZED=${placementChecks}`);
console.log(`MOVEMENT_MOVES_ANALYZED=${movementChecks}`);
console.log(`TERRITORIAL_CONTROL_CHANGES=${changedCellChecks}`);
console.log("GUIDED_ANALYSIS_ENGINE_GATE=PASS");
