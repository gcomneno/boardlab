export const CATALOGUE_SCHEMA_VERSION = 4;

function requireText(value, field) {
  if (typeof value !== "string" || value.trim() === "") {
    throw new TypeError(`${field} must be a non-empty string`);
  }
  return value;
}

function requireColors(value) {
  if (!Array.isArray(value) || value.length < 1 || value.length > 2) {
    throw new TypeError("colors must contain one or two colors");
  }

  return Object.freeze(value.map((color, index) => {
    if (typeof color !== "string" ||
        !/^#[0-9a-f]{6}$/i.test(color)) {
      throw new TypeError(`colors[${index}] must be a hex color`);
    }

    return color.toUpperCase();
  }));
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
  colors,
  logo,
  enrichment = null,
}) {
  return Object.freeze({
    provider: requireText(provider, "provider"),
    providerTeamId: requireProviderId(providerTeamId, "providerTeamId"),
    name: requireText(name, "name"),
    competitionId: requireProviderId(competitionId, "competitionId"),
    season: requireProviderId(season, "season"),
    colors: requireColors(colors),
    logo: requireText(logo, "logo"),
    ...(enrichment === null
      ? {}
      : { enrichment: structuredClone(enrichment) }),
  });
}

export function createPlayer({
  provider,
  providerPlayerId,
  name,
  position,
  shirtNumber = null,
  enrichment = null,
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
    ...(enrichment === null
      ? {}
      : { enrichment: structuredClone(enrichment) }),
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
