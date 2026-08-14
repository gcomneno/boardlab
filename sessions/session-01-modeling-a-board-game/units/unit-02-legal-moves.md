# Unit 02 — Legal moves

[English](unit-02-legal-moves.md) | [Italiano](unit-02-legal-moves.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- distinguish an imaginable action from a legal move;
- explain why move legality depends on both rules and current state;
- distinguish move representation from legality evaluation;
- explain why a strategy should receive or derive only legal choices;
- recognize duplicated rule logic as an architectural risk.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- the game contract must allow legal moves to be obtained;
- `Move` describes a possible action without imposing one common
  representation on every game;
- `Strategy` chooses among legal moves using the game's public contract;
- the engine must not contain rules belonging to a concrete game.

Supporting reference:

- `README.md`

BoardLab requires algorithms to depend on abstract contracts rather than on
specific games.

The explanations and examples below are original BoardLab teaching material.

## Mental model

A move describes an action candidate.

The game rules decide whether that candidate is legal in the current state.

This distinction matters:

Move
→ describes what action is being considered

Game rules + current state
→ decide whether that action is currently allowed

The same move representation can therefore be legal in one state and illegal
in another.

## Technical explanation

Legal moves are derived from the combination of:

- the game's rules;
- the current `GameState`.

A strategy should not invent its own interpretation of legality.

If different strategies independently reimplement the rules, they can disagree
about what moves are allowed.

BoardLab's separation of responsibilities instead requires game-specific
legality to remain with the game domain.

The exact representation of `Move` remains game-specific.

One game may represent a move as a destination cell.

Another may require a source position, destination position, action type, or
additional parameters.

The generic engine should not force unrelated games into one artificial move
shape.

## Original example

Continue with the invented **Three Stones** game.

Current state:

- space 1: North;
- space 2: empty;
- space 3: South;
- next player: North.

Imagine moves represented simply by the selected space number.

Candidate move:

`2`

This move is legal because space 2 is empty.

Candidate move:

`1`

This move is not legal because space 1 is already occupied.

The integer `1` is still a representable action candidate.

Its illegality comes from evaluating that action against the current state and
rules.

## Why strategies should not own legality

Suppose a random strategy generates numbers from 1 to 3 and contains its own
code for checking whether a space is empty.

Later, a Minimax strategy implements the same rule separately.

Now the same game rule exists in at least three places:

- the game;
- the random strategy;
- the Minimax strategy.

A future rule change could update one implementation but not the others.

The strategies would then disagree about the game itself.

This violates BoardLab's intended architecture.

A generic strategy should operate through the public game contract instead.

## Common modeling mistakes

### Treating every representable move as legal

A move object can be constructed, so the program assumes the action is allowed.

Consequence:

representation is confused with domain validity.

### Duplicating legality rules inside strategies

Each algorithm independently checks game-specific rules.

Consequence:

strategies become coupled to concrete games and can disagree with one another.

### Letting the generic engine know concrete move rules

The match runner knows that a particular board cell must be empty.

Consequence:

the engine is no longer generic.

### Calculating legality without the complete state

The legality check depends on hidden information that is not part of
`GameState`.

Consequence:

the same apparent state can produce inconsistent legal-move sets.

## Problem scenario

A random strategy says move X is legal.

A Minimax strategy says the same move X is illegal.

Both are supposedly playing the same game from the same state.

This disagreement strongly suggests that game-specific legality has leaked into
the strategies instead of remaining in one authoritative domain contract.

## Takeaway

A legal move is not merely an action that can be represented.

It is an action permitted by the game's rules in one specific state.

BoardLab should keep that knowledge in the game domain so generic strategies
can consume one authoritative set of legal choices.

## Learner exercise

Using the original game state you described in Unit 01, invent three candidate
moves:

- one clearly legal;
- one clearly illegal;
- one whose legality would be impossible to determine if some important state
  information were missing.

For each candidate, explain which part of the state and which rule are needed
to decide legality.

Do not implement a move class or legality function yet.
