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

const YELLOW_REASONS = Object.freeze([
  "reckless-challenge",
  "persistent-fouling",
  "dissent",
  "time-wasting",
  "unsporting-behaviour",
]);

const RED_REASONS = Object.freeze([
  "serious-foul-play",
  "violent-conduct",
  "denying-goal-opportunity",
]);

function randomReason(reasons, random) {
  return reasons[Math.floor(randomValue(random) * reasons.length)];
}

export function evaluateHandEvent(roster, eventState, minute,
                                  random = Math.random,
                                  { halftime = false } = {}) {
  const odds = eventProbabilities(minute, {
    halftime,
    substitutions: roster.substitutions,
  });
  const draw = randomValue(random);
  const yellowLimit = odds.yellow;
  const redLimit = yellowLimit + odds.red;
  const injuryLimit = redLimit + odds.injury;
  const substituteLimit = injuryLimit + odds.substitution;

  if (draw < yellowLimit) {
    const eligible = roster.active.filter(
      number => !eventState.yellowed.has(number)
    );
    if (!eligible.length) return null;

    const number = eligible[Math.floor(randomValue(random) * eligible.length)];
    eventState.yellowed.add(number);
    return {
      type: "yellow",
      minute,
      number,
      reason: randomReason(
        minute >= 80
          ? YELLOW_REASONS
          : YELLOW_REASONS.filter(reason => reason !== "time-wasting"),
        random,
      ),
    };
  }

  if (draw < redLimit) {
    const number = sendOff(roster, random);
    return number === null ? null : {
      type: "red",
      minute,
      number,
      reason: randomReason(RED_REASONS, random),
    };
  }

  if (draw < injuryLimit) {
    const change = substitute(
      roster,
      random,
      { allowSubstituteOut: true },
    );
    return change === null
      ? null
      : {
          type: "substitution",
          minute,
          ...change,
          reason: "injury",
        };
  }

  if (draw < substituteLimit) {
    const change = substitute(roster, random);
    return change === null ? null
      : {
          type: "substitution",
          minute,
          ...change,
          reason: "tactical",
        };
  }

  return null;
}
