# ChronoOops! — Mission 003

[English](mission-003.md) | [Italiano](mission-003.it.md)

## The Message That Should Never Have Arrived

- Project: GiadaWare BoardLab
- Document: Game Design Specification M003
- Version: 0.1
- Status: design proposal, not implemented
- Baseline: Missions 001–002, deterministic C11 engines

## 1. Objective

In 2025, B finds a record describing an incident during
a public demonstration of the Cronophone in 1985.

The record contains crucial information: the exact time
of an electrical discharge and the name of its witness.

B can transmit this information to A in 1985.

A must protect the people involved without erasing
the testimony that made the warning possible.

The world retains one mutable timeline.

## 2. Preserved Rules

- Eight turns, alternating A (1985) and B (2025).
- Two action points per turn.
- Each successful action costs one action point.
- Causal propagation at the end of each turn.
- Determinism, with no parallel timelines.
- Atomic rejection without spending action points.
- No modifications to the M001 and M002 engines.

## 3. Information and Provenance

The 2025 record exists because a witness documented
the incident in 1985.

B can read the record and transmit one warning.

The warning contains:
- the time of the electrical discharge;
- the witness's identity;
- a reference to its source record.

Transmission establishes a provenance constraint:
as long as the information is used, the testimony
justifying its origin must remain possible.

Receiving a message from the future does not
automatically create a paradox.

## 4. Two Historical Interventions

A can choose between:

### A. Cancel the demonstration

Cancel the public demonstration entirely.

This would prevent the discharge but would also
eliminate the testimony that produced the record.

If the warning has already been transmitted, this
choice is causally incompatible and is rejected.

### B. Make the demonstration safe

Move people away from the dangerous area and isolate
the affected circuit while keeping the demonstration
observable.

The discharge is recorded without injuring anyone.
The witness therefore retains an authentic event to
document, preserving the record's valid provenance.

The mission does not require harm to people
to preserve information.

## 5. Causal Constraint

After the warning has been transmitted, any action
that would erase its sole necessary documentary
origin must be rejected with a specific result.

The rejection:
- identifies the incompatible action;
- identifies the constrained record and testimony;
- explains which informational origin would be lost;
- leaves state, world and action points unchanged.

Making the demonstration safe, on the other hand,
preserves the source.

The engine does not introduce a general paradox system.

## 6. Nominal Sequence

| Turn | Agent | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Inspect equipment | Identify risk |
| 2 | B | Consult record | Transmit warning |
| 3 | A | Receive warning | Make demonstration safe |
| 4 | B | Verify testimony | Check record |
| 5 | A | Certify demonstration | Archive testimony |
| 6 | B | Validate provenance | Complete mission |

The historical intervention propagates at the end
of turn 3.

Nominal victory occurs on turn 6 when:
- people are safe;
- the testimony remains valid;
- the information has verifiable provenance;
- the mission is completed.

## 7. Incompatible Sequence

After the warning is transmitted on turn 2,
A attempts to cancel the entire demonstration.

The engine rejects this intervention on turn 3
without spending an action point.

A can still choose the compatible intervention
and continue toward victory.

The player receives a causal explanation,
not merely a generic error message.

## 8. Victory, Defeat and Deadlock

- Victory: people safe, source valid and provenance
  validated by turn 8.
- Defeat: objective incomplete at the end of turn 8.
- Early deadlock: only when the state makes reaching
  the objective permanently impossible.
- No victory if the warning is absent or its
  provenance can no longer be justified.

## 9. Verification Contract

- A/B alternation, maximum two action points, eight turns.
- The warning can be transmitted only once.
- Every message is linked to an identifiable source.
- Incompatible intervention is rejected atomically.
- Compatible intervention propagates at turn end.
- Provenance is preserved on the winning path.
- Nominal victory occurs on turn 6.
- No victory with an invalid information source.
- Identical action sequences produce identical results.
- Explanations reflect actual engine transitions.

## 10. Implementation Boundaries

Mission 003 uses its own C11 module, with state
and transitions separate from previous missions.

The unified CLI will be extended only after
the new engine has been qualified.

Excluded: parallel universes, AI, network multiplayer,
general inference systems, combat and 2D/3D graphics.

## 11. Qualification

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- CLI_INTEGRATION: NOT_STARTED
- PUBLICATION: DEFERRED
