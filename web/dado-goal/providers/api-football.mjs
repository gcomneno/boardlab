const BASE_URL = "https://v3.football.api-sports.io";

function apiError(payload, response) {
  const details = payload?.errors;

  if (details && (
    (Array.isArray(details) && details.length > 0) ||
    (!Array.isArray(details) && Object.keys(details).length > 0)
  )) {
    return new Error(`API-Football error: ${JSON.stringify(details)}`);
  }

  return new Error(`API-Football HTTP ${response.status}`);
}

export function createApiFootballAdapter({
  apiKey,
  fetchImpl = globalThis.fetch,
}) {
  if (typeof apiKey !== "string" || apiKey.trim() === "") {
    throw new TypeError("API-Football API key is required");
  }

  if (typeof fetchImpl !== "function") {
    throw new TypeError("fetch implementation is required");
  }

  async function request(path, parameters) {
    const url = new URL(path, BASE_URL);

    for (const [key, value] of Object.entries(parameters)) {
      url.searchParams.set(key, String(value));
    }

    const response = await fetchImpl(url, {
      method: "GET",
      headers: { "x-apisports-key": apiKey },
    });

    let payload;
    try {
      payload = await response.json();
    } catch {
      throw new Error("API-Football returned invalid JSON");
    }

    if (!response.ok || (
      payload?.errors &&
      Object.keys(payload.errors).length > 0
    )) {
      throw apiError(payload, response);
    }

    if (!Array.isArray(payload?.response)) {
      throw new Error("API-Football response collection is missing");
    }

    return payload.response;
  }

  return Object.freeze({
    name: "api-football",

    async fetchTeams({ competitionId, season }) {
      const rows = await request("/teams", {
        league: competitionId,
        season,
      });

      return rows.map(({ team }) => ({
        id: team.id,
        name: team.name,
        logo: team.logo,
      }));
    },

    async fetchPlayers({ teamId }) {
      const rows = await request("/players/squads", { team: teamId });
      const squad = rows[0];

      if (!squad || !Array.isArray(squad.players)) {
        return [];
      }

      return squad.players.map((player) => ({
        id: player.id,
        name: player.name,
        position: player.position,
        shirtNumber: player.number ?? null,
      }));
    },
  });
}
