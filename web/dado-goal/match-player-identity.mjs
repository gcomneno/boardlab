export function playerIdentity(roster, number) {
  if (!roster?.identities || number === null || number === undefined) {
    return null;
  }

  return roster.identities[number] ?? null;
}

export function enrichMatchEvent(roster, event) {
  if (!event) return null;

  if (event.type === "substitution") {
    return {
      ...event,
      outPlayer: playerIdentity(roster, event.out),
      inPlayer: playerIdentity(roster, event.in),
    };
  }

  if (event.type === "goal" ||
      event.type === "yellow" ||
      event.type === "red") {
    return {
      ...event,
      player: playerIdentity(roster, event.number),
    };
  }

  return { ...event };
}

export function playerLabel(identity, number) {
  return identity?.name
    ? `${identity.name} (#${number})`
    : String(number);
}

export function eventDisplayValues(event) {
  if (!event) return {};

  if (event.type === "substitution") {
    return {
      ...event,
      out: playerLabel(event.outPlayer, event.out),
      in: playerLabel(event.inPlayer, event.in),
    };
  }

  if (event.type === "goal" ||
      event.type === "yellow" ||
      event.type === "red") {
    return {
      ...event,
      number: playerLabel(event.player, event.number),
    };
  }

  return { ...event };
}
