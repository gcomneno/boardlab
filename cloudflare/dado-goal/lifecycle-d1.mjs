/**
 * D1 persistence primitives for match creation and guest admission.
 *
 * All IDs, tokens, digests, timestamps and game states are supplied
 * by the authoritative application layer.
 *
 * No client-provided state is trusted here.
 */

function requireBatch(results, count) {
  if (
    !Array.isArray(results) ||
    results.length !== count ||
    results.some(result => result.success !== true)
  ) {
    throw new Error("Invalid D1 batch result");
  }
  return results.map(result => result.meta?.changes);
}

/**
 * Create a waiting match and its host seat atomically.
 *
 * The two inserts share one D1 transaction. A failed insert
 * rolls back the entire batch.
 */
export async function createMatch(db, input) {
  const {
    matchId,
    stateJson,
    inviteDigest,
    hostDigest,
    createdAt,
    expiresAt,
    teamSelections = null,
  } = input;

  if (
    !matchId ||
    typeof stateJson !== "string" ||
    !inviteDigest ||
    !hostDigest ||
    !Number.isSafeInteger(createdAt) ||
    !Number.isSafeInteger(expiresAt) ||
    expiresAt <= createdAt ||
    (
      teamSelections !== null &&
      (
        !Array.isArray(teamSelections) ||
        teamSelections.length !== 2 ||
        teamSelections.some(team =>
          !team ||
          typeof team !== "object" ||
          Array.isArray(team) ||
          ["provider", "competitionId", "season", "providerTeamId"]
            .some(field =>
              typeof team[field] !== "string" ||
              team[field].length === 0
            )
        )
      )
    )
  ) {
    throw new TypeError("Invalid create input");
  }

  const statements = [
    db.prepare(`
      INSERT INTO lifecycle_matches (
        match_id, revision, status, state_json,
        invite_digest, invite_used,
        created_at, updated_at, expires_at, completed_at
      ) VALUES (?, 0, 'waiting', ?, ?, 0, ?, ?, ?, NULL)
    `).bind(
      matchId, stateJson, inviteDigest,
      createdAt, createdAt, expiresAt
    ),

    db.prepare(`
      INSERT INTO lifecycle_seats
        (match_id, seat, token_digest)
      VALUES (?, 0, ?)
    `).bind(matchId, hostDigest),
  ];

  if (teamSelections !== null) {
    for (const [seat, team] of teamSelections.entries()) {
      statements.push(db.prepare(`
        INSERT INTO lifecycle_match_teams (
          match_id,
          seat,
          provider,
          competition_id,
          season,
          provider_team_id
        )
        VALUES (?, ?, ?, ?, ?, ?)
      `).bind(
        matchId,
        seat,
        team.provider,
        team.competitionId,
        team.season,
        team.providerTeamId,
      ));
    }
  }

  const changes = requireBatch(
    await db.batch(statements),
    statements.length,
  );

  if (changes.some(value => value !== 1)) {
    throw new Error("Unexpected create result");
  }

  return { created: true, revision: 0 };
}

/**
 * Admit one guest with a conditional state transition.
 *
 * The UPDATE checks invitation validity and expiration again
 * inside the atomic batch, even if previously validated.
 *
 * A losing request cannot create a guest seat or event.
 */
function isValidJoinEvent(eventJson) {
  try {
    const event = JSON.parse(eventJson);
    return event !== null &&
      typeof event === "object" &&
      !Array.isArray(event) &&
      event.type === "joined";
  } catch {
    return false;
  }
}

export async function joinMatch(db, input) {
  const {
    matchId,
    inviteDigest,
    guestDigest,
    now,
    expiresAt,
    eventJson = '{"type":"joined"}',
  } = input;

  if (
    !matchId ||
    !inviteDigest ||
    !guestDigest ||
    !Number.isSafeInteger(now) ||
    !Number.isSafeInteger(expiresAt) ||
    expiresAt <= now ||
    typeof eventJson !== "string" ||
    !isValidJoinEvent(eventJson)
  ) {
    throw new TypeError("Invalid join input");
  }

  const changes = requireBatch(await db.batch([
    db.prepare(`
      UPDATE lifecycle_matches
      SET status = 'active',
          revision = revision + 1,
          invite_digest = NULL,
          invite_used = 1,
          updated_at = ?,
          expires_at = ?
      WHERE match_id = ?
        AND status = 'waiting'
        AND revision = 0
        AND invite_used = 0
        AND invite_digest = ?
        AND expires_at > ?
    `).bind(
      now, expiresAt, matchId, inviteDigest, now
    ),

    db.prepare(`
      INSERT INTO lifecycle_seats
        (match_id, seat, token_digest)
      SELECT ?, 1, ?
      WHERE changes() = 1
    `).bind(matchId, guestDigest),

    db.prepare(`
      INSERT INTO lifecycle_events
        (match_id, revision, event_json)
      SELECT ?, 1, ?
      WHERE changes() = 1
    `).bind(matchId, eventJson),
  ]), 3);

  if (changes.every(value => value === 0)) {
    return { joined: false, reason: "join_conflict" };
  }

  if (changes.every(value => value === 1)) {
    return { joined: true, revision: 1 };
  }

  throw new Error("Inconsistent join result");
}
