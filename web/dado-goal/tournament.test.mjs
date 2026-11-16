import assert from "node:assert/strict";
import test from "node:test";

import {
  championshipStandings,
  completeFixture,
  createChampionship,
  generateChampionshipFixtures,
} from "./tournament.mjs";
import { createMatchRecord } from "./team-history.mjs";

const teams = ["A", "B", "C", "D"].map((id, index) => ({
  provider: "test",
  providerTeamId: id,
  name: `Team ${id}`,
  competitionId: "test",
  season: "2026",
  colors: ["#000000"],
  logo: `team-${index}.png`,
}));

test("creates only valid championships", () => {
  assert.throws(
    () => createChampionship({
      championshipId: "x",
      participants: teams.slice(0, 2),
    }),
    /at least 4/,
  );

  assert.throws(
    () => createChampionship({
      championshipId: "x",
      participants: teams.slice(0, 3),
    }),
    /even number/,
  );

  assert.throws(
    () => createChampionship({
      championshipId: "x",
      participants: [...teams.slice(0, 3), teams[0]],
    }),
    /duplicate/,
  );

  assert.equal(
    createChampionship({
      championshipId: "champ-1",
      participants: teams,
    }).participants.length,
    4,
  );
});

test("generates deterministic double round-robin fixtures", () => {
  const first = generateChampionshipFixtures(teams);
  const second = generateChampionshipFixtures(teams);

  assert.deepEqual(first, second);
  assert.equal(first.length, teams.length * (teams.length - 1));

  const matchdays = new Set(first.map(fixture => fixture.matchday));
  assert.equal(matchdays.size, 2 * (teams.length - 1));

  for (const matchday of matchdays) {
    const fixtures = first.filter(fixture => fixture.matchday === matchday);
    const appearances = fixtures.flatMap(fixture => [
      fixture.homeProviderTeamId,
      fixture.awayProviderTeamId,
    ]);
    assert.equal(new Set(appearances).size, teams.length);
  }

  for (let a = 0; a < teams.length; a += 1) {
    for (let b = a + 1; b < teams.length; b += 1) {
      const left = teams[a].providerTeamId;
      const right = teams[b].providerTeamId;
      const meetings = first.filter(fixture =>
        [fixture.homeProviderTeamId, fixture.awayProviderTeamId]
          .includes(left) &&
        [fixture.homeProviderTeamId, fixture.awayProviderTeamId]
          .includes(right)
      );

      assert.equal(meetings.length, 2);
      assert.equal(
        meetings.filter(fixture =>
          fixture.homeProviderTeamId === left &&
          fixture.awayProviderTeamId === right
        ).length,
        1,
      );
      assert.equal(
        meetings.filter(fixture =>
          fixture.homeProviderTeamId === right &&
          fixture.awayProviderTeamId === left
        ).length,
        1,
      );
    }
  }
});

test("associates a canonical match exactly once", () => {
  const championship = createChampionship({
    championshipId: "champ-1",
    participants: teams,
  });

  const completed = completeFixture(
    championship,
    championship.fixtures[0].fixtureId,
    "match-1",
  );

  assert.equal(completed.fixtures[0].matchId, "match-1");
  assert.equal(championship.fixtures[0].matchId, null);

  assert.throws(
    () => completeFixture(
      completed,
      completed.fixtures[0].fixtureId,
      "match-2",
    ),
    /already completed/,
  );

  assert.throws(
    () => completeFixture(
      completed,
      completed.fixtures[1].fixtureId,
      "match-1",
    ),
    /already associated/,
  );
});

test("derives 3/1/0 standings from canonical completed-match records", () => {
  let championship = createChampionship({
    championshipId: "champ-1",
    participants: teams,
  });

  const [first, second] = championship.fixtures;
  championship = completeFixture(championship, first.fixtureId, "match-1");
  championship = completeFixture(championship, second.fixtureId, "match-2");

  const record = (matchId, fixture, homeGoals, awayGoals) =>
    createMatchRecord({
      matchId,
      playedAt: "2026-11-14T12:00:00Z",
      home: teams.find(
        team => team.providerTeamId === fixture.homeProviderTeamId,
      ),
      away: teams.find(
        team => team.providerTeamId === fixture.awayProviderTeamId,
      ),
      homeGoals,
      awayGoals,
    });

  const standings = championshipStandings(championship, [
    record("match-1", first, 2, 0),
    record("match-2", second, 1, 1),
  ]);

  assert.equal(
    standings.reduce((sum, row) => sum + row.played, 0),
    4,
  );
  assert.equal(
    standings.reduce((sum, row) => sum + row.won, 0),
    1,
  );
  assert.equal(
    standings.reduce((sum, row) => sum + row.drawn, 0),
    2,
  );
  assert.equal(
    standings.reduce((sum, row) => sum + row.lost, 0),
    1,
  );
  assert.equal(
    standings.reduce((sum, row) => sum + row.points, 0),
    5,
  );

  for (const row of standings) {
    assert.equal(
      row.goalDifference,
      row.goalsFor - row.goalsAgainst,
    );
  }
});

test("uses participant order as deterministic final ranking fallback", () => {
  const championship = createChampionship({
    championshipId: "champ-1",
    participants: teams,
  });

  assert.deepEqual(
    championshipStandings(championship, [])
      .map(row => row.team.providerTeamId),
    ["A", "B", "C", "D"],
  );
});

test("canonical structured events remain available through tournament results", () => {
  let championship = createChampionship({
    championshipId: "champ-1",
    participants: teams,
  });
  const fixture = championship.fixtures[0];

  championship = completeFixture(
    championship,
    fixture.fixtureId,
    "match-events",
  );

  const events = [
    { type: "substitution", team: 0, reason: "injury" },
    { type: "light-knock", team: 1 },
  ];

  const match = createMatchRecord({
    matchId: "match-events",
    playedAt: "2026-11-14T12:00:00Z",
    home: teams.find(
      team => team.providerTeamId === fixture.homeProviderTeamId,
    ),
    away: teams.find(
      team => team.providerTeamId === fixture.awayProviderTeamId,
    ),
    homeGoals: 0,
    awayGoals: 0,
    events,
  });

  championshipStandings(championship, [match]);
  assert.deepEqual(match.events, events);
});

test("historical standings exclude later matchdays", () => {
  const participants = [
    { providerTeamId: "A", name: "A" },
    { providerTeamId: "B", name: "B" },
  ];
  const championship = {
    participants,
    fixtures: [
      {
        fixtureId: "md1",
        matchday: 1,
        homeProviderTeamId: "A",
        awayProviderTeamId: "B",
        matchId: "m1",
      },
      {
        fixtureId: "md2",
        matchday: 2,
        homeProviderTeamId: "B",
        awayProviderTeamId: "A",
        matchId: "m2",
      },
    ],
  };
  const records = [
    {
      matchId: "m1",
      home: participants[0],
      away: participants[1],
      score: { home: 2, away: 0 },
    },
    {
      matchId: "m2",
      home: participants[1],
      away: participants[0],
      score: { home: 3, away: 0 },
    },
  ];

  const standingsAt = matchday => championshipStandings(
    {
      ...championship,
      fixtures: championship.fixtures.map(fixture => ({
        ...fixture,
        matchId: fixture.matchday <= matchday
          ? fixture.matchId
          : null,
      })),
    },
    records,
  );

  const first = standingsAt(1);
  const second = standingsAt(2);
  const back = standingsAt(1);

  assert.equal(first.find(row => row.team.providerTeamId === "A").points, 3);
  assert.equal(first.find(row => row.team.providerTeamId === "B").points, 0);
  assert.equal(second.find(row => row.team.providerTeamId === "B").points, 3);
  assert.deepEqual(back, first);
  assert.equal(
    championship.fixtures[1].matchId,
    "m2",
    "historical calculation must not mutate canonical fixtures",
  );
});
