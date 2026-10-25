import {
  player, legalMoves, applyMove, territoryScore,
  outcome, chooseMove
} from "./engine.mjs";

const DEFAULT_EXPLORATION = Math.SQRT2;

const schemas = Object.freeze({
  random: ["seed"],
  minimax: ["depth"],
  alpha_beta: ["depth"],
  mcts: ["simulations", "exploration", "seed"]
});

export const strategyNames = Object.freeze(Object.keys(schemas));

function integer(value, min, max, name) {
  if (!Number.isSafeInteger(value) || value < min || value > max) {
    throw new RangeError(`Invalid ${name}`);
  }
  return value;
}

function seedRandom(seed) {
  let x = seed >>> 0;
  return () => {
    x ^= x << 13;
    x ^= x >>> 17;
    x ^= x << 5;
    return (x >>> 0) / 4294967296;
  };
}

export function validateStrategy(config) {
  if (!config || typeof config !== "object" ||
      Array.isArray(config) || !schemas[config.algorithm]) {
    throw new Error("Unknown strategy");
  }

  const params = config.parameters ?? {};
  if (!params || typeof params !== "object" || Array.isArray(params)) {
    throw new Error("Invalid parameters");
  }

  for (const key of Object.keys(config)) {
    if (!["algorithm", "parameters"].includes(key)) {
      throw new Error(`Unknown configuration key: ${key}`);
    }
  }

  for (const key of Object.keys(params)) {
    if (!schemas[config.algorithm].includes(key)) {
      throw new Error(`Unsupported parameter: ${key}`);
    }
  }

  const { algorithm } = config;

  if (algorithm === "random") {
    return {
      algorithm,
      parameters: {
        seed: integer(params.seed ?? 42, 0, 4294967295, "seed")
      }
    };
  }

  if (algorithm === "minimax" || algorithm === "alpha_beta") {
    return {
      algorithm,
      parameters: {
        depth: integer(params.depth ?? 2, 1, 4, "depth")
      }
    };
  }

  const simulations = integer(
    params.simulations ?? 40, 1, 3000, "simulations"
  );
  const exploration = params.exploration ?? DEFAULT_EXPLORATION;

  if (typeof exploration !== "number" ||
      !Number.isFinite(exploration) || exploration < 0) {
    throw new RangeError("Invalid exploration");
  }

  return {
    algorithm,
    parameters: {
      simulations,
      exploration,
      seed: integer(params.seed ?? 42, 0, 4294967295, "seed")
    }
  };
}

function evaluate(position, root) {
  const result = outcome(position);
  if (result !== null) {
    if (result === "draw") return 0;
    return result === root ? Infinity : -Infinity;
  }

  const score = territoryScore(position);
  const difference = score.north - score.south;
  return root === "north" ? difference : -difference;
}

function minimax(state, depth) {
  const moves = legalMoves(state);
  const root = player(state);

  function search(position, remaining) {
    if (remaining === 0 || outcome(position) !== null) {
      return evaluate(position, root);
    }

    const values = legalMoves(position).map(move =>
      search(applyMove(position, move), remaining - 1)
    );

    return player(position) === root
      ? Math.max(...values)
      : Math.min(...values);
  }

  let best = moves[0];
  let value = -Infinity;

  for (const move of moves) {
    const candidate = search(applyMove(state, move), depth - 1);
    if (candidate > value) {
      best = move;
      value = candidate;
    }
  }
  return best;
}

function mcts(state, simulations, exploration, random) {
  const moves = legalMoves(state);

  function makeNode(position, lastPlayer, move = null) {
    return {
      position,
      lastPlayer,
      move,
      visits: 0,
      reward: 0,
      children: [],
      untried: legalMoves(position)
    };
  }

  const root = makeNode(state, player(state));

  for (let i = 0; i < simulations; i++) {
    let node = root;
    const path = [root];

    while (outcome(node.position) === null &&
           node.untried.length === 0 &&
           node.children.length > 0) {
      node = node.children.reduce((best, candidate) => {
        const score = child =>
          child.reward / child.visits +
          exploration * Math.sqrt(
            Math.log(node.visits) / child.visits
          );
        return score(candidate) > score(best) ? candidate : best;
      });
      path.push(node);
    }

    if (outcome(node.position) === null && node.untried.length) {
      const index = Math.floor(random() * node.untried.length);
      const [move] = node.untried.splice(index, 1);
      const next = applyMove(node.position, move);

      node = makeNode(next, player(node.position), move);
      path.at(-1).children.push(node);
      path.push(node);
    }

    let rollout = node.position;
    while (outcome(rollout) === null) {
      const choices = legalMoves(rollout);
      const move = choices[Math.floor(random() * choices.length)];
      rollout = applyMove(rollout, move);
    }

    const result = outcome(rollout);

    for (const visited of path.slice(1)) {
      visited.visits++;
      visited.reward += result === "draw"
        ? 0.5
        : result === visited.lastPlayer ? 1 : 0;
    }
    root.visits++;
  }

  let best = moves[0];
  let visits = -1;

  for (const move of moves) {
    const child = root.children.find(node =>
      JSON.stringify(node.move) === JSON.stringify(move)
    );
    const count = child?.visits ?? 0;
    if (count > visits) {
      best = move;
      visits = count;
    }
  }

  return best;
}

export function createStrategy(config) {
  const effective = validateStrategy(config);
  const { algorithm, parameters } = effective;
  const random = seedRandom(parameters.seed ?? 42);

  return Object.freeze({
    config: effective,

    choose(state) {
      const moves = legalMoves(state);
      if (!moves.length) throw new Error("No legal moves");

      switch (algorithm) {
        case "random":
          return moves[Math.floor(random() * moves.length)];
        case "minimax":
          return minimax(state, parameters.depth);
        case "alpha_beta":
          return chooseMove(state, parameters.depth);
        case "mcts":
          return mcts(
            state, parameters.simulations,
            parameters.exploration, random
          );
        default:
          throw new Error("Unknown strategy");
      }
    }
  });
}
