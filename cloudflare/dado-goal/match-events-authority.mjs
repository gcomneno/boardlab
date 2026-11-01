/**
 * Multiplayer adapter for the shared narrative match-event rules.
 *
 * The caller supplies the authoritative random source.
 * This module does not modify dice outcomes or scoring.
 */
import {
  createRoster
} from "../../web/dado-goal/match-roster.mjs";
import {
  createEventState,
  evaluateHandEvent
} from "../../web/dado-goal/match-events.mjs";
import { matchClock } from "./public/match-clock.mjs";

export function createMatchEventContext(random = Math.random) {
  return {
    rosters: [createRoster(random), createRoster(random)],
    refereeStates: [createEventState(), createEventState()],
  };
}

export function evaluateCompletedHand(
  before, after, context, random = Math.random
) {
  const seat = before.player;

  if (after.hands[seat] !== before.hands[seat] + 1) {
    return null;
  }

  const minute = Math.floor(matchClock(after).seconds / 60);
  const event = evaluateHandEvent(
    context.rosters[seat],
    context.refereeStates[seat],
    minute,
    random
  );

  return event ? { ...event, team: seat } : null;
}
