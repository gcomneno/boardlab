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


test("bilingual commentary preserves deterministic event selection", () => {
  const before = newMatch(1);
  const after = applyRoll(before, "Passaggio");
  const event = { seat: before.player, die: before.phase, face: "Passaggio" };
  const names = ["Gialli", "Blu"];
  const snapshot = structuredClone(after);

  const italian = describeRoll(event, before, after, names, 0, "it");
  const english = describeRoll(event, before, after, names, 0, "en");

  assert.match(italian, /La squadra fa girare/);
  assert.match(english, /moves the ball around/);
  assert.notEqual(italian, english);
  assert.equal(
    describeRoll(event, before, after, names, 0),
    italian
  );
  assert.equal(
    describeRoll(event, before, after, names, 0, "en"),
    english
  );
  assert.deepEqual(after, snapshot);
});

test("both locales cover every canonical dice face", async () => {
  const { DICE_ART } = await import("./dice-art.mjs");
  assert.ok(DICE_ART);

  const faces = [
    "Passaggio", "Passaggio intercettato!", "Cross!",
    "Tiro in porta!", "GOAL!", "PALO/TRAVERSA",
    "Parato!", "RIGORE", "FUORI!", "Respinto!",
    "Palla sul fondo", "Palla alta sulla traversa",
    "Palla intercettata", "Mezza rovesciata",
    "Rovesciata", "Colpo di testa"
  ];
  const before = newMatch(1);

  for (const face of faces) {
    for (const locale of ["it", "en"]) {
      const text = describeRoll(
        { seat: before.player, die: before.phase, face },
        before, before, ["Gialli", "Blu"], 0, locale
      );
      assert.ok(text.includes("Gialli"), `${locale}: ${face}`);
    }
  }
});

test("penalty is always explicitly announced by radio commentary", () => {
  const before = newMatch(1);
  const shot = applyRoll(before, "Tiro in porta!");
  const attack = applyRoll(shot, "Colpo di testa");
  const afterPenalty = applyRoll(attack, "RIGORE");
  const event = {
    seat: attack.player,
    die: attack.phase,
    face: "RIGORE",
  };

  const italian = describeRoll(
    event, attack, afterPenalty, ["Gialli", "Blu"], 0, "it"
  );
  const english = describeRoll(
    event, attack, afterPenalty, ["Gialli", "Blu"], 0, "en"
  );

  assert.match(italian, /RIGORE|dischetto/i);
  assert.match(english, /PENALTY|spot/i);
  assert.equal(afterPenalty.phase, "result");
  assert.equal(afterPenalty.player, attack.player);
});
