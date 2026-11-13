import { selectLatestTeamCoach } from "./coach-selection.mjs";

export function finalizeCoachEnrichment({
  catalogue,
  snapshots,
  retrievedAt,
}) {
  if (!catalogue || !Array.isArray(catalogue.teams)) {
    throw new TypeError("catalogue.teams must be an array");
  }
  if (!Array.isArray(snapshots)) {
    throw new TypeError("snapshots must be an array");
  }
  if (typeof retrievedAt !== "string" || retrievedAt.length === 0) {
    throw new TypeError("retrievedAt must be a non-empty string");
  }

  const byTeam = new Map();

  for (const snapshot of snapshots) {
    const teamId = String(snapshot.providerTeamId);
    if (!byTeam.has(teamId)) {
      byTeam.set(teamId, []);
    }
    byTeam.get(teamId).push(snapshot);
  }

  let selected = 0;
  let unresolved = 0;

  const teams = catalogue.teams.map((team) => {
    const candidates = byTeam.get(String(team.providerTeamId)) ?? [];
    const coach = selectLatestTeamCoach(candidates, team.providerTeamId);

    if (coach === null) {
      unresolved += 1;
      return team;
    }

    selected += 1;

    const coachEnrichment = {
      provider: "api-football",
      providerCoachId: coach.id,
      name: coach.name,
      firstname: coach.firstname,
      lastname: coach.lastname,
      age: coach.age,
      nationality: coach.nationality,
      photo: coach.photo,
      career: coach.career,
      provenance: {
        provider: "api-football",
        retrievedAt,
      },
    };

    return {
      ...team,
      enrichment: {
        ...(team.enrichment ?? {}),
        coach: coachEnrichment,
      },
    };
  });

  return {
    catalogue: {
      ...catalogue,
      teams,
    },
    stats: {
      teams: teams.length,
      selected,
      unresolved,
    },
  };
}
