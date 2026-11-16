import test from "node:test";
import assert from "node:assert/strict";

import {
  managerMatchParticipation,
} from "./player-participation.mjs";

const club = {
  players: [
    { providerPlayerId: "1", name: "Keeper", shirtNumber: 1 },
    { providerPlayerId: "2", name: "Starter", shirtNumber: 9 },
    { providerPlayerId: "3", name: "Bench", shirtNumber: 18 },
    { providerPlayerId: "4", name: "Unused", shirtNumber: 22 },
    { providerPlayerId: "5", name: "Not selected", shirtNumber: 30 },
  ],
  lineup: ["1", "2"],
  bench: ["3", "4"],
};

test("participation records starters, substitutes and non-selected players", () => {
  const result = managerMatchParticipation(club, 0, [
    {
      type: "substitution",
      team: 0,
      minute: 60,
      out: 2,
      in: 3,
      reason: "tactical",
      outPlayer: { providerPlayerId: "2", name: "Starter" },
      inPlayer: { providerPlayerId: "3", name: "Bench" },
    },
  ]);

  const byId = new Map(
    result.map(player => [player.providerPlayerId, player])
  );

  assert.equal(byId.get("1").selection, "starter");
  assert.equal(byId.get("1").minutesPlayed, 90);

  assert.equal(byId.get("2").enteredMinute, 0);
  assert.equal(byId.get("2").exitedMinute, 60);
  assert.equal(byId.get("2").minutesPlayed, 60);
  assert.equal(byId.get("2").exitReason, "substitution");

  assert.equal(byId.get("3").selection, "bench");
  assert.equal(byId.get("3").enteredMinute, 60);
  assert.equal(byId.get("3").minutesPlayed, 30);

  assert.equal(byId.get("4").enteredMinute, null);
  assert.equal(byId.get("4").minutesPlayed, 0);

  assert.equal(byId.get("5").selection, "not-selected");
  assert.equal(byId.get("5").enteredMinute, null);
});

test("injury substitution preserves its canonical exit reason", () => {
  const result = managerMatchParticipation(club, 0, [
    {
      type: "substitution",
      team: 0,
      minute: 17,
      out: 2,
      in: 3,
      reason: "injury",
    },
  ]);

  const starter = result.find(
    player => player.providerPlayerId === "2"
  );

  assert.equal(starter.minutesPlayed, 17);
  assert.equal(starter.exitReason, "injury");
});

test("red card terminates participation at the event minute", () => {
  const result = managerMatchParticipation(club, 0, [
    {
      type: "red",
      team: 0,
      minute: 72,
      number: 2,
      player: { providerPlayerId: "2", name: "Starter" },
    },
  ]);

  const starter = result.find(
    player => player.providerPlayerId === "2"
  );

  assert.equal(starter.exitedMinute, 72);
  assert.equal(starter.minutesPlayed, 72);
  assert.equal(starter.exitReason, "red");
});

test("opponent events never affect managed participation", () => {
  const result = managerMatchParticipation(club, 0, [
    {
      type: "red",
      team: 1,
      minute: 12,
      number: 2,
    },
  ]);

  const starter = result.find(
    player => player.providerPlayerId === "2"
  );

  assert.equal(starter.minutesPlayed, 90);
  assert.equal(starter.exitReason, null);
});
