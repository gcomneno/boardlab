# Unit 06 — Match coordination and responsibility boundaries

[English](unit-06-match-coordination.md) | [Italiano](unit-06-match-coordination.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- explain the responsibility of `Match`;
- connect `Game`, `GameState`, `Move`, `Player`, and `Strategy` into one
  complete conceptual flow;
- distinguish coordination from game-specific rules;
- distinguish coordination from move-selection logic;
- explain the intended dependency direction between engine, game, and strategy;
- recognize responsibility-boundary violations.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- `Match` coordinates turns, strategies, termination, and result;
- the engine must not import concrete game implementations;
- strategies must not know concrete games;
- the CLI may depend on the domain, while the domain must not depend on the
  CLI;
- statistics must be explicit results rather than global variables.

Supporting reference:

- `README.md`

BoardLab requires engine, game, and strategy to remain separated.

The explanations and examples below are original BoardLab teaching material.

## Mental model

A match coordinator is an orchestrator.

It does not define the rules.

It does not invent legal moves.

It does not decide how a strategy thinks.

It coordinates the participants and domain operations needed to progress from
the initial state to a terminal result.

A simplified mental model is:

`Game`
→ provides the game rules and initial state

`Match`
→ coordinates the flow

`Player`
→ identifies each participant

`Strategy`
→ selects a legal move

`Game`
→ applies the move and produces the next state

The cycle continues until the game reports a terminal state.

## Technical explanation

Conceptually, a `Match` needs to coordinate operations such as:

1. obtain the initial `GameState`;
2. determine the active `Player`;
3. obtain the strategy associated with that player;
4. provide the relevant public game information to the strategy;
5. receive one selected legal `Move`;
6. ask the game domain to apply that move;
7. obtain the successor `GameState`;
8. determine whether the new state is terminal;
9. repeat or produce the final result.

This sequence describes coordination.

It does not imply final Python method names or interfaces.

Those remain deliberately deferred.

## Responsibility boundaries

### `Game`

Owns game-specific semantics.

Examples:

- legal moves;
- transitions;
- terminal conditions;
- domain evaluation behavior;
- initial state.

### `GameState`

Represents one complete and valid game situation.

### `Move`

Represents one action candidate in the vocabulary of a concrete game.

### `Player`

Identifies one participant.

### `Strategy`

Chooses among legal options using the public game contract.

### `Match`

Coordinates the interaction between these responsibilities.

It should not absorb the responsibilities of the other concepts.

## Original example

Continue with **Three Stones**.

A conceptual match could proceed as follows:

1. the game creates the initial empty state;
2. North is the active player;
3. North's strategy receives the available legal moves;
4. the strategy selects space 2;
5. the game applies the move;
6. the resulting state says South is active;
7. the match checks whether the state is terminal;
8. if not terminal, South's strategy is invoked;
9. the process continues until the game reports termination;
10. the final outcome is obtained from the game domain.

The match coordinator does not need to know why space 2 was legal.

It does not need to know how the board is represented.

It does not need to know how Random Legal or Future Minimax chooses.

Those details remain behind their corresponding boundaries.

## Dependency direction

BoardLab's intended architecture keeps generic components depending on
contracts rather than concrete game knowledge.

Conceptually:

Match
→ Game contract
→ GameState / Move / Player

Match
→ Strategy contract

Strategy
→ public game contract

Concrete game
→ generic engine contracts

The generic engine must not import a concrete game merely to understand its
rules.

A concrete game may implement or depend on generic contracts.

The dependency direction preserves replaceability.

## Why coordination must remain thin

A coordinator naturally sees many components.

That makes it especially easy for responsibilities to accumulate inside it.

For example, developers may be tempted to put inside `Match`:

- board validation;
- game-specific legal-move checks;
- strategy heuristics;
- terminal rules;
- UI formatting;
- hidden global statistics.

Each addition makes the coordinator less generic and harder to test.

The match should coordinate domain behavior rather than become the domain.

## Common modeling mistakes

### Match owns concrete rules

The coordinator knows that one specific cell must be empty.

Consequence:

the generic engine becomes coupled to one game.

### Match chooses moves itself

The coordinator contains random, heuristic, or search logic.

Consequence:

coordination and strategy become fused.

### Strategy applies moves directly

The selected algorithm mutates the game state itself.

Consequence:

decision behavior and domain transition become fused.

### Game controls the entire match loop

The concrete game directly orchestrates strategies and participants.

Consequence:

game rules and generic match coordination become coupled.

### CLI becomes part of the domain

Game objects depend on terminal input or output.

Consequence:

the domain can no longer be reused independently of the interface.

## Problem scenario

A new game is added to BoardLab.

To support it, developers must modify the generic `Match` coordinator because
the coordinator contains assumptions about how the previous game's board works.

That requirement is a warning sign.

Adding a new concrete game should primarily require implementing the game
contract, not teaching the generic coordinator new game rules.

## Complete Session 01 mental model

The Session 01 model can now be summarized as:

`Game`
→ defines game-specific behavior

`GameState`
→ represents one complete situation

`Move`
→ represents an action candidate

`Player`
→ identifies a participant

`Strategy`
→ selects among legal choices

`Match`
→ coordinates the progression of play

Together they support the conceptual flow:

current state
→ legal moves
→ strategy selection
→ state transition
→ terminal check
→ next turn or result

This is the conceptual foundation on which later BoardLab implementation and
search algorithms can be built.

## Takeaway

A generic `Match` coordinates a game without owning its rules or its decision
algorithms.

Clear responsibility boundaries allow BoardLab to add new games and new
strategies independently.

The value of the architecture comes not only from the individual concepts, but
from preserving the direction of their dependencies.

## Learner exercise

Using your original game and the strategies invented in previous exercises:

1. describe one complete turn from current state to successor state;
2. identify which responsibility belongs to `Game`;
3. identify which responsibility belongs to `GameState`;
4. identify which responsibility belongs to `Move`;
5. identify which responsibility belongs to `Player`;
6. identify which responsibility belongs to `Strategy`;
7. identify which responsibility belongs to `Match`;
8. give one example of a responsibility that would make `Match` too
   game-specific.

Do not design the final Python interfaces yet.
