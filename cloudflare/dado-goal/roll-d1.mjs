/**
 * Authoritative Dado Goal roll.
 *
 * Authorization and game transitions are server-side.
 * A persisted receipt makes retries idempotent.
 * Optimistic concurrency prevents two commits at one revision.
 */

import { FACES, applyRoll, validateState } from "./game.mjs";
import { commitRoll } from "./atomic.mjs";

function randomFace(phase) {
  const faces = FACES[phase];
  if (!faces) throw new Error("No roll permitted");

  // Rejection sampling avoids modulo bias for a six-faced die.
  const byte = new Uint8Array(1);
  do {
    crypto.getRandomValues(byte);
  } while (byte[0] >= 252);

  return faces[byte[0] % faces.length];
}

async function receipt(db, matchId, seat, requestId) {
  return db.prepare(`
    SELECT expected_revision, response_json
    FROM lifecycle_receipts
    WHERE match_id = ? AND seat = ? AND request_id = ?
  `).bind(matchId, seat, requestId).first();
}

function replay(saved, expectedRevision) {
  if (saved.expected_revision !== expectedRevision) {
    throw new Error("Request ID reused with other revision");
  }
  return { ...JSON.parse(saved.response_json), duplicate: true };
}

export async function rollAuthorized(
  authority, matchId, seatToken, requestId, expectedRevision
) {
  if (typeof requestId !== "string" ||
      requestId.length < 1 || requestId.length > 128) {
    throw new Error("Invalid request ID");
  }
  if (!Number.isSafeInteger(expectedRevision) || expectedRevision < 0) {
    throw new Error("Invalid revision");
  }

  const seat = await authority.seat(matchId, seatToken);
  const db = authority.db;
  const row = await authority.match(matchId);
  const now = authority.now();

  if (now >= row.expires_at) throw new Error("Match expired");

  const saved = await receipt(db, matchId, seat, requestId);
  if (saved) return replay(saved, expectedRevision);

  if (row.status !== "active") throw new Error("Match is not active");
  if (row.revision !== expectedRevision) throw new Error("Stale revision");

  const state = validateState(JSON.parse(row.state_json));
  if (state.player !== seat) throw new Error("Not your turn");

  const phase = state.phase;
  const face = randomFace(phase);
  const next = applyRoll(state, face);

  const status = next.phase === "finished" ? "completed" : "active";
  const completedTTL = authority.completedTTL ?? 86400;
  const ttl = status === "completed" ? completedTTL : authority.activeTTL;
  const updatedAt = Math.max(now, row.updated_at);

  if (updatedAt >= row.expires_at) throw new Error("Match expired");

  const response = {
    revision: expectedRevision + 1,
    die: phase,
    face,
    status,
    state: next,
    duplicate: false,
  };

  const commit = await commitRoll(db, {
    matchId,
    expectedRevision,
    status,
    stateJson: JSON.stringify(next),
    updatedAt,
    expiresAt: updatedAt + ttl,
    completedAt: status === "completed" ? updatedAt : null,
    eventJson: JSON.stringify({ type: "roll", seat, die: phase, face }),
    seat,
    requestId,
    responseJson: JSON.stringify(response),
  });

  if (commit.committed) return response;

  // The request may have lost a race to its own earlier attempt.
  // Check the durable receipt before reporting a revision conflict.
  const concurrentReceipt = await receipt(db, matchId, seat, requestId);
  if (concurrentReceipt) return replay(concurrentReceipt, expectedRevision);

  throw new Error("Stale revision");
}
