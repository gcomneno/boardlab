# Tre Sigilli — Game Specification v0.2

[English](tre-sigilli.md) | [Italiano](tre-sigilli.it.md)

## Status and scope

Approved rules for BoardLab's first original game. This document defines game behavior and acceptance criteria, not an implementation. Strategic balance remains unproven.

## Board and participants

The board is a four-by-four grid of sixteen cells. Two participants, NORTH and SOUTH, each control three seals. NORTH moves first. Turns alternate, and a turn is one completed action. There is no randomness.

Cells are indexed from zero to fifteen in row-major order. Orthogonal adjacency means sharing an edge; diagonals do not count.

## Phases and actions

### Placement

Turns zero through five are placement turns. The active participant places one of their unplaced seals on any empty cell. NORTH acts on even-numbered turns and SOUTH on odd-numbered turns.

After six completed turns, each participant has exactly three seals on the board.

### Maneuver

Turns six through seventeen are maneuver turns. The active participant moves exactly one of their seals to an orthogonally adjacent empty cell. Moving another participant's seal, moving diagonally, moving to an occupied cell, and passing are illegal.

No action is permitted after termination.

## Influence and territory

Each seal influences its occupied cell and all existing orthogonally adjacent cells.

For every cell, compare the number of influencing NORTH seals with the number of influencing SOUTH seals.

A cell belongs to the participant with strictly greater influence and contributes one point to that participant's territorial score.

Equal positive influence makes a cell contested. Zero influence from both participants makes it neutral. Contested and neutral cells each contribute zero points.

Scores are recalculated from the current board configuration; they are not accumulated across turns.

## Termination and outcome

The game terminates after eighteen completed turns or when the active participant has no legal maneuver move after placement has finished.

A blocked participant does not automatically lose. At termination, the participant with the higher territorial score wins. Equal scores produce a draw.

The outcome is expressed from the requested participant's perspective as win, loss, or draw. An outcome cannot be requested for an ongoing game.

## State and transition contract

A complete state contains immutable board occupancy and the number of completed turns. The active participant is derived from turn parity for nonterminal states.

Placement actions and maneuver actions are distinct move types. A successful transition returns an independent successor without modifying the input state.

Invalid states and illegal actions are rejected. A rejected transition has no observable effect on the original state.

Structural validation requires sixteen valid cells, disjoint ownership, an integer turn count between zero and eighteen, and seal counts consistent with the placement phase and subsequent maneuver phase.

Structural validity does not claim that every accepted configuration is reachable through a legal game history.

## Acceptance criteria

- The initial board is empty and NORTH is the first participant.
- Placement alternates for six turns and never overwrites an occupied cell.
- Maneuver preserves exactly three seals per participant and requires an empty orthogonally adjacent destination.
- Legal actions are deterministic and do not mutate the state.
- Termination is detected at the eighteen-turn limit or when the active participant cannot maneuver.
- A blocked participant is evaluated by territorial score, not by automatic forfeiture.
- Territory distinguishes controlled, contested, and neutral cells.
- Invalid states, illegal actions, and premature outcome requests are rejected.
- The concrete game conforms to the existing generic game contract without modifying the generic engine.

## Evidence and limitations

Read-only in-memory audits established reachable blocked positions after six, seven, and eight completed turns, respecting participant order.

All 160,160 disjoint configurations with three seals per participant were checked geometrically. Four block NORTH, four block SOUTH, and none block both simultaneously.

The geometric score distribution after placement is symmetric between participants. This does not establish strategic fairness, first-player advantage, or the frequency of outcomes under actual play.

No repetition rule, voluntary pass, or special immobilization bonus is part of version 0.2.
