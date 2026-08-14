# Quiz 02 — Termination, strategy, and coordination

[English](quiz-02-termination-strategy-coordination.md) | [Italiano](quiz-02-termination-strategy-coordination.it.md)

## Scope

This quiz covers:

- Unit 04 — Terminal states and outcome;
- Unit 05 — Player and strategy;
- Unit 06 — Match coordination and responsibility boundaries.

Answer each question before consulting later review material.

## Question 1 — Terminality versus evaluation

A strategy considers an ongoing `GameState` highly favorable for North.

The game rules still permit several legal moves.

Which statement is most accurate?

A. The state must be terminal because it already has a strong evaluation.

B. Strategic evaluation and terminal-state detection are different concepts.

C. The strategy should end the match immediately.

D. `Player` should decide whether the state is terminal.

## Question 2 — Player-relative outcome

A terminal state represents a North victory and a South defeat.

What does this example demonstrate most clearly?

A. One terminal state may have different meaning depending on player perspective.

B. Every player should receive the same outcome label.

C. Outcome belongs entirely to the CLI.

D. A terminal state must contain one strategy per player.

## Question 3 — Player versus strategy

A match uses these associations:

- North uses First Legal;
- South uses Random Legal.

A second match swaps the two strategies without changing the game rules.

What architectural property makes this possible?

A. Player identity and strategy behavior are separate responsibilities.

B. Every player owns one permanent strategy implementation.

C. `Match` implements both strategies internally.

D. The game rules depend on which strategy is assigned.

## Question 4 — Strategy boundary

A generic search strategy contains this concrete rule:

"Space 2 may be selected only when that board cell is empty."

What is the main problem?

A. The strategy contains game-specific legality knowledge.

B. The strategy should instead store the rule in a global variable.

C. `Player` should duplicate the same rule.

D. Every generic strategy must know concrete board layouts.

## Question 5 — Match responsibility

During a turn, which responsibility most appropriately belongs to `Match`?

A. Decide whether a particular board space is legal according to one concrete game's rules.

B. Choose the move using Minimax logic directly.

C. Coordinate the active player, associated strategy, selected move, state transition, and termination check.

D. Define how a concrete game's pieces are represented.

## Question 6 — Dependency boundary

A new concrete game is added to BoardLab.

Developers discover that they must modify generic `Match` logic because it
contains assumptions about the previous game's board layout.

What is the strongest architectural warning?

A. The new game should be removed.

B. `Match` has absorbed concrete game knowledge and is no longer sufficiently generic.

C. Every new game should require changes to generic orchestration.

D. The CLI should own the board layout instead.

## Attempt record

The existence of this quiz does not mean the assessment has been completed.

The learner should mark it as attempted only after actually answering all six
questions.
