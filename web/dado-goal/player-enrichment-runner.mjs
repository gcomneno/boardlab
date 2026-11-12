import {
  collectPlayerSnapshots,
} from "./player-enrichment-collector.mjs";
import {
  createPlayerEnrichmentCheckpoint,
} from "./player-enrichment-checkpoint.mjs";

export async function runPlayerEnrichmentBatch({
  catalogue,
  adapter,
  requestBudget,
  checkpoint = null,
  requestIntervalMs = 6500,
  wait,
}) {
  const completedTeamIds = checkpoint?.completedTeamIds ?? [];
  const existingSnapshots = checkpoint?.snapshots ?? [];

  const result = await collectPlayerSnapshots({
    catalogue,
    adapter,
    requestBudget,
    completedTeamIds,
    existingSnapshots,
    requestIntervalMs,
    ...(wait === undefined ? {} : { wait }),
  });

  const nextCheckpoint = createPlayerEnrichmentCheckpoint({
    completedTeamIds: [
      ...completedTeamIds,
      ...result.processedTeamIds,
    ],
    snapshots: result.snapshots,
  });

  return Object.freeze({
    checkpoint: nextCheckpoint,
    requests: result.requests,
    remainingTeamIds: result.remainingTeamIds,
    complete: result.complete,
  });
}
