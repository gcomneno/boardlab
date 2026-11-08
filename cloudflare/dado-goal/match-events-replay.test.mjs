import test from "node:test";
import assert from "node:assert/strict";

import { createRoster } from "../../web/dado-goal/match-roster.mjs";
import { evaluateExtraordinaryEvent } from "../../web/dado-goal/extraordinary-events.mjs";
import { replayMatchEvents } from "./match-events-replay.mjs";

test("#69: replay restores extraordinary family and type caps after reconnect", () => {
  const rosters = [createRoster(() => 0.5), createRoster(() => 0.5)];

  const history = [
    {
      type: "joined",
      rosters,
    },
    {
      type: "roll",
      seat: 0,
      matchEvent: {
        type: "heavy-rain",
        family: "weather",
        classification: "narrative",
        minute: 20,
        team: 0,
      },
    },
  ];

  const context = replayMatchEvents(history);

  assert.ok(context.extraordinaryState.occurredTypes.has("heavy-rain"));
  assert.ok(context.extraordinaryState.occurredFamilies.has("weather"));

  const before = structuredClone(context.rosters);

  // With weather already capped, draw 0 must select the first still-eligible
  // event: light-knock. Heavy rain must never be emitted again.
  const event = evaluateExtraordinaryEvent(
    context.rosters[0],
    context.extraordinaryState,
    30,
    () => 0,
  );

  assert.equal(event.type, "light-knock");
  assert.notEqual(event.type, "heavy-rain");
  assert.deepEqual(context.rosters, before);
});

test("#69: ordinary match event has priority over extraordinary draw", async () => {
  const { newMatch, applyRoll } =
    await import("../../web/dado-goal/engine.mjs");
  const {
    createMatchEventContext,
    evaluateCompletedHand,
  } = await import("./match-events-authority.mjs");

  let before = newMatch(5);
  before = applyRoll(before, "Tiro in porta!");
  before = applyRoll(before, "Mezza rovesciata");

  const after = applyRoll(before, "Parato!");
  const context = createMatchEventContext(() => 0.5);

  /*
   * A zero draw makes the ordinary event evaluator emit first.
   * The same draw would also make an extraordinary event eligible if
   * evaluation reached that stage. The ordinary event must therefore win.
   */
  const event = evaluateCompletedHand(before, after, context, () => 0);

  assert.ok(event);
  assert.ok(
    ["yellow", "red", "substitution", "injury"].includes(event.type),
    `expected ordinary event, got ${event.type}`
  );
  assert.equal(event.team, before.player);
  assert.equal(context.extraordinaryState.occurredTypes.size, 0);
  assert.equal(context.extraordinaryState.occurredFamilies.size, 0);
});

test("#69: authoritative real goalkeeper substitution preserves position through replay", async () => {
  const { evaluateCompletedHand } =
    await import("./match-events-authority.mjs");

  const roster = {
    active: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
    bench: [12, 13],
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
    identities: {
      1: { position: "Goalkeeper" },
      2: { position: "Defender" },
      3: { position: "Defender" },
      4: { position: "Defender" },
      5: { position: "Defender" },
      6: { position: "Midfielder" },
      7: { position: "Midfielder" },
      8: { position: "Midfielder" },
      9: { position: "Attacker" },
      10: { position: "Attacker" },
      11: { position: "Attacker" },
      12: { position: "Goalkeeper" },
      13: { position: "Attacker" },
    },
  };

  const context = replayMatchEvents([
    {
      type: "joined",
      rosters: [structuredClone(roster), structuredClone(roster)],
    },
  ]);

  const before = {
    handsPerTeam: 10,
    hands: [8, 8],
    player: 0,
    phase: "team",
  };
  const after = {
    ...before,
    hands: [9, 8],
    player: 1,
  };

  const draws = [0.10, 0, 0];
  const event = evaluateCompletedHand(
    before,
    after,
    context,
    () => draws.shift() ?? 0,
  );

  assert.ok(event);
  assert.ok(["substitution", "injury"].includes(event.type));
  assert.equal(event.team, 0);
  assert.equal(event.out, 1);
  assert.equal(event.in, 12);
  assert.equal(context.rosters[0].identities[event.out].position, "Goalkeeper");
  assert.equal(context.rosters[0].identities[event.in].position, "Goalkeeper");

  const replayed = replayMatchEvents([
    {
      type: "joined",
      rosters: [structuredClone(roster), structuredClone(roster)],
    },
    {
      type: "roll",
      seat: 0,
      matchEvent: event,
    },
  ]);

  assert.ok(!replayed.rosters[0].active.includes(1));
  assert.ok(replayed.rosters[0].active.includes(12));
  assert.equal(replayed.rosters[0].identities[12].position, "Goalkeeper");
});

test("#69: replay rejects goalkeeper replacement by an outfield player", () => {
  const roster = {
    active: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
    bench: [12, 13],
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
    identities: {
      1: { position: "Goalkeeper" },
      2: { position: "Defender" },
      3: { position: "Defender" },
      4: { position: "Defender" },
      5: { position: "Defender" },
      6: { position: "Midfielder" },
      7: { position: "Midfielder" },
      8: { position: "Midfielder" },
      9: { position: "Attacker" },
      10: { position: "Attacker" },
      11: { position: "Attacker" },
      12: { position: "Goalkeeper" },
      13: { position: "Attacker" },
    },
  };

  assert.throws(
    () => replayMatchEvents([
      {
        type: "joined",
        rosters: [structuredClone(roster), structuredClone(roster)],
      },
      {
        type: "roll",
        seat: 0,
        matchEvent: {
          type: "substitution",
          team: 0,
          minute: 76,
          out: 1,
          in: 13,
          reason: "tactical",
        },
      },
    ]),
    /Invalid goalkeeper substitution replay/,
  );
});

test("#69: multiplayer authority emits narrative VAR only when eligible", async () => {
  const {
    createMatchEventContext,
    evaluateCompletedHand,
  } = await import("./match-events-authority.mjs");

  const before = {
    handsPerTeam: 10,
    hands: [4, 4],
    player: 0,
    phase: "result",
  };
  const after = {
    ...before,
    hands: [5, 4],
    player: 1,
  };

  const context = createMatchEventContext(() => 0.5);
  const draws = [0.999, 0.0125];

  const event = evaluateCompletedHand(
    before,
    after,
    context,
    () => draws.shift() ?? 0.999,
    { varEligible: true },
  );

  assert.ok(event);
  assert.equal(event.type, "var-review");
  assert.equal(event.family, "technical");
  assert.equal(event.classification, "narrative");
  assert.equal(event.team, 0);
});

test("#69: multiplayer authority suppresses VAR when preceding action is not eligible", async () => {
  const {
    createMatchEventContext,
    evaluateCompletedHand,
  } = await import("./match-events-authority.mjs");

  const before = {
    handsPerTeam: 10,
    hands: [4, 4],
    player: 0,
    phase: "result",
  };
  const after = {
    ...before,
    hands: [5, 4],
    player: 1,
  };

  const context = createMatchEventContext(() => 0.5);
  const draws = [0.999, 0.0125];

  const event = evaluateCompletedHand(
    before,
    after,
    context,
    () => draws.shift() ?? 0.999,
  );

  assert.equal(event, null);
  assert.equal(
    context.extraordinaryState.occurredTypes.has("var-review"),
    false,
  );
});
