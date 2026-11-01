import test from "node:test";
import assert from "node:assert/strict";
import { createRoster } from "./match-roster.mjs";
import {
  eventProbabilities, createEventState, evaluateHandEvent
} from "./match-events.mjs";

test("substitution frequency follows match-minute bands", () => {
  assert.equal(eventProbabilities(0).substitution, 0.002);
  assert.equal(eventProbabilities(60).substitution, 0.002);
  assert.equal(eventProbabilities(61).substitution, 0.02);
  assert.equal(eventProbabilities(75).substitution, 0.02);
  assert.equal(eventProbabilities(76).substitution, 0.08);
  assert.equal(eventProbabilities(90).substitution, 0.08);
  assert.equal(eventProbabilities(90).yellow, 0.02);
  assert.equal(eventProbabilities(90).red, 0.002);
});

test("yellow cards target active players without duplicates", () => {
  const roster = createRoster(() => 0.5);
  const events = createEventState();
  const first = evaluateHandEvent(roster, events, 30, () => 0);
  const second = evaluateHandEvent(roster, events, 31, () => 0);

  assert.equal(first.type, "yellow");
  assert.equal(second.type, "yellow");
  assert.notEqual(first.number, second.number);
  assert.equal(events.yellowed.size, 2);
});

test("red card removes a player from the pitch", () => {
  const roster = createRoster(() => 0.5);
  const event = evaluateHandEvent(
    roster, createEventState(), 40,
    (() => {
      const values = [0.021, 0];
      return () => values.shift();
    })()
  );

  assert.equal(event.type, "red");
  assert.ok(roster.sentOff.includes(event.number));
  assert.ok(!roster.active.includes(event.number));
});

test("substitutions respect the five-player limit", () => {
  const roster = createRoster(() => 0.5);
  const state = createEventState();

  for (let i = 0; i < 5; i += 1) {
    const event = evaluateHandEvent(roster, state, 80,
      (() => {
        const values = [0.05, 0, 0];
        return () => values.shift();
      })()
    );
    assert.equal(event.type, "substitution");
    assert.ok(!roster.active.includes(event.out));
  }

  assert.equal(roster.substitutions, 5);
  assert.equal(evaluateHandEvent(roster, state, 80, () => 0.05), null);
});

test("no event and invalid random input", () => {
  const roster = createRoster(() => 0.5);
  const state = createEventState();

  assert.equal(evaluateHandEvent(roster, state, 80, () => 0.9), null);
  assert.throws(
    () => evaluateHandEvent(roster, state, 80, () => 1), RangeError
  );
  assert.throws(() => eventProbabilities(91), RangeError);
});
