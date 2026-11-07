import assert from "node:assert/strict";
import test from "node:test";

import {
  createPlayerRatings,
  createRatedPlayer,
  RATING_FIELDS,
  RATING_LEVELS,
} from "./player-ratings.mjs";

test("manual ratings use explicit descriptive levels", () => {
  assert.deepEqual(RATING_FIELDS, [
    "attack",
    "defence",
    "technique",
    "physical",
  ]);

  assert.deepEqual(RATING_LEVELS, [
    "poor",
    "fair",
    "good",
    "very-good",
    "excellent",
  ]);
});

test("ratings are manual metadata with deterministic defaults", () => {
  assert.deepEqual(createPlayerRatings(), {
    attack: "fair",
    defence: "fair",
    technique: "fair",
    physical: "fair",
  });
});

test("rated player preserves provider identity and official name", () => {
  const player = createRatedPlayer({
    provider: "api-football",
    providerPlayerId: "123",
    name: "José Ángel",
    position: "Attacker",
    shirtNumber: 7,
  }, {
    attack: "excellent",
    defence: "poor",
    technique: "very-good",
    physical: "good",
  });

  assert.equal(player.name, "José Ángel");
  assert.equal(player.providerPlayerId, "123");
  assert.equal(player.ratings.attack, "excellent");
});

test("invalid manual rating is rejected", () => {
  assert.throws(
    () => createPlayerRatings({ attack: "99" }),
    /invalid rating level/,
  );
});
