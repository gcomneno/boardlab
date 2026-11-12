import {
  CATALOGUE_SCHEMA_VERSION,
  createCatalogue,
  createPlayer,
  createTeam,
} from "./team-catalogue.mjs";

export function exportCatalogueJson(catalogue) {
  if (catalogue?.schemaVersion !== CATALOGUE_SCHEMA_VERSION) {
    throw new TypeError("unsupported catalogue schema");
  }

  return JSON.stringify(catalogue, null, 2);
}

export function importCatalogueJson(source) {
  if (typeof source !== "string") {
    throw new TypeError("catalogue JSON must be a string");
  }

  let raw;
  try {
    raw = JSON.parse(source);
  } catch {
    throw new TypeError("invalid catalogue JSON");
  }

  if (raw?.schemaVersion !== CATALOGUE_SCHEMA_VERSION &&
      raw?.schemaVersion !== 3) {
    throw new TypeError("unsupported catalogue schema");
  }

  if (!Array.isArray(raw.teams)) {
    throw new TypeError("catalogue teams must be an array");
  }

  const teams = raw.teams.map((rawTeam) => {
    const team = createTeam({
      provider: rawTeam.provider,
      providerTeamId: rawTeam.providerTeamId,
      name: rawTeam.name,
      competitionId: rawTeam.competitionId,
      season: rawTeam.season,
      colors: rawTeam.colors,
      logo: rawTeam.logo,
      enrichment: rawTeam.enrichment ?? null,
    });

    if (!Array.isArray(rawTeam.players)) {
      throw new TypeError("team players must be an array");
    }

    return {
      ...team,
      players: rawTeam.players.map((player) => createPlayer({
        provider: player.provider,
        providerPlayerId: player.providerPlayerId,
        name: player.name,
        position: player.position,
        shirtNumber: player.shirtNumber ?? null,
        enrichment: player.enrichment ?? null,
      })),
    };
  });

  return createCatalogue({
    provider: raw.provider,
    competitionId: raw.competitionId,
    season: raw.season,
    importedAt: raw.importedAt,
    teams,
  });
}
