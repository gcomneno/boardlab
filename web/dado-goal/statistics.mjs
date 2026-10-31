/**
 * Pure Dado Goal match statistics.
 *
 * Input events use the server's authoritative shape:
 * { seat: 0|1, die: "team"|"pink"|"result", face: string }
 *
 * No game transitions, persistence or network access.
 */
import { FACES } from "./engine.mjs";

const CATEGORIES = Object.freeze({
  team: Object.freeze({
    "Tiro in porta!": "shots",
    "Cross!": "crosses",
    "Passaggio": "passes",
    "Passaggio intercettato!": "interceptions",
  }),
  pink: Object.freeze({
    "Palla sul fondo": "ballOut",
    "Mezza rovesciata": "halfBicycleKicks",
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

  for (const event of events) {
    if (!event || !Number.isInteger(event.seat) ||
        ![0, 1].includes(event.seat) ||
        !Object.hasOwn(FACES, event.die) ||
        !FACES[event.die].includes(event.face)) {
      throw new RangeError("Invalid roll event");
    }

    const category = CATEGORIES[event.die][event.face];
    if (!category) throw new RangeError("Unmapped roll event");
    teams[category === "saves" ? 1 - event.seat : event.seat][category] += 1;
  }

  return teams;
}
