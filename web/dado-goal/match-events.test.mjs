import test from "node:test";
import assert from "node:assert/strict";
import { createRoster } from "./match-roster.mjs";
import {
  eventProbabilities, createEventState, evaluateHandEvent
} from "./match-events.mjs";

test("substitution frequency follows match-minute bands", () => {
  assert.equal(eventProbabilities(0).substitution, 0);
  assert.equal(eventProbabilities(45).substitution, 0);
  assert.equal(eventProbabilities(46).substitution, 0.04);
  assert.equal(eventProbabilities(60).substitution, 0.04);
  assert.equal(eventProbabilities(61).substitution, 0.10);
  assert.equal(eventProbabilities(75).substitution, 0.10);
  assert.equal(eventProbabilities(76).substitution, 0.20);
  assert.equal(eventProbabilities(90).substitution, 0.20);
  assert.equal(eventProbabilities(90).yellow, 0.02);
  assert.equal(eventProbabilities(90).red, 0.002);
});

test("halftime increases only substitution probability", () => {
  const ordinary = eventProbabilities(45);
  const halftime = eventProbabilities(45, { halftime: true });

  assert.ok(halftime.substitution > ordinary.substitution);
  assert.ok(halftime.substitution < 1);
  assert.equal(halftime.yellow, ordinary.yellow);
  assert.equal(halftime.red, ordinary.red);
});

test("halftime context reaches event selection", () => {
  const roster = createRoster(() => 0.5);
  const events = createEventState();

  const ordinary = evaluateHandEvent(
    roster, events, 45, () => 0.05
  );
  assert.equal(ordinary, null);

  const halftimeRoster = createRoster(() => 0.5);
  const halftimeEvent = evaluateHandEvent(
    halftimeRoster,
    createEventState(),
    45,
    (() => {
      const values = [0.05, 0, 0];
      return () => values.shift();
    })(),
    { halftime: true }
  );

  assert.equal(halftimeEvent.type, "substitution");
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
      const values = [0.021, 0, 0];
      return () => values.shift();
    })()
  );

  assert.equal(event.type, "red");
  assert.ok(roster.sentOff.includes(event.number));
  assert.ok(!roster.active.includes(event.number));
});

test("disciplinary events carry machine-readable reasons", () => {
  const yellowRoster = createRoster(() => 0.5);
  const yellow = evaluateHandEvent(
    yellowRoster,
    createEventState(),
    30,
    (() => {
      const values = [0, 0, 0];
      return () => values.shift();
    })()
  );

  assert.equal(yellow.type, "yellow");
  assert.equal(yellow.reason, "reckless-challenge");

  const redRoster = createRoster(() => 0.5);
  const red = evaluateHandEvent(
    redRoster,
    createEventState(),
    30,
    (() => {
      const values = [0.021, 0, 0];
      return () => values.shift();
    })()
  );

  assert.equal(red.type, "red");
  assert.equal(red.reason, "serious-foul-play");
});

test("an injury can justify an early substitution", () => {
  const roster = createRoster(() => 0.5);
  const event = evaluateHandEvent(
    roster,
    createEventState(),
    18,
    (() => {
      const values = [0.025, 0, 0];
      return () => values.shift();
    })()
  );

  assert.equal(event.type, "substitution");
  assert.equal(event.reason, "injury");
  assert.equal(roster.substitutions, 1);
  assert.ok(!roster.active.includes(event.out));
  assert.ok(roster.active.includes(event.in));
});

test("ordinary tactical substitutions remain impossible in first half", () => {
  const roster = createRoster(() => 0.5);
  const event = evaluateHandEvent(
    roster,
    createEventState(),
    18,
    () => 0.5
  );

  assert.equal(event, null);
  assert.equal(roster.substitutions, 0);
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

test("ordinary substitutions cannot occur during the first half", () => {
  assert.equal(eventProbabilities(1).substitution, 0);
  assert.equal(eventProbabilities(30).substitution, 0);
  assert.equal(eventProbabilities(45).substitution, 0);
});

test("halftime remains a probabilistic substitution opportunity", () => {
  const halftime = eventProbabilities(45, { halftime: true });
  assert.ok(halftime.substitution > 0);
  assert.ok(halftime.substitution < 1);
});

test("a team with no substitutions gets stronger late-match pressure", () => {
  const ordinary = eventProbabilities(80, { substitutions: 2 });
  const noneMade = eventProbabilities(80, { substitutions: 0 });

  assert.ok(noneMade.substitution > ordinary.substitution);
  assert.ok(noneMade.substitution < 1);
});

test("late substitution pressure remains probabilistic", () => {
  const odds = eventProbabilities(90, { substitutions: 0 });
  assert.ok(odds.substitution > 0);
  assert.ok(odds.substitution < 1);
});

test("injury without an available substitution emits no incomplete event", () => {
  const roster = createRoster(() => 0.5);
  roster.bench.splice(0, roster.bench.length);

  const event = evaluateHandEvent(
    roster,
    createEventState(),
    70,
    (() => {
      const values = [0.025, 0.5];
      return () => values.shift();
    })(),
  );

  assert.equal(event, null);
});
