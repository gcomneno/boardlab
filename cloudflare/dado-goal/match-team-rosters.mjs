import {
  createCatalogueRoster,
} from "../../web/dado-goal/catalogue-roster.mjs";

import {
  getFootballCatalogue,
} from "./football-catalogue-d1.mjs";

function requireSelection(selection) {
  if (
    !selection ||
    typeof selection !== "object" ||
    Array.isArray(selection)
  ) {
    throw new TypeError("Invalid team selection");
  }

  for (const field of [
    "provider",
    "competitionId",
    "season",
    "providerTeamId",
  ]) {
    if (
      typeof selection[field] !== "string" ||
      selection[field].length === 0
    ) {
      throw new TypeError("Invalid team selection");
    }
  }

  return selection;
}

export async function resolveMatchTeam(db, selection) {
  const selected = requireSelection(selection);

  const catalogue = await getFootballCatalogue(db, {
    provider: selected.provider,
    competitionId: selected.competitionId,
    season: selected.season,
  });

  if (!catalogue || !Array.isArray(catalogue.teams)) {
    throw new RangeError("Unknown football catalogue");
  }

  const team = catalogue.teams.find(candidate =>
    String(candidate.providerTeamId) === selected.providerTeamId
  );

  if (!team) {
    throw new RangeError("Unknown football team");
  }

  return team;
}

export async function createAuthoritativeMatchRosters(db, selections) {
  if (!Array.isArray(selections) || selections.length !== 2) {
    throw new TypeError("Exactly two team selections are required");
  }

  const teams = await Promise.all(
    selections.map(selection => resolveMatchTeam(db, selection)),
  );

  if (
    teams[0].provider === teams[1].provider &&
    String(teams[0].providerTeamId) ===
      String(teams[1].providerTeamId)
  ) {
    throw new RangeError("Teams must be different");
  }

  return teams.map(team => createCatalogueRoster(team));
}
