import assert from "node:assert/strict";
import test from "node:test";

import {
  createManagerClub,
  managerClubMatchTeam,
  updateManagerSelection,
  validateManagerClub,
} from "./manager-profile.mjs";
import { createCatalogueRoster } from "./catalogue-roster.mjs";

function team() {
  return {
    provider: "fixture",
    providerTeamId: "club-7",
    name: "Manager FC",
    players: Array.from({ length: 22 }, (_, index) => ({
      provider: "fixture",
      providerPlayerId: `p${index + 1}`,
      name: `Player ${index + 1}`,
      position: index === 0 ? "Goalkeeper" : "Midfielder",
      shirtNumber: index + 1,
    })),
  };
}

test("catalogue team becomes a persistent manager club", () => {
  const club = createManagerClub(team());

  assert.equal(club.providerTeamId, "club-7");
  assert.equal(club.players.length, 22);
  assert.equal(club.lineup.length, 11);
  assert.equal(club.bench.length, 9);
  assert.equal(club.lineup[0], "p1");
  assert.equal(club.bench[0], "p12");
});

test("lineup and bench decisions persist by canonical player identity", () => {
  const club = createManagerClub(team());
  const next = updateManagerSelection(club, {
    lineup: ["p1", "p2", "p3", "p4", "p5", "p6",
      "p7", "p8", "p9", "p10", "p12"],
    bench: ["p11", "p13", "p14"],
  });

  assert.equal(next.lineup.at(-1), "p12");
  assert.equal(next.bench[0], "p11");
  assert.equal(club.lineup.at(-1), "p11");
});

test("invalid manager selections are rejected", () => {
  const club = createManagerClub(team());

  assert.throws(
    () => updateManagerSelection(club, {
      lineup: club.lineup.slice(0, 10),
      bench: club.bench,
    }),
    /exactly 11/,
  );

  assert.throws(
    () => updateManagerSelection(club, {
      lineup: club.lineup,
      bench: [club.lineup[0]],
    }),
    /must not overlap/,
  );

  assert.throws(
    () => updateManagerSelection(club, {
      lineup: [...club.lineup.slice(0, 10), "unknown"],
      bench: club.bench,
    }),
    /unknown player/,
  );
});

test("manager selection adapts to the normal catalogue roster contract", () => {
  const club = updateManagerSelection(createManagerClub(team()), {
    lineup: ["p1", "p2", "p3", "p4", "p5", "p6",
      "p7", "p8", "p9", "p10", "p12"],
    bench: ["p11", "p13", "p14"],
  });

  const matchTeam = managerClubMatchTeam(club);
  const roster = createCatalogueRoster(matchTeam);

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 3);
  assert.equal(roster.identities[11].providerPlayerId, "p12");
  assert.equal(roster.identities[12].providerPlayerId, "p11");
});

test("validation does not introduce career simulation state", () => {
  const club = createManagerClub(team());

  validateManagerClub(club);
  assert.equal("finances" in club, false);
  assert.equal("transfers" in club, false);
  assert.equal("fatigue" in club, false);
  assert.equal("contracts" in club, false);
});
