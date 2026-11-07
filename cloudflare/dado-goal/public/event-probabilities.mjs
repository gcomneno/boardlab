/**
 * Shared event probabilities for singleplayer and authoritative multiplayer.
 * Exactly one auxiliary event can be selected per completed hand.
 */
export function eventProbabilities(
  minute,
  { halftime = false, substitutions = null } = {},
) {
  if (!Number.isFinite(minute) || minute < 0 || minute > 90) {
    throw new RangeError("Invalid match minute");
  }

  let substitution;

  if (halftime) {
    substitution = 0.12;
  } else if (minute <= 45) {
    substitution = 0;
  } else if (minute <= 60) {
    substitution = 0.04;
  } else if (minute <= 75) {
    substitution = 0.10;
  } else {
    substitution = 0.20;
  }

  // A team still at zero changes late in the match gets a strong,
  // but never mandatory, incentive to make its first substitution.
  if (!halftime && minute >= 76 && substitutions === 0) {
    substitution = 0.35;
  }

  return {
    yellow: 0.02,
    red: 0.002,
    injury: 0.01,
    substitution,
  };
}
