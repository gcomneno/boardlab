import assert from "node:assert/strict";
import test from "node:test";

import {
  getMatchTeams,
  putMatchTeams,
} from "./match-teams-d1.mjs";

const teams = [
  {
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    providerTeamId: "487",
  },
  {
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    providerTeamId: "529",
  },
];

function fakeDb({ rows = [], changes = 1 } = {}) {
  const binds = [];

  return {
    binds,
    prepare(sql) {
      return {
        bind(...values) {
          binds.push({ sql, values });
          return {
            all: async () => ({ results: rows }),
          };
        },
      };
    },
    async batch(statements) {
      return statements.map(() => ({
        success: true,
        meta: { changes },
      }));
    },
  };
}

test("persists exactly two authoritative team references", async () => {
  const db = fakeDb();

  const result = await putMatchTeams(db, "match-1", teams);

  assert.deepEqual(result, teams);
  assert.equal(db.binds.length, 2);
  assert.deepEqual(
    db.binds.map(entry => entry.values),
    [
      ["match-1", 0, "api-football", "135", "2024", "487"],
      ["match-1", 1, "api-football", "135", "2024", "529"],
    ],
  );
});

test("loads persisted team references in seat order", async () => {
  const db = fakeDb({
    rows: [
      {
        seat: 0,
        provider: "api-football",
        competition_id: "135",
        season: "2024",
        provider_team_id: "487",
      },
      {
        seat: 1,
        provider: "api-football",
        competition_id: "135",
        season: "2024",
        provider_team_id: "529",
      },
    ],
  });

  assert.deepEqual(await getMatchTeams(db, "match-1"), teams);
});

test("legacy match without team metadata returns null", async () => {
  const db = fakeDb();

  assert.equal(await getMatchTeams(db, "legacy-match"), null);
});

test("partial persisted team metadata fails closed", async () => {
  const db = fakeDb({
    rows: [{
      seat: 0,
      provider: "api-football",
      competition_id: "135",
      season: "2024",
      provider_team_id: "487",
    }],
  });

  await assert.rejects(
    () => getMatchTeams(db, "broken-match"),
    /Invalid persisted match teams/,
  );
});

test("invalid team selection is rejected before persistence", async () => {
  const db = fakeDb();

  await assert.rejects(
    () => putMatchTeams(db, "match-1", [
      teams[0],
      { ...teams[1], providerTeamId: "" },
    ]),
    /Invalid team selection/,
  );
});
