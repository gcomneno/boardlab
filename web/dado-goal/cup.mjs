const VALID_SIZES = new Set([4, 8, 16, 32]);

export function resolveCupTie({
  fixtures,
  matches,
  penalties = null,
} = {}) {
  if (!Array.isArray(fixtures) ||
      ![1, 2].includes(fixtures.length) ||
      !Array.isArray(matches)) {
    throw new TypeError("invalid cup tie");
  }

  const first = fixtures[0];
  const homeId = String(first.homeProviderTeamId);
  const awayId = String(first.awayProviderTeamId);

  if (!homeId || !awayId || homeId === awayId ||
      fixtures.some(fixture =>
        fixture.round !== first.round ||
        fixture.pair !== first.pair
      )) {
    throw new TypeError("invalid cup pairing");
  }

  if (fixtures.length === 2 &&
      (String(fixtures[1].homeProviderTeamId) !== awayId ||
       String(fixtures[1].awayProviderTeamId) !== homeId)) {
    throw new TypeError("invalid return leg");
  }

  const records = new Map();
  for (const match of matches) {
    if (!match || typeof match.matchId !== "string" ||
        records.has(match.matchId)) {
      throw new TypeError("invalid or duplicate match record");
    }
    records.set(match.matchId, match);
  }

  let homeGoals = 0;
  let awayGoals = 0;

  for (const fixture of fixtures) {
    if (typeof fixture.matchId !== "string" ||
        !fixture.matchId.trim()) {
      throw new TypeError("cup fixture is incomplete");
    }

    const match = records.get(fixture.matchId);
    if (!match ||
        String(match.home?.providerTeamId) !==
          String(fixture.homeProviderTeamId) ||
        String(match.away?.providerTeamId) !==
          String(fixture.awayProviderTeamId)) {
      throw new TypeError("canonical cup match mismatch");
    }

    const h = match.score?.home;
    const a = match.score?.away;

    if (!Number.isSafeInteger(h) ||
        !Number.isSafeInteger(a) || h < 0 || a < 0) {
      throw new TypeError("invalid cup score");
    }

    if (String(fixture.homeProviderTeamId) === homeId) {
      homeGoals += h;
      awayGoals += a;
    } else {
      homeGoals += a;
      awayGoals += h;
    }
  }

  if (!Number.isSafeInteger(homeGoals) ||
      !Number.isSafeInteger(awayGoals)) {
    throw new TypeError("cup aggregate overflow");
  }

  if (homeGoals !== awayGoals) {
    if (penalties !== null) {
      throw new TypeError("penalties not allowed without a tie");
    }
    return {
      homeGoals,
      awayGoals,
      winnerProviderTeamId: homeGoals > awayGoals ? homeId : awayId,
      decidedBy: "goals",
    };
  }

  if (penalties === null) {
    return {
      homeGoals,
      awayGoals,
      winnerProviderTeamId: null,
      decidedBy: "pending-penalties",
    };
  }

  const pHome = penalties.home;
  const pAway = penalties.away;

  if (!Number.isSafeInteger(pHome) ||
      !Number.isSafeInteger(pAway) ||
      pHome < 0 || pAway < 0 || pHome === pAway) {
    throw new TypeError("invalid penalty shootout");
  }

  return {
    homeGoals,
    awayGoals,
    winnerProviderTeamId: pHome > pAway ? homeId : awayId,
    decidedBy: "penalties",
    penalties: { home: pHome, away: pAway },
  };
}


export function createCup({
  cupId,
  participants,
  legs = 1,
} = {}) {
  if (typeof cupId !== "string" || !cupId.trim()) {
    throw new TypeError("cupId is required");
  }

  if (!VALID_SIZES.has(participants?.length)) {
    throw new TypeError("cup requires 4, 8, 16 or 32 teams");
  }

  if (legs !== 1 && legs !== 2) {
    throw new TypeError("cup legs must be 1 or 2");
  }

  const teams = structuredClone(participants);
  const ids = teams.map(team => {
    if (!team || typeof team !== "object" ||
        Array.isArray(team) ||
        team.providerTeamId === null ||
        team.providerTeamId === undefined ||
        !String(team.providerTeamId).trim()) {
      throw new TypeError("invalid cup participant");
    }
    return String(team.providerTeamId);
  });

  if (new Set(ids).size !== ids.length) {
    throw new TypeError("duplicate cup participant");
  }

  const fixtures = [];

  for (let pair = 0; pair < teams.length / 2; pair += 1) {
    const homeId = ids[pair * 2];
    const awayId = ids[pair * 2 + 1];

    fixtures.push({
      fixtureId: `r1-p${pair + 1}-l1`,
      round: 1,
      pair: pair + 1,
      leg: 1,
      homeProviderTeamId: homeId,
      awayProviderTeamId: awayId,
      matchId: null,
    });

    if (legs === 2) {
      fixtures.push({
        fixtureId: `r1-p${pair + 1}-l2`,
        round: 1,
        pair: pair + 1,
        leg: 2,
        homeProviderTeamId: awayId,
        awayProviderTeamId: homeId,
        matchId: null,
      });
    }
  }

  return {
    cupId,
    legs,
    participants: teams,
    fixtures,
  };
}

export function advanceCup(cup, matches, shootouts = {}) {
  if (!cup || ![1, 2].includes(cup.legs) ||
      ![4, 8, 16, 32].includes(cup.participants?.length) ||
      !Array.isArray(cup.fixtures) ||
      !Array.isArray(matches) ||
      !shootouts || typeof shootouts !== "object" ||
      Array.isArray(shootouts)) {
    throw new TypeError("invalid cup advancement input");
  }

  const next = structuredClone(cup);
  if (next.championProviderTeamId !== undefined) {
    throw new TypeError("cup is already completed");
  }

  const round = Math.max(...next.fixtures.map(f => f.round));
  if (!Number.isSafeInteger(round) || round < 1) {
    throw new TypeError("invalid cup round");
  }

  const current = next.fixtures.filter(f => f.round === round);
  const expectedPairs = next.participants.length / (2 ** round);

  if (!Number.isInteger(expectedPairs) || expectedPairs < 1 ||
      current.length !== expectedPairs * next.legs) {
    throw new TypeError("invalid cup bracket");
  }

  const winners = [];

  for (let pair = 1; pair <= expectedPairs; pair += 1) {
    const fixtures = current.filter(f => f.pair === pair)
      .sort((a, b) => a.leg - b.leg);

    if (fixtures.length !== next.legs ||
        fixtures.some((f, i) =>
          f.leg !== i + 1 ||
          f.fixtureId !== `r${round}-p${pair}-l${i + 1}`
        )) {
      throw new TypeError("invalid cup tie fixtures");
    }

    const key = `r${round}-p${pair}`;
    const result = resolveCupTie({
      fixtures,
      matches,
      penalties: Object.hasOwn(shootouts, key)
        ? shootouts[key]
        : null,
    });

    if (result.winnerProviderTeamId === null) {
      throw new TypeError(`unresolved cup tie: ${key}`);
    }

    winners.push(result.winnerProviderTeamId);
  }

  if (new Set(winners).size !== winners.length) {
    throw new TypeError("duplicate cup winners");
  }

  if (winners.length === 1) {
    next.championProviderTeamId = winners[0];
    return next;
  }

  const nextRound = round + 1;
  for (let i = 0; i < winners.length; i += 2) {
    const homeId = winners[i];
    const awayId = winners[i + 1];
    const pair = i / 2 + 1;

    next.fixtures.push({
      fixtureId: `r${nextRound}-p${pair}-l1`,
      round: nextRound,
      pair,
      leg: 1,
      homeProviderTeamId: homeId,
      awayProviderTeamId: awayId,
      matchId: null,
    });

    if (next.legs === 2) {
      next.fixtures.push({
        fixtureId: `r${nextRound}-p${pair}-l2`,
        round: nextRound,
        pair,
        leg: 2,
        homeProviderTeamId: awayId,
        awayProviderTeamId: homeId,
        matchId: null,
      });
    }
  }

  return next;
}

export function advanceCupIfReady(cup, matches, shootouts = {}) {
  if (!cup || !Array.isArray(cup.fixtures) ||
      !Array.isArray(matches) ||
      !shootouts || typeof shootouts !== "object" ||
      Array.isArray(shootouts)) {
    throw new TypeError("invalid cup readiness input");
  }

  if (cup.championProviderTeamId !== undefined) {
    return {
      status: "completed",
      cup: structuredClone(cup),
    };
  }

  const round = Math.max(...cup.fixtures.map(f => f.round));
  const current = cup.fixtures.filter(f => f.round === round);

  if (current.some(f => f.matchId === null)) {
    return {
      status: "pending-matches",
      cup: structuredClone(cup),
    };
  }

  for (const pair of new Set(current.map(f => f.pair))) {
    const fixtures = current.filter(f => f.pair === pair)
      .sort((a, b) => a.leg - b.leg);
    const key = `r${round}-p${pair}`;
    const result = resolveCupTie({
      fixtures,
      matches,
      penalties: Object.hasOwn(shootouts, key)
        ? shootouts[key]
        : null,
    });

    if (result.decidedBy === "pending-penalties") {
      return {
        status: "pending-penalties",
        round,
        pair,
        cup: structuredClone(cup),
      };
    }
  }

  return {
    status: "advanced",
    cup: advanceCup(cup, matches, shootouts),
  };
}

export function completeCupFixture(cup, fixtureId, matchId) {
  if (!cup || !Array.isArray(cup.fixtures)) {
    throw new TypeError("invalid cup");
  }

  if (typeof fixtureId !== "string" || !fixtureId.trim()) {
    throw new TypeError("fixtureId is required");
  }

  if (typeof matchId !== "string" || !matchId.trim()) {
    throw new TypeError("matchId is required");
  }

  if (cup.championProviderTeamId !== undefined) {
    throw new TypeError("cup is already completed");
  }

  const matches = cup.fixtures.filter(
    fixture => fixture.fixtureId === fixtureId
  );

  if (matches.length !== 1) {
    throw new TypeError("unknown or duplicate cup fixture");
  }

  if (matches[0].matchId !== null) {
    throw new TypeError("cup fixture is already completed");
  }

  if (cup.fixtures.some(fixture => fixture.matchId === matchId)) {
    throw new TypeError("match is already associated with a cup fixture");
  }

  const next = structuredClone(cup);
  const fixture = next.fixtures.find(
    item => item.fixtureId === fixtureId
  );
  fixture.matchId = matchId;
  return next;
}

export function validateCupProgression(cup, matches, shootouts = {}) {
  if (!cup || !Array.isArray(cup.fixtures) ||
      !Array.isArray(matches) ||
      !shootouts || typeof shootouts !== "object" ||
      Array.isArray(shootouts)) {
    throw new TypeError("invalid cup progression input");
  }

  let reconstructed = createCup({
    cupId: cup.cupId,
    participants: cup.participants,
    legs: cup.legs,
  });

  const fixtureCount = cup.fixtures.length;
  const seenMatches = new Set();
  let index = 0;

  while (index < fixtureCount) {
    const current = reconstructed.fixtures;
    const pending = current.slice(index);

    if (pending.length === 0) {
      throw new TypeError("cup has extra fixtures");
    }

    for (const expected of pending) {
      const actual = cup.fixtures[index];

      if (!actual ||
          actual.fixtureId !== expected.fixtureId ||
          actual.round !== expected.round ||
          actual.pair !== expected.pair ||
          actual.leg !== expected.leg ||
          actual.homeProviderTeamId !== expected.homeProviderTeamId ||
          actual.awayProviderTeamId !== expected.awayProviderTeamId) {
        throw new TypeError("non-canonical cup bracket");
      }

      if (actual.matchId !== null) {
        if (typeof actual.matchId !== "string" ||
            !actual.matchId.trim() ||
            seenMatches.has(actual.matchId)) {
          throw new TypeError("invalid cup match association");
        }
        const canonicalMatches = matches.filter(
          match => match?.matchId === actual.matchId
        );
        if (canonicalMatches.length !== 1) {
          throw new TypeError("missing or ambiguous canonical cup match");
        }

        const canonical = canonicalMatches[0];
        if (String(canonical.home?.providerTeamId) !==
              String(expected.homeProviderTeamId) ||
            String(canonical.away?.providerTeamId) !==
              String(expected.awayProviderTeamId) ||
            !canonical.score ||
            !Number.isSafeInteger(canonical.score.home) ||
            !Number.isSafeInteger(canonical.score.away) ||
            canonical.score.home < 0 ||
            canonical.score.away < 0) {
          throw new TypeError("canonical cup match does not match fixture");
        }

        seenMatches.add(actual.matchId);
        reconstructed = completeCupFixture(
          reconstructed,
          expected.fixtureId,
          actual.matchId,
        );
      }

      index += 1;
    }

    if (index === fixtureCount) break;

    reconstructed = advanceCup(reconstructed, matches, shootouts);
  }

  if (cup.championProviderTeamId !== undefined) {
    reconstructed = advanceCup(reconstructed, matches, shootouts);
    if (reconstructed.championProviderTeamId !==
        cup.championProviderTeamId) {
      throw new TypeError("non-canonical cup champion");
    }
  }

  if (reconstructed.fixtures.length !== cup.fixtures.length ||
      reconstructed.championProviderTeamId !==
        cup.championProviderTeamId) {
    throw new TypeError("incomplete cup reconstruction");
  }

  return structuredClone(cup);
}
