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

function managedMatchSeat(match, providerTeamId) {
  const target = String(providerTeamId);

  if (String(match?.home?.providerTeamId) === target) return 0;
  if (String(match?.away?.providerTeamId) === target) return 1;
  return null;
}

/**
 * Cumulative Manager statistics.
 *
 * Source of truth: canonical persisted match events only.
 * The supplied roster seeds zero-valued rows so the Manager panel keeps the
 * whole managed squad visible even before a player records an event.
 *
 * Deliberately unsupported here:
 * - appearances/minutes played, because completed match records do not yet
 *   persist a canonical per-match lineup/minutes ledger;
 * - shots/passes/saves, because those are team-level roll statistics and do
 *   not carry canonical player identity.
 */
export function managerPlayerStatistics(
  matches,
  providerTeamId,
  roster = [],
) {
  if (!Array.isArray(matches)) {
    throw new TypeError("manager player statistics require match history");
  }
  if (!Array.isArray(roster)) {
    throw new TypeError("manager player statistics require a roster");
  }

  const records = new Map();
  const rosterByNumber = new Map();

  const ensure = ({
    providerPlayerId = null,
    name = null,
    number = null,
  }) => {
    let key = null;

    if (providerPlayerId != null) {
      key = `provider:${providerPlayerId}`;
    } else if (number != null && rosterByNumber.has(String(number))) {
      key = rosterByNumber.get(String(number));
    } else if (number != null) {
      key = `number:${number}`;
    } else {
      return null;
    }

    if (!records.has(key)) {
      records.set(key, {
        providerPlayerId,
        name,
        number,
        goals: 0,
        penalties: 0,
        yellowCards: 0,
        redCards: 0,
        subbedOn: 0,
        subbedOff: 0,
        injurySubbedOff: 0,
        appearances: 0,
        minutesPlayed: 0,
        starts: 0,
        benchUnused: 0,
        notSelected: 0,
      });
    }

    const current = records.get(key);

    if (current.providerPlayerId == null && providerPlayerId != null) {
      current.providerPlayerId = providerPlayerId;
    }
    if (!current.name && name) current.name = name;
    if (current.number == null && number != null) current.number = number;

    return current;
  };

  for (const player of roster) {
    const record = ensure({
      providerPlayerId: player.providerPlayerId ?? null,
      name: player.name ?? null,
      number: player.shirtNumber ?? null,
    });

    if (record && player.shirtNumber != null) {
      const key = record.providerPlayerId != null
        ? `provider:${record.providerPlayerId}`
        : `number:${record.number}`;
      rosterByNumber.set(String(player.shirtNumber), key);
    }
  }

  for (const match of matches) {
    const seat = managedMatchSeat(match, providerTeamId);
    if (seat === null) continue;

    const participation = Array.isArray(match.participation)
      ? match.participation
      : [];

    for (const entry of participation) {
      const record = ensure({
        providerPlayerId: entry.providerPlayerId ?? null,
        name: entry.name ?? null,
        number: entry.shirtNumber ?? null,
      });

      if (!record) continue;

      if (entry.enteredMinute !== null) {
        record.appearances += 1;
      }

      record.minutesPlayed += Number.isInteger(entry.minutesPlayed)
        ? entry.minutesPlayed
        : 0;

      if (entry.selection === "starter") {
        record.starts += 1;
      } else if (
        entry.selection === "bench" &&
        entry.enteredMinute === null
      ) {
        record.benchUnused += 1;
      } else if (entry.selection === "not-selected") {
        record.notSelected += 1;
      }
    }

    const events = Array.isArray(match.events) ? match.events : [];
    const perMatch = playerStatistics(events)[seat];

    for (const player of perMatch) {
      const record = ensure({
        providerPlayerId: player.providerPlayerId,
        name: player.name,
        number: player.number,
      });

      if (!record) continue;

      record.goals += player.goals;
      record.penalties += player.penalties;
      record.yellowCards += player.yellowCards;
      record.redCards += player.redCards;
      record.subbedOn += player.subbedOn;
      record.subbedOff += player.subbedOff;
    }

    for (const event of events) {
      if (
        event?.type !== "substitution" ||
        event.team !== seat ||
        event.reason !== "injury"
      ) {
        continue;
      }

      const record = ensure({
        providerPlayerId: event.outPlayer?.providerPlayerId ?? null,
        name: event.outPlayer?.name ?? null,
        number: event.out ?? null,
      });

      if (record) record.injurySubbedOff += 1;
    }
  }

  return [...records.values()].sort((a, b) => {
    const numberA = Number.isInteger(a.number)
      ? a.number
      : Number.MAX_SAFE_INTEGER;
    const numberB = Number.isInteger(b.number)
      ? b.number
      : Number.MAX_SAFE_INTEGER;

    return numberA - numberB ||
      String(a.name ?? "").localeCompare(String(b.name ?? ""));
  });
}
