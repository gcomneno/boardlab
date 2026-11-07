import test from "node:test";
import assert from "node:assert/strict";
import { createRoster, substitute, sendOff, pickScorer } from "./match-roster.mjs";

test("11 starters and 9 substitutes have unique shirt numbers", () => {
  const roster = createRoster(() => 0.5);
  const all = [...roster.active, ...roster.bench];

  assert.equal(roster.active.length, 11);
  assert.equal(roster.bench.length, 9);
  assert.equal(new Set(all).size, 20);
  assert.ok(all.every(n => Number.isInteger(n) && n >= 1 && n <= 99));
});

test("departed players never return and substitutions stop at five", () => {
  const roster = createRoster(() => 0.25);
  const initial = new Set([...roster.active, ...roster.bench]);

  for (let i = 0; i < 5; i += 1) {
    const change = substitute(roster, () => 0);
    assert.ok(change);
    assert.ok(initial.has(change.out));
    assert.ok(initial.has(change.in));
    assert.ok(!roster.active.includes(change.out));
    assert.ok(!roster.bench.includes(change.out));
  }

  assert.equal(roster.substitutions, 5);
  assert.equal(substitute(roster, () => 0), null);
  assert.equal(new Set(roster.departed).size, 5);
});

test("tactical substitutions do not immediately recycle substitutes", () => {
  const roster = createRoster(() => 0.5);

  const first = substitute(roster, () => 0);
  assert.ok(first);
  assert.ok(roster.substitutesIn.includes(first.in));

  const second = substitute(roster, () => 0);
  assert.ok(second);
  assert.notEqual(second.out, first.in);
  assert.ok(roster.active.includes(first.in));
});

test("injury substitutions may replace a previous substitute", () => {
  const roster = createRoster(() => 0.5);

  const first = substitute(roster, () => 0);
  assert.ok(first);

  const index = roster.active.indexOf(first.in);
  const draw = index / roster.active.length;

  const second = substitute(
    roster,
    () => draw,
    { allowSubstituteOut: true },
  );

  assert.ok(second);
  assert.equal(second.out, first.in);
});

test("sent-off players cannot return through substitutions", () => {
  const roster = createRoster(() => 0.75);
  const expelled = sendOff(roster, () => 0);

  assert.ok(roster.sentOff.includes(expelled));
  assert.ok(!roster.active.includes(expelled));

  for (let i = 0; i < 5; i += 1) substitute(roster, () => 0);
  assert.ok(!roster.active.includes(expelled));
});

test("random input is validated", () => {
  assert.throws(() => createRoster(() => 1), RangeError);
  assert.throws(() => createRoster(() => -0.1), RangeError);
});

test("goalscorer belongs to the active lineup", () => {
  const roster = createRoster(() => 0.5);
  const number = pickScorer(roster, () => 0.25);

  assert.ok(roster.active.includes(number));
  assert.ok(!roster.bench.includes(number));
});

test("substituted and expelled players cannot score", () => {
  const roster = createRoster(() => 0.5);
  const change = substitute(roster, () => 0);
  const expelled = sendOff(roster, () => 0);

  for (let index = 0; index < 50; index += 1) {
    const number = pickScorer(roster, () => index / 50);
    assert.ok(roster.active.includes(number));
    assert.notEqual(number, change.out);
    assert.notEqual(number, expelled);
  }
});

test("scorer selection is deterministic and validates random input", () => {
  const roster = createRoster(() => 0.5);

  assert.equal(pickScorer(roster, () => 0),
    pickScorer(roster, () => 0));
  assert.throws(() => pickScorer(roster, () => 1), RangeError);
  assert.equal(pickScorer({ active: [] }, () => 0), null);
});


test("players who leave the pitch are never eligible for later player events", () => {
  const roster = createRoster(() => 0.5);

  const changed = substitute(roster, () => 0);
  assert.ok(changed);
  assert.ok(!roster.active.includes(changed.out));
  assert.ok(roster.departed.includes(changed.out));

  for (let i = 0; i < 40; i += 1) {
    const scorer = pickScorer(roster, () => i / 40);
    assert.notEqual(scorer, changed.out);
  }

  const expelled = sendOff(roster, () => 0);
  assert.ok(expelled !== null);
  assert.ok(!roster.active.includes(expelled));
  assert.ok(roster.sentOff.includes(expelled));

  for (let i = 0; i < 40; i += 1) {
    const scorer = pickScorer(roster, () => i / 40);
    assert.notEqual(scorer, changed.out);
    assert.notEqual(scorer, expelled);
  }
});

test("a goalkeeper can only be replaced by another goalkeeper", () => {
  const roster = {
    active: [1, 2],
    bench: [12, 13],
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
    identities: {
      1: { position: "Goalkeeper" },
      2: { position: "Defender" },
      12: { position: "Defender" },
      13: { position: "Goalkeeper" },
    },
  };

  const change = substitute(roster, () => 0);

  assert.deepEqual(change, { out: 1, in: 13 });
  assert.ok(roster.active.includes(13));
  assert.ok(!roster.active.includes(1));
  assert.ok(roster.bench.includes(12));
});

test("a goalkeeper is not automatically replaced by an outfield player", () => {
  const roster = {
    active: [1, 2],
    bench: [12],
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
    identities: {
      1: { position: "Goalkeeper" },
      2: { position: "Defender" },
      12: { position: "Defender" },
    },
  };

  const snapshot = structuredClone(roster);
  const change = substitute(roster, () => 0);

  assert.equal(change, null);
  assert.deepEqual(roster, snapshot);
});
