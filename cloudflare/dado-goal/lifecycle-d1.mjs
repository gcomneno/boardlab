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
  } = input;

  if (
    !matchId ||
    typeof stateJson !== "string" ||
    !inviteDigest ||
    !hostDigest ||
    !Number.isSafeInteger(createdAt) ||
    !Number.isSafeInteger(expiresAt) ||
    expiresAt <= createdAt
  ) {
    throw new TypeError("Invalid create input");
  }

  const changes = requireBatch(await db.batch([
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
  ]), 2);

  if (changes[0] !== 1 || changes[1] !== 1) {
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
export async function joinMatch(db, input) {
  const {
    matchId,
    inviteDigest,
    guestDigest,
    now,
    expiresAt,
  } = input;

  if (
    !matchId ||
    !inviteDigest ||
    !guestDigest ||
    !Number.isSafeInteger(now) ||
    !Number.isSafeInteger(expiresAt) ||
    expiresAt <= now
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
      SELECT ?, 1, '{"type":"joined"}'
      WHERE changes() = 1
    `).bind(matchId),
  ]), 3);

  if (changes.every(value => value === 0)) {
    return { joined: false, reason: "join_conflict" };
  }

  if (changes.every(value => value === 1)) {
    return { joined: true, revision: 1 };
  }

  throw new Error("Inconsistent join result");
}
