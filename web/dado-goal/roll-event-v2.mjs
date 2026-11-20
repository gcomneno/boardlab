import { FACES_V2, RULES_VERSION_V2 } from "./engine-v2.mjs";

/*
 * M034: isolated V2 event adapter.
 * Face index is semantic identity; labels are presentation data.
 * No V1 interpretation, persistence or gameplay mutation.
 */
const KINDS = Object.freeze({
  team: Object.freeze([
    "attack", "long-range-attack", "intercepted-pass",
    "possession-lost", "cross", "successful-pass",
  ]),
  pink: Object.freeze([
    "powerful-shot", "half-bicycle-kick", "bicycle-kick",
    "header", "shot-blocked", "ball-out",
  ]),
  result: Object.freeze([
    "goal", "save", "wide", "woodwork",
    "blocked", "deflected-wide",
  ]),
  penalty: Object.freeze([
    "penalty-goal", "penalty-goal", "penalty-goal",
    "penalty-goal", "penalty-goal", "penalty-missed",
  ]),
});

export function normalizeV2Event(event) {
  if (!event || typeof event !== "object" || Array.isArray(event) ||
      event.rulesVersion !== RULES_VERSION_V2 ||
      !Number.isInteger(event.seat) ||
      ![0, 1].includes(event.seat)) {
    throw new RangeError("Invalid V2 event identity");
  }

  let kind;

  if (event.type === "penalty-awarded") {
    if ("die" in event || "faceIndex" in event || "face" in event) {
      throw new RangeError("Penalty award cannot be a die roll");
    }
    kind = "penalty-awarded";
  } else if (event.type === "roll") {
    if (!Object.hasOwn(FACES_V2, event.die) ||
        !Number.isInteger(event.faceIndex) ||
        event.faceIndex < 0 ||
        event.faceIndex >= FACES_V2[event.die].length ||
        event.face !== FACES_V2[event.die][event.faceIndex]) {
      throw new RangeError("Invalid V2 physical face");
    }
    kind = KINDS[event.die][event.faceIndex];
  } else {
    throw new RangeError("Invalid V2 event type");
  }

  if (event.kind !== undefined && event.kind !== kind) {
    throw new RangeError("Conflicting V2 event kind");
  }

  return Object.freeze({ ...event, kind });
}
