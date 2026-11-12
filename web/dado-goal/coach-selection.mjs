function requireProviderId(value, field) {
  if ((typeof value !== "string" && typeof value !== "number") ||
      String(value).trim() === "") {
    throw new TypeError(`${field} must be a provider identifier`);
  }

  return String(value);
}

export function selectLatestTeamCoach(coaches, teamId) {
  if (!Array.isArray(coaches)) {
    throw new TypeError("coaches must be an array");
  }

  const normalizedTeamId = requireProviderId(teamId, "teamId");

  const candidates = coaches.flatMap((coach) => {
    if (!Array.isArray(coach?.career)) {
      return [];
    }

    return coach.career
      .filter((entry) =>
        String(entry?.team?.id ?? "") === normalizedTeamId &&
        typeof entry?.start === "string" &&
        entry.start !== "")
      .map((entry) => ({
        coach,
        start: entry.start,
      }));
  });

  candidates.sort((left, right) =>
    right.start.localeCompare(left.start) ||
    String(left.coach.id).localeCompare(String(right.coach.id)));

  return candidates[0]?.coach ?? null;
}
