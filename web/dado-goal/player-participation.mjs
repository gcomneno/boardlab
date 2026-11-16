/**
 * Canonical Manager per-match participation ledger.
 *
 * Participation is derived only from:
 * - the Manager lineup/bench selected for the match;
 * - canonical structured match events carrying minute and player identity.
 *
 * No commentary, UI text or localized strings are parsed.
 */

function minute(value) {
  if (!Number.isInteger(value) || value < 0 || value > 90) {
    throw new RangeError("participation event requires minute 0..90");
  }
  return value;
}

export function managerMatchParticipation(club, team, events) {
  if (!club || !Array.isArray(club.players) ||
      !Array.isArray(club.lineup) ||
      !Array.isArray(club.bench)) {
    throw new TypeError("Manager club is required");
  }

  if (team !== 0 && team !== 1) {
    throw new RangeError("managed team seat must be 0 or 1");
  }

  if (!Array.isArray(events)) {
    throw new TypeError("match events must be an array");
  }

  const players = new Map(
    club.players.map(player => [
      String(player.providerPlayerId),
      player,
    ]),
  );

  const starters = new Set(club.lineup.map(String));
  const bench = new Set(club.bench.map(String));
  const orderedSelected = [
    ...club.lineup.map(String),
    ...club.bench.map(String),
  ];

  const records = new Map();

  for (const player of club.players) {
    const id = String(player.providerPlayerId);
    const selection = starters.has(id)
      ? "starter"
      : bench.has(id)
        ? "bench"
        : "not-selected";

    records.set(id, {
      providerPlayerId: id,
      name: player.name,
      shirtNumber: player.shirtNumber ?? null,
      selection,
      enteredMinute: selection === "starter" ? 0 : null,
      exitedMinute: null,
      exitReason: null,
      minutesPlayed: 0,
    });
  }

  const idFromIdentityOrNumber = (identity, number) => {
    if (identity?.providerPlayerId != null) {
      return String(identity.providerPlayerId);
    }

    if (Number.isInteger(number) &&
        number >= 1 &&
        number <= orderedSelected.length) {
      return orderedSelected[number - 1];
    }

    return null;
  };

  const recordFor = (identity, number) => {
    const id = idFromIdentityOrNumber(identity, number);
    return id === null ? null : records.get(id) ?? null;
  };

  for (const event of events) {
    if (!event || event.team !== team) continue;

    if (event.type === "substitution") {
      const at = minute(event.minute);

      const outgoing = recordFor(event.outPlayer, event.out);
      if (outgoing && outgoing.enteredMinute !== null &&
          outgoing.exitedMinute === null) {
        outgoing.exitedMinute = at;
        outgoing.exitReason =
          event.reason === "injury" ? "injury" : "substitution";
      }

      const incoming = recordFor(event.inPlayer, event.in);
      if (incoming && incoming.enteredMinute === null) {
        incoming.enteredMinute = at;
      }

      continue;
    }

    if (event.type === "red") {
      const at = minute(event.minute);
      const player = recordFor(event.player, event.number);

      if (player && player.enteredMinute !== null &&
          player.exitedMinute === null) {
        player.exitedMinute = at;
        player.exitReason = "red";
      }
    }
  }

  for (const record of records.values()) {
    if (record.enteredMinute !== null) {
      if (record.exitedMinute === null) {
        record.exitedMinute = 90;
      }

      record.minutesPlayed = Math.max(
        0,
        record.exitedMinute - record.enteredMinute,
      );
    }
  }

  return [...records.values()];
}
