# Dado Goal — Extended Rules v1

## 1. Status

Dado Goal Extended is the single current gameplay ruleset.

The historical Dado Goal Original rules remain documented as the origin of the
project, but they are not maintained as a separate selectable gameplay mode.

All current and future Dado Goal gameplay work must converge on the Extended
ruleset unless a later contract explicitly supersedes this document.

## 2. Design principles

- Preserve the identity and recognisable flow of Dado Goal.
- Extend gameplay through explicit football situations rather than opaque
  probability changes.
- Keep single-player and authoritative multiplayer aligned on the same rules.
- Preserve deterministic state transitions and testability.
- Keep the complete Extended ruleset reproducible with physical dice and
  written tabletop rules; digital implementations must not depend on mechanics
  that cannot be represented physically.
- Do not let imported real-world team or player data modify random outcomes
  unless a separate approved contract explicitly introduces such behaviour.

## 3. Penalty kick

The existing `RIGORE` face remains the trigger for a penalty kick.

A penalty no longer rerolls the normal result die.

Instead, it enters a dedicated penalty resolution using a fifth six-sided die.

The physical and visual identity of this die is canonical:

- colour: bottle green;
- purpose: penalty resolution only.

Its six faces are:

- `GOAL!`
- `GOAL!`
- `GOAL!`
- `PALO/TRAVERSA`
- `PARATO/RESPINTO`
- `FUORI`

This gives a 50% goal probability for penalties.

The penalty die is used exclusively for penalty resolution.

Extended v1 introduces no other mandatory physical die beyond this fifth
bottle-green penalty die.

A penalty result cannot generate another penalty.

## 4. Corner kick

The existing `Cross!` face is reinterpreted as the start of a corner-kick
sequence.

The gameplay meaning is:

`CALCIO D'ANGOLO / CROSS`

The corner itself is the set piece; the cross is its execution.

The exact deterministic resolution sequence for the corner is intentionally
left for a dedicated implementation contract before engine changes are made.

## 5. Free kick from the edge of the area

The existing `Mezza rovesciata` face is replaced by:

`PUNIZIONE DAL LIMITE`

This becomes a distinct set-piece situation rather than another generic
attacking technique.

The exact deterministic resolution sequence for the free kick is intentionally
left for a dedicated implementation contract before engine changes are made.

## 6. Injuries

Injuries remain part of the gameplay event system.

They must remain structured canonical match events and, when they require a
replacement, retain the explicit substitution reason:

`reason: "injury"`

Extended gameplay should make injuries more visible to the player without
silently changing their probability or persistence semantics.

Any probability change requires explicit approval.

## 7. Ruleset convergence

Single-player and authoritative multiplayer must ultimately implement the same
Extended gameplay rules.

Temporary implementation skew is allowed only during a controlled migration and
must not be treated as a permanent product split.

AUTO behaviour remains surface-specific where already designed differently;
this contract does not force single-player AUTO semantics onto multiplayer.

## 8. Historical compatibility

References to Dado Goal Original in historical changelogs, release notes and
archaeological documentation remain valid and must not be rewritten.

Existing contracts that previously required preserving Original rules as the
active baseline now defer to this Extended rules contract for gameplay rules,
while retaining their own transport, persistence, lifecycle and authority
constraints.

## 9. Implementation boundary

This document defines the gameplay baseline only.

Before implementation:

- penalty resolution may proceed directly from the frozen die defined here;
- corner resolution requires a dedicated deterministic design;
- free-kick resolution requires a dedicated deterministic design;
- injury visibility may be improved without changing injury probability.

No engine modification is authorised merely by this document beyond work
explicitly approved in a subsequent implementation step.
