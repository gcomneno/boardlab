import { resolveCupTie } from "./cup.mjs";

export function cupFixturePresentation(cup, fixture, matches) {
  if (!cup || !fixture || !Array.isArray(matches)) {
    throw new TypeError("invalid cup presentation input");
  }

  const record = fixture.matchId === null
    ? null
    : matches.find(match => match.matchId === fixture.matchId);

  if (fixture.matchId !== null && !record) {
    throw new TypeError("missing canonical cup match");
  }

  const pairFixtures = cup.fixtures.filter(item =>
    item.round === fixture.round && item.pair === fixture.pair
  ).sort((a, b) => a.leg - b.leg);

  const pairCompleted = pairFixtures.length === cup.legs &&
    pairFixtures.every(item => item.matchId !== null);

  let tie = null;
  if (pairCompleted) {
    tie = resolveCupTie({
      fixtures: pairFixtures,
      matches,
      penalties: cup.shootouts?.[`r${fixture.round}-p${fixture.pair}`] ?? null,
    });
  }

  return {
    status: record === null ? "pending" :
      tie?.decidedBy === "pending-penalties" ? "penalties" : "completed",
    score: record ? `${record.score.home}–${record.score.away}` : "–",
    winnerProviderTeamId: tie?.winnerProviderTeamId ?? null,
    aggregate: pairCompleted && cup.legs === 2
      ? `${tie.homeGoals}–${tie.awayGoals}`
      : null,
    decidedBy: tie?.decidedBy ?? null,
  };
}
