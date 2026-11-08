import test from "node:test";
import assert from "node:assert/strict";

import {
  createExtraordinaryEventState,
  evaluateExtraordinaryEvent,
  extraordinaryEventCatalogue,
} from "./extraordinary-events.mjs";

function roster() {
  return { active: [1, 2, 3, 4, 5] };
}

test("catalogue contains only narrative v1 events", () => {
  assert.equal(extraordinaryEventCatalogue.length, 11);
  assert.ok(extraordinaryEventCatalogue.every(
    event => Number.isFinite(event.probability) && event.probability > 0
  ));
});

test("light knock selects an active player without mutating roster", () => {
  const team = roster();
  const before = [...team.active];
  const state = createExtraordinaryEventState();
  const values = [0, 0.4];

  const event = evaluateExtraordinaryEvent(
    team, state, 22, () => values.shift()
  );

  assert.equal(event.type, "light-knock");
  assert.equal(event.number, 3);
  assert.deepEqual(team.active, before);
});

test("weather family is capped at one event per match", () => {
  const team = roster();
  const state = createExtraordinaryEventState();

  const first = evaluateExtraordinaryEvent(team, state, 30, () => 0.0061);
  assert.equal(first.type, "heavy-rain");

  const second = evaluateExtraordinaryEvent(team, state, 50, () => 0.0061);
  assert.equal(second.type, "pitch-invasion");
  assert.equal(second.family, "external");
});

test("external family is capped at one event per match", () => {
  const team = roster();
  const state = createExtraordinaryEventState();

  const first = evaluateExtraordinaryEvent(team, state, 40, () => 0.0106);
  assert.equal(first.type, "pitch-invasion");

  const second = evaluateExtraordinaryEvent(team, state, 60, () => 0.0106);
  assert.equal(second.type, "floodlight-failure");
  assert.equal(second.family, "technical");
});

test("technical family is capped at one event per match", () => {
  const team = roster();
  const state = createExtraordinaryEventState();

  const first = evaluateExtraordinaryEvent(team, state, 55, () => 0.0114);
  assert.equal(first.type, "floodlight-failure");

  const second = evaluateExtraordinaryEvent(team, state, 70, () => 0.0114);
  assert.equal(second, null);
});

test("ordinary draw emits no extraordinary event", () => {
  const event = evaluateExtraordinaryEvent(
    roster(),
    createExtraordinaryEventState(),
    25,
    () => 0.5,
  );

  assert.equal(event, null);
});

test("invalid random input is rejected", () => {
  assert.throws(
    () => evaluateExtraordinaryEvent(
      roster(),
      createExtraordinaryEventState(),
      25,
      () => 1,
    ),
    RangeError,
  );
});

test("capped family does not consume eligibility of later families", () => {
  const team = roster();
  const state = createExtraordinaryEventState();

  state.occurredFamilies.add("weather");

  const event = evaluateExtraordinaryEvent(
    team,
    state,
    65,
    () => 0.0061,
  );

  assert.equal(event.type, "pitch-invasion");
  assert.equal(event.family, "external");
});

test("empty active roster excludes player-dependent events", () => {
  const team = { active: [] };
  const state = createExtraordinaryEventState();

  const event = evaluateExtraordinaryEvent(
    team,
    state,
    20,
    () => 0,
  );

  assert.equal(event.type, "heavy-rain");
  assert.equal(event.family, "weather");
});


test("VAR review requires an eligible preceding football action", () => {
  const team = roster();
  const state = createExtraordinaryEventState();

  const withoutEligibleAction = evaluateExtraordinaryEvent(
    team,
    state,
    62,
    () => 0.012,
  );

  assert.equal(withoutEligibleAction, null);

  const withEligibleAction = evaluateExtraordinaryEvent(
    team,
    state,
    62,
    () => 0.012,
    { varEligible: true },
  );

  assert.equal(withEligibleAction.type, "var-review");
  assert.equal(withEligibleAction.family, "technical");
  assert.equal(withEligibleAction.classification, "narrative");
});
