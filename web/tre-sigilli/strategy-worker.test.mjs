import assert from "node:assert/strict";

import {
  initial, legalMoves, applyMove
} from "./engine.mjs";

import { createStrategy } from "./strategies.mjs";

const responses = [];
globalThis.self = {
  postMessage(message) {
    responses.push(message);
  }
};

await import("./strategy-worker.mjs");

function send(data) {
  self.onmessage({ data });
  return responses.at(-1);
}

let checks = 0;

const configurations = [
  { algorithm: "random", parameters: { seed: 42 } },
  { algorithm: "minimax", parameters: { depth: 2 } },
  { algorithm: "alpha_beta", parameters: { depth: 2 } },
  {
    algorithm: "mcts",
    parameters: {
      simulations: 40,
      exploration: Math.SQRT2,
      seed: 42
    }
  }
];

for (const config of configurations) {
  const expected = createStrategy(config);

  const configured = send({
    id: checks,
    type: "configure",
    config
  });

  assert.equal(configured.type, "configured");
  assert.deepEqual(configured.config, expected.config);
  checks++;

  let state = initial();

  for (let turn = 0; turn < 4; turn++) {
    const snapshot = structuredClone(state);

    const response = send({
      id: checks,
      type: "choose",
      state
    });

    assert.equal(response.type, "move");
    assert.deepEqual(response.move, expected.choose(state));
    assert.deepEqual(state, snapshot);

    assert(
      legalMoves(state).some(move =>
        JSON.stringify(move) === JSON.stringify(response.move)
      )
    );

    state = applyMove(state, response.move);
    checks++;
  }
}

const invalid = send({
  id: checks,
  type: "configure",
  config: {
    algorithm: "mcts",
    parameters: { simulations: 0 }
  }
});

assert.equal(invalid.type, "error");
checks++;

const unknown = send({
  id: checks,
  type: "unexpected"
});

assert.equal(unknown.type, "error");
checks++;

console.log(`STRATEGY_WORKER_PROTOCOL_ASSERTIONS=${checks}`);
console.log("STRATEGY_WORKER_PROTOCOL_GATE=PASS");
