export const FACES = Object.freeze({
  team: Object.freeze([
    "Tiro in porta!",
    "Tiro in porta!",
    "Passaggio intercettato!",
    "Passaggio intercettato!",
    "Cross!",
    "Passaggio",
  ]),
  pink: Object.freeze([
    "Palla sul fondo",
    "Mezza rovesciata",
    "Palla alta sulla traversa",
    "Palla intercettata",
    "Rovesciata",
    "Colpo di testa",
  ]),
  result: Object.freeze([
    "Respinto!",
    "GOAL!",
    "FUORI!",
    "PALO/TRAVERSA",
    "RIGORE",
    "Parato!",
  ]),
});

export function newMatch(handsPerTeam = 10) {
  if (!Number.isSafeInteger(handsPerTeam) || handsPerTeam < 1) {
    throw new RangeError("Invalid number of hands");
  }

  return {
    handsPerTeam,
    player: 0,
    hands: [0, 0],
    goals: [0, 0],
    phase: "team",
    lastRoll: null,
  };
}

export function rollFace(phase, random = Math.random) {
  const faces = FACES[phase];
  if (!faces) throw new RangeError("Invalid die phase");

  const value = random();
  if (!Number.isFinite(value) || value < 0 || value >= 1) {
    throw new RangeError("Invalid random value");
  }

  return faces[Math.floor(value * faces.length)];
}

function endHand(state, goal) {
  const hands = [...state.hands];
  const goals = [...state.goals];

  hands[state.player] += 1;
  if (goal) goals[state.player] += 1;

  const finished = hands.every(
    (count) => count === state.handsPerTeam
  );

  return {
    ...state,
    hands,
    goals,
    player: 1 - state.player,
    phase: finished ? "finished" : "team",
  };
}

export function applyRoll(state, face) {
  if (!state || state.phase === "finished") {
    throw new RangeError("Match already finished or invalid");
  }

  const validFaces = FACES[state.phase];
  if (!validFaces?.includes(face)) {
    throw new RangeError("Invalid face for current die");
  }

  const next = { ...state, lastRoll: face };

  if (state.phase === "team") {
    if (face === "Passaggio") return next;
    if (face === "Passaggio intercettato!") {
      return endHand(next, false);
    }
    return { ...next, phase: "pink" };
  }

  if (state.phase === "pink") {
    if ([
      "Mezza rovesciata",
      "Rovesciata",
      "Colpo di testa",
    ].includes(face)) {
      return { ...next, phase: "result" };
    }
    return endHand(next, false);
  }

  if (face === "RIGORE") return next;
  return endHand(next, face === "GOAL!");
}
