import assert from "node:assert/strict";
import test from "node:test";

import {
  createAuthoritativeMatchRosters,
  resolveMatchTeam,
} from "./match-team-rosters.mjs";

const players = position => Array.from({ length: 20 }, (_, index) => ({
  provider: "api-football",
  providerPlayerId: String(index + 1),
  name: `Player ${index + 1}`,
  position: index === 0 || index === 11
    ? "Goalkeeper"
    : position,
  shirtNumber: index + 1,
}));

const catalogue = {
  provider: "api-football",
  competitionId: "135",
  season: "2024",
  teams: [
    {
      provider: "api-football",
      providerTeamId: "487",
      name: "Lazio",
      players: players("Defender"),
    },
    {
      provider: "api-football",
      providerTeamId: "529",
      name: "Roma",
      players: players("Midfielder").map((player, index) => ({
        ...player,
        providerPlayerId: String(100 + index),
      })),
    },
  ],
};

function fakeDb(value = catalogue) {
  return {
    prepare() {
      return {
        bind() {
          return {
            first: async () => value === null
              ? null
              : { catalogue_json: JSON.stringify(value) },
          };
        },
      };
    },
  };
}

const lazio = {
  provider: "api-football",
  competitionId: "135",
  season: "2024",
  providerTeamId: "487",
};

const roma = {
  provider: "api-football",
  competitionId: "135",
  season: "2024",
  providerTeamId: "529",
};

test("resolves selected team from authoritative D1 catalogue", async () => {
  const team = await resolveMatchTeam(fakeDb(), lazio);

  assert.equal(team.name, "Lazio");
  assert.equal(team.providerTeamId, "487");
});

test("builds provider-backed authoritative rosters", async () => {
  const rosters = await createAuthoritativeMatchRosters(
    fakeDb(),
    [lazio, roma],
  );

  assert.equal(rosters.length, 2);
  assert.equal(rosters[0].active.length, 11);
  assert.equal(rosters[0].bench.length, 9);
  assert.equal(rosters[0].identities[1].position, "Goalkeeper");
  assert.equal(rosters[0].identities[12].position, "Goalkeeper");
  assert.equal(rosters[1].identities[1].provider, "api-football");
});

test("unknown catalogue fails closed", async () => {
  await assert.rejects(
    () => resolveMatchTeam(fakeDb(null), lazio),
    /Unknown football catalogue/,
  );
});

test("unknown team fails closed", async () => {
  await assert.rejects(
    () => resolveMatchTeam(fakeDb(), {
      ...lazio,
      providerTeamId: "999999",
    }),
    /Unknown football team/,
  );
});

test("same team cannot occupy both seats", async () => {
  await assert.rejects(
    () => createAuthoritativeMatchRosters(
      fakeDb(),
      [lazio, lazio],
    ),
    /Teams must be different/,
  );
});
