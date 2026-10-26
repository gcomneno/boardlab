# Dado Goal — Authoritative state wire schema

Status: IMPLEMENTED LOCALLY / NOT DEPLOYED
Tracking: https://github.com/gcomneno/boardlab/issues/27

## Ownership

Game-specific adapter for Dado Goal.

No changes to the shared BoardLab engine are required.

## Schema version 1

Example initial state:

```json
{
  "schemaVersion": 1,
  "handsPerTeam": 10,
  "player": 0,
  "hands": [0, 0],
  "goals": [0, 0],
  "phase": "team"
}
```

The six fields are mandatory. Unknown fields are rejected.

Phase values:

- team
- pink
- result
- finished

Match length is restricted to 1–1000 hands per team,
matching the current browser configuration limit.

## Serialization

Python is the authoritative transition model.

`encode_state` converts validated Python states into
JSON-compatible dictionaries.

`decode_state` strictly reconstructs and validates states.

These functions do not confer authority upon data
received from clients.

A server must never overwrite its authoritative state
using a client-provided snapshot.

## Presentation fields

Browser-specific `lastRoll`, player names, colors and
animation state are not part of the mathematical state.

The authoritative server will deliver roll outcomes
as separately versioned match events.

## Security boundaries

This module does not implement:

- Authentication or player identity.
- Match creation or invitation credentials.
- Cryptographically secure die rolls.
- Revision checks and idempotency.
- Persistent transactional storage.
- WebSocket, HTTP or other transport.

Those capabilities belong to subsequent #27 milestones.

## Compatibility

Python/JavaScript transition parity was verified
against 800 reachable-state transitions for match
lengths 1, 2 and 3.

The wire schema retains the JavaScript engine's field
names while keeping its own explicit schema version.

## Maintenance

Update this game's changelog when the adapter milestone
has been qualified and is ready for a local commit.
