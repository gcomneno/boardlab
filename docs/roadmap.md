# Roadmap

[English](roadmap.md) | [Italiano](roadmap.it.md)

## Phase 0 — Generic engine

Define Game, GameState, Move, Player, Strategy, and Match.

## Phase 1 — Tre Sigilli

Introduce the first original game and a random strategy.

## Phase 2 — Minimax

Implement a readable, tested, and instrumented version.

## Phase 3 — Alpha-Beta Pruning

Compare Alpha-Beta with Minimax in terms of move quality,
explored nodes, and execution time.

## Phase 4 — Monte Carlo Tree Search

Study selection, expansion, simulation, and backpropagation.

## Phase 5 — Scalability

Introduce a game with a larger state space and more
elaborate mechanics.

## Completed extension — Reversi 6×6 benchmarks

Implement a reproducible protocol to compare Minimax,
Alpha-Beta, and MCTS on Reversi 6×6, including the harness,
tests, and documented results.

Completed through PR #7. This extension is separate from
the proposed mobile distribution described in the
LEARN/BUILD/SHIP map.

## Android extension status

The SHIP extension produced Python/Kotlin parity contracts,
a Kotlin implementation of the Tre Sigilli domain,
a platform-independent controller, and a Jetpack Compose
user interface.

A complete match was verified on a real device, alongside
Android CI builds and the generation of an unsigned release APK.

Evidence: PRs #8–#13. Architecture lessons were documented
through PR #14.

These achievements do not retroactively change the original
roadmap phases and do not imply publication or production readiness.

The remaining qualification work includes automated Android
tests, release signing, key management, lifecycle handling,
accessibility, and distribution.

Active study is tracked separately in
[Learning progress](progress.md).
