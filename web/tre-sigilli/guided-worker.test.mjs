import assert from "node:assert/strict";

import {
  initial, legalMoves, applyMove, terminal
} from "./engine.mjs";

import {
  createStrategy
} from "./strategies.mjs";

import {
  analyzeMove, explainMoveItalian
} from "./guided-analysis.mjs";

const responses = [];

globalThis.self = {
  postMessage(message) {
    responses.push(message);
  }
};

await import("./strategy-worker.mjs");

function send(data) {
  const count = responses.length;
  self.onmessage({ data });
  assert.equal(responses.length, count + 1);
  return responses.at(-1);
}

let checks = 0;

const config = {
  algorithm: "random",
  parameters: { seed: 42 }
};

const configured = send({
  id: 0,
  type: "configure",
  config
});

assert.equal(configured.type, "configured");
checks++;

const opponentReference = createStrategy(config);
const tutorReference = createStrategy({
  algorithm: "alpha_beta",
  parameters: { depth: 2 }
});

let state = initial();
let placement = 0;
let movement = 0;

for (let turn = 0; turn < 18; turn++) {
  if (terminal(state)) break;

  const snapshot = structuredClone(state);

  const response = send({
    id: turn + 1,
    type: "suggest",
    state
  });

  assert.equal(response.id, turn + 1);
  assert.equal(response.type, "suggestion");
  assert.deepEqual(state, snapshot);
  checks += 3;

  const expected = tutorReference.choose(state);
  assert.deepEqual(response.move, expected);
  checks++;

  assert(
    legalMoves(state).some(move =>
      JSON.stringify(move) === JSON.stringify(response.move)
    )
  );
  checks++;

  assert.deepEqual(
    response.analysis,
    analyzeMove(state, response.move)
  );
  checks++;

  assert.deepEqual(
    response.explanation,
    explainMoveItalian(response.analysis)
  );
  checks++;

  if (state.turn < 6) placement++;
  else movement++;

  // Alternate between following the tutor and another legal move.
  const moves = legalMoves(state);
  const selected = turn % 2 === 0
    ? response.move
    : moves[moves.length - 1];

  state = applyMove(state, selected);
}

assert(placement > 0);
assert(movement > 0);
checks += 2;

// A suggestion must not consume the opponent's random sequence.
const opponentState = initial();

const actualOpponent = send({
  id: 100,
  type: "choose",
  state: opponentState
});

assert.equal(actualOpponent.type, "move");
assert.deepEqual(
  actualOpponent.move,
  opponentReference.choose(opponentState)
);
checks += 2;

// A malformed suggestion returns an error without killing the worker.
const invalid = send({
  id: 101,
  type: "suggest",
  state: { cells: [], turn: 0 }
});

assert.equal(invalid.type, "error");
checks++;

const recovered = send({
  id: 102,
  type: "suggest",
  state: initial()
});

assert.equal(recovered.type, "suggestion");
checks++;

console.log(`GUIDED_WORKER_ASSERTIONS=${checks}`);
console.log(`GUIDED_WORKER_PLACEMENT_POSITIONS=${placement}`);
console.log(`GUIDED_WORKER_MOVEMENT_POSITIONS=${movement}`);
console.log("GUIDED_WORKER_OPPONENT_ISOLATION_GATE=PASS");
console.log("GUIDED_WORKER_SUGGESTION_GATE=PASS");
