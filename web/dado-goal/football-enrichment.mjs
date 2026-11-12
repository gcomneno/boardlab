import { createCatalogue, createTeam } from "./team-catalogue.mjs";

export async function enrichFootballCatalogue({
  catalogue,
  adapter,
  retrievedAt = new Date().toISOString(),
}) {
  if (!catalogue || !Array.isArray(catalogue.teams)) {
    throw new TypeError("catalogue is required");
  }

  if (!adapter ||
      typeof adapter.fetchTeams !== "function" ||
      typeof adapter.name !== "string") {
    throw new TypeError("invalid provider adapter");
  }

  const providerTeams = await adapter.fetchTeams({
    competitionId: catalogue.competitionId,
    season: catalogue.season,
  });

  const byId = new Map(
    providerTeams.map((team) => [String(team.id), team]),
  );

  const teams = catalogue.teams.map((team) => {
    const providerTeam = byId.get(String(team.providerTeamId));

    if (!providerTeam) {
      return team;
    }

    return {
      ...createTeam({
        provider: team.provider,
        providerTeamId: team.providerTeamId,
        name: team.name,
        competitionId: team.competitionId,
        season: team.season,
        colors: team.colors,
        logo: team.logo,
        enrichment: {
          ...(team.enrichment ?? {}),
          code: providerTeam.code ?? null,
          country: providerTeam.country ?? null,
          founded: providerTeam.founded ?? null,
          venue: providerTeam.venue ?? null,
          provenance: {
            provider: adapter.name,
            retrievedAt,
          },
        },
      }),
      players: team.players,
    };
  });

  return createCatalogue({
    provider: catalogue.provider,
    competitionId: catalogue.competitionId,
    season: catalogue.season,
    importedAt: catalogue.importedAt,
    teams,
  });
}
