export const CATALOGUE_SCHEMA_VERSION = 1;

function requireText(value, field) {
  if (typeof value !== "string" || value.trim() === "") {
    throw new TypeError(`${field} must be a non-empty string`);
  }
  return value;
}

function requireProviderId(value, field) {
  if ((typeof value !== "string" && typeof value !== "number") ||
      String(value).trim() === "") {
    throw new TypeError(`${field} must be a provider identifier`);
  }
  return String(value);
}

export function createTeam({
  provider,
  providerTeamId,
  name,
  competitionId,
  season,
}) {
  return Object.freeze({
    provider: requireText(provider, "provider"),
    providerTeamId: requireProviderId(providerTeamId, "providerTeamId"),
    name: requireText(name, "name"),
    competitionId: requireProviderId(competitionId, "competitionId"),
    season: requireProviderId(season, "season"),
  });
}

export function createPlayer({
  provider,
  providerPlayerId,
  name,
  position,
  shirtNumber = null,
}) {
  if (shirtNumber !== null &&
      (!Number.isInteger(shirtNumber) || shirtNumber <= 0)) {
    throw new TypeError("shirtNumber must be a positive integer or null");
  }

  return Object.freeze({
    provider: requireText(provider, "provider"),
    providerPlayerId: requireProviderId(
      providerPlayerId,
      "providerPlayerId",
    ),
    name: requireText(name, "name"),
    position: requireText(position, "position"),
    shirtNumber,
  });
}

export function createCatalogue({
  provider,
  competitionId,
  season,
  importedAt,
  teams,
}) {
  requireText(importedAt, "importedAt");

  if (!Array.isArray(teams)) {
    throw new TypeError("teams must be an array");
  }

  return Object.freeze({
    schemaVersion: CATALOGUE_SCHEMA_VERSION,
    provider: requireText(provider, "provider"),
    competitionId: requireProviderId(competitionId, "competitionId"),
    season: requireProviderId(season, "season"),
    importedAt,
    teams,
  });
}
