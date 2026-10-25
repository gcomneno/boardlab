# Dado Goal — Multiplayer Lifecycle v4

Status: DESIGN ONLY — NOT IMPLEMENTED

Tracking: https://github.com/gcomneno/boardlab/issues/27

## 1. Scope

Introduce bounded match lifetime and retention for the
durable authoritative multiplayer implementation.

Do not change Dado Goal Original rules, dice probabilities,
browser animations, or unrelated BoardLab games.

## 2. Schema versioning

- Preserve SQLite schema v3 unchanged.
- Introduce schema v4 as a separate, explicit version.
- Never silently migrate v1, v2 or v3 databases.
- Reject incompatible or incomplete schemas.
- Any migration requires a separate, qualified procedure.

## 3. Required lifecycle metadata

Each match requires:

- created_at
- updated_at
- expires_at
- completed_at (nullable)

Store timestamps in a documented UTC representation.

The authoritative server clock defines lifecycle decisions.
Client clocks must not determine expiration.

## 4. States and expiration

Waiting:
- Invitation remains valid only until waiting expiry.
- Expired invitations cannot create guest seats.

Active:
- Accepted authoritative activity can extend idle expiry
  according to an explicit policy.
- Snapshot polling must not extend match lifetime.
- Expired matches reject further rolls.

Completed:
- Match result remains available for a bounded retention
  period.
- Retention expiry makes the match eligible for cleanup.

Expired:
- All seat and invitation credentials become unusable.
- No new authoritative actions are accepted.
- Expiration and cleanup must be race-safe.

## 5. Duration policy

The following durations require explicit qualification:

- waiting_ttl
- active_idle_ttl
- completed_retention_ttl

No implicit defaults are authorized.

Values must be positive, bounded and validated before
creating a persistent multiplayer service.

## 6. Transaction contract

- Check expiration within the same transaction that
  authorizes joins and rolls.
- Preserve authoritative revision and request receipts.
- A rejected expired action must not advance the dice.
- Cleanup must not race with an accepted write.
- Preserve crash recovery and replay guarantees.

## 7. Cleanup contract

- Cleanup must be explicit, bounded and observable.
- Delete dependent events, receipts and seats consistently.
- Never expose credential material in logs.
- Never delete unrelated SQLite databases or filesystem
  paths.
- Qualify concurrent cleanup and active-write behavior.

## 8. Test contract

Required deterministic tests:

- Injected/fake clock at expiration boundaries.
- Waiting invitation expiry.
- Active idle expiry.
- Polling does not extend lifetime.
- Completed-match retention.
- Rejection without revision increment.
- Concurrent roll versus expiry and cleanup.
- Crash and restart recovery.
- Durable receipt replay before expiry.
- Existing v3 rejection without database mutation.

## 9. Deployment boundary

Schema v4 alone does not authorize public hosting.

Rate limiting, HTTPS, trusted proxy/origin policy,
resource limits and remote-browser acceptance are
separately required before Internet deployment.
