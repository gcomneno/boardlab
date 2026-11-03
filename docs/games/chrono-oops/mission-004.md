# ChronoOops! — Mission 004

[English](mission-004.md) | [Italiano](mission-004.it.md)

## Two Events, One Cause

- Project: GiadaWare BoardLab
- Document: Game Design Specification M004
- Version: 0.1
- Status: design proposal, not implemented
- Baseline: Missions 001–003, deterministic C11 engines

## 1. Narrative Premise

In 1985, the Cronophone generates a synchronization pulse
that travels through a distribution circuit.

In 2025, two systems depend on that historical pulse:

- the Beacon Signal, needed to locate a station;
- the Central Register, needed to verify event
  synchronization.

Both systems must be active.

An old switch apparently offers only two choices:
BEACON or REGISTER.

The player must discover that the circuit also has
an initially undocumented dual contact.

The challenge is not to generate two pulses:
it is to distribute the same pulse correctly.

## 2. Objective

A (1985) must locate and configure the historical
pulse distributor.

B (2025) must examine both effects, verify that they
originate from the same cause and confirm the
configuration's stability.

Victory: both systems operational, common origin verified
and mission completed within eight turns.

## 3. Preserved Rules

- One mutable timeline.
- Maximum eight turns, alternating A/B.
- A operates in 1985; B operates in 2025.
- Two action points per turn.
- Each successful action consumes one action point.
- Rejected actions leave state and action points unchanged.
- Historical effects propagate at the end of A's turn.
- Deterministic, reproducible results.
- No changes to the M001, M002 or M003 engines.

## 4. Causal Model

There is only one historical pulse, identified as PULSE-85.

The pulse is an activation signal, not a limited
energy reserve that must be divided.

The distributor supports three configurations:

- BEACON: activates only the Beacon Signal;
- REGISTER: activates only the Central Register;
- DUAL: distributes the signal to both.

The DUAL configuration requires A to discover
and verify the additional contact.

The distributor does not create new causes or duplicate
the historical event: it connects two effects
to the same pulse.

## 5. Choices and Consequences

### BEACON Path

A configures the circuit exclusively for the Beacon.

After propagation, B observes:
- Beacon Signal active;
- Central Register inactive.

The mission cannot be completed with this configuration.

### REGISTER Path

A configures the circuit exclusively for the Register.

After propagation, B observes:
- Beacon Signal inactive;
- Central Register active.

This configuration is also insufficient.

### DUAL Path

A discovers the dual contact and enables it.

After propagation, B observes:
- Beacon Signal active;
- Central Register active.

B must still verify that both depend on PULSE-85.

This verification distinguishes causal reconciliation
from merely observing two active indicators.

## 6. Reversibility

BEACON and REGISTER configurations do not
automatically cause defeat.

A can correct an exclusive choice during a later
turn of their own, provided enough action points
and time remain.

After each change, the 2025 effects update only
at the end of the historical turn.

Checks performed on a previous configuration
do not automatically remain valid after a change.

A certified and stabilized configuration cannot
be altered after mission completion.

Neither parallel universes nor a general rollback
system are required.

## 7. Nominal Sequence

| Turn | Agent | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Inspect distributor | Identify pulse |
| 2 | B | Examine Beacon | Examine Register |
| 3 | A | Discover dual contact | Enable dual distribution |
| 4 | B | Verify Beacon | Verify Register |
| 5 | A | Certify configuration | Record common origin |
| 6 | B | Validate coherence | Complete mission |

The DUAL activation propagates at the end of turn 3.

Nominal victory occurs on turn 6.

## 8. Alternative Path

The player may select BEACON or REGISTER
before discovering the dual contact.

The exclusive effect becomes visible in 2025
after propagation.

Observing the missing system provides a clue
to search for another configuration.

If A later discovers the dual contact,
the distributor can be corrected.

Success depends on the remaining turns and action points.

## 9. Rejections and Invariants

- DUAL cannot be enabled before discovering
  its contact.
- Inactive future effects cannot be verified.
- Coherence cannot be validated without both
  checks and a certified common origin.
- A check becomes obsolete when the circuit changes.
- A configuration produces no effects in 2025
  before end-of-turn propagation.
- Rejected actions are atomic and spend no action points.
- An exclusive configuration is not a paradox:
  it is an incomplete solution.

## 10. Victory and Defeat

Victory requires:
- both effects active;
- both checked after the latest modification;
- historical configuration certified;
- PULSE-85 recorded as their common origin;
- coherence validated;
- mission completed by turn 8.

Defeat occurs when:
- turn 8 ends without completing the objectives;
- an irreversible state makes completion impossible.

No early defeat solely because an exclusive
configuration is still correctable.

## 11. Verification Contract

- A/B alternation, 2 action points, maximum 8 turns.
- Three distinct configurations: BEACON, REGISTER, DUAL.
- A single historical cause.
- Propagation only at the end of A's turn.
- BEACON does not activate the Register.
- REGISTER does not activate the Beacon.
- DUAL activates both effects.
- Changes invalidate obsolete checks.
- Rejected actions are atomic.
- Nominal path wins on turn 6.
- The alternative path is correctable within limits.
- Determinism for identical action sequences.
- No changes to existing mission engines.

## 12. Implementation Boundaries

Mission 004 will have an independent C11 module,
its own tests and a dedicated CLI.

The unified CLI will be extended after
engine qualification.

Excluded:
- parallel timelines;
- generalized retroactive propagation;
- realistic electrical simulations;
- external frameworks;
- new dependencies;
- 2D/3D graphics;
- multiplayer.

## 13. Qualification

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- CLI_INTEGRATION: NOT_STARTED
- PUBLICATION: DEFERRED
