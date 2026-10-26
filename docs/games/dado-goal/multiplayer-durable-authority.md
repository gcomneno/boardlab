# Dado Goal — Durable authoritative match integration

Status: DESIGN / NOT IMPLEMENTED
Issue: https://github.com/gcomneno/boardlab/issues/27

## 1. Goal

Integrate the existing Dado Goal match manager with
SQLite persistence without weakening game authority,
seat authentication or idempotency.

The existing memory-only service remains the regression
baseline until the durable adapter passes qualification.

## 2. Ownership

All implementation remains specific to Dado Goal.

Do not modify the shared BoardLab engine, Tre Sigilli
or Reversi.

## 3. Current components

- dado_goal.py: deterministic game rules.
- dado_goal_wire.py: validated JSON state schema.
- dado_goal_match.py: in-memory match authority.
- dado_goal_store.py: isolated transactional repository.
- dado_goal_http.py: localhost HTTP transport.

The present store accepts a caller-supplied seat and
die face. This is laboratory functionality only.

It must never become a publicly reachable operation.

## 4. Durable authority boundary

The durable service accepts:

- A match identifier.
- A seat bearer credential.
- A unique request identifier.
- An expected match revision.

It never accepts an authoritative die face,
score, game phase or replacement state.

The service owns authentication, RNG, validation,
transaction boundaries and persistence.

## 5. Durable match creation

A single committed transaction creates:

- A unique match identifier.
- The validated initial state.
- Revision zero.
- A waiting-room lifecycle status.
- Independent host and invitation credentials.
- Stored credential verifiers and timestamps.

No partially initialized match may become visible.

Credential-generation failures must not leave a match.

## 6. Durable invitation redemption

Joining must be an atomic operation:

1. Verify match existence and lifecycle.
2. Validate the invitation credential.
3. Verify that the guest seat is still free.
4. Create the guest-seat credential.
5. Store the verifier.
6. Consume the invitation.
7. Advance revision and record the event.
8. Commit.

Two concurrent join requests may produce only
one successful guest assignment.

## 7. Durable authoritative roll

Each roll is one serialized transaction:

1. Begin a write transaction.
2. Authenticate the calling seat.
3. Find any prior receipt for the request ID.
4. Return an identical prior outcome on valid retry.
5. Check match status, turn and expected revision.
6. Select the required die from current phase.
7. Generate one server-side secure random outcome.
8. Apply the existing deterministic game model.
9. Persist updated state, revision and event.
10. Persist the idempotency receipt.
11. Commit before acknowledging the roll.

A rollback must leave no accepted partial action.

If a crash occurs after commit but before HTTP reply,
a retry with the same request ID must recover the
committed receipt without generating another roll.

## 8. Credential protection

Do not store plaintext seat or invitation tokens.

Use cryptographically appropriate token verifiers.

Verification must avoid meaningful timing leakage.

Credentials must never enter ordinary events,
snapshots, logs or publicly readable assets.

Tokens are bearer capabilities, not proof of
real-world player identity.

## 9. Schema and migrations

Schema versioning is mandatory.

The present laboratory schema is not yet a complete
production schema: it does not persist seat verifiers,
invitation redemption or retention timestamps.

Design a new versioned schema before integration.

Never silently upgrade or overwrite an unknown
database version.

## 10. Concurrency and integrity

SQLite write transactions provide serialization
for this single-database deployment model.

All match validation must occur inside the same
transaction that commits its state transition.

Revision checks must prevent lost updates.

Concurrent duplicate requests must generate no
additional committed dice outcomes.

A storage error must produce a controlled failure
without issuing a success response.

## 11. Recovery semantics

The qualified recovery contract must cover:

- Process restart after successful commit.
- Retry after a lost HTTP response.
- Failed transaction and rollback.
- Database lock contention.
- Corrupt or unsupported schema.
- Expired or abandoned match.
- Complete-match restoration.

A process restart must not invalidate valid
unexpired seat credentials.

## 12. Browser session policy

Server durability and browser credential persistence
are separate concerns.

No implicit persistent browser storage is authorized.

An optional "remember this device" facility requires
explicit consent and security qualification.

Until then, browser reload recovery remains incomplete.

## 13. Operational restrictions

- Localhost-only HTTP during development.
- Explicit database path outside public assets.
- No Internet deployment.
- No multi-worker deployment without qualification.
- No mandatory paid services.
- No claims of production crash safety yet.

## 14. Engineering qualification sequence

D1 — Review storage schema gaps and credential model.

D2 — Implement versioned durable credentials and
atomic invitation redemption.

D3 — Integrate authenticated server-side RNG and
transactional idempotency.

D4 — Verify concurrent join and roll behavior.

D5 — Verify lost-response and restart recovery.

D6 — Add expiration, cleanup and rate limiting.

D7 — Qualify optional browser session recovery.

D8 — Repeat real two-browser acceptance.

D9 — Evaluate low-cost production hosting.

## 15. Acceptance gate

The durable manager is qualified only when it can
complete a match, restart and recover both seats,
replay a committed response without another roll,
and preserve all previously qualified game rules.

Passing the standalone SQLite tests alone does not
satisfy this gate.
