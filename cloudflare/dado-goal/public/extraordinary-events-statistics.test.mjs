import test from "node:test";
import assert from "node:assert/strict";

import { createRoster } from "./match-roster.mjs";
import {
  createExtraordinaryEventState,
  evaluateExtraordinaryEvent,
  extraordinaryEventCatalogue,
} from "./extraordinary-events.mjs";

function sequenceRandom(values) {
  let index = 0;
  return () => {
    const value = values[index % values.length];
    index += 1;
    return value;
  };
}

test("fresh-state extraordinary frequency matches catalogue probability mass", () => {
  const samples = 100_000;
  let emitted = 0;

  const expected = extraordinaryEventCatalogue
    .filter(event => !event.requiresVarEligible)
    .reduce((sum, event) => sum + event.probability, 0);

  for (let i = 0; i < samples; i += 1) {
    const roster = createRoster(() => 0.5);
    const state = createExtraordinaryEventState();
    const draw = (i + 0.5) / samples;

    const event = evaluateExtraordinaryEvent(
      roster,
      state,
      30,
      sequenceRandom([draw, 0.5]),
    );

    if (event) emitted += 1;
  }

  const observed = emitted / samples;
  assert.ok(
    Math.abs(observed - expected) <= 0.00002,
    `observed=${observed} expected=${expected}`,
  );
});

test("narrative extraordinary events never mutate roster state", () => {
  const draws = [
    0.0001,
    0.0061,
    0.0086,
    0.0101,
    0.0106,
    0.0109,
    0.0113,
    0.0114,
    0.0116,
    0.0118,
  ];

  for (const draw of draws) {
    const roster = createRoster(() => 0.5);
    const before = structuredClone(roster);

    evaluateExtraordinaryEvent(
      roster,
      createExtraordinaryEventState(),
      55,
      sequenceRandom([draw, 0.5]),
    );

    assert.deepEqual(roster, before);
  }
});

test("family caps remove only their own eligible probability mass", () => {
  const roster = createRoster(() => 0.5);

  const fresh = createExtraordinaryEventState();
  const weatherCapped = createExtraordinaryEventState();
  weatherCapped.occurredFamilies.add("weather");

  const freshCatalogueMass = extraordinaryEventCatalogue
    .filter(event => !event.requiresVarEligible)
    .reduce((sum, event) => sum + event.probability, 0);

  const weatherMass = extraordinaryEventCatalogue
    .filter(event => event.family === "weather")
    .reduce((sum, event) => sum + event.probability, 0);

  const expectedAfterCap = freshCatalogueMass - weatherMass;

  const samples = 100_000;
  let freshCount = 0;
  let cappedCount = 0;

  for (let i = 0; i < samples; i += 1) {
    const draw = (i + 0.5) / samples;

    const a = evaluateExtraordinaryEvent(
      roster,
      {
        occurredFamilies: new Set(fresh.occurredFamilies),
        occurredTypes: new Set(fresh.occurredTypes),
      },
      60,
      sequenceRandom([draw, 0.5]),
    );

    const b = evaluateExtraordinaryEvent(
      roster,
      {
        occurredFamilies: new Set(weatherCapped.occurredFamilies),
        occurredTypes: new Set(weatherCapped.occurredTypes),
      },
      60,
      sequenceRandom([draw, 0.5]),
    );

    if (a) freshCount += 1;
    if (b) cappedCount += 1;
  }

  const freshObserved = freshCount / samples;
  const cappedObserved = cappedCount / samples;

  assert.ok(Math.abs(freshObserved - freshCatalogueMass) <= 0.00002);
  assert.ok(Math.abs(cappedObserved - expectedAfterCap) <= 0.00002);
});

test("VAR eligibility adds exactly its configured probability mass", () => {
  const samples = 100_000;

  const ordinaryMass = extraordinaryEventCatalogue
    .filter(event => !event.requiresVarEligible)
    .reduce((sum, event) => sum + event.probability, 0);

  const eligibleMass = extraordinaryEventCatalogue.reduce(
    (sum, event) => sum + event.probability,
    0,
  );

  let emitted = 0;

  for (let i = 0; i < samples; i += 1) {
    const roster = createRoster(() => 0.5);
    const state = createExtraordinaryEventState();
    const draw = (i + 0.5) / samples;

    const event = evaluateExtraordinaryEvent(
      roster,
      state,
      60,
      sequenceRandom([draw, 0.5]),
      { varEligible: true },
    );

    if (event) emitted += 1;
  }

  const observed = emitted / samples;

  assert.ok(
    Math.abs(observed - eligibleMass) <= 0.00002,
    `observed=${observed} expected=${eligibleMass}`,
  );
  const varProbability = extraordinaryEventCatalogue
    .find(event => event.type === "var-review").probability;

  assert.ok(
    Math.abs((eligibleMass - ordinaryMass) - varProbability) <= 1e-12,
    `delta=${eligibleMass - ordinaryMass} var=${varProbability}`,
  );
});

test("extraordinary catalogue remains rare", () => {
  const total = extraordinaryEventCatalogue.reduce(
    (sum, event) => sum + event.probability,
    0,
  );

  assert.ok(total > 0);
  assert.ok(total < 0.02, `extraordinary probability mass=${total}`);
});
