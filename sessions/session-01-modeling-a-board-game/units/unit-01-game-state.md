# Unit 01 — Game state

[English](unit-01-game-state.md) | [Italiano](unit-01-game-state.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- distinguish a game from one particular situation within that game;
- identify the information required to describe a complete game state;
- explain why state validity is a domain responsibility;
- explain why independent states matter for search and simulation;
- recognize hidden mutable context as a modeling risk.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- `GameState` represents a complete and valid configuration of a match;
- search must not clandestinely mutate a state;
- the conceptual game contract must support independent states usable during
  search.

Supporting project reference:

- `README.md`

BoardLab requires components to remain understandable, testable, and
replaceable.

The explanations and examples below are original BoardLab teaching material.

## Mental model

A game is the set of rules that defines what can happen.

A game state is one complete snapshot of what is true at a particular moment.

Think of the distinction as:

Game
→ defines the rules

Game state
→ records the current situation under those rules

A state should contain enough information to answer questions about the current
position without relying on invisible external variables.

## Technical explanation

A complete state contains every piece of domain information needed to describe
the current situation relevant to gameplay.

Depending on the game, that may include:

- positions of pieces;
- resources owned by players;
- whose turn it is;
- counters or phase information;
- previously established effects that still influence legal play.

The exact representation varies from game to game.

The architectural requirement is not that every game use the same fields. The
requirement is that a state be complete for its own game's rules.

A state should also be valid.

For example, if the rules say that exactly one player has the turn, a state
claiming that both players simultaneously have the turn would violate the
domain model.

## Original example

Consider an invented game called **Three Stones**.

Two players, North and South, alternately place one stone into one of three
empty spaces.

A possible state could be described as:

- space 1: North;
- space 2: empty;
- space 3: South;
- next player: North.

This snapshot is different from the game rules themselves.

The rules explain what players are allowed to do.

The state records what is currently true.

If `next player` were stored in an unrelated global variable instead of in the
state, the snapshot would no longer be self-contained.

That hidden dependency would make reasoning, testing, replay, and search more
fragile.

## State independence

Search algorithms need to explore hypothetical futures.

If applying a hypothetical move silently modifies the original current state,
different search branches can interfere with one another.

BoardLab therefore requires a state representation that can produce an
independent successor state for search.

This does not prescribe a particular Python implementation yet.

The important property is conceptual:

examining one hypothetical future must not corrupt another.

## Common modeling mistakes

### Incomplete state

Some gameplay-relevant information exists only in an external variable.

Consequence:

the same visible state can behave differently depending on hidden context.

### Mixing rules into the snapshot

A state contains procedural logic describing how the game works instead of
representing the current situation.

Consequence:

state representation and game rules become unnecessarily coupled.

### Accidental shared mutation

Two supposedly independent hypothetical states share mutable data.

Consequence:

changing one branch can alter another branch.

### Storing irrelevant presentation data as domain truth

UI layout or display-only information is treated as part of the game state.

Consequence:

the domain model becomes coupled to a particular interface.

## Problem scenario

A search algorithm explores two candidate moves from the same current state.

It generates state A for the first candidate and state B for the second.

Changing a piece inside state A also changes the corresponding piece inside
state B.

The two search branches are therefore not independent.

The problem is not primarily a search-algorithm error.

It is a state-modeling error.

## Takeaway

A `GameState` should be a complete, valid, and independently usable
representation of one game situation.

If important gameplay information lives outside the state, or hypothetical
states unexpectedly affect one another, later search algorithms cannot reason
reliably.

## Learner exercise

Imagine an original two-player game in which players alternately claim cells on
a small board and each player has a limited number of tokens.

List the minimum information you believe one complete game state must contain.

For each item, explain briefly why leaving it outside the state could create
ambiguity.

Do not design Python classes yet.
