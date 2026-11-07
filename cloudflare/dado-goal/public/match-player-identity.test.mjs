import assert from "node:assert/strict";
import test from "node:test";

import {
  enrichMatchEvent,
  playerIdentity,
} from "./match-player-identity.mjs";

const roster = {
  active: [7, 10],
  bench: [21],
  identities: {
    7: {
      provider: "api-football",
      providerPlayerId: "700",
      name: "José Ángel",
      position: "Attacker",
      shirtNumber: 7,
    },
    10: {
      provider: "api-football",
      providerPlayerId: "1000",
      name: "Mario Rossi",
      position: "Midfielder",
      shirtNumber: 10,
    },
    21: {
      provider: "api-football",
      providerPlayerId: "2100",
      name: "Luca Bianchi",
      position: "Defender",
      shirtNumber: 21,
    },
  },
};

test("resolves provider-backed identity by match number", () => {
  assert.equal(playerIdentity(roster, 7).name, "José Ángel");
  assert.equal(playerIdentity(roster, 7).providerPlayerId, "700");
});

test("synthetic roster has no external player identity", () => {
  assert.equal(
    playerIdentity({ active: [7], bench: [] }, 7),
    null,
  );
});

test("goal keeps canonical number and adds player identity", () => {
  const event = enrichMatchEvent(roster, {
    type: "goal",
    team: 0,
    minute: 42,
    number: 7,
  });

  assert.equal(event.number, 7);
  assert.equal(event.player.name, "José Ángel");
  assert.equal(event.player.providerPlayerId, "700");
});

test("cards receive player identity without changing event type", () => {
  const yellow = enrichMatchEvent(roster, {
    type: "yellow",
    team: 0,
    minute: 20,
    number: 10,
  });

  const red = enrichMatchEvent(roster, {
    type: "red",
    team: 0,
    minute: 65,
    number: 7,
  });

  assert.equal(yellow.type, "yellow");
  assert.equal(yellow.player.name, "Mario Rossi");
  assert.equal(red.type, "red");
  assert.equal(red.player.name, "José Ángel");
});

test("substitution maps both outgoing and incoming players", () => {
  const event = enrichMatchEvent(roster, {
    type: "substitution",
    team: 0,
    minute: 70,
    out: 10,
    in: 21,
  });

  assert.equal(event.out, 10);
  assert.equal(event.in, 21);
  assert.equal(event.outPlayer.name, "Mario Rossi");
  assert.equal(event.inPlayer.name, "Luca Bianchi");
});

test("synthetic events preserve legacy fields with null identity", () => {
  const event = enrichMatchEvent(
    { active: [9], bench: [] },
    { type: "goal", team: 1, minute: 12, number: 9 },
  );

  assert.equal(event.number, 9);
  assert.equal(event.player, null);
});

test("real player display label includes official name and shirt number", async () => {
  const { playerLabel } = await import("./match-player-identity.mjs");

  assert.equal(
    playerLabel({ name: "José Ángel" }, 7),
    "José Ángel (#7)",
  );
});

test("synthetic player display label remains the shirt number", async () => {
  const { playerLabel } = await import("./match-player-identity.mjs");

  assert.equal(playerLabel(null, 9), "9");
});

test("event display values map real goal identity", async () => {
  const { eventDisplayValues } =
    await import("./match-player-identity.mjs");

  const values = eventDisplayValues({
    type: "goal",
    number: 7,
    player: { name: "José Ángel" },
  });

  assert.equal(values.number, "José Ángel (#7)");
});

test("event display values map both real substitution identities", async () => {
  const { eventDisplayValues } =
    await import("./match-player-identity.mjs");

  const values = eventDisplayValues({
    type: "substitution",
    out: 10,
    in: 21,
    outPlayer: { name: "Mario Rossi" },
    inPlayer: { name: "Luca Bianchi" },
  });

  assert.equal(values.out, "Mario Rossi (#10)");
  assert.equal(values.in, "Luca Bianchi (#21)");
});
