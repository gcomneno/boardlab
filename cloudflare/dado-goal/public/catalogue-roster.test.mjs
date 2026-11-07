import assert from "node:assert/strict";
import test from "node:test";

import {
  createCatalogueRoster,
  createMatchRoster,
} from "./catalogue-roster.mjs";

function teamWithPlayers(count) {
  return {
    name: "SSC Napoli",
    players: Array.from({ length: count }, (_, index) => ({
      provider: "api-football",
      providerPlayerId: String(1000 + index),
      name: `Player ${index + 1}`,
      position: index === 0 ? "Goalkeeper" : "Field",
      shirtNumber: index + 1,
    })),
  };
}

test("catalogue roster creates eleven starters and substitutes", () => {
  const roster = createCatalogueRoster(teamWithPlayers(20));

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 9);
  assert.equal(roster.substitutions, 0);
  assert.equal(roster.identities[1].providerPlayerId, "1000");
  assert.deepEqual(roster.departed, []);
  assert.deepEqual(roster.sentOff, []);
  assert.deepEqual(roster.substitutesIn, []);
  assert.equal(roster.substitutions, 0);
  assert.equal(roster.identities[1].name, "Player 1");
});

test("catalogue roster separates runtime identity from provider shirt number", () => {
  const team = teamWithPlayers(11);
  team.players[0].shirtNumber = 99;

  const roster = createCatalogueRoster(team);

  assert.equal(roster.active[0], 1);
  assert.equal(roster.identities[1].shirtNumber, 99);
});

test("rejects teams with fewer than eleven players", () => {
  assert.throws(
    () => createCatalogueRoster(teamWithPlayers(10)),
    /at least 11 players/,
  );
});

test("catalogue roster accepts duplicate and missing provider shirt numbers", () => {
  const team = teamWithPlayers(11);
  team.players[0].shirtNumber = 10;
  team.players[1].shirtNumber = 10;
  team.players[2].shirtNumber = null;

  const roster = createCatalogueRoster(team);

  assert.deepEqual(roster.active, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]);
  assert.equal(roster.identities[1].shirtNumber, 10);
  assert.equal(roster.identities[2].shirtNumber, 10);
  assert.equal(roster.identities[3].shirtNumber, null);
});

test("quick match still uses the original roster path", () => {
  const random = () => 0.5;
  const roster = createMatchRoster({ random });

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 9);
  assert.equal(roster.substitutions, 0);
  assert.equal("identities" in roster, false);
});
