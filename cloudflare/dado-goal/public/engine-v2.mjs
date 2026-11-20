/**
 * M034 — isolated rules V2 prototype.
 * No imports from, or modifications to, the canonical V1 engine.
 */

export const RULES_VERSION_V2 = 2;
export const PENALTY_AWARD_RATE_V2 = 0.025;

export const FACES_V2 = Object.freeze({
  team: Object.freeze([
    "Affondo offensivo",
    "Tiro dalla distanza",
    "Passaggio intercettato",
    "Palla persa",
    "Cross",
    "Passaggio riuscito",
  ]),
  pink: Object.freeze([
    "Tiro potente",
    "Mezza rovesciata",
    "Rovesciata",
    "Colpo di testa",
    "Tiro murato",
    "Palla sul fondo",
  ]),
  result: Object.freeze([
    "GOAL!",
    "Parato!",
    "FUORI!",
    "PALO/TRAVERSA",
    "Respinto!",
    "Deviazione fuori",
  ]),
  penalty: Object.freeze([
    "GOAL!",
    "GOAL!",
    "GOAL!",
    "GOAL!",
    "GOAL!",
    "SBAGLIATO!",
  ]),
});

const PHASES_V2 = new Set([
  "team", "pink", "result", "penalty", "finished",
]);

export function validateV2State(state) {
  if (
    !state ||
    state.rulesVersion !== RULES_VERSION_V2 ||
    state.schemaVersion !== 1 ||
    !Number.isSafeInteger(state.handsPerTeam) ||
    state.handsPerTeam < 1 ||
    state.handsPerTeam > 1000 ||
    ![0, 1].includes(state.player) ||
    !PHASES_V2.has(state.phase) ||
    !Array.isArray(state.hands) ||
    !Array.isArray(state.goals) ||
    state.hands.length !== 2 ||
    state.goals.length !== 2
  ) {
    throw new RangeError("Invalid V2 match state");
  }

  for (let i = 0; i < 2; i += 1) {
    if (
      !Number.isInteger(state.hands[i]) ||
      !Number.isInteger(state.goals[i]) ||
      state.hands[i] < 0 ||
      state.hands[i] > state.handsPerTeam ||
      state.goals[i] < 0 ||
      state.goals[i] > state.hands[i]
    ) {
      throw new RangeError("Invalid V2 score or hand count");
    }
  }

  const complete = state.hands.every(
    count => count === state.handsPerTeam,
  );
  if ((state.phase === "finished") !== complete) {
    throw new RangeError("Invalid V2 completion state");
  }

  return state;
}

export function newV2Match(handsPerTeam = 10) {
  return validateV2State({
    schemaVersion: 1,
    rulesVersion: RULES_VERSION_V2,
    handsPerTeam,
    player: 0,
    hands: [0, 0],
    goals: [0, 0],
    phase: "team",
  });
}

function completeHand(state, goal = false) {
  const hands = [...state.hands];
  const goals = [...state.goals];
  hands[state.player] += 1;
  if (goal) goals[state.player] += 1;

  const finished = hands.every(
    count => count === state.handsPerTeam,
  );

  return validateV2State({
    ...state,
    hands,
    goals,
    player: finished ? state.player : 1 - state.player,
    phase: finished ? "finished" : "team",
  });
}

export function applyV2Event(input, event) {
  const state = validateV2State(input);

  if (state.phase === "finished") {
    throw new RangeError("Match already finished");
  }

  if (
    !event ||
    typeof event !== "object" ||
    Array.isArray(event) ||
    event.rulesVersion !== RULES_VERSION_V2 ||
    event.seat !== state.player
  ) {
    throw new RangeError("Invalid V2 event identity");
  }

  if (event.type === "penalty-awarded") {
    if (state.phase !== "result") {
      throw new RangeError("Penalty award outside final resolution");
    }
    return validateV2State({ ...state, phase: "penalty" });
  }

  if (
    event?.type !== "roll" ||
    event.die !== state.phase ||
    !Number.isInteger(event.faceIndex) ||
    event.faceIndex < 0 ||
    event.faceIndex >= FACES_V2[state.phase].length ||
    (
      event.face !== undefined &&
      event.face !== FACES_V2[state.phase][event.faceIndex]
    )
  ) {
    throw new RangeError("Invalid V2 roll event");
  }

  const index = event.faceIndex;

  if (state.phase === "team") {
    if (index === 5) return state;
    if (index === 2 || index === 3) return completeHand(state);
    return validateV2State({ ...state, phase: "pink" });
  }

  if (state.phase === "pink") {
    if (index >= 4) return completeHand(state);
    return validateV2State({ ...state, phase: "result" });
  }

  if (state.phase === "result") {
    return completeHand(state, index === 0);
  }

  return completeHand(state, index < 5);
}

function unitRandom(random) {
  const value = random();
  if (
    typeof value !== "number" ||
    !Number.isFinite(value) ||
    value < 0 ||
    value >= 1
  ) {
    throw new RangeError("Random source must return [0, 1)");
  }
  return value;
}

export function rollV2(input, random = Math.random) {
  const state = validateV2State(input);

  if (state.phase === "finished") {
    throw new RangeError("Match already finished");
  }

  // This decision is recorded if awarded; replay never rerolls it.
  if (
    state.phase === "result" &&
    unitRandom(random) < PENALTY_AWARD_RATE_V2
  ) {
    const event = Object.freeze({
      type: "penalty-awarded",
      rulesVersion: RULES_VERSION_V2,
      seat: state.player,
    });
    return {
      event,
      state: applyV2Event(state, event),
    };
  }

  const die = state.phase;
  const faceIndex = Math.floor(
    unitRandom(random) * FACES_V2[die].length,
  );
  const event = Object.freeze({
    type: "roll",
    rulesVersion: RULES_VERSION_V2,
    seat: state.player,
    die,
    faceIndex,
    face: FACES_V2[die][faceIndex],
  });

  return {
    event,
    state: applyV2Event(state, event),
  };
}
