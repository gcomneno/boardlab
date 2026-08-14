# Unit 05 — Player and strategy

[English](unit-05-player-and-strategy.md) | [Italiano](unit-05-player-and-strategy.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- distinguish player identity from move-selection logic;
- explain the responsibility of `Player`;
- explain the responsibility of `Strategy`;
- explain why strategies should depend on the public game contract;
- reason about interchangeable strategies without changing game rules;
- recognize game-specific rule leakage inside generic strategies.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- `Player` identifies one participant in the match;
- `Strategy` chooses a move among the legal moves;
- strategies use only the public contract of the game;
- strategies must not know Tre Sigilli or other concrete games.

Supporting reference:

- `README.md`

BoardLab requires engine, game, and strategy to remain separated and algorithms
to depend only on abstract contracts.

The explanations and examples below are original BoardLab teaching material.

## Mental model

A player answers:

Who is participating?

A strategy answers:

How is that participant's next move selected?

These are different responsibilities.

The same player identity can be associated with different strategies across
different experiments.

Likewise, the same generic strategy can potentially be used by different
players and different games when the public contracts are compatible.

## Technical explanation

`Player` represents participant identity within the game model.

Depending on the future implementation, that identity may be represented by an
identifier, enum-like value, domain object, or another simple mechanism.

This unit does not decide the final Python form.

`Strategy` represents decision behavior.

Conceptually, a strategy needs enough public information to:

- inspect the relevant state;
- obtain legal choices;
- select one move;
- optionally produce explicit search statistics in later phases.

The strategy should not own the rules that determine whether moves are legal.

It should not directly mutate the game state according to concrete game rules.

It should not decide when a particular concrete game is over.

Those responsibilities belong to the game domain.

## Original example

Continue with the invented **Three Stones** game.

Participants:

- North;
- South.

These are player identities.

Now imagine two possible strategies:

**First Legal**

Choose the first legal move returned by the game.

**Random Legal**

Choose one move randomly from the legal moves returned by the game.

North could use First Legal while South uses Random Legal.

In another match, the strategies could be swapped.

Nothing about the rules of Three Stones needs to change.

The players remain the participants.

The strategies determine how moves are selected for those participants.

## Strategy interchangeability

Separating player identity from strategy makes controlled experiments possible.

For example:

Match A:

- North → First Legal;
- South → Random Legal.

Match B:

- North → Random Legal;
- South → First Legal.

The game rules remain identical.

Only the decision behavior changes.

This is essential for later BoardLab experiments comparing algorithms.

If player identity and strategy logic were fused together, changing an
algorithm could require modifying participant or game-domain code.

## Public-contract boundary

A generic strategy should depend on abstractions exposed by the game.

Conceptually, it may need operations such as:

- obtain legal moves from the current state;
- produce successor states;
- determine terminal states;
- evaluate states from a player perspective.

It should not contain rules such as:

"space 2 is legal only when empty"

or:

"three occupied spaces end Three Stones."

Those are concrete game rules.

The strategy should consume their results through the public game contract.

## Common modeling mistakes

### Player and strategy are the same object conceptually

Participant identity and decision behavior are fused.

Consequence:

experiments with different algorithms become harder to configure and reason
about.

### Strategy reimplements legality

A generic algorithm checks concrete board rules directly.

Consequence:

the strategy becomes coupled to one game.

### Strategy mutates domain state directly

The strategy changes pieces or counters itself.

Consequence:

decision logic also becomes transition logic.

### Strategy owns terminal rules

The algorithm decides whether a concrete game has ended.

Consequence:

termination semantics are duplicated outside the game domain.

### Strategy assumes one universal player perspective

Evaluation implicitly favors a fixed participant.

Consequence:

the same strategy cannot reason cleanly from another player's point of view.

## Problem scenario

BoardLab has two strategies:

- Random Legal;
- Future Minimax.

Both contain their own implementation of one concrete game's move-legality
rules.

A game rule changes.

Random Legal is updated.

Future Minimax is not.

The two strategies now disagree about the legal-move set for the same state.

The architectural defect is duplicated game knowledge inside strategies.

## Takeaway

`Player` identifies who participates.

`Strategy` determines how a move is selected for a participant.

Keeping them separate makes strategies interchangeable, supports controlled
experiments, and prevents generic algorithms from becoming coupled to concrete
games.

## Learner exercise

Using your original game:

1. identify the two player identities;
2. invent two different move-selection strategies without changing the game
   rules;
3. describe how the strategies could be swapped between players;
4. list two pieces of information a strategy needs from the game contract;
5. give one example of a concrete game rule that must not be implemented inside
   the strategy.

Do not implement strategy classes yet.
