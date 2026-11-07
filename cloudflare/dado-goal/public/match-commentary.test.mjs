import test from "node:test";
import assert from "node:assert/strict";
import { describeMatchEvent } from "./commentary.mjs";

const teams = ["Aquile", "Leoni"];

test("yellow card commentary in Italian and English", () => {
  const event = { type: "yellow", team: 0, minute: 45, number: 48 };
  assert.match(describeMatchEvent(event, teams, "it"), /Ammonito.*48.*Aquile/);
  assert.match(describeMatchEvent(event, teams, "en"), /Yellow card.*48.*Aquile/);
});

test("red card commentary in Italian and English", () => {
  const event = { type: "red", team: 1, minute: 80, number: 9 };
  assert.match(describeMatchEvent(event, teams, "it"), /Espulso.*9.*Leoni/);
  assert.match(describeMatchEvent(event, teams, "en"), /Red card.*9.*Leoni/);
});

test("substitution commentary preserves outgoing and incoming players", () => {
  const event = { type: "substitution", team: 0, minute: 76, out: 7, in: 23 };
  const italian = describeMatchEvent(event, teams, "it");
  const english = describeMatchEvent(event, teams, "en");
  assert.match(italian, /Esce il numero 7, entra il numero 23/);
  assert.match(english, /Number 7 off, number 23 on/);
  assert.notEqual(italian, english);
});

test("disciplinary and injury reasons are rendered bilingually", () => {
  const yellow = {
    type: "yellow",
    team: 0,
    minute: 12,
    number: 8,
    reason: "reckless-challenge",
  };
  const injury = {
    type: "substitution",
    team: 1,
    minute: 18,
    out: 9,
    in: 27,
    reason: "injury",
  };

  assert.match(
    describeMatchEvent(yellow, teams, "it"),
    /intervento imprudente/
  );
  assert.match(
    describeMatchEvent(yellow, teams, "en"),
    /reckless challenge/
  );
  assert.match(
    describeMatchEvent(injury, teams, "it"),
    /infortunio/
  );
  assert.match(
    describeMatchEvent(injury, teams, "en"),
    /injury/
  );
});

test("commentary does not mutate referee events", () => {
  const event = Object.freeze({
    type: "yellow", team: 0, minute: 30, number: 11
  });
  const before = JSON.stringify(event);
  describeMatchEvent(event, teams, "en");
  describeMatchEvent(event, teams, "it");
  assert.equal(JSON.stringify(event), before);
});

test("invalid event is rejected", () => {
  assert.throws(
    () => describeMatchEvent(null, teams, "it"),
    RangeError
  );
});
