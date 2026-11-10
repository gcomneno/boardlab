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

  const lineup = players.slice(0, 11).map(player => player.providerPlayerId);
  const bench = players.slice(11, 20).map(player => player.providerPlayerId);

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
