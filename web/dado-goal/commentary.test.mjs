import test from "node:test";
import assert from "node:assert/strict";
import { newMatch, applyRoll } from "./engine.mjs";
import { describeRoll } from "./commentary.mjs";

test("recorded game produces deterministic commentary without altering state", () => {
  let state = newMatch(1);
  const recording = [
    "Tiro in porta!", "Colpo di testa", "GOAL!",
    "Passaggio intercettato!",
  ];
  const transcript = [];

  for (const [index, face] of recording.entries()) {
    const before = state;
    const after = applyRoll(before, face);
    const event = { seat: before.player, die: before.phase, face };
    const first = describeRoll(event, before, after, ["Gialli", "Blu"], index);
    const second = describeRoll(event, before, after, ["Gialli", "Blu"], index);
    assert.equal(first, second);
    assert.deepEqual(before, index === 0 ? newMatch(1) : before);
    transcript.push(first);
    state = after;
  }

  assert.match(transcript[2], /Segna Gialli/);
  assert.match(transcript[3], /Triplice fischio/);
  assert.deepEqual(state.goals, [1, 0]);
});

test("commentary is independent of the game rules", () => {
  const before = newMatch(1);
  const after = applyRoll(before, "Passaggio");
  const snapshot = structuredClone(after);
  const event = { seat: 0, die: "team", face: "Passaggio" };

  assert.ok(describeRoll(event, before, after));
  assert.deepEqual(after, snapshot);
  assert.deepEqual(after, applyRoll(before, "Passaggio"));
});

test("invalid commentary input fails closed", () => {
  const state = newMatch(1);
  assert.throws(() => describeRoll({ seat: 9, face: "GOAL!" }, state, state));
  assert.throws(() => describeRoll({ seat: 0, face: "inesistente" }, state, state));
});
