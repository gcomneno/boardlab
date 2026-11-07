import {
  createExtraordinaryEventState,
  extraordinaryEventCatalogue,
} from "../../web/dado-goal/extraordinary-events.mjs";

const EXTRAORDINARY_TYPES = new Set(
  extraordinaryEventCatalogue.map(event => event.type)
);

/**
 * Reconstruct authoritative narrative state from persisted D1 events.
 *
 * Legacy matches without initial rosters have no event context.
 * Replay never draws random numbers and never changes dice or scoring.
 */
export function replayMatchEvents(entries) {
  let context = null;

  for (const entry of entries) {
    const event = typeof entry.event_json === "string"
      ? JSON.parse(entry.event_json)
      : entry;

    if (event.type === "joined" && Array.isArray(event.rosters)) {
      if (context !== null || event.rosters.length !== 2) {
        throw new Error("Invalid initial roster event");
      }

      context = {
        rosters: structuredClone(event.rosters),
        refereeStates: [
          { yellowed: new Set() },
          { yellowed: new Set() },
        ],
        extraordinaryState: createExtraordinaryEventState(),
      };
      continue;
    }

    if (!context || event.type !== "roll" || !event.matchEvent) {
      continue;
    }

    const change = event.matchEvent;
    const seat = change.team;

    if (seat !== 0 && seat !== 1) {
      throw new Error("Invalid event team");
    }

    const roster = context.rosters[seat];
    const referee = context.refereeStates[seat];

    if (change.classification === "narrative") {
      const catalogueEvent = extraordinaryEventCatalogue.find(
        event => event.type === change.type
      );

      if (!catalogueEvent || !EXTRAORDINARY_TYPES.has(change.type) ||
          change.family !== catalogueEvent.family) {
        throw new Error("Invalid extraordinary event replay");
      }

      context.extraordinaryState.occurredTypes.add(change.type);
      if (["weather", "external", "technical"].includes(change.family)) {
        context.extraordinaryState.occurredFamilies.add(change.family);
      }
      continue;
    }

    if (change.type === "yellow") {
      if (!roster.active.includes(change.number) ||
          referee.yellowed.has(change.number)) {
        throw new Error("Invalid yellow card replay");
      }
      referee.yellowed.add(change.number);
    } else if (change.type === "red") {
      const index = roster.active.indexOf(change.number);
      if (index < 0) throw new Error("Invalid red card replay");
      roster.active.splice(index, 1);
      roster.sentOff.push(change.number);
    } else if (change.type === "substitution") {
      const outgoing = roster.active.indexOf(change.out);
      const incoming = roster.bench.indexOf(change.in);

      if (outgoing < 0 || incoming < 0 || roster.substitutions >= 5) {
        throw new Error("Invalid substitution replay");
      }

      const outgoingPosition =
        roster.identities?.[change.out]?.position;
      const incomingPosition =
        roster.identities?.[change.in]?.position;

      if (outgoingPosition === "Goalkeeper" &&
          incomingPosition !== "Goalkeeper") {
        throw new Error("Invalid goalkeeper substitution replay");
      }

      roster.active.splice(outgoing, 1, change.in);
      roster.bench.splice(incoming, 1);
      roster.departed.push(change.out);
      roster.substitutions += 1;
    } else {
      throw new Error("Unknown match event");
    }
  }

  return context;
}
