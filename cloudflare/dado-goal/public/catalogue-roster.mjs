import { createRoster } from "./match-roster.mjs";

function requireTeam(team) {
  if (!team || !Array.isArray(team.players)) {
    throw new TypeError("catalogue team with players is required");
  }

  if (team.players.length < 11) {
    throw new RangeError("catalogue team requires at least 11 players");
  }
}

export function createCatalogueRoster(team) {
  requireTeam(team);

  const numbered = team.players.map((player, index) => ({
    ...player,
    matchNumber: index + 1,
  }));

  const activePlayers = numbered.slice(0, 11);
  const benchPlayers = numbered.slice(11, 20);

  return {
    active: activePlayers.map((player) => player.matchNumber),
    bench: benchPlayers.map((player) => player.matchNumber),
    departed: [],
    sentOff: [],
    substitutesIn: [],
    substitutions: 0,
    identities: Object.fromEntries(
      numbered.map((player) => [
        player.matchNumber,
        {
          provider: player.provider,
          providerPlayerId: player.providerPlayerId,
          name: player.name,
          position: player.position,
          shirtNumber: player.shirtNumber,
        },
      ]),
    ),
  };
}

export function createMatchRoster({ team = null, random = Math.random } = {}) {
  if (team === null) {
    return createRoster(random);
  }

  return createCatalogueRoster(team);
}
