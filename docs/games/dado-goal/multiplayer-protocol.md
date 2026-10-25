# Dado Goal — Online Multiplayer Protocol

Status: DESIGN / NOT IMPLEMENTED
Tracking: https://github.com/gcomneno/boardlab/issues/27
Priority: P0

## 1. Objective

Allow two real players to complete a Dado Goal match
from separate browsers.

The multiplayer implementation must preserve the
confirmed Dado Goal Original rules.

No strategic AI is required.

## 2. Ownership

This protocol belongs to the Dado Goal game instance.

Shared BoardLab infrastructure must not be modified
without a separate architectural decision.

The online mode is optional. Existing local play
must remain independent of network availability.

## 3. Authority model

The match server is authoritative.

The server owns:

- Match configuration.
- Player-to-team assignments.
- Current game state.
- Active player and required die.
- Random die outcomes.
- Validated state transitions.
- Match completion and final score.
- Ordered event history.

Clients must never choose or report an authoritative
die face, score, phase or next player.

The existing deterministic game model is the
reference for legal state transitions.

## 4. Session lifecycle

States:

CREATED
WAITING_FOR_OPPONENT
ACTIVE
COMPLETED
ABANDONED

Proposed flow:

1. Host creates a match with a positive number N
   of complete turns per team.
2. Server creates a private invitation identifier.
3. Host shares a link or code with a friend.
4. Second player joins the available seat.
5. Server establishes player/team assignments
   and the starting player.
6. Match becomes ACTIVE.
7. Players take turns requesting allowed rolls.
8. Server broadcasts the resulting state.
9. Match ends after N turns per player.

The exact starting-player selection mechanism
requires a product decision; the original tocco
counting convention is not fully specified.

## 5. Initial protocol commands

CREATE_MATCH
JOIN_MATCH
GET_MATCH
REQUEST_ROLL
LEAVE_MATCH

These are logical commands, not a commitment to
a specific network transport.

Every state-changing request must include:

- Match identifier.
- Authenticated session/seat identity.
- Unique request identifier.
- Expected match revision.

Never authorize a seat using only a public match ID.

## 6. Authoritative roll transaction

The server must atomically:

1. Authenticate the participant.
2. Verify the match is ACTIVE.
3. Verify the participant owns the active turn.
4. Verify the expected revision.
5. Verify that the current phase requires a roll.
6. Generate one server-side die outcome with a
   suitable cryptographically secure random source.
7. Apply the outcome through the validated game
   transition function.
8. Persist the new revision and event.
9. Return/broadcast the authoritative result.

The resulting die face must respect the confirmed
six-face distributions for the corresponding die.

Independent fair digital rolls are an implementation
assumption, not a historical claim about handmade dice.

## 7. Idempotency and ordering

Each accepted action increments a monotonic
match revision.

A repeated request identifier must never produce
another die roll.

Duplicate submissions return the original result
or an explicit duplicate response.

Stale revisions are rejected with enough information
for the client to resynchronize.

Transactions must be serialized per match to prevent
concurrent or out-of-turn rolls.

## 8. Server events

MATCH_CREATED
PLAYER_JOINED
MATCH_STARTED
ROLL_RESOLVED
TURN_COMPLETED
MATCH_COMPLETED
PLAYER_DISCONNECTED
PLAYER_RECONNECTED
MATCH_ABANDONED

Events carry an authoritative revision.

ROLL_RESOLVED includes the required die, resulting
face, and revised state.

Event payloads must not expose private join credentials.

## 9. Client synchronization

The browser displays server-confirmed game states.

Pending rolls show an explicit waiting indicator.

A browser refresh must recover the authoritative
match snapshot.

After reconnection, the client requests a snapshot
and catches up to the current revision.

A disconnected player must not lose their match
solely because their browser temporarily closed.

Define an explicit retention and expiry policy
before production deployment.

## 10. Security

- Unpredictable invitation/session credentials.
- Independent authentication for player seats.
- Validation of every server-side transition.
- No trust in client-side random results.
- Rate limits for creation, joining and actions.
- Explicit limits on match lifetime and storage.
- Protection against duplicate and racing actions.
- No mandatory collection of personal identity
  for the initial anonymous-friends MVP.

Client-side JavaScript must not contain
server credentials or signing secrets.

## 11. Transport and hosting decision

No backend, provider or paid plan is selected yet.

Evaluate:

A. WebSocket with a small authoritative server.
B. Request/response with polling or long polling.
C. Managed realtime infrastructure, if free-tier
   limits and operational constraints are acceptable.

Selection criteria:

- Two-player usability.
- Correctness and security.
- Reconnection support.
- Operational simplicity.
- Zero or minimal recurring cost.
- Provider portability.

Do not claim that GitHub Pages alone provides
an authoritative multiplayer backend.

## 12. Acceptance gates

M1 — Protocol and threat model qualified.
M2 — Authoritative match engine adapter tested.
M3 — Session creation and invitation tested.
M4 — Two-browser synchronization tested.
M5 — Disconnect, retry and race tests passed.
M6 — Deployment costs and operational limits verified.
M7 — Full remote multiplayer acceptance passed.

No deployment before explicit approval.

## 13. Explicit exclusions

- Team management and championship statistics (#29).
- AI-assisted commentary (#30).
- Visual redesign (#28).
- Player accounts and matchmaking.
- Tournaments and rankings.
- Changes to the confirmed original rules.
- Changes to Tre Sigilli or Reversi.

## 14. Next engineering decision

Inspect the precise Python and JavaScript state
schemas and their serialization contracts.

Choose how to run the authoritative transition
model on the server.

Select the cheapest technically sound transport
only after validating the deployment constraints.
