import test from "node:test";
import assert from "node:assert/strict";
import { D1Authority, AuthorityError } from "./authority.mjs";
import { initialState } from "./game.mjs";

test("authenticated D1 snapshot aggregates rolls, excludes joins and preserves lastRoll", async () => {
  const token = "abcdefghijklmnopqrstuvwxyz123456";
  const calls = [];

  const db = {
    batch: async () => [],
    prepare(sql) {
      return {
        bind(...args) {
          return {
            async first() {
              calls.push(sql);
              if (sql.includes("FROM lifecycle_matches")) {
                return {
                  revision: 5,
                  status: "active",
                  state_json: JSON.stringify(initialState(6)),
                  expires_at: Math.floor(Date.now() / 1000) + 3600,
                };
              }
              throw new Error("Unexpected first()");
            },
            async all() {
              calls.push(sql);
              if (sql.includes("FROM lifecycle_events")) {
                return {
                  results: [
                    { revision: 1, event_json: '{"type":"joined"}' },
                    { revision: 2, event_json: '{"type":"roll","seat":0,"die":"team","face":"Passaggio"}' },
                    { revision: 3, event_json: '{"type":"roll","seat":0,"die":"result","face":"Parato!"}' },
                    { revision: 4, event_json: '{"type":"roll","seat":1,"die":"team","face":"Cross!"}' },
                    { revision: 5, event_json: '{"type":"roll","seat":1,"die":"result","face":"GOAL!","goal":{"type":"goal","team":1,"minute":15}}' },
                  ],
                };
              }
              throw new Error("Unexpected all()");
            },
          };
        },
      };
    },
  };

  const authority = new D1Authority(db);
  authority.seat = async (_matchId, suppliedToken) => {
    if (suppliedToken !== token) throw new AuthorityError("Unauthorized participant");
    return 0;
  };

  await assert.rejects(
    authority.snapshot("match-1", "invalid"),
    AuthorityError,
  );
  assert.equal(calls.length, 0, "unauthorized requests must not read match data");

  const result = await authority.snapshot("match-1", token);

  assert.equal(result.seat, 0);
  assert.deepEqual(result.rollEvents, [
    { revision: 2, seat: 0, die: "team", face: "Passaggio" },
    { revision: 3, seat: 0, die: "result", face: "Parato!" },
    { revision: 4, seat: 1, die: "team", face: "Cross!" },
    { revision: 5, seat: 1, die: "result", face: "GOAL!",
      goal: { type: "goal", team: 1, minute: 15 } },
  ]);
  assert.equal(result.statistics[0].passes, 1);
  assert.equal(result.statistics[0].saves, 0);
  assert.equal(result.statistics[1].saves, 1);
  assert.equal(result.statistics[1].crosses, 1);
  assert.equal(result.statistics[1].goals, 1);
  assert.deepEqual(result.lastRoll, {
    revision: 5,
    seat: 1,
    die: "result",
    face: "GOAL!",
  });
  assert.equal(calls.length, 2);
});

test("M2D: snapshot JSON preserves yellow-card numbers", async () => {
  const roster = {
    active: [7, 11],
    bench: [23],
    departed: [],
    sentOff: [],
    substitutions: 0,
  };
  const history = [
    {
      revision: 1,
      event_json: JSON.stringify({
        type: "joined",
        rosters: [roster, structuredClone(roster)],
      }),
    },
    {
      revision: 2,
      event_json: JSON.stringify({
        type: "roll",
        seat: 0,
        die: "result",
        face: "Parato!",
        matchEvent: {
          type: "yellow", team: 0, minute: 15, number: 7,
        },
      }),
    },
  ];

  const db = {
    batch: async () => [],
    prepare(sql) {
      return {
        bind() {
          return {
            async first() {
              if (sql.includes("FROM lifecycle_matches")) {
                return {
                  revision: 2,
                  status: "active",
                  state_json: JSON.stringify(initialState(6)),
                  expires_at: Math.floor(Date.now() / 1000) + 3600,
                };
              }
              throw new Error("Unexpected first()");
            },
            async all() {
              if (sql.includes("FROM lifecycle_events")) {
                return { results: history };
              }
              throw new Error("Unexpected all()");
            },
          };
        },
      };
    },
  };

  const authority = new D1Authority(db);
  authority.seat = async () => 0;

  const result = await authority.snapshot("match-m2d", "test-token");
  const serialized = JSON.parse(JSON.stringify(result));

  assert.deepEqual(serialized.matchEventContext.refereeStates, [
    { yellowed: [7] },
    { yellowed: [] },
  ]);
  assert.deepEqual(serialized.matchEventContext.rosters[0], roster);
  assert.deepEqual(serialized.matchEventContext.rosters[1], roster);
  assert.deepEqual(serialized.rollEvents[0].matchEvent, {
    type: "yellow", team: 0, minute: 15, number: 7,
  });
  assert.deepEqual(JSON.parse(history[0].event_json).rosters[0], roster);
});
