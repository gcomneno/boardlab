/**
 * Pure Dado Goal match statistics.
 *
 * Input events use the server's authoritative shape:
 * { seat: 0|1, die: "team"|"pink"|"result"|"penalty", face: string }
 *
 * No game transitions, persistence or network access.
 */
import { FACES } from "./engine.mjs";

const CATEGORIES = Object.freeze({
  team: Object.freeze({
    "Tiro in porta!": "shots",
    "Cross!": "corners",
    "Passaggio": "passes",
    "Passaggio intercettato!": "interceptions",
  }),
  pink: Object.freeze({
    "Palla sul fondo": "ballOut",
    "Mezza rovesciata": "freeKicks",
    "Palla alta sulla traversa": "overBar",
    "Palla intercettata": "ballIntercepted",
    "Rovesciata": "bicycleKicks",
    "Colpo di testa": "headers",
  }),
  result: Object.freeze({
    "Respinto!": "blocked",
    "GOAL!": "goals",
    "FUORI!": "wide",
    "PALO/TRAVERSA": "woodwork",
    "RIGORE": "penalties",
    "Parato!": "saves",
  }),
  penalty: Object.freeze({
    "GOAL!": "penaltyGoals",
    "PALO/TRAVERSA": "penaltyWoodwork",
    "PARATO/RESPINTO": "penaltySaved",
    "FUORI": "penaltyWide",
  }),
});

const KEYS = Object.freeze([
  ...new Set(Object.values(CATEGORIES).flatMap(group => Object.values(group))),
]);

function emptyTeam() {
  return Object.fromEntries(KEYS.map(key => [key, 0]));
}

export function matchStatistics(events) {
  if (!Array.isArray(events)) {
    throw new TypeError("Events must be an array");
  }

  const teams = [emptyTeam(), emptyTeam()];
  const possessionRolls = [0, 0];

  for (const event of events) {
    if (!event || !Number.isInteger(event.seat) ||
        ![0, 1].includes(event.seat) ||
        !Object.hasOwn(FACES, event.die) ||
        !FACES[event.die].includes(event.face)) {
      throw new RangeError("Invalid roll event");
    }

    const category = CATEGORIES[event.die][event.face];
    if (!category) throw new RangeError("Unmapped roll event");

    possessionRolls[event.seat] += 1;

    if (category === "saves") {
      teams[1 - event.seat][category] += 1;
    } else if (category === "penaltySaved") {
      teams[1 - event.seat][category] += 1;
    } else {
      teams[event.seat][category] += 1;
    }
  }

  const totalRolls = possessionRolls[0] + possessionRolls[1];
  const homePossession = totalRolls === 0
    ? 50
    : Math.round((possessionRolls[0] / totalRolls) * 100);

  teams[0].possession = homePossession;
  teams[1].possession = 100 - homePossession;
  teams[0].possessionRolls = possessionRolls[0];
  teams[1].possessionRolls = possessionRolls[1];

  return teams;
}
