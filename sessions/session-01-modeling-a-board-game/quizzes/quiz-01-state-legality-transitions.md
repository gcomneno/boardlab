# Quiz 01 — State, legality, and transitions

[English](quiz-01-state-legality-transitions.md) | [Italiano](quiz-01-state-legality-transitions.it.md)

## Scope

This quiz covers:

- Unit 01 — Game state;
- Unit 02 — Legal moves;
- Unit 03 — State transitions.

Answer each question before consulting later review material.

## Question 1 — Complete state

An invented two-player game uses a three-cell board.

The visible board is:

- cell 1: North;
- cell 2: empty;
- cell 3: South.

The active player is stored only in a global variable outside `GameState`.

Which statement best describes the modeling problem?

A. There is no problem because the active player can still be read by the program.

B. The state is incomplete because gameplay behavior depends on information not contained in the state.

C. The state is invalid because every valid state must contain exactly three fields.

D. The active player belongs to `Strategy`, not to the game state.

## Question 2 — Independent hypothetical states

A search explores two candidate moves from the same current `GameState`.

After generating successor A, modifying one of its pieces unexpectedly changes
the corresponding piece in successor B.

What architectural property has most clearly been violated?

A. Player identity must be immutable.

B. Every move must have a unique textual representation.

C. Hypothetical successor states must be independently usable.

D. A strategy must always choose moves randomly.

## Question 3 — Representable versus legal

In **Three Stones**, moves are represented by the number of the selected space.

Current state:

- space 1: North;
- space 2: empty;
- space 3: South;
- next player: North.

The value `1` is a representable move candidate.

What additional information is required to determine whether it may actually be
played?

A. Only the Python type used to represent the move.

B. The current state together with the game's rules.

C. The strategy that generated the candidate.

D. The CLI command used to start the match.

## Question 4 — Responsibility for legality

BoardLab gains a Random Legal strategy and a future search strategy.

Both strategies separately implement the concrete rule that a move may target
only an empty board space.

What is the main architectural problem?

A. Each strategy should instead use a different representation for `Move`.

B. Concrete game-rule knowledge has been duplicated inside generic decision
   algorithms.

C. The strategies should store the active player globally.

D. `Match` should contain a third copy of the same rule.

## Question 5 — Legal move, invalid successor

A game's current state is valid and the selected move is legal.

The rules require turns to alternate between North and South.

After North's legal move, the produced successor state still records North as
the active player.

Which statement best characterizes the defect?

A. A legal move guarantees that every possible successor representation is valid.

B. The move representation is necessarily wrong.

C. Move legality succeeded, but the state transition produced a state that
   violates the game rules.

D. The strategy should repair the active player before returning the move.

## Question 6 — Search branch contamination

A search evaluates candidate A by applying it directly to the current state.

It then evaluates candidate B using that same object, which still contains the
changes produced while examining candidate A.

Where is the most fundamental problem?

A. The transition process failed to preserve an independent starting state for
   hypothetical branches.

B. Candidate B should always be evaluated before candidate A.

C. `Player` should restore the state after each strategy call.

D. Search algorithms require concrete knowledge of the game's board layout.

## Attempt record

Do not record completion in repository progress merely because this quiz file
exists.

The learner should mark the assessment as attempted only after actually
answering all six questions.
