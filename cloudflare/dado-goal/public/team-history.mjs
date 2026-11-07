export const HISTORY_SCHEMA_VERSION = 1;

function snapshot(value) {
  return structuredClone(value);
}

export function createMatchRecord({
  matchId,
  playedAt,
  home,
  away,
  homeGoals,
  awayGoals,
  events = [],
  statistics = {},
  ratings = [],
}) {
  if (typeof matchId !== "string" || matchId.length === 0) {
    throw new TypeError("matchId is required");
  }

  if (typeof playedAt !== "string" || playedAt.length === 0) {
    throw new TypeError("playedAt is required");
  }

  for (const [name, team] of [["home", home], ["away", away]]) {
    if (!team || typeof team.name !== "string" || team.name.length === 0) {
      throw new TypeError(`${name} team is required`);
    }
  }

  for (const goals of [homeGoals, awayGoals]) {
    if (!Number.isSafeInteger(goals) || goals < 0) {
      throw new TypeError("goals must be non-negative integers");
    }
  }

  if (!Array.isArray(events) || !Array.isArray(ratings)) {
    throw new TypeError("events and ratings must be arrays");
  }

  return Object.freeze({
    schemaVersion: HISTORY_SCHEMA_VERSION,
    matchId,
    playedAt,
    home: Object.freeze(snapshot(home)),
    away: Object.freeze(snapshot(away)),
    score: Object.freeze({
      home: homeGoals,
      away: awayGoals,
    }),
    events: Object.freeze(snapshot(events)),
    statistics: Object.freeze(snapshot(statistics)),
    ratings: Object.freeze(snapshot(ratings)),
  });
}

export function teamStatistics(records, teamId) {
  if (!Array.isArray(records)) {
    throw new TypeError("records must be an array");
  }

  const stats = {
    played: 0,
    won: 0,
    drawn: 0,
    lost: 0,
    goalsFor: 0,
    goalsAgainst: 0,
  };

  for (const match of records) {
    const home = match.home.providerTeamId === teamId;
    const away = match.away.providerTeamId === teamId;

    if (!home && !away) continue;

    const goalsFor = home ? match.score.home : match.score.away;
    const goalsAgainst = home ? match.score.away : match.score.home;

    stats.played += 1;
    stats.goalsFor += goalsFor;
    stats.goalsAgainst += goalsAgainst;

    if (goalsFor > goalsAgainst) stats.won += 1;
    else if (goalsFor < goalsAgainst) stats.lost += 1;
    else stats.drawn += 1;
  }

  return stats;
}

export function exportHistoryJson(records) {
  if (!Array.isArray(records)) {
    throw new TypeError("records must be an array");
  }

  return JSON.stringify({
    schemaVersion: HISTORY_SCHEMA_VERSION,
    matches: records,
  }, null, 2);
}

export function importHistoryJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("history JSON must be a string");
  }

  let data;
  try {
    data = JSON.parse(source);
  } catch {
    throw new TypeError("invalid history JSON");
  }

  if (data?.schemaVersion !== HISTORY_SCHEMA_VERSION ||
      !Array.isArray(data.matches)) {
    throw new TypeError("unsupported history schema");
  }

  return data.matches.map((match) => createMatchRecord({
    matchId: match.matchId,
    playedAt: match.playedAt,
    home: match.home,
    away: match.away,
    homeGoals: match.score?.home,
    awayGoals: match.score?.away,
    events: match.events,
    statistics: match.statistics,
    ratings: match.ratings,
  }));
}
