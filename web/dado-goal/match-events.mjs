/**
 * Optional narrative events evaluated once per completed hand.
 * Probabilities are absolute, mutually exclusive and independent
 * of scoring. Random input can be injected for deterministic tests.
 */
import { substitute, sendOff } from "./match-roster.mjs";

import { eventProbabilities } from "./event-probabilities.mjs";
export { eventProbabilities } from "./event-probabilities.mjs";

function randomValue(random) {
  const value = random();
  if (typeof value !== "number" ||
      !Number.isFinite(value) || value < 0 || value >= 1) {
    throw new RangeError("Invalid random value");
  }
  return value;
}

export function createEventState() {
  return { yellowed: new Set() };
}

export function evaluateHandEvent(roster, eventState, minute,
                                  random = Math.random) {
  const odds = eventProbabilities(minute);
  const draw = randomValue(random);
  const yellowLimit = odds.yellow;
  const redLimit = yellowLimit + odds.red;
  const substituteLimit = redLimit + odds.substitution;

  if (draw < yellowLimit) {
    const eligible = roster.active.filter(
      number => !eventState.yellowed.has(number)
    );
    if (!eligible.length) return null;

    const number = eligible[Math.floor(randomValue(random) * eligible.length)];
    eventState.yellowed.add(number);
    return { type: "yellow", minute, number };
  }

  if (draw < redLimit) {
    const number = sendOff(roster, random);
    return number === null ? null : { type: "red", minute, number };
  }

  if (draw < substituteLimit) {
    const change = substitute(roster, random);
    return change === null ? null
      : { type: "substitution", minute, ...change };
  }

  return null;
}
