import assert from "node:assert/strict";
import test from "node:test";

import {
  createNextMatch,
  validateNextMatch,
} from "./manager-next-match.mjs";

test("next match stores only the canonical opponent identity", () => {
  assert.deepEqual(createNextMatch({
    managedProviderTeamId: "club-a",
    opponentProviderTeamId: "club-b",
  }), {
    opponentProviderTeamId: "club-b",
  });
});

test("managed club cannot play itself", () => {
  assert.throws(() => createNextMatch({
    managedProviderTeamId: "club-a",
    opponentProviderTeamId: "club-a",
  }), /cannot play itself/);
});

test("null represents no prepared next match", () => {
  assert.equal(validateNextMatch(null, "club-a"), null);
});

test("next-match state rejects implicit career systems", () => {
  assert.throws(() => validateNextMatch({
    opponentProviderTeamId: "club-b",
    fixtureRound: 2,
  }, "club-a"), /unsupported next-match state/);

  assert.throws(() => validateNextMatch({
    opponentProviderTeamId: "club-b",
    standings: [],
  }, "club-a"), /unsupported next-match state/);
});
