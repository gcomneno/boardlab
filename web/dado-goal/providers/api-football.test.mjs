import assert from "node:assert/strict";
import test from "node:test";

import { createApiFootballAdapter } from "./api-football.mjs";

function response(body, ok = true, status = 200) {
  return {
    ok,
    status,
    async json() {
      return body;
    },
  };
}

test("maps API-Football teams without altering official names", async () => {
  let request;

  const adapter = createApiFootballAdapter({
    apiKey: "secret",
    fetchImpl: async (url, options) => {
      request = { url, options };
      return response({
        errors: [],
        response: [
          {
            team: {
              id: 492,
              name: "SSC Napoli",
              logo: "https://media.api-sports.io/football/teams/492.png",
            },
          },
        ],
      });
    },
  });

  const teams = await adapter.fetchTeams({
    competitionId: 135,
    season: 2026,
  });

  assert.deepEqual(teams, [{
    id: 492,
    name: "SSC Napoli",
    logo: "https://media.api-sports.io/football/teams/492.png",
  }]);
  assert.equal(request.url.searchParams.get("league"), "135");
  assert.equal(request.url.searchParams.get("season"), "2026");
  assert.equal(request.options.headers["x-apisports-key"], "secret");
});

test("maps API-Football squad players", async () => {
  const adapter = createApiFootballAdapter({
    apiKey: "secret",
    fetchImpl: async () => response({
      errors: [],
      response: [{
        team: { id: 492, name: "SSC Napoli" },
        players: [{
          id: 123,
          name: "José Ángel",
          age: 25,
          number: 7,
          position: "Attacker",
          photo: "ignored",
        }],
      }],
    }),
  });

  assert.deepEqual(
    await adapter.fetchPlayers({ teamId: 492 }),
    [{
      id: 123,
      name: "José Ángel",
      position: "Attacker",
      shirtNumber: 7,
    }],
  );
});

test("does not expose the API key in the request URL", async () => {
  let capturedUrl;

  const adapter = createApiFootballAdapter({
    apiKey: "top-secret",
    fetchImpl: async (url) => {
      capturedUrl = url.toString();
      return response({ errors: [], response: [] });
    },
  });

  await adapter.fetchTeams({ competitionId: 135, season: 2026 });

  assert.equal(capturedUrl.includes("top-secret"), false);
});

test("rejects provider errors", async () => {
  const adapter = createApiFootballAdapter({
    apiKey: "secret",
    fetchImpl: async () => response({
      errors: { rateLimit: "quota exceeded" },
      response: [],
    }),
  });

  await assert.rejects(
    adapter.fetchTeams({ competitionId: 135, season: 2026 }),
    /API-Football error/,
  );
});
