# ChronoOops! — Mission 002

[English](mission-002.md) | [Italiano](mission-002.it.md)

## Borrowing from Tomorrow

- Project: GiadaWare BoardLab
- Document: Game Design Specification M002
- Version: 0.1
- Status: design proposal, not implemented
- Baseline: Mission 001, deterministic C11 engine

## 1. Objective

In 2025, the Cronophone in Sottosopra is about to shut down.
B must reactivate it by turn 8 using a temporal module.
The module can be borrowed ahead of time from the future,
but doing so creates a causal debt that A must make
repayable in 1985.

The world has a single, mutable timeline.

## 2. Preserved Rules

- Alternating turns: A (1985), then B (2025).
- Two action points per turn.
- Eight turns in total.
- Each specified action costs one action point.
- Causal propagation occurs at the end of each turn.
- Determinism, with no parallel universes.
- Rejected actions leave state and action points unchanged.

## 3. Temporal Borrowing

B may borrow exactly one temporal module during the mission.

Borrowing creates:
- provisional availability of the module in 2025;
- an outstanding causal debt;
- an obligation to establish a compatible historical origin.

An outstanding debt is not, by itself, a paradox.

A repays the debt by approving, in 1985, the documentation
required to produce the module. The effect propagates at
the end of A's turn.

The pilot has no interest, randomness, or multiple debts.

## 4. Paradox Prevention

While the debt remains outstanding, A cannot destroy
the only documentation capable of justifying the module's origin.

Such an attempt:
- returns a result distinct from a missing prerequisite;
- leaves state, inventory, and action points unchanged;
- identifies the debt and the incompatible cause.

The mission does not introduce alternative timelines or
permit temporarily unresolved paradoxes.

## 5. Nominal Sequence

| Turn | Agent | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Consult archive | Identify module design |
| 2 | B | Inspect Cronophone | Borrow module |
| 3 | A | Record origin | Approve production |
| 4 | B | Install module | Reactivate Cronophone |
| 5 | A | Verify documentation | Wait |
| 6 | B | Confirm stability | Complete mission |

The debt is created on turn 2 and repaid when the
historical approval propagates at the end of turn 3.

Nominal victory occurs on turn 6, with the debt repaid
and the Cronophone operational.

## 6. Incompatible Sequence

After borrowing on turn 2, A attempts to destroy the
only design that can justify the module's existence.

The engine rejects the action on turn 3 without spending
an action point. A can still approve production and proceed.

The explanation distinguishes:
- cause: destruction of the design;
- constraint: the module's origin is still owed;
- prevented effect: making debt repayment impossible.

## 7. Victory and Defeat

- Victory: Cronophone reactivated and debt repaid by turn 8.
- Defeat: objective incomplete at the end of turn 8.
- Deadlock: no legal sequence can still achieve the objective.
- Borrowing does not automatically constitute repayment.

## 8. Verification Contract

- Correct player alternation and two-action-point limit.
- The module cannot be borrowed twice.
- Debt is outstanding immediately after borrowing.
- Debt is repaid only after historical propagation.
- Incompatible attempts are rejected atomically.
- Nominal path wins on turn 6.
- No victory while debt remains outstanding.
- Identical decisions produce reproducible results.
- Explanations derive from computed transitions.

## 9. Implementation Boundaries

Mission 001 remains unchanged.

The C11 extension must clearly separate M001 and M002
scenario rules, reusing engine principles without adding
a general-purpose paradox framework.

Excluded: network multiplayer, AI, combat, general
inventory, multiple loans, and parallel universes.

## 10. Qualification

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- PUBLICATION: NOT_STARTED
