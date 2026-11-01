# Slot Trasparente — Experiment 001

[English](mission-001.md) | [Italiano](mission-001.it.md)

- **Project:** GiadaWare BoardLab
- **Document:** M001 Mathematical and Educational Contract
- **Version:** 1.0
- **Status:** initial specification; implementation not performed
- **Domain:** virtual-credit simulation, without real money

## 1. Purpose

Slot Trasparente is an educational experiment about randomness,
expected value, and risk of ruin.

The game displays probabilities, wagers, payouts, net results,
and differences between observed outcomes and theoretical values.

It is not a therapeutic instrument and does not claim to treat
gambling disorder.

Principle: the system explains outcomes without manipulating them.

## 2. Reels

- Five independent reels.
- Three visible rows per reel.
- One paying line: the central row.
- Twenty equally likely positions on each circular reel strip.
- Identical symbol frequencies across all five reels.
- Each spin costs one virtual credit.
- No wild symbols or bonus features in M001.

| Symbol | Positions per reel | Probability |
|---|---:|---:|
| Cherry | 6 | 30% |
| Lemon | 5 | 25% |
| Orange | 4 | 20% |
| Grape | 3 | 15% |
| Bell | 1 | 5% |
| Seven | 1 | 5% |

The physical ordering of symbols on the strips remains undefined.
It does not change central-line probabilities, but determines
which symbols appear above and below the central row.

## 3. Payout rules

Only 3, 4, or 5 consecutive identical symbols, starting
from the leftmost reel, qualify for a payout.

Only the longest qualifying combination pays.
Prizes represent total payouts, including the wager.

For a symbol with probability p:

- Exactly 3: p^3 × (1-p).
- Exactly 4: p^4 × (1-p).
- Exactly 5: p^5.

Reels following the first mismatch do not affect the payout.

## 4. House-favored paytable

| Symbol | Prize 3 | Prize 4 | Prize 5 |
|---|---:|---:|---:|
| Cherry | 5 | 15 | 40 |
| Lemon | 8 | 25 | 80 |
| Orange | 12 | 40 | 150 |
| Grape | 20 | 80 | 300 |
| Bell | 100 | 500 | 2500 |
| Seven | 150 | 1000 | 5000 |

Exact theoretical return: 546167/640000 = 85.33859375%.
Expected loss per spin: 0.1466140625 credits.

## 5. Fair paytable

| Symbol | Prize 3 | Prize 4 | Prize 5 |
|---|---:|---:|---:|
| Cherry | 6 | 18 | 47 |
| Lemon | 9 | 29 | 94 |
| Orange | 14 | 47 | 176 |
| Grape | 23 | 96 | 352 |
| Bell | 117 | 586 | 2930 |
| Seven | 176 | 1176 | 5859 |

Exact theoretical return: 1 = 100%.
Expected net result per spin: zero.

Mathematical fairness does not guarantee breaking even,
recovering losses, or preserving available capital.

## 6. Custom paytable

Prizes can be modified, and theoretical return is recalculated.
Reels, symbol frequencies, and payout rules remain unchanged.

The paytable is fixed at the beginning of each session.
Prize validation and editor limits remain to be specified.

## 7. Probability and expected value

For each symbol s, with probability p_s and prizes P3, P4, P5:

RTP = sum_s [
    p_s^3 × (1-p_s) × P3
  + p_s^4 × (1-p_s) × P4
  + p_s^5 × P5
].

Theoretical probability of receiving a prize: 5.425%.

Net spin result = payout - 1.
Cumulative net result = total payouts - total wagers.

Mathematical verification uses exact rational arithmetic;
decimal display formatting does not determine validity.

## 8. Reproducibility

The engine will be implemented in ISO C11.

An explicit seed will make sequences reproducible.
The PRNG algorithm, uniform sampling of twenty positions,
reel strips, log format, and independent verification
must be specified before implementation.

Outcomes must not be changed to artificially correct
observed prize frequencies or returns.

Different paytables can be compared against the same
sequence of sampled reel positions.

## 9. Play / Study mode

Display during a session:

- Available capital and wager.
- Total wagers, total payouts, and cumulative net result.
- The selected paytable's theoretical expected result.
- Observed and theoretical prize frequencies.
- Explanation of positions, symbols, combinations, and payouts.
- Individual steps of the evaluation algorithm.

The individual spin result and cumulative session result
must remain clearly distinguishable.

## 10. Experiment mode

Initial configuration: 100 sessions of 100 spins,
with an initial capital of 100 credits per session.

Present charts, individual results, exhausted sessions,
first exhaustion spin, and sessions still active.

Under this configuration, capital cannot be exhausted
before spin 100. Both the observation horizon and
initial capital must therefore become configurable.

A playable session ends when capital is insufficient
to place the next wager.

Any subsequent draws belong exclusively to an explicitly
separate accounting-only simulation.

For sessions surviving the observation horizon, display:
"Balance not exhausted within the observed spins."

## 11. Educational messages

- Losses do not increase the next spin's winning probability.
- A win does not change future probabilities.
- A 100% RTP does not guarantee an individual break-even result.
- Fairness, capital survival, and ruin risk are distinct.
- A positive spin result does not imply a positive session balance.
- With finite initial capital and no top-ups, indefinite play
  eventually leads to ruin with probability 1, even under
  the fair paytable; no finite spin guarantees ruin.

No automatic top-ups or disproportionate win celebrations.

## 12. M001 boundaries

This phase defines the specification, not the software.

Before operational engine implementation, the following
remain to be defined: reel-strip ordering, PRNG and sampling,
seed handling, log format, custom prize validation,
experiment parameters, and testing criteria.

The C11 engine must remain independent of the interface.
A CLI will precede any graphical interface.

No use of real money.
