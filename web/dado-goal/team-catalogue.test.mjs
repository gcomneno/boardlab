import assert from "node:assert/strict";
import test from "node:test";

import {
  CATALOGUE_SCHEMA_VERSION,
  createCatalogue,
  createPlayer,
  createTeam,
} from "./team-catalogue.mjs";

test("creates canonical provider-backed team and player identities", () => {
  const team = createTeam({
    provider: "fixture",
    providerTeamId: 42,
    name: "Original Team Name",
    competitionId: 135,
    season: 2026,
    colors: ["#112233"],
    logo: "https://media.api-sports.io/football/teams/42.png",
  });

  const player = createPlayer({
    provider: "fixture",
    providerPlayerId: 9001,
    name: "Original Player Name",
    position: "Goalkeeper",
    shirtNumber: 1,
  });

  assert.equal(team.providerTeamId, "42");
  assert.equal(team.name, "Original Team Name");
  assert.equal(player.providerPlayerId, "9001");
  assert.equal(player.name, "Original Player Name");
  assert.equal(player.shirtNumber, 1);
});

test("catalogue records provenance and schema version", () => {
  const catalogue = createCatalogue({
    provider: "fixture",
    competitionId: 135,
    season: 2026,
    importedAt: "2026-11-02T10:00:00Z",
    teams: [],
  });

  assert.equal(catalogue.schemaVersion, CATALOGUE_SCHEMA_VERSION);
  assert.equal(catalogue.provider, "fixture");
  assert.equal(catalogue.competitionId, "135");
  assert.equal(catalogue.season, "2026");
});

test("official names are preserved verbatim", () => {
  const team = createTeam({
    provider: "fixture",
    providerTeamId: "7",
    name: "São Paulo FC",
    competitionId: "71",
    season: "2026",
    colors: ["#112233"],
    logo: "https://media.api-sports.io/football/teams/7.png",
  });

  const player = createPlayer({
    provider: "fixture",
    providerPlayerId: "8",
    name: "José Ángel",
    position: "Midfielder",
  });

  assert.equal(team.name, "São Paulo FC");
  assert.equal(player.name, "José Ángel");
});

test("rejects invalid shirt numbers", () => {
  assert.throws(
    () => createPlayer({
      provider: "fixture",
      providerPlayerId: 1,
      name: "Player",
      position: "Forward",
      shirtNumber: 0,
    }),
    /shirtNumber/,
  );
});
