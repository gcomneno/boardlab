/**
 * Pure Dado Goal match statistics.
 *
 * Current canonical input:
 * { seat: 0|1, die: "team"|"pink"|"result"|"penalty",
 *   kind: string, face: string }
 *
 * Legacy persisted events without `kind` are normalized once at the
 * compatibility boundary. Statistical derivation itself uses only `kind`.
 *
 * No commentary, localization, game transitions, persistence or network
 * access participates in statistical derivation.
 */
import { normalizeRollEvents } from "./roll-event.mjs";

const CATEGORY_BY_KIND = Object.freeze({
  shot: "shots",
  corner: "corners",
  pass: "passes",
  "intercepted-pass": "interceptions",

  "ball-out": "ballOut",
  "free-kick": "freeKicks",
  "over-bar": "overBar",
  "ball-intercepted": "ballIntercepted",
  "bicycle-kick": "bicycleKicks",
  header: "headers",

  blocked: "blocked",
  goal: "goals",
  wide: "wide",
  woodwork: "woodwork",
  "penalty-awarded": "penalties",
  save: "saves",

  "penalty-goal": "penaltyGoals",
  "penalty-woodwork": "penaltyWoodwork",
  "penalty-saved": "penaltySaved",
  "penalty-wide": "penaltyWide",
});

const KEYS = Object.freeze([
  ...new Set(Object.values(CATEGORY_BY_KIND)),
]);

function emptyTeam() {
  return Object.fromEntries(KEYS.map(key => [key, 0]));
}

export function matchStatistics(events) {
  const canonicalEvents = normalizeRollEvents(events);
  const teams = [emptyTeam(), emptyTeam()];
  const possessionRolls = [0, 0];

  for (const event of canonicalEvents) {
    const category = CATEGORY_BY_KIND[event.kind];
    if (!category) throw new RangeError("Unmapped roll event kind");

    possessionRolls[event.seat] += 1;

    if (category === "saves") {
      teams[1 - event.seat][category] += 1;
    } else if (category === "penaltyGoals") {
      teams[event.seat].penaltyGoals += 1;
      teams[event.seat].goals += 1;
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
