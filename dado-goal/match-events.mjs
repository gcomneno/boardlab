/**
 * Optional narrative events evaluated once per completed hand.
 * Probabilities are absolute, mutually exclusive and independent
 * of scoring. Random input can be injected for deterministic tests.
 */
import { substitute, sendOff } from "./match-roster.mjs";

export function eventProbabilities(minute) {
  if (!Number.isFinite(minute) || minute < 0 || minute > 90) {
    throw new RangeError("Invalid match minute");
  }

  return {
    yellow: 0.02,
    red: 0.002,
    substitution: minute <= 60 ? 0.002
      : minute <= 75 ? 0.02 : 0.08,
  };
}

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
