/**
 * Persist one prevalidated roll using a D1 atomic batch.
 *
 * The game engine must validate the player, token, phase and roll before
 * calling this function. This is a persistence primitive, not an API.
 *
 * A conditional UPDATE provides optimistic concurrency control.
 * changes() ensures that losing requests cannot create events or receipts.
 */
export async function commitRoll(db, input) {
  const {
    matchId,
    expectedRevision,
    status,
    stateJson,
    updatedAt,
    expiresAt,
    completedAt,
    eventJson,
    seat,
    requestId,
    responseJson,
  } = input;

  if (
    !Number.isSafeInteger(expectedRevision) ||
    expectedRevision < 0 ||
    !Number.isSafeInteger(seat) ||
    ![0, 1].includes(seat) ||
    !matchId ||
    !requestId
  ) {
    throw new TypeError("Invalid atomic roll input");
  }

  const nextRevision = expectedRevision + 1;

  const results = await db.batch([
    db.prepare(`
      UPDATE lifecycle_matches
      SET revision = ?, status = ?, state_json = ?,
          updated_at = ?, expires_at = ?, completed_at = ?
      WHERE match_id = ?
        AND revision = ?
        AND status = 'active'
    `).bind(
      nextRevision, status, stateJson,
      updatedAt, expiresAt, completedAt,
      matchId, expectedRevision,
    ),

    // changes() refers to the preceding UPDATE.
    db.prepare(`
      INSERT INTO lifecycle_events (match_id, revision, event_json)
      SELECT ?, ?, ?
      WHERE changes() = 1
    `).bind(matchId, nextRevision, eventJson),

    // changes() now refers to the event INSERT.
    db.prepare(`
      INSERT INTO lifecycle_receipts
        (match_id, seat, request_id, expected_revision, response_json)
      SELECT ?, ?, ?, ?, ?
      WHERE changes() = 1
    `).bind(
      matchId, seat, requestId, expectedRevision, responseJson,
    ),
  ]);

  if (
    !Array.isArray(results) ||
    results.length !== 3 ||
    results.some((result) => result.success !== true)
  ) {
    throw new Error("Invalid D1 batch result");
  }

  const changes = results.map((result) => result.meta?.changes);

  if (changes[0] === 0 && changes[1] === 0 && changes[2] === 0) {
    return { committed: false, reason: "revision_conflict" };
  }

  if (changes.every((count) => count === 1)) {
    return { committed: true, revision: nextRevision };
  }

  throw new Error("Inconsistent D1 batch result");
}
