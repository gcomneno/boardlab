/**
 * Simulated football match clock.
 *
 * Each completed hand advances the match by an equal fraction
 * of 90 minutes. Rolls within an unfinished hand do not advance time.
 *
 * The clock is derived from existing game state; it has no timer,
 * persistence, random input or dependency on a game engine.
 */
export function matchClock(state) {
  if (!state || !Number.isSafeInteger(state.handsPerTeam) ||
      state.handsPerTeam < 1 || state.handsPerTeam > 1000 ||
      !Array.isArray(state.hands) || state.hands.length !== 2 ||
      state.hands.some(h => !Number.isSafeInteger(h) ||
        h < 0 || h > state.handsPerTeam)) {
    throw new RangeError("Invalid match clock state");
  }

  const completed = state.hands[0] + state.hands[1];
  const total = 2 * state.handsPerTeam;
  const seconds = Math.round(5400 * completed / total);

  return {
    seconds,
    label: `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(seconds % 60).padStart(2, "0")}`,
    half: completed < state.handsPerTeam ? 1 : 2,
    halftime: completed === state.handsPerTeam,
    finished: completed === total,
  };
}
