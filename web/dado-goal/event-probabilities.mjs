/**
 * Shared event probabilities for singleplayer and authoritative multiplayer.
 * Exactly one auxiliary event can be selected per completed hand.
 */
export function eventProbabilities(minute) {
  if (!Number.isFinite(minute) || minute < 0 || minute > 90) {
    throw new RangeError("Invalid match minute");
  }

  return {
    yellow: 0.02,
    red: 0.002,
    substitution: minute <= 60 ? 0.02
      : minute <= 75 ? 0.08 : 0.20,
  };
}
