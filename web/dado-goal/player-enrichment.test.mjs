import assert from "node:assert/strict";
import test from "node:test";

import { enrichCataloguePlayers } from "./player-enrichment.mjs";

function fixtureCatalogue() {
  return {
    schemaVersion: 4,
    provider: "api-football",
    competitionId: "39",
    season: "2024",
    importedAt: "2026-11-12T00:00:00.000Z",
    teams: [{
      provider: "api-football",
      providerTeamId: "33",
      name: "Team",
      competitionId: "39",
      season: "2024",
      colors: ["#000000"],
      logo: "team.png",
      enrichment: { country: "England" },
      players: [{
        provider: "api-football",
        providerPlayerId: "18885",
        name: "K. Darlow",
        position: "Goalkeeper",
        shirtNumber: 1,
      }, {
        provider: "api-football",
        providerPlayerId: "99999",
        name: "Untouched Player",
        position: "Defender",
        shirtNumber: 4,
      }],
    }],
  };
}

test("incrementally enriches players without rebuilding canonical roster", () => {
  const original = fixtureCatalogue();

  const result = enrichCataloguePlayers({
    catalogue: original,
    provider: "api-football",
    retrievedAt: "2026-11-12T18:00:00.000Z",
    snapshots: [{
      providerPlayerId: 18885,
      providerTeamId: "33",
      firstname: "Karl",
      lastname: "Darlow",
      nationality: "England",
      height: "190",
      weight: "88",
      birth: {
        date: "1990-10-08",
        place: "Northampton",
        country: "England",
      },
    }],
  });

  const player = result.catalogue.teams[0].players[0];

  assert.equal(player.name, "K. Darlow");
  assert.equal(player.position, "Goalkeeper");
  assert.equal(player.shirtNumber, 1);
  assert.equal(player.enrichment.firstname, "Karl");
  assert.equal(player.enrichment.birth.place, "Northampton");
  assert.deepEqual(player.enrichment.provenance, {
    provider: "api-football",
    retrievedAt: "2026-11-12T18:00:00.000Z",
  });

  assert.equal(
    result.catalogue.teams[0].players[1],
    original.teams[0].players[1],
  );

  assert.deepEqual(result.stats, {
    snapshots: 1,
    matched: 1,
    unmatched: 0,
  });
});

test("does not mutate the source catalogue", () => {
  const original = fixtureCatalogue();
  const before = structuredClone(original);

  enrichCataloguePlayers({
    catalogue: original,
    provider: "api-football",
    retrievedAt: "2026-11-12T18:00:00.000Z",
    snapshots: [{
      providerPlayerId: "18885",
      providerTeamId: "33",
      nationality: "England",
    }],
  });

  assert.deepEqual(original, before);
});

test("reports provider snapshots absent from the catalogue", () => {
  const result = enrichCataloguePlayers({
    catalogue: fixtureCatalogue(),
    provider: "api-football",
    retrievedAt: "2026-11-12T18:00:00.000Z",
    snapshots: [{
      providerPlayerId: "123456",
      providerTeamId: "33",
      nationality: "Italy",
    }],
  });

  assert.deepEqual(result.stats, {
    snapshots: 1,
    matched: 0,
    unmatched: 1,
  });
});

test("rejects duplicate provider player snapshots", () => {
  assert.throws(
    () => enrichCataloguePlayers({
      catalogue: fixtureCatalogue(),
      provider: "api-football",
      retrievedAt: "2026-11-12T18:00:00.000Z",
      snapshots: [
        { providerPlayerId: "18885", providerTeamId: "33" },
        { providerPlayerId: 18885, providerTeamId: "33" },
      ],
    }),
    /duplicate player snapshot: 33:18885/,
  );
});


test("allows the same provider player in different teams", () => {
  const source = {
    schemaVersion: 4,
    provider: "api-football",
    competitionId: "135",
    season: "2024",
    teams: [
      {
        providerTeamId: "509",
        players: [{
          providerPlayerId: "341978",
          name: "L. D&apos;Andrea",
          position: "Attacker",
          shirtNumber: 16,
        }],
      },
      {
        providerTeamId: "518",
        players: [{
          providerPlayerId: "341978",
          name: "L. D&apos;Andrea",
          position: "Attacker",
          shirtNumber: 16,
        }],
      },
    ],
  };

  const result = enrichCataloguePlayers({
    catalogue: source,
    snapshots: [
      {
        providerPlayerId: "341978",
        providerTeamId: "509",
        age: 21,
        photo: "player.png",
      },
      {
        providerPlayerId: "341978",
        providerTeamId: "518",
        age: 21,
        photo: "player.png",
      },
    ],
    provider: "api-football",
    retrievedAt: "2026-11-12T00:00:00.000Z",
  });

  assert.equal(result.stats.matched, 2);
  assert.equal(result.stats.unmatched, 0);
  assert.equal(result.catalogue.teams[0].players[0].enrichment.age, 21);
  assert.equal(result.catalogue.teams[1].players[0].enrichment.age, 21);
});

test("accepts normalized squad metadata as incremental enrichment", () => {
  const result = enrichCataloguePlayers({
    catalogue: fixtureCatalogue(),
    provider: "api-football",
    retrievedAt: "2026-11-12T18:30:00.000Z",
    snapshots: [{
      providerPlayerId: "18885",
      providerTeamId: "33",
      id: 18885,
      name: "K. Darlow",
      position: "Goalkeeper",
      shirtNumber: 1,
      age: 36,
      photo: "https://media.api-sports.io/football/players/18885.png",
    }],
  });

  const player = result.catalogue.teams[0].players[0];

  assert.equal(player.name, "K. Darlow");
  assert.equal(player.position, "Goalkeeper");
  assert.equal(player.shirtNumber, 1);
  assert.equal(player.enrichment.age, 36);
  assert.equal(
    player.enrichment.photo,
    "https://media.api-sports.io/football/players/18885.png",
  );

  assert.equal("id" in player.enrichment, false);
  assert.equal("name" in player.enrichment, false);
  assert.equal("position" in player.enrichment, false);
  assert.equal("shirtNumber" in player.enrichment, false);
});
