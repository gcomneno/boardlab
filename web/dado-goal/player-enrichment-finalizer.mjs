import { exportCatalogueJson } from "./catalogue-json.mjs";
import { enrichCataloguePlayers } from "./player-enrichment.mjs";

function requireCompleteCheckpoint(catalogue, checkpoint) {
  if (!Array.isArray(checkpoint?.completedTeamIds)) {
    throw new TypeError("checkpoint completedTeamIds must be an array");
  }

  if (!Array.isArray(checkpoint?.snapshots)) {
    throw new TypeError("checkpoint snapshots must be an array");
  }

  const completed = new Set(checkpoint.completedTeamIds.map(String));

  const missing = catalogue.teams
    .map((team) => String(team.providerTeamId))
    .filter((teamId) => !completed.has(teamId));

  if (missing.length > 0) {
    throw new TypeError(
      `incomplete player enrichment checkpoint: ${missing.join(",")}`,
    );
  }
}

export function finalizePlayerEnrichment({
  catalogue,
  checkpoint,
  provider,
  retrievedAt,
}) {
  requireCompleteCheckpoint(catalogue, checkpoint);

  const result = enrichCataloguePlayers({
    catalogue,
    snapshots: checkpoint.snapshots,
    provider,
    retrievedAt,
  });

  return {
    ...result,
    json: `${exportCatalogueJson(result.catalogue)}\n`,
  };
}
