import { applyRoll, newMatch, rollFace } from "./engine.mjs";

export function simulateMatch({
  handsPerTeam = 6,
  random = Math.random,
} = {}) {
  let state = newMatch(handsPerTeam);
  const rolls = [];

  while (state.phase !== "finished") {
    const seat = state.player;
    const die = state.phase;
    const face = rollFace(die, random);

    rolls.push({ seat, die, face });
    state = applyRoll(state, face);
  }

  return {
    homeGoals: state.goals[0],
    awayGoals: state.goals[1],
    rolls,
  };
}
