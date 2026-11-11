function canonicalTeamId(value) {
  if (value === null || value === undefined || String(value).trim() === "") {
    throw new TypeError("team id is required");
  }
  return String(value);
}

export function createNextMatch({
  managedProviderTeamId,
  opponentProviderTeamId,
} = {}) {
  const managed = canonicalTeamId(managedProviderTeamId);
  const opponent = canonicalTeamId(opponentProviderTeamId);

  if (managed === opponent) {
    throw new TypeError("managed club cannot play itself");
  }

  return {
    opponentProviderTeamId: opponent,
  };
}

export function validateNextMatch(nextMatch, managedProviderTeamId) {
  if (nextMatch === null) return null;

  if (!nextMatch || typeof nextMatch !== "object" || Array.isArray(nextMatch)) {
    throw new TypeError("invalid next match");
  }

  const keys = Object.keys(nextMatch);
  if (keys.length !== 1 || keys[0] !== "opponentProviderTeamId") {
    throw new TypeError("unsupported next-match state");
  }

  return createNextMatch({
    managedProviderTeamId,
    opponentProviderTeamId: nextMatch.opponentProviderTeamId,
  });
}
