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

The existing `Cross!` face is reinterpreted as:

`CALCIO D'ANGOLO / CROSS`

The corner itself is the set piece; the cross is its execution.

This does not introduce a new die or a new gameplay phase.

The existing flow is preserved:

`team -> pink`

Therefore, when the `CALCIO D'ANGOLO / CROSS` face is rolled, the attack
continues directly with the existing pink die.

## 5. Free kick from the edge of the area

The existing `Mezza rovesciata` face is replaced by:

`PUNIZIONE DAL LIMITE`

This becomes a direct set-piece situation rather than another generic
attacking technique.

This does not introduce a new die or a new gameplay phase.

The existing flow is preserved:

`pink -> result`

Therefore, when `PUNIZIONE DAL LIMITE` is rolled, the attack continues
directly with the existing result die.

## 6. Injuries

Injuries remain part of the gameplay event system.

They must remain structured canonical match events and, when they require a
replacement, retain the explicit substitution reason:

`reason: "injury"`

Extended gameplay should make injuries more visible to the player without
silently changing their probability or persistence semantics.

Any probability change requires explicit approval.

## 7. Match statistics

Extended defines a richer canonical statistical view of each completed match.

Statistics remain derived data. They must be reproducible from canonical roll
events and structured match events and must never become an independent source
of truth.

### 7.1 Ball possession

Ball possession is defined from valid die rolls.

Each valid die roll performed by a team counts as one possession unit.

For team `A`:

`possession(A) = rolls(A) / (rolls(A) + rolls(B)) * 100`

Displayed possession percentages are rounded to whole percentages and must be
presented as complementary values summing to exactly 100%.

This definition is intentionally physical-game compatible: in a tabletop match,
possession can be reproduced simply by counting the valid die rolls performed
by each team.

Simulation qualification over 100,000 matches with the current 10-hands-per-team
structure produced:

- mean home possession: 50.02%;
- 5th percentile: 40.91%;
- median: 50.00%;
- 95th percentile: 59.09%;
- only 0.60% of matches outside the 35%-65% interval.

This distribution is accepted as the Extended v1 possession model.

### 7.2 Extended statistical baseline

The canonical match statistics model must support at least:

- possession;
- goals;
- shots / attacking conclusions;
- shots wide;
- shots saved or blocked;
- posts / crossbars;
- corner kicks;
- free kicks from the edge of the area;
- penalties;
- passes;
- yellow cards;
- red cards;
- injuries.

Existing roll-event categories are reinterpreted consistently with Extended:

- the former `Cross!` statistical category becomes corner kicks;
- the former `Mezza rovesciata` statistical category becomes free kicks from
  the edge of the area;
- `FUORI!` remains distinguishable from `Respinto!` and `Parato!`;
- `PALO/TRAVERSA` remains a separate category;
- penalty attempts remain separately countable.

Injury totals are derived from structured canonical match events rather than
from die-face counts.

The exact presentation order and UI layout of these statistics are an
implementation concern and may evolve without changing their canonical meaning.

## 8. Ruleset convergence

Single-player and authoritative multiplayer must ultimately implement the same
Extended gameplay rules.

Temporary implementation skew is allowed only during a controlled migration and
must not be treated as a permanent product split.

AUTO behaviour remains surface-specific where already designed differently;
this contract does not force single-player AUTO semantics onto multiplayer.

## 9. Historical compatibility

References to Dado Goal Original in historical changelogs, release notes and
archaeological documentation remain valid and must not be rewritten.

Existing contracts that previously required preserving Original rules as the
active baseline now defer to this Extended rules contract for gameplay rules,
while retaining their own transport, persistence, lifecycle and authority
constraints.

## 10. Implementation boundary

This document defines the gameplay baseline only.

Before implementation:

- penalty resolution may proceed directly from the frozen die defined here;
- corner resolution reuses the existing `team -> pink` transition;
- free-kick resolution reuses the existing `pink -> result` transition;
- injury visibility may be improved without changing injury probability.

No engine modification is authorised merely by this document beyond work
explicitly approved in a subsequent implementation step.
