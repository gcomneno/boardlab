import assert from "node:assert/strict";
import {
  initial, legalMoves, applyMove, chooseMove, outcome
} from "./engine.mjs";
import {
  createStrategy, validateStrategy, strategyNames
} from "./strategies.mjs";

assert.deepEqual(
  strategyNames, ["random", "minimax", "alpha_beta", "mcts"]
);

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

let checks = 1;

for (const config of configurations) {
  const first = createStrategy(config);
  const second = createStrategy(config);
  let a = initial();
  let b = initial();

  for (let i = 0; i < 4; i++) {
    const original = structuredClone(a);
    const m1 = first.choose(a);
    const m2 = second.choose(b);

    assert.deepEqual(m1, m2);
    checks++;
    assert(legalMoves(a).some(m =>
      JSON.stringify(m) === JSON.stringify(m1)
    ));
    checks++;
    assert.deepEqual(a, original);
    checks++;

    a = applyMove(a, m1);
    b = applyMove(b, m2);
  }

  assert.deepEqual(a, b);
  checks++;
}

for (const depth of [1, 2, 3]) {
  const ai = createStrategy({
    algorithm: "minimax",
    parameters: { depth }
  });

  let state = initial();
  for (let i = 0; i < 5; i++) {
    assert.deepEqual(ai.choose(state), chooseMove(state, depth));
    checks++;
    state = applyMove(state, legalMoves(state)[i % legalMoves(state).length]);
  }
}

for (const bad of [
  { algorithm: "unknown" },
  { algorithm: "random", parameters: { depth: 2 } },
  { algorithm: "random", parameters: { seed: -1 } },
  { algorithm: "minimax", parameters: { depth: 0 } },
  { algorithm: "alpha_beta", parameters: { depth: 5 } },
  { algorithm: "mcts", parameters: { simulations: 0 } },
  { algorithm: "mcts", parameters: { exploration: -1 } },
  { algorithm: "mcts", parameters: { seed: 3.5 } }
]) {
  assert.throws(() => validateStrategy(bad));
  checks++;
}

for (const config of configurations) {
  const strategy = createStrategy(config);
  let state = initial();
  let safety = 0;

  while (outcome(state) === null) {
    state = applyMove(state, strategy.choose(state));
    if (++safety > 18) throw new Error("Game did not terminate");
  }

  assert(outcome(state) !== null);
  checks++;
}

console.log(`STRATEGY_LAB_ADAPTER_ASSERTIONS=${checks}`);
console.log("STRATEGY_LAB_FOUR_ALGORITHMS_GATE=PASS");
