import { FACES } from "./engine.mjs";

/**
 * Stable semantic identifiers for dice outcomes.
 *
 * `face` remains presentation/history-compatible data.
 * `kind` is the canonical machine-readable meaning used by statistics.
 */
const KIND_BY_DIE_AND_FACE = Object.freeze({
  team: Object.freeze({
    "Tiro in porta!": "shot",
    "Passaggio intercettato!": "intercepted-pass",
    "Cross!": "corner",
    "Passaggio": "pass",
  }),
  pink: Object.freeze({
    "Palla sul fondo": "ball-out",
    "Mezza rovesciata": "free-kick",
    "Palla alta sulla traversa": "over-bar",
    "Palla intercettata": "ball-intercepted",
    "Rovesciata": "bicycle-kick",
    "Colpo di testa": "header",
  }),
  result: Object.freeze({
    "Respinto!": "blocked",
    "GOAL!": "goal",
    "FUORI!": "wide",
    "PALO/TRAVERSA": "woodwork",
    "RIGORE": "penalty-awarded",
    "Parato!": "save",
  }),
  penalty: Object.freeze({
    "GOAL!": "penalty-goal",
    "PALO/TRAVERSA": "penalty-woodwork",
    "PARATO/RESPINTO": "penalty-saved",
    "FUORI": "penalty-wide",
  }),
});

export const ROLL_EVENT_KINDS = Object.freeze(
  Object.fromEntries(
    Object.entries(KIND_BY_DIE_AND_FACE).map(([die, faces]) => [
      die,
      Object.freeze([...new Set(Object.values(faces))]),
    ]),
  ),
);

function requireSeat(seat) {
  if (!Number.isInteger(seat) || ![0, 1].includes(seat)) {
    throw new RangeError("Invalid roll event seat");
  }
}

function requireDie(die) {
  if (!Object.hasOwn(FACES, die)) {
    throw new RangeError("Invalid roll event die");
  }
}

export function rollEventKind(die, face) {
  requireDie(die);

  if (!FACES[die].includes(face)) {
    throw new RangeError("Invalid roll event face");
  }

  const kind = KIND_BY_DIE_AND_FACE[die][face];
  if (!kind) {
    throw new RangeError("Unmapped roll event face");
  }

  return kind;
}

export function createRollEvent({ seat, die, face }) {
  requireSeat(seat);

  return Object.freeze({
    seat,
    die,
    kind: rollEventKind(die, face),
    face,
  });
}

/**
 * Migration boundary for persisted/runtime events produced before `kind`
 * existed. Current producers always use createRollEvent().
 *
 * Once `kind` is present it is authoritative for machine processing.
 * `face` is retained for UI/commentary and historical compatibility.
 */
export function normalizeRollEvent(event) {
  if (!event || typeof event !== "object" || Array.isArray(event)) {
    throw new RangeError("Invalid roll event");
  }

  requireSeat(event.seat);
  requireDie(event.die);

  if (event.kind === undefined) {
    return createRollEvent(event);
  }

  if (!ROLL_EVENT_KINDS[event.die].includes(event.kind)) {
    throw new RangeError("Invalid roll event kind");
  }

  return Object.freeze({
    ...event,
    seat: event.seat,
    die: event.die,
    kind: event.kind,
  });
}

export function normalizeRollEvents(events) {
  if (!Array.isArray(events)) {
    throw new TypeError("Events must be an array");
  }

  return events.map(normalizeRollEvent);
}
