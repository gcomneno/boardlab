import { createCatalogue, createPlayer, createTeam } from "./team-catalogue.mjs";
import { socialColors } from "./team-colors.mjs";

export async function importFootballCatalogue({
  adapter,
  competitionId,
  season,
  importedAt = new Date().toISOString(),
}) {
  if (!adapter || typeof adapter.fetchTeams !== "function" ||
      typeof adapter.fetchPlayers !== "function") {
    throw new TypeError("invalid provider adapter");
  }

  const rawTeams = await adapter.fetchTeams({ competitionId, season });
  if (!Array.isArray(rawTeams)) {
    throw new TypeError("fetchTeams() must return an array");
  }

  const teams = [];

  for (const rawTeam of rawTeams) {
    const team = createTeam({
      provider: adapter.name,
      providerTeamId: rawTeam.id,
      name: rawTeam.name,
      competitionId,
      season,
      colors: socialColors(rawTeam.id),
      logo: rawTeam.logo,
    });

    const rawPlayers = await adapter.fetchPlayers({
      teamId: rawTeam.id,
      season,
    });

    if (!Array.isArray(rawPlayers)) {
      throw new TypeError("fetchPlayers() must return an array");
    }

    teams.push(Object.freeze({
      ...team,
      players: rawPlayers.map((player) => createPlayer({
        provider: adapter.name,
        providerPlayerId: player.id,
        name: player.name,
        position: player.position,
        shirtNumber: player.shirtNumber ?? null,
      })),
    }));
  }

  return createCatalogue({
    provider: adapter.name,
    competitionId,
    season,
    importedAt,
    teams,
  });
}
