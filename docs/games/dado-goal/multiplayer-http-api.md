# Dado Goal — Multiplayer HTTP API

Status: DESIGN / NOT IMPLEMENTED
Tracking: https://github.com/gcomneno/boardlab/issues/27

## Objective

Expose the existing authoritative MatchManager through
a small HTTP interface for two real players.

The interface is specific to Dado Goal.

No changes to the shared BoardLab engine are required.

## Architecture

Browser
  -> HTTPS JSON API
  -> authoritative MatchManager
  -> Python Dado Goal transition engine

The server generates dice outcomes.

The browser never submits authoritative die faces,
scores, turn changes or complete game states.

## API version

All endpoints use the /api/v1 prefix.

Requests and responses use application/json.

All state-changing commands use POST.

Responses must include Cache-Control: no-store.

## Create match

POST /api/v1/matches

Request:

{
  "handsPerTeam": 10
}

Response: 201 Created

{
  "matchId": "...",
  "hostToken": "...",
  "inviteToken": "...",
  "snapshot": {}
}

The host token is secret.

The invitation token allows the invited friend
to claim the second player seat exactly once.

## Join match

POST /api/v1/matches/{matchId}/join

Request:

{
  "inviteToken": "..."
}

Response: 200 OK

{
  "guestToken": "...",
  "snapshot": {}
}

The invitation token must never authorize a roll.

## Read authoritative snapshot

GET /api/v1/matches/{matchId}

Header:

Authorization: Bearer <seat-token>

Response: 200 OK

{
  "revision": 1,
  "status": "active",
  "seat": 0,
  "state": {}
}

The state follows wire schema version 1.

## Request a roll

POST /api/v1/matches/{matchId}/roll

Header:

Authorization: Bearer <seat-token>

Request:

{
  "requestId": "client-generated-unique-id",
  "expectedRevision": 1
}

Response: 200 OK

{
  "revision": 2,
  "die": "team",
  "face": "Tiro in porta!",
  "status": "active",
  "state": {},
  "duplicate": false
}

The server exclusively generates the face.

A repeated request ID with the original expected
revision returns the stored receipt.

A repeated request must not consume new randomness.

## Synchronization

MVP: short-interval HTTP polling while a match is active.

Use authoritative snapshot revisions.

Do not poll continuously after match completion.

On reconnect, fetch the current snapshot.

Client animation is presentation only and must not
delay authoritative server transitions.

WebSocket is deferred until its operational benefit
justifies additional complexity.

## Error responses

All errors follow a stable envelope:

{
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable description"
  }
}

Proposed HTTP mappings:

400 BAD_REQUEST
401 UNAUTHORIZED
403 NOT_YOUR_TURN
404 MATCH_NOT_FOUND
409 STALE_REVISION
409 MATCH_NOT_JOINABLE
409 MATCH_NOT_ACTIVE
409 REQUEST_ID_CONFLICT
413 PAYLOAD_TOO_LARGE
415 UNSUPPORTED_MEDIA_TYPE
429 RATE_LIMITED
503 CAPACITY_EXCEEDED

Errors must not expose seat tokens, invitation
tokens, server internals or Python tracebacks.

The adapter must distinguish error types
without depending on English error-message matching.

## Request validation

- Strict JSON object decoding.
- Reject unknown properties.
- Enforce request-size limits before reading bodies.
- Reject duplicate JSON object keys.
- Reject malformed UTF-8 and invalid content types.
- Validate numeric and string types strictly.
- Reject non-finite numeric values.
- Bound match configuration and identifier lengths.
- Never accept client-provided game state as authority.

## Security

Production requires HTTPS.

Restrict browser origins explicitly.

Do not use permissive wildcard CORS.

Do not log credentials or authorization headers.

Avoid placing seat credentials in shareable URLs.

If invitation URLs contain tokens, define redaction,
referrer and history-handling protections.

Prevent browser caching of authenticated snapshots.

Introduce request rate limits and session expiry
before internet exposure.

Seat tokens are bearer credentials: possession
grants access, not verified real-world identity.

## Concurrency

Single-process MatchManager locking is already
covered by concurrent regression tests.

This does not establish cross-process safety.

Production must use one authoritative writer per
match or transactional persistent coordination.

Do not deploy multiple independent in-memory
workers sharing the same public matchmaking API.

## Recovery and persistence

The current implementation is memory-only.

A process restart loses matches.

Durable snapshots, event replay, credential recovery
and retention limits require separate qualification.

Until implemented, restart recovery is unsupported
and must be disclosed to players.

## MVP acceptance

- Two browser sessions can create and join a match.
- Each session can retrieve its authorized snapshot.
- Only the active seat can request a roll.
- Both clients converge to the same revision and state.
- Duplicate roll requests do not duplicate results.
- Out-of-turn and malformed requests are rejected.
- Refresh synchronizes to the current server state.
- Full match completion is verified over HTTP.
- Access-control and concurrency tests pass.
- No production deployment without explicit approval.

## Implementation boundary

Next milestone:

Implement a localhost-only HTTP adapter with
Python standard-library tooling and integration tests.

Keep transport separate from MatchManager.

No public interface, hosting account, cloud deployment,
paid service or firewall change is authorized here.

## Exclusions

- Dado Goal management module (#29).
- AI-assisted commentary (#30).
- Visual redesign (#28).
- Changes to Tre Sigilli or Reversi.
- Shared BoardLab transport framework.
