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
                    { revision: 5, event_json: '{"type":"roll","seat":1,"die":"result","face":"GOAL!"}' },
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
