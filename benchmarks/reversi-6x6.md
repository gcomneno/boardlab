# Reversi 6x6 — Experimental Benchmark Protocol

[English](reversi-6x6.md) | [Italiano](reversi-6x6.it.md)

## Purpose

Compare Random, Minimax, Alpha-Beta and MCTS on Reversi 6x6 using
reproducible matches and explicitly declared computational budgets.

This document specifies the experimental protocol. It does not report
experimental results.

## Pilot configuration

| Parameter | Value |
| --- | --- |
| Game | Reversi 6x6 |
| Baseline | Random |
| Minimax depth | 2 |
| Alpha-Beta depth | 2 |
| MCTS simulations per decision | 32 |
| Seeds | 0, 1, 2, 3, 4 |
| Player roles | Both colors, swapped for each seed |
| Evaluation for deterministic search | Disc difference |
| MCTS exploration | Strategy default |
| Random generators | Explicitly seeded |

These values are pilot parameters, not optimized configurations.
Search depth and simulation count are not equivalent computational budgets.

## Match design

Run each search strategy against the Random baseline with both player
roles for each seed. This produces 10 matches per strategy and 30
matches overall in the initial pilot.

Use independent random generators for each randomized participant.
Record the strategy configuration, seed, assigned color and outcome.
Apply the same protocol to every comparison.

A full round-robin tournament is outside the initial pilot scope.

## Measurements

Record wins, losses, draws and number of actions for each match.
Measure elapsed wall-clock time separately from deterministic outcomes.

For Minimax and Alpha-Beta, collect available search statistics,
including visited nodes. MCTS simulation counts are configuration
budgets, not equivalent to visited nodes.

Report aggregates together with the individual match observations.
Do not interpret raw elapsed times as reproducible across machines.

## Reproducibility

Fixed seeds, fixed strategy parameters, explicit player-role swaps
and a recorded code revision are required.

Repeated runs must produce identical game outcomes and action counts
under the same software configuration. Elapsed time is excluded from
exact reproducibility assertions.

## Pilot acceptance criteria

- All matches terminate with valid outcomes.
- Both player roles are exercised for every seed and strategy.
- Identical configurations reproduce outcomes and action counts.
- Reported aggregates reconcile with individual observations.
- Runtime remains practical for local execution.
- Repository QA passes without adding long-running experiments to
  the default test suite.

## Scope boundaries

The generic engine and existing strategy implementations remain
unchanged. No new runtime dependencies are planned.

The implementation and calibration of the benchmark harness belong
to subsequent milestones. This document does not claim results or
comparative superiority.
