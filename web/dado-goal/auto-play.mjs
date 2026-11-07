const MODES = new Set(["all", "team-0", "team-1"]);

export function createAutoConfig({
  mode = "all",
  handLimit = null,
  rollDelay = 350,
  handDelay = 350,
} = {}) {
  if (!MODES.has(mode)) throw new RangeError("Invalid AUTO mode");

  for (const [name, value] of [
    ["rollDelay", rollDelay],
    ["handDelay", handDelay],
  ]) {
    if (!Number.isInteger(value) || value < 0 || value > 10000) {
      throw new RangeError(`Invalid ${name}`);
    }
  }

  if (handLimit !== null &&
      (!Number.isInteger(handLimit) || handLimit < 1)) {
    throw new RangeError("Invalid AUTO hand limit");
  }

  return { mode, handLimit, rollDelay, handDelay };
}

export function autoAllowsTeam(config, team) {
  if (team !== 0 && team !== 1) throw new RangeError("Invalid team");
  if (config.mode === "team-0") return team === 0;
  if (config.mode === "team-1") return team === 1;
  return true;
}

export function autoQuotaReached(config, completedHands) {
  if (!Number.isInteger(completedHands) || completedHands < 0) {
    throw new RangeError("Invalid completed AUTO hands");
  }
  return config.handLimit !== null &&
    completedHands >= config.handLimit;
}

export function createInterruptibleDelay(setTimer = setTimeout,
                                         clearTimer = clearTimeout) {
  let timer = null;
  let resolvePending = null;

  return {
    wait(ms) {
      if (!Number.isInteger(ms) || ms < 0) {
        throw new RangeError("Invalid AUTO delay");
      }

      return new Promise(resolve => {
        resolvePending = resolve;
        timer = setTimer(() => {
          timer = null;
          resolvePending = null;
          resolve(true);
        }, ms);
      });
    },

    cancel() {
      if (timer === null) return false;

      clearTimer(timer);
      timer = null;

      const resolve = resolvePending;
      resolvePending = null;
      resolve(false);
      return true;
    }
  };
}

export function remainingHands(state) {
  if (!state ||
      !Number.isInteger(state.handsPerTeam) ||
      state.handsPerTeam < 1 ||
      !Array.isArray(state.hands) ||
      state.hands.length !== 2 ||
      state.hands.some(value =>
        !Number.isInteger(value) ||
        value < 0 ||
        value > state.handsPerTeam)) {
    throw new RangeError("Invalid match hand state");
  }

  return state.hands.reduce(
    (remaining, completed) =>
      remaining + state.handsPerTeam - completed,
    0
  );
}
