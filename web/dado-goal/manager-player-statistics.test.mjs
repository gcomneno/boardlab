import test from "node:test";
import assert from "node:assert/strict";

import {
  managerPlayerStatistics,
} from "./player-statistics.mjs";

const roster = [
  {
    providerPlayerId: 101,
    name: "Portiere",
    shirtNumber: 1,
  },
  {
    providerPlayerId: 109,
    name: "Attaccante",
    shirtNumber: 9,
  },
  {
    providerPlayerId: 118,
    name: "Riserva",
    shirtNumber: 18,
  },
];

function player(id, name) {
  return {
    providerPlayerId: id,
    name,
  };
}

test("Manager statistics aggregate canonical events across home and away matches", () => {
  const matches = [
    {
      home: { providerTeamId: 10 },
      away: { providerTeamId: 20 },
      events: [
        {
          type: "goal",
          team: 0,
          number: 9,
          player: player(109, "Attaccante"),
        },
        {
          type: "yellow",
          team: 0,
          number: 9,
          player: player(109, "Attaccante"),
        },
        {
          type: "goal",
          team: 1,
          number: 7,
          player: player(207, "Avversario"),
        },
      ],
    },
    {
      home: { providerTeamId: 30 },
      away: { providerTeamId: 10 },
      events: [
        {
          type: "penalty",
          team: 1,
          number: 9,
          outcome: "GOAL!",
          player: player(109, "Attaccante"),
        },
        {
          type: "substitution",
          team: 1,
          out: 9,
          in: 18,
          reason: "injury",
          outPlayer: player(109, "Attaccante"),
          inPlayer: player(118, "Riserva"),
        },
      ],
    },
  ];

  const stats = managerPlayerStatistics(matches, 10, roster);
  const striker = stats.find(player => player.providerPlayerId === 109);
  const reserve = stats.find(player => player.providerPlayerId === 118);

  assert.deepEqual(
    {
      goals: striker.goals,
      penalties: striker.penalties,
      yellowCards: striker.yellowCards,
      redCards: striker.redCards,
      subbedOn: striker.subbedOn,
      subbedOff: striker.subbedOff,
      injurySubbedOff: striker.injurySubbedOff,
    },
    {
      goals: 2,
      penalties: 1,
      yellowCards: 1,
      redCards: 0,
      subbedOn: 0,
      subbedOff: 1,
      injurySubbedOff: 1,
    },
  );

  assert.equal(reserve.subbedOn, 1);
});

test("Manager statistics keep the whole managed roster visible at zero", () => {
  const stats = managerPlayerStatistics([], 10, roster);

  assert.equal(stats.length, 3);
  assert.deepEqual(
    stats.map(player => [
      player.providerPlayerId,
      player.goals,
      player.penalties,
      player.yellowCards,
      player.redCards,
      player.subbedOn,
      player.subbedOff,
      player.injurySubbedOff,
    ]),
    [
      [101, 0, 0, 0, 0, 0, 0, 0],
      [109, 0, 0, 0, 0, 0, 0, 0],
      [118, 0, 0, 0, 0, 0, 0, 0],
    ],
  );
});

test("Manager statistics ignore matches and events belonging to other clubs", () => {
  const matches = [
    {
      home: { providerTeamId: 20 },
      away: { providerTeamId: 30 },
      events: [
        {
          type: "goal",
          team: 0,
          number: 9,
          player: player(109, "Attaccante"),
        },
      ],
    },
  ];

  const stats = managerPlayerStatistics(matches, 10, roster);
  assert.equal(
    stats.find(player => player.providerPlayerId === 109).goals,
    0,
  );
});

test("unsupported player metrics are not inferred from team roll statistics", () => {
  const matches = [
    {
      home: { providerTeamId: 10 },
      away: { providerTeamId: 20 },
      rollEvents: [
        {
          seat: 0,
          die: "team",
          kind: "shot",
          face: "Tiro in porta!",
        },
      ],
      events: [],
    },
  ];

  const stats = managerPlayerStatistics(matches, 10, roster);

  assert.equal(
    Object.hasOwn(stats[0], "shots"),
    false,
  );
  assert.equal(stats[0].appearances, 0);
  assert.equal(stats[0].minutesPlayed, 0);
});
