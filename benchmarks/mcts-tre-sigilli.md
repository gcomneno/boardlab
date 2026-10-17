# MCTS — Tre Sigilli benchmark

[English](mcts-tre-sigilli.md) | [Italiano](mcts-tre-sigilli.it.md)

## Method

The benchmark compares MCTS against Random, Minimax and Alpha-Beta
using the existing `Match` coordinator.

- Game: Tre Sigilli
- MCTS: 40 simulations per move, default UCT exploration
- Minimax: depth 1, territorial evaluator
- Alpha-Beta: depth 1, same territorial evaluator
- Random: seeded pseudorandom move selection
- Seeds: 0 through 4
- Player roles: NORTH and SOUTH for each seed
- Matches: 10 per opponent, 30 total
- Reproducibility: verified by repeated experiments
- Implementation: `tests/test_mcts_benchmark.py`

## Results

Results are reported from the MCTS player's perspective.

| Opponent | Wins | Draws | Losses | Average turns |
| --- | ---: | ---: | ---: | ---: |
| Random | 10 | 0 | 0 | 18.00 |
| Minimax (depth 1) | 3 | 2 | 5 | 18.00 |
| Alpha-Beta (depth 1) | 3 | 2 | 5 | 18.00 |

## Interpretation

MCTS won all sampled matches against Random.

Against depth-1 Minimax and Alpha-Beta, MCTS recorded more losses
than wins. The identical aggregate results for the two deterministic
strategies are consistent with their shared evaluation objective.

These observations describe this experiment only.

## Limitations

- Ten matches per opponent are insufficient for robust statistical
  conclusions.
- Forty MCTS simulations and depth-1 deterministic search do not
  represent equivalent computational budgets.
- No wall-clock timing or node-count comparison was performed.
- All matches use one game and one fixed set of seeds.
- The benchmark tests reproducibility and legal completion; it does
  not assert that any strategy must win a particular number of games.

Future experiments may increase sample size and compare strategies
under measured computational budgets.
