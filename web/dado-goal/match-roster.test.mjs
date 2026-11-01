import test from "node:test";
import assert from "node:assert/strict";
import { createRoster, substitute, sendOff } from "./match-roster.mjs";

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
