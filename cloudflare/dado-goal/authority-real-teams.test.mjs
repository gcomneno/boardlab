import assert from "node:assert/strict";
import test from "node:test";

import { D1Authority } from "./authority.mjs";

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

function player(id, position) {
  return {
    provider: "api-football",
    providerPlayerId: String(id),
    name: `Player ${id}`,
    position,
    shirtNumber: id,
  };
}

function team(providerTeamId, name, offset) {
  return {
    provider: "api-football",
    providerTeamId,
    name,
    competitionId: "135",
    season: "2024",
    colors: ["#000000"],
    logo: `https://example.invalid/${providerTeamId}.png`,
    players: Array.from({ length: 20 }, (_, index) =>
      player(
        offset + index + 1,
        index === 0 || index === 11
          ? "Goalkeeper"
          : "Defender",
      )
    ),
  };
}

const catalogue = {
  schemaVersion: 3,
  provider: "api-football",
  competitionId: "135",
  season: "2024",
  importedAt: "2026-11-04T00:00:00Z",
  teams: [
    team("487", "Lazio", 0),
    team("529", "Roma", 100),
  ],
};

function fakeD1() {
  const matchTeams = [];
  const events = [];
  let match = null;
  let inviteDigest = null;

  function statement(sql, args = []) {
    return {
      sql,
      args,
      bind(...values) {
        return statement(sql, values);
      },
      async first() {
        if (sql.includes("FROM football_catalogues")) {
          return { catalogue_json: JSON.stringify(catalogue) };
        }

        if (sql.includes("FROM lifecycle_matches")) {
          return match;
        }

        throw new Error(`Unexpected first(): ${sql}`);
      },
      async all() {
        if (sql.includes("FROM lifecycle_match_teams")) {
          return {
            results: matchTeams
              .filter(row => row.match_id === args[0])
              .sort((a, b) => a.seat - b.seat),
          };
        }

        if (sql.includes("FROM lifecycle_events")) {
          return {
            results: events.map((event, index) => ({
              revision: index + 1,
              event_json: JSON.stringify(event),
            })),
          };
        }

        throw new Error(`Unexpected all(): ${sql}`);
      },
    };
  }

  return {
    prepare: statement,

    async batch(statements) {
      const results = [];

      for (const entry of statements) {
        const { sql, args } = entry;

        if (sql.includes("INSERT INTO lifecycle_matches")) {
          match = {
            revision: 0,
            status: "waiting",
            state_json: args[1],
            invite_digest: args[2],
            invite_used: 0,
            created_at: args[3],
            updated_at: args[4],
            expires_at: args[5],
            completed_at: null,
          };
          inviteDigest = args[2];
          results.push({ success: true, meta: { changes: 1 } });
          continue;
        }

        if (sql.includes("INSERT INTO lifecycle_seats")) {
          results.push({ success: true, meta: { changes: 1 } });
          continue;
        }

        if (sql.includes("INSERT INTO lifecycle_match_teams")) {
          matchTeams.push({
            match_id: args[0],
            seat: args[1],
            provider: args[2],
            competition_id: args[3],
            season: args[4],
            provider_team_id: args[5],
          });
          results.push({ success: true, meta: { changes: 1 } });
          continue;
        }

        if (sql.includes("UPDATE lifecycle_matches") &&
            sql.includes("invite_used")) {
          const suppliedDigest = args[3];

          const digestMatches =
            suppliedDigest !== undefined &&
            inviteDigest !== null &&
            Buffer.from(suppliedDigest).equals(
              Buffer.from(inviteDigest)
            );

          if (!match ||
              match.status !== "waiting" ||
              match.invite_used !== 0 ||
              !digestMatches) {
            results.push({ success: true, meta: { changes: 0 } });
            continue;
          }

          match.revision = 1;
          match.status = "active";
          match.invite_used = 1;
          results.push({ success: true, meta: { changes: 1 } });
          continue;
        }

        if (sql.includes("INSERT INTO lifecycle_events")) {
          events.push(JSON.parse(args[1]));
          results.push({ success: true, meta: { changes: 1 } });
          continue;
        }

        results.push({ success: true, meta: { changes: 1 } });
      }

      return results;
    },

    get matchTeams() {
      return matchTeams;
    },

    get events() {
      return events;
    },
  };
}

test("#69: create and join persist authoritative real rosters", async () => {
  const db = fakeD1();
  const authority = new D1Authority(db);

  const created = await authority.create(6, teams);

  assert.equal(created.snapshot.status, "waiting");
  assert.equal(db.matchTeams.length, 2);

  const joined = await authority.join(
    created.matchId,
    created.inviteToken,
  );

  assert.equal(joined.snapshot.status, "active");
  assert.equal(db.events.length, 1);
  assert.equal(db.events[0].type, "joined");

  const rosters = db.events[0].rosters;

  assert.equal(rosters.length, 2);
  assert.equal(rosters[0].active.length, 11);
  assert.equal(rosters[0].bench.length, 9);
  assert.equal(rosters[1].active.length, 11);
  assert.equal(rosters[1].bench.length, 9);

  assert.equal(
    rosters[0].identities[1].position,
    "Goalkeeper",
  );
  assert.equal(
    rosters[0].identities[12].position,
    "Goalkeeper",
  );
  assert.equal(
    rosters[1].identities[1].position,
    "Goalkeeper",
  );
  assert.equal(
    rosters[1].identities[12].position,
    "Goalkeeper",
  );

  assert.equal(
    rosters[0].identities[1].provider,
    "api-football",
  );
  assert.equal(
    rosters[1].identities[1].provider,
    "api-football",
  );
});
