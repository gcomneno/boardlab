/**
 * Deterministic port of dado_goal.py and dado_goal_wire.py.
 *
 * No random numbers, HTTP requests or database operations belong here.
 * Only the server authority may choose a die face.
 */

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

const PHASES = new Set(["team", "pink", "result", "finished"]);
const FIELDS = [
  "schemaVersion", "handsPerTeam", "player",
  "hands", "goals", "phase",
];

export function validateState(state) {
  if (
    !state ||
    typeof state !== "object" ||
    Array.isArray(state) ||
    Object.keys(state).length !== FIELDS.length ||
    FIELDS.some(key => !Object.hasOwn(state, key))
  ) {
    throw new TypeError("Invalid snapshot fields");
  }

  const { schemaVersion, handsPerTeam, player, hands, goals, phase } = state;

  if (
    schemaVersion !== 1 ||
    !Number.isSafeInteger(handsPerTeam) ||
    handsPerTeam < 1 ||
    handsPerTeam > 1000 ||
    !Number.isInteger(player) ||
    ![0, 1].includes(player) ||
    !PHASES.has(phase) ||
    !Array.isArray(hands) ||
    !Array.isArray(goals) ||
    hands.length !== 2 ||
    goals.length !== 2
  ) {
    throw new TypeError("Invalid Dado Goal state");
  }

  for (let i = 0; i < 2; i++) {
    if (
      !Number.isInteger(hands[i]) ||
      hands[i] < 0 ||
      hands[i] > handsPerTeam ||
      !Number.isInteger(goals[i]) ||
      goals[i] < 0 ||
      goals[i] > hands[i]
    ) {
      throw new TypeError("Invalid Dado Goal counters");
    }
  }

  const total = hands[0] + hands[1];

  if (
    hands[0] !== Math.ceil(total / 2) ||
    hands[1] !== Math.floor(total / 2)
  ) {
    throw new TypeError("Invalid completed hands");
  }

  const finished =
    hands[0] === handsPerTeam &&
    hands[1] === handsPerTeam;

  if (
    (phase === "finished") !== finished ||
    (!finished && player !== total % 2)
  ) {
    throw new TypeError("Invalid phase or active player");
  }

  return {
    schemaVersion: 1,
    handsPerTeam,
    player,
    hands: [...hands],
    goals: [...goals],
    phase,
  };
}

export function initialState(handsPerTeam) {
  if (
    !Number.isInteger(handsPerTeam) ||
    handsPerTeam < 1 ||
    handsPerTeam > 1000
  ) {
    throw new TypeError("Invalid match length");
  }

  return validateState({
    schemaVersion: 1,
    handsPerTeam,
    player: 0,
    hands: [0, 0],
    goals: [0, 0],
    phase: "team",
  });
}

function endHand(state, scored = false) {
  const hands = [...state.hands];
  const goals = [...state.goals];

  hands[state.player]++;

  if (scored) {
    goals[state.player]++;
  }

  const finished = hands.every(
    count => count === state.handsPerTeam
  );

  return validateState({
    ...state,
    player: 1 - state.player,
    hands,
    goals,
    phase: finished ? "finished" : "team",
  });
}

export function applyRoll(input, face) {
  const state = validateState(input);
  const phase = state.phase;

  if (phase === "finished") {
    throw new TypeError("Cannot roll after match completion");
  }

  if (!FACES[phase].includes(face)) {
    throw new TypeError("Invalid die face for phase");
  }

  if (phase === "team") {
    if (face === "Passaggio") {
      return state;
    }
    if (face === "Passaggio intercettato!") {
      return endHand(state);
    }
    return validateState({ ...state, phase: "pink" });
  }

  if (phase === "pink") {
    if ([
      "Mezza rovesciata",
      "Rovesciata",
      "Colpo di testa",
    ].includes(face)) {
      return validateState({ ...state, phase: "result" });
    }
    return endHand(state);
  }

  if (face === "RIGORE") {
    return state;
  }

  return endHand(state, face === "GOAL!");
}
