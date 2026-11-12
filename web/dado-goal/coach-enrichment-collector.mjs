function requirePositiveInteger(value, field) {
  if (!Number.isInteger(value) || value < 1) {
    throw new TypeError(`${field} must be a positive integer`);
  }
  return value;
}

export async function collectCoachSnapshots({
  catalogue,
  adapter,
  requestBudget,
  completedTeamIds = [],
  existingSnapshots = [],
  requestIntervalMs = 6500,
  wait = (milliseconds) =>
    new Promise((resolve) => setTimeout(resolve, milliseconds)),
}) {
  if (!catalogue || !Array.isArray(catalogue.teams)) {
    throw new TypeError("catalogue teams must be an array");
  }

  if (!adapter || typeof adapter.fetchCoaches !== "function") {
    throw new TypeError("adapter.fetchCoaches is required");
  }

  requirePositiveInteger(requestBudget, "requestBudget");

  if (!Number.isInteger(requestIntervalMs) || requestIntervalMs < 0) {
    throw new TypeError("requestIntervalMs must be a non-negative integer");
  }

  if (typeof wait !== "function") {
    throw new TypeError("wait must be a function");
  }

  if (!Array.isArray(completedTeamIds)) {
    throw new TypeError("completedTeamIds must be an array");
  }

  if (!Array.isArray(existingSnapshots)) {
    throw new TypeError("existingSnapshots must be an array");
  }

  const completed = new Set(completedTeamIds.map(String));
  const snapshots = structuredClone(existingSnapshots);
  const processedTeamIds = [];
  let requests = 0;

  for (const team of catalogue.teams) {
    const teamId = String(team.providerTeamId);

    if (completed.has(teamId)) {
      continue;
    }

    if (requests >= requestBudget) {
      break;
    }

    if (requests > 0 && requestIntervalMs > 0) {
      await wait(requestIntervalMs);
    }

    const coaches = await adapter.fetchCoaches({ teamId });
    requests += 1;

    if (!Array.isArray(coaches)) {
      throw new TypeError(
        `adapter.fetchCoaches must return an array for team ${teamId}`,
      );
    }

    for (const coach of coaches) {
      snapshots.push({
        providerCoachId: coach.id,
        ...structuredClone(coach),
        providerTeamId: teamId,
      });
    }

    processedTeamIds.push(teamId);
  }

  const remainingTeamIds = catalogue.teams
    .map((team) => String(team.providerTeamId))
    .filter((teamId) =>
      !completed.has(teamId) && !processedTeamIds.includes(teamId));

  return Object.freeze({
    snapshots,
    processedTeamIds: Object.freeze(processedTeamIds),
    remainingTeamIds: Object.freeze(remainingTeamIds),
    requests,
    complete: remainingTeamIds.length === 0,
  });
}
