import assert from "node:assert/strict";
import test from "node:test";

import { buildFootballCatalogue } from "./import-football-catalogue.mjs";

function response(body) {
  return {
    ok: true,
    status: 200,
    async json() {
      return body;
    },
  };
}

test("paces provider requests and builds the normalized catalogue", async () => {
  const waits = [];
  const requests = [];

  const fetchImpl = async (url) => {
    requests.push(url.pathname);

    if (url.pathname === "/teams") {
      return response({
        errors: [],
        response: [
          { team: { id: 497, name: "AS Roma", logo: "https://example.test/497.png" } },
          { team: { id: 492, name: "SSC Napoli", logo: "https://example.test/492.png" } },
        ],
      });
    }

    const teamId = Number(url.searchParams.get("team"));

    return response({
      errors: [],
      response: [{
        team: { id: teamId },
        players: [{
          id: teamId * 10,
          name: teamId === 497 ? "Roma Player" : "Napoli Player",
          number: 9,
          position: "Attacker",
        }],
      }],
    });
  };

  const catalogue = await buildFootballCatalogue({
    apiKey: "secret",
    competitionId: 135,
    season: 2024,
    fetchImpl,
    wait: async (milliseconds) => waits.push(milliseconds),
    requestIntervalMs: 6500,
  });

  assert.deepEqual(requests, [
    "/teams",
    "/players/squads",
    "/players/squads",
  ]);
  assert.deepEqual(waits, [6500, 6500]);

  assert.equal(catalogue.teams.length, 2);
  assert.equal(catalogue.teams[0].name, "AS Roma");
  assert.equal(catalogue.teams[1].name, "SSC Napoli");
});


test("persists the imported catalogue in D1", async () => {
  let bindings = null;

  const db = {
    prepare() {
      return {
        bind(...values) {
          bindings = values;
          return {
            async run() {
              return { success: true };
            },
          };
        },
      };
    },
  };

  const fetchImpl = async (url) => {
    if (url.pathname === "/teams") {
      return response({
        errors: [],
        response: [
          { team: { id: 497, name: "AS Roma", logo: "https://example.test/497.png" } },
        ],
      });
    }

    return response({
      errors: [],
      response: [{
        team: { id: 497 },
        players: [{
          id: 4970,
          name: "Roma Player",
          number: 10,
          position: "Attacker",
        }],
      }],
    });
  };

  const { importAndPersistFootballCatalogue } =
    await import("./import-football-catalogue.mjs");

  const catalogue = await importAndPersistFootballCatalogue({
    db,
    apiKey: "secret",
    competitionId: 135,
    season: 2024,
    fetchImpl,
    wait: async () => {},
    requestIntervalMs: 0,
  });

  assert.equal(catalogue.teams[0].name, "AS Roma");
  assert.equal(bindings[0], "api-football");
  assert.equal(bindings[1], "135");
  assert.equal(bindings[2], "2024");

  const persisted = JSON.parse(bindings[3]);
  assert.equal(persisted.teams[0].name, "AS Roma");
});
