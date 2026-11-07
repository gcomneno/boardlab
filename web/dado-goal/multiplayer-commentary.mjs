import {
  describeFace,
  describeMatchEvent,
} from "./commentary.mjs";

import {
  describeExtraordinaryEvent,
} from "./extraordinary-commentary.mjs";

const EXTRAORDINARY_TYPES = new Set([
  "light-knock",
  "heavy-rain",
  "strong-wind",
  "snow-poor-visibility",
  "pitch-invasion",
  "object-on-pitch",
  "animal-on-pitch",
  "floodlight-failure",
  "damaged-goal-net",
  "referee-equipment-problem",
]);

export function describeMultiplayerRollEvent(
  roll,
  teamNames = ["Squadra 1", "Squadra 2"],
  locale = "it",
) {
  if (!roll || !Number.isInteger(roll.seat) ||
      ![0, 1].includes(roll.seat)) {
    throw new RangeError("Invalid multiplayer roll event");
  }

  const entries = [];

  if (roll.goal &&
      roll.goal.type === "goal" &&
      roll.goal.team === roll.seat &&
      Number.isInteger(roll.goal.minute) &&
      roll.goal.minute >= 0) {
    const goal = locale === "en"
      ? `${roll.goal.minute}' — GOAL! ${teamNames[roll.seat]} scores!`
      : `${roll.goal.minute}' — GOAL! Segna ${teamNames[roll.seat]}!`;
    entries.push(goal);
  }

  if (roll.die === "result" && roll.face === "RIGORE") {
    entries.push(
      `${teamNames[roll.seat]}: ${describeFace(roll.face, 0, locale)}`,
    );
  }

  const change = roll.matchEvent;
  if (change) {
    entries.push(
      EXTRAORDINARY_TYPES.has(change.type)
        ? describeExtraordinaryEvent(change, locale)
        : describeMatchEvent(change, teamNames, locale),
    );
  }

  return entries;
}
