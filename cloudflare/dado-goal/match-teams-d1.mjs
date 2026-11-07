function requireSelection(selection) {
  if (
    !selection ||
    typeof selection !== "object" ||
    Array.isArray(selection)
  ) {
    throw new TypeError("Invalid team selection");
  }

  const fields = [
    "provider",
    "competitionId",
    "season",
    "providerTeamId",
  ];

  for (const field of fields) {
    if (
      typeof selection[field] !== "string" ||
      selection[field].length === 0
    ) {
      throw new TypeError("Invalid team selection");
    }
  }

  return selection;
}

export async function putMatchTeams(db, matchId, selections) {
  if (
    !db ||
    typeof db.prepare !== "function" ||
    typeof db.batch !== "function" ||
    typeof matchId !== "string" ||
    matchId.length === 0 ||
    !Array.isArray(selections) ||
    selections.length !== 2
  ) {
    throw new TypeError("Invalid match teams input");
  }

  const teams = selections.map(requireSelection);

  const results = await db.batch(
    teams.map((team, seat) => db.prepare(`
      INSERT INTO lifecycle_match_teams (
        match_id,
        seat,
        provider,
        competition_id,
        season,
        provider_team_id
      )
      VALUES (?, ?, ?, ?, ?, ?)
    `).bind(
      matchId,
      seat,
      team.provider,
      team.competitionId,
      team.season,
      team.providerTeamId,
    )),
  );

  if (
    !Array.isArray(results) ||
    results.length !== 2 ||
    results.some(result =>
      result.success !== true ||
      result.meta?.changes !== 1
    )
  ) {
    throw new Error("Invalid match teams persistence result");
  }

  return teams;
}

export async function getMatchTeams(db, matchId) {
  if (
    !db ||
    typeof db.prepare !== "function" ||
    typeof matchId !== "string" ||
    matchId.length === 0
  ) {
    throw new TypeError("Invalid match teams lookup");
  }

  const result = await db.prepare(`
    SELECT
      seat,
      provider,
      competition_id,
      season,
      provider_team_id
    FROM lifecycle_match_teams
    WHERE match_id = ?
    ORDER BY seat
  `).bind(matchId).all();

  const rows = result.results ?? [];

  if (rows.length === 0) return null;

  if (
    rows.length !== 2 ||
    rows[0].seat !== 0 ||
    rows[1].seat !== 1
  ) {
    throw new Error("Invalid persisted match teams");
  }

  return rows.map(row => ({
    provider: row.provider,
    competitionId: row.competition_id,
    season: row.season,
    providerTeamId: row.provider_team_id,
  }));
}
