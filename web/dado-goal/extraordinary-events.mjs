const EVENTS = Object.freeze([
  Object.freeze({
    type: "light-knock",
    family: "injury",
    probability: 0.006,
    requiresPlayer: true,
  }),
  Object.freeze({
    type: "heavy-rain",
    family: "weather",
    probability: 0.0025,
  }),
  Object.freeze({
    type: "strong-wind",
    family: "weather",
    probability: 0.0015,
  }),
  Object.freeze({
    type: "snow-poor-visibility",
    family: "weather",
    probability: 0.0005,
  }),
  Object.freeze({
    type: "pitch-invasion",
    family: "external",
    probability: 0.0003,
  }),
  Object.freeze({
    type: "object-on-pitch",
    family: "external",
    probability: 0.0004,
  }),
  Object.freeze({
    type: "animal-on-pitch",
    family: "external",
    probability: 0.0001,
  }),
  Object.freeze({
    type: "floodlight-failure",
    family: "technical",
    probability: 0.0002,
  }),
  Object.freeze({
    type: "damaged-goal-net",
    family: "technical",
    probability: 0.0002,
  }),
  Object.freeze({
    type: "referee-equipment-problem",
    family: "technical",
    probability: 0.0003,
  }),
]);

export const extraordinaryEventCatalogue = EVENTS;

export function createExtraordinaryEventState() {
  return {
    occurredFamilies: new Set(),
    occurredTypes: new Set(),
  };
}

function draw(random) {
  const value = random();
  if (!Number.isFinite(value) || value < 0 || value >= 1) {
    throw new RangeError("Invalid random value");
  }
  return value;
}

export function evaluateExtraordinaryEvent(
  roster,
  state,
  minute,
  random = Math.random,
) {
  if (!roster || !Array.isArray(roster.active) ||
      !state || !(state.occurredFamilies instanceof Set) ||
      !(state.occurredTypes instanceof Set) ||
      !Number.isInteger(minute) || minute < 0 || minute > 90) {
    throw new RangeError("Invalid extraordinary event input");
  }

  const eligibleEvents = EVENTS.filter(event => {
    if (state.occurredTypes.has(event.type)) return false;

    if (["weather", "external", "technical"].includes(event.family) &&
        state.occurredFamilies.has(event.family)) {
      return false;
    }

    if (event.requiresPlayer && roster.active.length === 0) return false;

    return true;
  });

  const value = draw(random);
  let limit = 0;

  for (const event of eligibleEvents) {
    limit += event.probability;

    if (value >= limit) continue;

    const result = {
      type: event.type,
      family: event.family,
      minute,
      classification: "narrative",
    };

    if (event.requiresPlayer) {
      const playerIndex = Math.floor(draw(random) * roster.active.length);
      result.number = roster.active[playerIndex];
    }

    state.occurredTypes.add(event.type);

    if (["weather", "external", "technical"].includes(event.family)) {
      state.occurredFamilies.add(event.family);
    }

    return result;
  }

  return null;
}
