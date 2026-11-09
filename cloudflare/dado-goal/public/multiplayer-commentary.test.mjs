import test from "node:test";
import assert from "node:assert/strict";

import {
  describeMultiplayerRollEvent,
  multiplayerMatchEvents,
} from "./multiplayer-commentary.mjs";

test("multiplayer penalty is always explicitly announced in commentary", () => {
  const roll = {
    revision: 12,
    seat: 0,
    die: "result",
    face: "RIGORE",
  };

  const it = describeMultiplayerRollEvent(
    roll,
    ["Giocatore 1", "Giocatore 2"],
    "it",
  ).join(" ");

  const en = describeMultiplayerRollEvent(
    roll,
    ["Player 1", "Player 2"],
    "en",
  ).join(" ");

  assert.match(it, /rigore|dischetto/i);
  assert.match(en, /penalty|spot/i);
});

test("multiplayer penalty announcement does not depend on its later outcome", () => {
  const penalty = describeMultiplayerRollEvent({
    revision: 20,
    seat: 1,
    die: "result",
    face: "RIGORE",
  });

  const laterOutcome = describeMultiplayerRollEvent({
    revision: 21,
    seat: 1,
    die: "result",
    face: "GOAL!",
  });

  assert.equal(penalty.length, 1);
  assert.equal(laterOutcome.length, 0);
});

test("authoritative multiplayer goal is announced in IT and EN", () => {
  const roll = {
    revision: 7,
    seat: 1,
    die: "result",
    face: "GOAL!",
    goal: {
      type: "goal",
      team: 1,
      minute: 15,
    },
  };

  const teamsIt = ["Giocatore 1", "Giocatore 2"];
  const teamsEn = ["Player 1", "Player 2"];

  const it = describeMultiplayerRollEvent(roll, teamsIt, "it");
  const en = describeMultiplayerRollEvent(roll, teamsEn, "en");

  assert.equal(it.length, 1);
  assert.match(it[0], /15'.*GOAL.*Segna Giocatore 2/i);

  assert.equal(en.length, 1);
  assert.match(en[0], /15'.*GOAL.*Player 2 scores/i);
});

test("multiplayer commentary rejects mismatched goal ownership", () => {
  const roll = {
    revision: 7,
    seat: 0,
    die: "result",
    face: "GOAL!",
    goal: {
      type: "goal",
      team: 1,
      minute: 15,
    },
  };

  assert.deepEqual(
    describeMultiplayerRollEvent(
      roll,
      ["Giocatore 1", "Giocatore 2"],
      "it",
    ),
    [],
  );
});

test("multiplayer match event projection includes ordinary authoritative goals", () => {
  const events = multiplayerMatchEvents([
    {
      type: "roll",
      seat: 0,
      die: "result",
      face: "GOAL!",
      goal: {
        type: "goal",
        team: 0,
        minute: 23,
        number: 9,
      },
    },
  ]);

  assert.deepEqual(events, [{
    type: "goal",
    team: 0,
    minute: 23,
    number: 9,
  }]);
});

test("multiplayer match event projection does not duplicate a scored penalty", () => {
  const penalty = {
    type: "penalty",
    team: 1,
    minute: 64,
    outcome: "GOAL!",
    number: 10,
  };

  const events = multiplayerMatchEvents([
    {
      type: "roll",
      seat: 1,
      die: "result",
      face: "GOAL!",
      penalty,
      goal: {
        type: "goal",
        team: 1,
        minute: 64,
        number: 10,
      },
    },
  ]);

  assert.deepEqual(events, [penalty]);
});
