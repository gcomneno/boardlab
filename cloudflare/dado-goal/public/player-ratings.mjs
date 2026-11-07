export const RATING_LEVELS = Object.freeze([
  "poor",
  "fair",
  "good",
  "very-good",
  "excellent",
]);

export const RATING_FIELDS = Object.freeze([
  "attack",
  "defence",
  "technique",
  "physical",
]);

function validateLevel(value) {
  if (!RATING_LEVELS.includes(value)) {
    throw new TypeError(`invalid rating level: ${value}`);
  }
  return value;
}

export function createPlayerRatings(values = {}) {
  const ratings = {};

  for (const field of RATING_FIELDS) {
    ratings[field] = validateLevel(values[field] ?? "fair");
  }

  return Object.freeze(ratings);
}

export function createRatedPlayer(player, ratings) {
  if (!player || typeof player !== "object") {
    throw new TypeError("player is required");
  }

  return Object.freeze({
    ...player,
    ratings: createPlayerRatings(ratings),
  });
}
