# Dado Goal — Durable multiplayer matches

Status: DESIGN / NOT IMPLEMENTED

Tracking:
https://github.com/gcomneno/boardlab/issues/27

## 1. Objective

Preserve an authoritative Dado Goal match across:

- HTTP requests.
- Browser reloads.
- Temporary player disconnections.
- Controlled server restarts.
- Unexpected process termination.

The existing offline game must remain independent.

## 2. Persistence choice

Candidate: SQLite through the Python standard library.

No external database service is required for the
single-instance MVP.

The storage file must reside outside the public
Web asset directory.

Its filesystem path must be explicitly configured.

No storage file may be created implicitly during
module import.

## 3. Authoritative data

Persist:

- Match ID.
- Match lifecycle status.
- Validated mathematical state, wire schema v1.
- Monotonic match revision.
- Both player-seat assignments.
- Invitation status.
- Authoritative roll event history.
- Idempotency receipts.
- Creation, update and expiry timestamps.

Do not persist client animation state.

The server remains authoritative after recovery.

## 4. Transaction contract

Each accepted roll must be one atomic transaction:

1. Authenticate the seat.
2. Check match lifecycle.
3. Check active player.
4. Check expected revision.
5. Check request-ID idempotency.
6. Generate server-side die result.
7. Validate the deterministic transition.
8. Persist state, event and receipt together.
9. Commit the transaction.
10. Return the authoritative result.

A failed transaction must not publish a partial
state, event or receipt.

A committed action must survive process restart.

The idempotency receipt must be durable before
the server acknowledges success.

## 5. Concurrency

SQLite transaction serialization must protect
against concurrent writers.

The in-memory RLock is not sufficient for
multi-process coordination.

Use explicit transactional locking and compare
the expected revision inside the transaction.

Reject or retry database-busy conditions through
a bounded, documented policy.

Do not introduce independent authoritative
in-memory copies across server processes.

## 6. Credential storage

Host and guest seat credentials are bearer secrets.

Persist only keyed or cryptographically suitable
one-way verifiers, not plaintext seat tokens.

Use constant-time comparison where appropriate.

Do not log or return seat credentials in snapshots.

The public match ID is not an authentication token.

An invitation token authorizes claiming the guest
seat once; it is not a seat credential.

Invitation consumption must be atomic.

## 7. Browser session recovery

A page reload currently discards the seat token.

Recovery requires an explicit user-facing policy.

Candidate MVP:

- The player may opt to remember this browser.
- Store the seat credential only with explicit consent.
- Restore it from same-origin browser storage.
- Revalidate it with the authoritative server.
- Retrieve the latest snapshot and revision.
- Offer a clear "Forget this device" action.

Threat considerations:

- XSS can expose browser-held credentials.
- Shared devices may expose remembered sessions.
- Browser storage can be cleared independently.
- Credential recovery is not equivalent to
  verifying a real-world person's identity.

No persistent client credential storage is
authorized until its security design is approved.

## 8. Crash recovery

On startup:

- Open the configured SQLite database.
- Validate schema version.
- Verify match record consistency.
- Restore only valid, nonexpired sessions.
- Preserve authoritative revision ordering.
- Preserve idempotency receipt semantics.
- Refuse corrupted or unsupported records.

A crash between committed storage and HTTP reply
must not trigger a second die roll on retry.

## 9. Session lifecycle

The MVP requires explicit policies for:

- Waiting-room expiration.
- Active-match idle expiration.
- Completed-match retention.
- Abandoned-match handling.
- Expired invitation rejection.
- Cleanup and maximum storage growth.

Exact durations are product decisions.

Do not invent defaults without qualification.

## 10. Data boundaries

The Dado Goal storage implementation remains
game-specific.

BoardLab platform storage abstractions are
out of scope unless independently justified.

Tre Sigilli and Reversi are unaffected.

## 11. Security and operations

Before public exposure:

- Restrict storage-file permissions.
- Establish request rate limiting.
- Define backup and recovery behavior.
- Prevent credential disclosure in logs.
- Enforce HTTPS and explicit browser origins.
- Bound match count and database growth.
- Test filesystem and database error handling.
- Review concurrent-process behavior.
- Confirm hosting persistence guarantees.
- Document zero-cost or low-cost hosting limits.

## 12. Qualification milestones

P1 — SQLite schema and migration contract.

P2 — Transactional state, event and receipt storage.

P3 — Concurrent writes and replay qualification.

P4 — Restart and crash-recovery tests.

P5 — Session expiration and cleanup.

P6 — Browser credential recovery with explicit consent.

P7 — End-to-end two-browser restart acceptance.

P8 — Production security and hosting qualification.

## 13. Explicit exclusions

- Public deployment.
- New paid services.
- Player account registration.
- Tournament management (#29).
- AI-assisted commentary (#30).
- Visual redesign (#28).
- Changes to original Dado Goal rules.
- Changes to other BoardLab games.

## 14. Next implementation step

Build a standalone SQLite repository adapter
with temporary-file tests.

Do not modify MatchManager until storage
semantics and failure behavior are qualified.

Commit and deploy require separate approval.
