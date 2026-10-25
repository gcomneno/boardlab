import {
  legalMoves,
  applyMove,
  player,
  territoryScore,
  influence
} from "./engine.mjs";

function ownerOf(counts) {
  if (counts.north > counts.south) return "north";
  if (counts.south > counts.north) return "south";
  return null;
}

function sameMove(a, b) {
  if (a?.type !== b?.type) return false;

  if (a.type === "place") {
    return a.cell === b.cell;
  }

  if (a.type === "slide") {
    return a.origin === b.origin &&
           a.destination === b.destination;
  }

  return false;
}

export function analyzeMove(state, move) {
  const legal = legalMoves(state);

  if (!legal.some(candidate => sameMove(candidate, move))) {
    throw new Error("Illegal suggested move");
  }

  const active = player(state);
  const before = territoryScore(state);
  const next = applyMove(state, move);
  const after = territoryScore(next);

  const changes = [];

  for (let cell = 0; cell < 16; cell++) {
    const previousInfluence = influence(state, cell);
    const nextInfluence = influence(next, cell);

    const previousOwner = ownerOf(previousInfluence);
    const nextOwner = ownerOf(nextInfluence);

    if (previousOwner !== nextOwner) {
      changes.push({
        cell,
        beforeOwner: previousOwner,
        afterOwner: nextOwner,
        beforeInfluence: previousInfluence,
        afterInfluence: nextInfluence
      });
    }
  }

  const delta = {
    north: after.north - before.north,
    south: after.south - before.south
  };

  return {
    move: { ...move },
    player: active,
    phase: move.type === "place" ? "placement" : "movement",
    scoreBefore: { ...before },
    scoreAfter: { ...after },
    scoreDelta: delta,
    changedCells: changes,
    resultingTurn: next.turn
  };
}

export function explainMoveItalian(analysis) {
  const move = analysis.move;

  const description = move.type === "place"
    ? `Colloca un sigillo nella casella ${move.cell + 1}.`
    : `Sposta il sigillo dalla casella ${move.origin + 1} ` +
      `alla casella ${move.destination + 1}.`;

  const delta = analysis.scoreDelta;

  const summary =
    `Territorio dopo la mossa: ` +
    `Avorio ${analysis.scoreAfter.north} ` +
    `(variazione ${delta.north >= 0 ? "+" : ""}${delta.north}), ` +
    `Terracotta ${analysis.scoreAfter.south} ` +
    `(variazione ${delta.south >= 0 ? "+" : ""}${delta.south}).`;

  const changed = analysis.changedCells.length;

  const detail = changed === 0
    ? "Nessuna casella cambia controllo immediatamente."
    : changed === 1
      ? "Una casella cambia controllo immediatamente."
      : `${changed} caselle cambiano controllo immediatamente.`;

  return {
    description,
    summary,
    detail,
    disclaimer:
      "Questa è la conseguenza territoriale immediata, " +
      "non una garanzia di vittoria."
  };
}
