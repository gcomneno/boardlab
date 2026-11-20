import { normalizeV2Event } from "./roll-event-v2.mjs";

/* M034: V2-only statistics. Does not reinterpret V1 history. */
const METRICS = Object.freeze({
  attack: ["attacks"],
  "long-range-attack": ["longRangeAttacks"],
  "intercepted-pass": ["interceptions"],
  "possession-lost": ["possessionLost"],
  cross: ["crosses"],
  "successful-pass": ["passes"],
  "powerful-shot": ["shots", "powerfulShots"],
  "half-bicycle-kick": ["shots", "halfBicycleKicks"],
  "bicycle-kick": ["shots", "bicycleKicks"],
  header: ["shots", "headers"],
  "shot-blocked": ["shots", "blockedShots"],
  "ball-out": ["ballOut"],
  goal: ["goals"],
  save: ["saves"],
  wide: ["wide"],
  woodwork: ["woodwork"],
  blocked: ["blocked"],
  "deflected-wide": ["deflectedWide"],
  "penalty-awarded": ["penalties"],
  "penalty-goal": ["penaltyGoals", "goals"],
  "penalty-missed": ["penaltyMissed"],
});

const KEYS = Object.freeze([
  ...new Set(Object.values(METRICS).flat()),
]);

function emptyTeam() {
  return {
    ...Object.fromEntries(KEYS.map(key => [key, 0])),
    possession: 50,
    possessionRolls: 0,
  };
}

export function matchStatisticsV2(events) {
  if (!Array.isArray(events)) {
    throw new TypeError("V2 events must be an array");
  }

  const teams = [emptyTeam(), emptyTeam()];
  const rolls = [0, 0];

  for (const raw of events) {
    const event = normalizeV2Event(raw);
    const metrics = METRICS[event.kind];

    if (!metrics) {
      throw new RangeError("Unmapped V2 statistic kind");
    }

    if (event.type === "roll") {
      rolls[event.seat] += 1;
    }

    for (const metric of metrics) {
      const owner = metric === "saves"
        ? 1 - event.seat
        : event.seat;
      teams[owner][metric] += 1;
    }
  }

  const total = rolls[0] + rolls[1];
  const home = total === 0
    ? 50
    : Math.round(100 * rolls[0] / total);

  teams[0].possession = home;
  teams[1].possession = 100 - home;
  teams[0].possessionRolls = rolls[0];
  teams[1].possessionRolls = rolls[1];

  return teams;
}
