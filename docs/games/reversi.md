# Reversi 6x6 — Game Specification v1.0

[English](reversi.md) | [Italiano](reversi.it.md)

## Status and scope

This document describes the implemented Reversi 6x6 rules and their acceptance criteria in BoardLab. It documents the concrete game, not a new generic engine or a claim of strategic balance.

## Board and participants

The board is a six-by-six grid containing 36 cells, indexed from zero to 35 in row-major order. Each cell is empty or occupied by a BLACK or WHITE disc.

BLACK acts first. The initial board contains WHITE discs at indices 14 and 21 and BLACK discs at indices 15 and 20. There is no randomness in the game rules.

## Actions and captures

A placement uses `Place(cell)` on an empty cell. It is legal only if at least one straight line in any of the eight directions contains one or more consecutive opposing discs followed by a disc belonging to the active participant.

All opposing discs bounded in this way are flipped. Lines cannot wrap across board edges. Legal placements are returned in ascending cell-index order.

When the active participant cannot place but the opponent can, the only legal action is `Pass()`. Passing preserves the board and transfers the turn. Passing is forbidden when a placement exists.

## Termination and outcome

The game terminates when neither participant has a legal placement, including positions with empty cells remaining.

The participant with more discs wins. Equal disc counts produce a draw. Results are expressed from the requested participant's perspective as win, loss, or draw.

An outcome cannot be requested before termination. Terminal positions have no active participant available through `current_player` and permit no further actions.

## State and transition contract

A state consists of an immutable tuple of 36 cells and an explicit active participant. `Place` and `Pass` are distinct immutable action types.

A successful action produces a new state without mutating its predecessor. Malformed states and illegal actions are rejected.

Structural validation checks the board shape, cell values, and active participant. It does not prove that every structurally valid position is reachable through a legal history.

The game implements the existing generic `Game` contract and works with `Match`, `RandomStrategy`, `MinimaxStrategy`, `AlphaBetaStrategy`, and `MCTSStrategy` without changes to the generic engine.

## Acceptance criteria

- The standard opening has the expected four discs and BLACK to act.
- Legal placements and flips are deterministic and respect all eight directions and board boundaries.
- Placements without captures and moves to occupied cells are rejected.
- An explicit pass is mandatory only when the opponent can place and the active participant cannot.
- Termination occurs when neither participant can place, even if the board is not full.
- Terminal outcomes use disc counts and support wins, losses, and draws.
- Invalid states, illegal moves, and premature outcome requests are rejected without mutating the input.
- The concrete game interoperates with the existing generic match and search strategies.

## Evidence and limitations

Automated tests cover the value model, legal actions, adversarial captures, terminal outcomes, seeded complete matches, forced passes, and generic strategy compatibility.

Seeded Monte Carlo Tree Search reproducibility and agreement between Minimax and Alpha-Beta on selected search depths are tested. These checks do not establish strategic optimality, fairness, or performance at larger search budgets.

No graphical interface, persistent game history, or rule variant is included in this specification.
