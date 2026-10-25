export const initial = () => ({ cells: Array(16).fill(null), turn: 0 });

export function validate(s) {
  if (!s || !Array.isArray(s.cells) || s.cells.length !== 16 ||
      !Number.isInteger(s.turn) || s.turn < 0 || s.turn > 18 ||
      s.cells.some(x => x !== null && x !== "north" && x !== "south") ||
      s.cells.filter(x => x === "north").length !== Math.min(Math.floor((s.turn + 1) / 2), 3) ||
      s.cells.filter(x => x === "south").length !== Math.min(Math.floor(s.turn / 2), 3)) {
    throw new Error("Invalid game state");
  }
}

export const player = s => s.turn % 2 === 0 ? "north" : "south";

export function adjacent(a, b) {
  return Math.abs(Math.floor(a / 4) - Math.floor(b / 4)) +
         Math.abs(a % 4 - b % 4) === 1;
}

export function legalMoves(s) {
  validate(s);
  if (s.turn === 18) return [];
  if (s.turn < 6) {
    return s.cells.flatMap((owner, cell) =>
      owner === null ? [{ type: "place", cell }] : []);
  }
  const moves = [];
  for (let origin = 0; origin < 16; origin++) {
    if (s.cells[origin] !== player(s)) continue;
    for (let destination = 0; destination < 16; destination++) {
      if (s.cells[destination] === null && adjacent(origin, destination)) {
        moves.push({ type: "slide", origin, destination });
      }
    }
  }
  return moves;
}

export const terminal = s => legalMoves(s).length === 0;

export function applyMove(s, move) {
  const legal = legalMoves(s).some(m =>
    m.type === move?.type &&
    (m.type === "place"
      ? m.cell === move.cell
      : m.origin === move.origin && m.destination === move.destination)
  );
  if (!legal) throw new Error("Illegal move");
  const cells = [...s.cells];
  if (move.type === "place") {
    cells[move.cell] = player(s);
  } else {
    cells[move.origin] = null;
    cells[move.destination] = player(s);
  }
  const next = { cells, turn: s.turn + 1 };
  validate(next);
  return next;
}

export function influence(s, cell) {
  validate(s);
  if (!Number.isInteger(cell) || cell < 0 || cell > 15) {
    throw new Error("Invalid cell");
  }
  const counts = { north: 0, south: 0 };
  for (let origin = 0; origin < 16; origin++) {
    if (origin !== cell && !adjacent(origin, cell)) continue;
    const owner = s.cells[origin];
    if (owner !== null) counts[owner]++;
  }
  return counts;
}

export function territoryScore(s) {
  validate(s);
  const score = { north: 0, south: 0 };
  for (let i = 0; i < 16; i++) {
    const n = influence(s, i);
    if (n.north > n.south) score.north++;
    if (n.south > n.north) score.south++;
  }
  return score;
}

export function outcome(s) {
  if (!terminal(s)) return null;
  const score = territoryScore(s);
  return score.north === score.south
    ? "draw"
    : score.north > score.south ? "north" : "south";
}

export function chooseMove(s, depth = 2) {
  if (!Number.isInteger(depth) || depth < 1) {
    throw new Error("Invalid depth");
  }
  const moves = legalMoves(s);
  if (moves.length === 0) throw new Error("No legal moves");
  const root = player(s);

  function evaluate(position) {
    const result = outcome(position);
    if (result !== null) {
      if (result === "draw") return 0;
      return result === root ? Infinity : -Infinity;
    }
    const score = territoryScore(position);
    const diff = score.north - score.south;
    return root === "north" ? diff : -diff;
  }

  function search(position, remaining, lower, upper) {
    if (outcome(position) !== null || remaining === 0) {
      return evaluate(position);
    }
    const maximizing = player(position) === root;
    let alpha = lower, beta = upper;
    let best = maximizing ? -Infinity : Infinity;
    for (const move of legalMoves(position)) {
      const value = search(applyMove(position, move), remaining - 1, alpha, beta);
      if (maximizing) {
        best = Math.max(best, value);
        alpha = Math.max(alpha, best);
      } else {
        best = Math.min(best, value);
        beta = Math.min(beta, best);
      }
      if (alpha >= beta) break;
    }
    return best;
  }

  let bestMove = moves[0], bestValue = -Infinity, alpha = -Infinity;
  for (const move of moves) {
    const value = search(applyMove(s, move), depth - 1, alpha, Infinity);
    if (value > bestValue) {
      bestValue = value;
      bestMove = move;
    }
    alpha = Math.max(alpha, bestValue);
  }
  return bestMove;
}
