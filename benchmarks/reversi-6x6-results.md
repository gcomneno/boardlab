# Reversi 6×6 — Pilot results

[English](reversi-6x6-results.md) · [Italiano](reversi-6x6-results.it.md) ·
[Protocol](reversi-6x6.md) ·
[Machine-readable results](reversi-6x6-results.json)

## Experimental setup

- Baseline: Random.
- Candidates: Minimax, Alpha-Beta, MCTS.
- Seeds: 0, 1, 2, 3, 4.
- Both candidate roles: BLACK and WHITE.
- Minimax and Alpha-Beta: depth 2, disc-difference evaluation.
- MCTS: 32 simulations per decision.
- Ten matches per candidate; 30 matches overall.
- Source commit: `3296607`.

## Outcomes

All outcomes are from the candidate perspective.

| Candidate | Matches | Wins | Draws | Losses |
|---|---:|---:|---:|---:|
| Minimax | 10 | 7 | 0 | 3 |
| Alpha-Beta | 10 | 7 | 0 | 3 |
| MCTS | 10 | 10 | 0 | 0 |

## Observed costs

| Candidate | Total actions | Elapsed seconds | Visited nodes | Evaluated leaves |
|---|---:|---:|---:|---:|
| Minimax | 331 | 0.552 | 4668 | 3778 |
| Alpha-Beta | 331 | 0.381 | 2929 | 2039 |
| MCTS | 330 | 8.979 | N/A | N/A |

Elapsed time is observational, environment-dependent and not a
deterministic reproducibility criterion. Action counts include both
participants' turns. Search statistics are accumulated for the candidate
only. MCTS simulations are not equivalent to Minimax or Alpha-Beta
visited nodes, so search budgets are not directly comparable.

## Interpretation and limitations

MCTS achieved the highest win count in this pilot against Random.
This is a descriptive observation, not evidence of general algorithmic
superiority. The sample contains only ten matches per candidate,
one baseline opponent, one board size and fixed search settings.
The experiment is not a full round-robin and does not establish
equal computational budgets.

Deterministic outcomes, actions and applicable search statistics can
be checked by rerunning the same protocol. Wall-clock measurements
may differ across runs and machines.

Results SHA-256: `a6fedd8f05605ba36609edc15b5b6f86be9697f1639707a17bb89fa49c45ec87`.
