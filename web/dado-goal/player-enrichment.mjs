function requireSnapshot(snapshot) {
  if (!snapshot ||
      (typeof snapshot.providerPlayerId !== "string" &&
       typeof snapshot.providerPlayerId !== "number")) {
    throw new TypeError("snapshot providerPlayerId is required");
  }

  return snapshot;
}

function snapshotEnrichment(snapshot, { provider, retrievedAt }) {
  const {
    providerPlayerId: _providerPlayerId,
    id: _id,
    name: _name,
    position: _position,
    number: _number,
    shirtNumber: _shirtNumber,
    ...details
  } = snapshot;

  return {
    ...structuredClone(details),
    provenance: {
      provider,
      retrievedAt,
    },
  };
}

export function enrichCataloguePlayers({
  catalogue,
  snapshots,
  provider,
  retrievedAt,
}) {
  if (!catalogue || !Array.isArray(catalogue.teams)) {
    throw new TypeError("catalogue teams must be an array");
  }

  if (!Array.isArray(snapshots)) {
    throw new TypeError("snapshots must be an array");
  }

  if (typeof provider !== "string" || provider.trim() === "") {
    throw new TypeError("provider must be a non-empty string");
  }

  if (typeof retrievedAt !== "string" || retrievedAt.trim() === "") {
    throw new TypeError("retrievedAt must be a non-empty string");
  }

  const byPlayerKey = new Map();

  for (const rawSnapshot of snapshots) {
    const snapshot = requireSnapshot(rawSnapshot);
    const id = String(snapshot.providerPlayerId);
    const teamId = String(snapshot.providerTeamId);
    const key = `${teamId}:${id}`;

    if (byPlayerKey.has(key)) {
      throw new TypeError(`duplicate player snapshot: ${key}`);
    }

    byPlayerKey.set(key, snapshot);
  }

  const matched = new Set();

  const teams = catalogue.teams.map((team) => ({
    ...team,
    players: team.players.map((player) => {
      const id = String(player.providerPlayerId);
      const key = `${String(team.providerTeamId)}:${id}`;
      const snapshot = byPlayerKey.get(key);

      if (!snapshot) {
        return player;
      }

      matched.add(key);

      return {
        ...player,
        enrichment: snapshotEnrichment(snapshot, {
          provider,
          retrievedAt,
        }),
      };
    }),
  }));

  return {
    catalogue: {
      ...catalogue,
      teams,
    },
    stats: Object.freeze({
      snapshots: snapshots.length,
      matched: matched.size,
      unmatched: snapshots.length - matched.size,
    }),
  };
}
