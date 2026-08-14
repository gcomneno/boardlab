# Unit 03 — State transitions

[English](unit-03-state-transitions.md) | [Italiano](unit-03-state-transitions.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- explain a state transition as the application of one legal move to one state;
- distinguish a move from the state produced by that move;
- explain why successor states must preserve domain validity;
- explain why search requires independent successor states;
- recognize hidden side effects during state transition as a modeling defect.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- the conceptual game contract must allow a move to be applied;
- a search must not clandestinely mutate a state;
- the game must be able to produce an independent state suitable for search;
- `GameState` represents a complete and valid game configuration.

Supporting reference:

- `README.md`

BoardLab requires engine, game, and strategy responsibilities to remain
separated and components to remain understandable and testable.

The explanations and examples below are original BoardLab teaching material.

## Mental model

Gameplay can be viewed as a sequence of snapshots connected by actions.

The basic shape is:

Current state
→ legal move
→ next state

The move describes what happens.

The transition describes how applying that move changes the complete game
situation.

The next state is not the move itself.

It is a new description of what is true after the move has been applied.

## Technical explanation

A state transition receives two conceptual inputs:

- one valid current `GameState`;
- one legal `Move`.

It produces a successor `GameState`.

That successor must satisfy the game's rules and contain all information needed
to continue play.

For example, applying a move may change:

- piece positions;
- ownership;
- remaining resources;
- active player;
- phase or counters;
- effects that influence future legal moves.

A transition therefore belongs to the game domain.

A generic strategy should not decide how game state changes.

A generic match coordinator should not contain concrete rules describing how a
specific move updates a specific board.

## Original example

Continue with the invented **Three Stones** game.

Current state:

- space 1: North;
- space 2: empty;
- space 3: South;
- next player: North.

Legal move:

North chooses space 2.

After applying that move, the successor state becomes:

- space 1: North;
- space 2: North;
- space 3: South;
- next player: South.

The move can be represented simply as the selection of space 2.

The successor state contains much more information than the move.

It records the entire resulting game situation.

## Transition validity

A legal move applied to a valid state should produce another valid state.

Suppose the transition above instead produced:

- space 1: North;
- space 2: North;
- space 3: South;
- next player: North.

If the rules require alternating turns, this successor would violate the game
model.

The candidate move may have been legal.

The transition implementation would still be wrong.

Move legality and transition correctness are related but distinct concerns.

## Independent successor states

Search algorithms often explore several legal moves from the same state.

Conceptually:

Current state
→ candidate A
→ successor A

Current state
→ candidate B
→ successor B

Successor A and successor B must be independently usable.

Exploring candidate A must not alter the state from which candidate B is later
derived.

BoardLab therefore requires state transitions that preserve independence
between hypothetical search branches.

This requirement does not yet prescribe whether the future Python
implementation will use immutable objects, copying, persistent data structures,
or another technique.

The required observable behavior is independence.

## Common modeling mistakes

### Mutating the original state unexpectedly

Applying a hypothetical move modifies the state supplied by the caller.

Consequence:

later operations no longer start from the state they expected.

### Returning an incomplete successor

The transition changes the visible board but forgets to update the active
player or another gameplay-relevant field.

Consequence:

the successor does not describe a complete game situation.

### Letting strategies perform transitions

A strategy directly modifies pieces or resources according to game-specific
rules.

Consequence:

decision logic becomes coupled to the concrete game.

### Confusing a move with its result

The program treats the action description as though it already contained the
complete successor state.

Consequence:

responsibilities between action representation and domain transition become
blurred.

## Problem scenario

A strategy evaluates two candidate moves.

After evaluating the first candidate, the supposedly original current state has
changed.

The second candidate is therefore explored from a different starting point.

The search can now produce inconsistent results even if the search algorithm
itself is logically correct.

The underlying defect is uncontrolled state mutation during transition.

## Takeaway

A state transition connects one valid `GameState` to another by applying one
legal `Move`.

The successor must be complete, valid, and independent enough for later play or
hypothetical search.

The game domain owns this transformation.

## Learner exercise

Using the original game you described in the previous exercises:

1. write one valid current state;
2. choose one legal move;
3. describe the complete successor state after applying that move;
4. list every field that changed;
5. explain one bug that could occur if the original state and successor state
   accidentally shared mutable gameplay data.

Do not implement the transition in Python yet.
