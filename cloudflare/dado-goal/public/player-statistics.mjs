/**
 * Pure per-player statistics derived exclusively from canonical match events.
 * Dice-roll statistics remain a separate team-level concern.
 */

function requireTeam(team) {
  if (team !== 0 && team !== 1) {
    throw new RangeError("player statistic event requires team 0 or 1");
  }
  return team;
}

function playerKey(identity, number) {
  if (identity?.providerPlayerId != null) {
    return `provider:${identity.providerPlayerId}`;
  }
  return `number:${number}`;
}

function playerRecord(identity, number) {
  return {
    providerPlayerId: identity?.providerPlayerId ?? null,
    name: identity?.name ?? null,
    number,
    goals: 0,
    penalties: 0,
    yellowCards: 0,
    redCards: 0,
    subbedOn: 0,
    subbedOff: 0,
  };
}

function requireNumber(number) {
  if (!Number.isInteger(number) || number <= 0) {
    throw new TypeError("player statistic event requires a valid shirt number");
  }
  return number;
}

export function playerStatistics(events) {
  if (!Array.isArray(events)) {
    throw new TypeError("match events must be an array");
  }

  const teams = [new Map(), new Map()];

  const record = (team, number, identity) => {
    const seat = requireTeam(team);
    const shirt = requireNumber(number);
    const key = playerKey(identity, shirt);

    if (!teams[seat].has(key)) {
      teams[seat].set(key, playerRecord(identity, shirt));
    }

    return teams[seat].get(key);
  };

  for (const event of events) {
    if (!event || typeof event !== "object") continue;

    if (event.type === "goal") {
      record(event.team, event.number, event.player).goals += 1;
      continue;
    }

    if (event.type === "penalty") {
      const player = record(event.team, event.number, event.player);
      player.penalties += 1;
      if (event.outcome === "GOAL!") {
        player.goals += 1;
      }
      continue;
    }

    if (event.type === "yellow") {
      record(event.team, event.number, event.player).yellowCards += 1;
      continue;
    }

    if (event.type === "red") {
      record(event.team, event.number, event.player).redCards += 1;
      continue;
    }

    if (event.type === "substitution") {
      record(event.team, event.out, event.outPlayer).subbedOff += 1;
      record(event.team, event.in, event.inPlayer).subbedOn += 1;
    }
  }

  return teams.map(team => [...team.values()].sort((a, b) =>
    a.number - b.number ||
    String(a.name ?? "").localeCompare(String(b.name ?? ""))
  ));
}
