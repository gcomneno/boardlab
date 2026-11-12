import {
  collectCoachSnapshots,
} from "./coach-enrichment-collector.mjs";
import {
  createCoachEnrichmentCheckpoint,
} from "./coach-enrichment-checkpoint.mjs";

export async function runCoachEnrichmentBatch({
  catalogue,
  adapter,
  requestBudget,
  checkpoint = null,
  requestIntervalMs = 6500,
  wait,
}) {
  const completedTeamIds = checkpoint?.completedTeamIds ?? [];
  const existingSnapshots = checkpoint?.snapshots ?? [];

  const result = await collectCoachSnapshots({
    catalogue,
    adapter,
    requestBudget,
    completedTeamIds,
    existingSnapshots,
    requestIntervalMs,
    ...(wait === undefined ? {} : { wait }),
  });

  const nextCheckpoint = createCoachEnrichmentCheckpoint({
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
