function requirePlayerIds(values, label) {
  if (!Array.isArray(values)) {
    throw new TypeError(`${label} must be an array`);
  }

  const ids = values.map(String);
  if (ids.some(id => id.length === 0) || new Set(ids).size !== ids.length) {
    throw new TypeError(`${label} contains invalid or duplicate player ids`);
  }

  return ids;
}

export function createManagerClub(team) {
  if (!team || !Array.isArray(team.players) || team.players.length < 11) {
    throw new TypeError("catalogue team with at least 11 players is required");
  }

  const players = team.players.map(player => ({
    provider: player.provider,
    providerPlayerId: String(player.providerPlayerId),
    name: player.name,
    position: player.position,
    shirtNumber: player.shirtNumber ?? null,
  }));

  const formation = [
    ["Goalkeeper", 1],
    ["Defender", 4],
    ["Midfielder", 4],
    ["Attacker", 2],
  ];

  const lineupPlayers = formation.flatMap(([position, count]) =>
    players.filter(player => player.position === position).slice(0, count)
  );

  if (lineupPlayers.length !== 11) {
    throw new RangeError(
      "catalogue team cannot provide the default 1-4-4-2 manager lineup"
    );
  }

  const lineupIds = new Set(
    lineupPlayers.map(player => player.providerPlayerId)
  );
  const benchPlayers = players
    .filter(player => !lineupIds.has(player.providerPlayerId))
    .slice(0, 9);

  const lineup = lineupPlayers.map(player => player.providerPlayerId);
  const bench = benchPlayers.map(player => player.providerPlayerId);

  return {
    provider: team.provider,
    providerTeamId: String(team.providerTeamId),
    name: team.name,
    players,
    lineup,
    bench,
  };
}

export function validateManagerClub(club) {
  if (!club || !Array.isArray(club.players) || club.players.length < 11) {
    throw new TypeError("invalid manager club");
  }

  const playerIds = new Set(
    club.players.map(player => String(player.providerPlayerId)),
  );
  const lineup = requirePlayerIds(club.lineup, "lineup");
  const bench = requirePlayerIds(club.bench, "bench");

  if (lineup.length !== 11) {
    throw new RangeError("manager lineup must contain exactly 11 players");
  }
  if (bench.length > 9) {
    throw new RangeError("manager bench cannot contain more than 9 players");
  }

  const selected = [...lineup, ...bench];
  if (new Set(selected).size !== selected.length) {
    throw new TypeError("lineup and bench must not overlap");
  }
  if (selected.some(id => !playerIds.has(id))) {
    throw new TypeError("manager selection references unknown player");
  }

  return club;
}

export function updateManagerSelection(club, { lineup, bench }) {
  const next = structuredClone(club);
  next.lineup = requirePlayerIds(lineup, "lineup");
  next.bench = requirePlayerIds(bench, "bench");
  validateManagerClub(next);
  return next;
}

export function managerClubMatchTeam(club) {
  validateManagerClub(club);

  const byId = new Map(
    club.players.map(player => [String(player.providerPlayerId), player]),
  );

  const orderedIds = [...club.lineup, ...club.bench];
  const players = orderedIds.map(id => structuredClone(byId.get(String(id))));

  return {
    provider: club.provider,
    providerTeamId: club.providerTeamId,
    name: club.name,
    players,
  };
}

const MANAGER_PLAYER_POSITIONS = Object.freeze([
  "Goalkeeper",
  "Defender",
  "Midfielder",
  "Attacker",
]);

export function updateManagerPlayers(club, edits) {
  validateManagerClub(club);

  if (!Array.isArray(edits)) {
    throw new TypeError("manager player edits must be an array");
  }

  const editsById = new Map();

  for (const edit of edits) {
    const playerId = String(edit?.providerPlayerId ?? "");
    const name = String(edit?.name ?? "").trim();
    const position = String(edit?.position ?? "");
    const rawShirtNumber = edit?.shirtNumber;
    const shirtNumber = rawShirtNumber === "" || rawShirtNumber == null
      ? null
      : Number(rawShirtNumber);

    if (!playerId || editsById.has(playerId)) {
      throw new TypeError("invalid or duplicate manager player edit");
    }
    if (!name) {
      throw new TypeError("manager player name is required");
    }
    if (!MANAGER_PLAYER_POSITIONS.includes(position)) {
      throw new TypeError("invalid manager player position");
    }
    if (shirtNumber !== null &&
        (!Number.isSafeInteger(shirtNumber) ||
         shirtNumber < 1 ||
         shirtNumber > 99)) {
      throw new RangeError(
        "manager player shirt number must be between 1 and 99"
      );
    }

    editsById.set(playerId, { name, position, shirtNumber });
  }

  const next = structuredClone(club);

  next.players = next.players.map(player => {
    const edit = editsById.get(String(player.providerPlayerId));
    return edit ? { ...player, ...edit } : player;
  });

  if ([...editsById.keys()].some(
    id => !next.players.some(
      player => String(player.providerPlayerId) === id
    )
  )) {
    throw new TypeError("manager player edit references unknown player");
  }

  validateManagerClub(next);
  return next;
}
