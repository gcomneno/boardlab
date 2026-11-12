export const PLAYER_ENRICHMENT_CHECKPOINT_VERSION = 1;

export function createPlayerEnrichmentCheckpoint({
  completedTeamIds,
  snapshots,
}) {
  if (!Array.isArray(completedTeamIds)) {
    throw new TypeError("completedTeamIds must be an array");
  }

  if (!Array.isArray(snapshots)) {
    throw new TypeError("snapshots must be an array");
  }

  return {
    version: PLAYER_ENRICHMENT_CHECKPOINT_VERSION,
    completedTeamIds: completedTeamIds.map(String),
    snapshots: structuredClone(snapshots),
  };
}

export function exportPlayerEnrichmentCheckpoint(checkpoint) {
  return `${JSON.stringify(checkpoint, null, 2)}\n`;
}

export function importPlayerEnrichmentCheckpoint(source) {
  let checkpoint;

  try {
    checkpoint = JSON.parse(source);
  } catch {
    throw new TypeError("checkpoint must be valid JSON");
  }

  if (checkpoint?.version !== PLAYER_ENRICHMENT_CHECKPOINT_VERSION) {
    throw new TypeError("unsupported checkpoint version");
  }

  if (!Array.isArray(checkpoint.completedTeamIds)) {
    throw new TypeError("checkpoint completedTeamIds must be an array");
  }

  if (!Array.isArray(checkpoint.snapshots)) {
    throw new TypeError("checkpoint snapshots must be an array");
  }

  return createPlayerEnrichmentCheckpoint({
    completedTeamIds: checkpoint.completedTeamIds,
    snapshots: checkpoint.snapshots,
  });
}
