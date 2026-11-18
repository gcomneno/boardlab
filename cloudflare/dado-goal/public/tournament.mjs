function canonicalTeamId(value) {
  if (value === null || value === undefined || String(value).trim() === "") {
    throw new TypeError("team id is required");
  }
  return String(value);
}

function validateParticipants(participants) {
  if (!Array.isArray(participants) ||
      participants.length < 4 ||
      participants.length % 2 !== 0) {
    throw new TypeError("championship requires an even number of at least 4 teams");
  }

  const snapshots = structuredClone(participants);
  const ids = snapshots.map(team => {
    if (!team || typeof team !== "object" || Array.isArray(team)) {
      throw new TypeError("invalid championship participant");
    }
    return canonicalTeamId(team.providerTeamId);
  });

  if (new Set(ids).size !== ids.length) {
    throw new TypeError("duplicate championship participant");
  }

  return snapshots.map(team => Object.freeze(team));
}

export function generateChampionshipFixtures(participants) {
  const teams = validateParticipants(participants);
  const count = teams.length;
  const rotation = Array.from({ length: count }, (_, index) => index);
  const firstLeg = [];

  for (let round = 0; round < count - 1; round += 1) {
    for (let pair = 0; pair < count / 2; pair += 1) {
      const left = rotation[pair];
      const right = rotation[count - 1 - pair];

      // Alternating the fixed participant's side avoids a permanent
      // home/away bias while remaining deterministic.
      const swap = pair === 0 && round % 2 === 1;
      const homeIndex = swap ? right : left;
      const awayIndex = swap ? left : right;

      firstLeg.push({
        fixtureId: `md${round + 1}-f${pair + 1}`,
        matchday: round + 1,
        homeProviderTeamId: String(teams[homeIndex].providerTeamId),
        awayProviderTeamId: String(teams[awayIndex].providerTeamId),
        matchId: null,
      });
    }

    rotation.splice(1, 0, rotation.pop());
  }

  const secondLeg = firstLeg.map((fixture, index) => ({
    fixtureId: `md${fixture.matchday + count - 1}-f${(index % (count / 2)) + 1}`,
    matchday: fixture.matchday + count - 1,
    homeProviderTeamId: fixture.awayProviderTeamId,
    awayProviderTeamId: fixture.homeProviderTeamId,
    matchId: null,
  }));

  return [...firstLeg, ...secondLeg];
}

export function createChampionship({
  championshipId,
  participants,
} = {}) {
  if (typeof championshipId !== "string" || championshipId.trim() === "") {
    throw new TypeError("championshipId is required");
  }

  const participantSnapshots = validateParticipants(participants);

  return {
    championshipId,
    participants: participantSnapshots,
    fixtures: generateChampionshipFixtures(participantSnapshots),
  };
}

export function completeFixture(championship, fixtureId, matchId) {
  if (!championship || !Array.isArray(championship.fixtures)) {
    throw new TypeError("invalid championship");
  }

  if (typeof fixtureId !== "string" || fixtureId === "") {
    throw new TypeError("fixtureId is required");
  }

  if (typeof matchId !== "string" || matchId === "") {
    throw new TypeError("matchId is required");
  }

  const index = championship.fixtures.findIndex(
    fixture => fixture.fixtureId === fixtureId,
  );

  if (index < 0) {
    throw new TypeError("unknown championship fixture");
  }

  if (championship.fixtures[index].matchId !== null) {
    throw new TypeError("championship fixture is already completed");
  }

  if (championship.fixtures.some(fixture => fixture.matchId === matchId)) {
    throw new TypeError("match is already associated with a fixture");
  }

  const next = structuredClone(championship);
  next.fixtures[index].matchId = matchId;
  return next;
}

export function championshipStandings(championship, matchRecords) {
  if (!championship ||
      !Array.isArray(championship.participants) ||
      !Array.isArray(championship.fixtures) ||
      !Array.isArray(matchRecords)) {
    throw new TypeError("invalid championship standings input");
  }

  const participantOrder = new Map();
  const rows = new Map();

  championship.participants.forEach((team, index) => {
    const id = canonicalTeamId(team.providerTeamId);
    participantOrder.set(id, index);
    rows.set(id, {
      position: 0,
      team: structuredClone(team),
      played: 0,
      won: 0,
      drawn: 0,
      lost: 0,
      goalsFor: 0,
      goalsAgainst: 0,
      goalDifference: 0,
      points: 0,
    });
  });

  const records = new Map(
    matchRecords.map(record => [record.matchId, record]),
  );

  for (const fixture of championship.fixtures) {
    if (fixture.matchId === null) continue;

    const match = records.get(fixture.matchId);
    if (!match) continue;

    const homeId = canonicalTeamId(fixture.homeProviderTeamId);
    const awayId = canonicalTeamId(fixture.awayProviderTeamId);
    const home = rows.get(homeId);
    const away = rows.get(awayId);

    if (!home || !away) {
      throw new TypeError("fixture references a non-participant");
    }

    if (String(match.home?.providerTeamId) !== homeId ||
        String(match.away?.providerTeamId) !== awayId) {
      throw new TypeError("canonical match does not match fixture");
    }

    const homeGoals = match.score?.home;
    const awayGoals = match.score?.away;

    if (!Number.isSafeInteger(homeGoals) ||
        !Number.isSafeInteger(awayGoals) ||
        homeGoals < 0 ||
        awayGoals < 0) {
      throw new TypeError("invalid canonical match score");
    }

    home.played += 1;
    away.played += 1;
    home.goalsFor += homeGoals;
    home.goalsAgainst += awayGoals;
    away.goalsFor += awayGoals;
    away.goalsAgainst += homeGoals;

    if (homeGoals > awayGoals) {
      home.won += 1;
      away.lost += 1;
      home.points += 3;
    } else if (homeGoals < awayGoals) {
      away.won += 1;
      home.lost += 1;
      away.points += 3;
    } else {
      home.drawn += 1;
      away.drawn += 1;
      home.points += 1;
      away.points += 1;
    }
  }

  const standings = [...rows.values()];
  for (const row of standings) {
    row.goalDifference = row.goalsFor - row.goalsAgainst;
  }

  standings.sort((a, b) =>
    b.points - a.points ||
    b.goalDifference - a.goalDifference ||
    b.goalsFor - a.goalsFor ||
    participantOrder.get(String(a.team.providerTeamId)) -
      participantOrder.get(String(b.team.providerTeamId))
  );

  standings.forEach((row, index) => {
    row.position = index + 1;
  });

  return standings;
}
