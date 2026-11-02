import assert from "node:assert/strict";
import test from "node:test";

import { createMatchRoster } from "./catalogue-roster.mjs";

function importedTeam() {
  return {
    provider: "api-football",
    providerTeamId: "492",
    name: "SSC Napoli",
    competitionId: "135",
    season: "2026",
    players: Array.from({ length: 20 }, (_, index) => ({
      provider: "api-football",
      providerPlayerId: String(1000 + index),
      name: `Player ${index + 1}`,
      position: index === 0 ? "Goalkeeper" : "Field",
      shirtNumber: index + 1,
    })),
  };
}

test("selected real team produces provider-backed match roster", () => {
  const roster = createMatchRoster({ team: importedTeam() });

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 9);
  assert.equal(roster.identities[1].provider, "api-football");
  assert.equal(roster.identities[1].providerPlayerId, "1000");
});

test("absence of selected team preserves quick-match roster", () => {
  const roster = createMatchRoster({
    team: null,
    random: () => 0.5,
  });

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 9);
  assert.equal("identities" in roster, false);
});
