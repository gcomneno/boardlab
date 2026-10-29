# Architecture overview

[English](overview.md) | [Italiano](overview.it.md)

## Objective

BoardLab separates the game domain from the algorithms that select moves.

The engine coordinates a match but contains no rules specific to any game.

## Main responsibilities

### Game

Defines the rules and creates a game's initial state.

### GameState

Represents a complete, valid match configuration.

### Move

Describes a possible action without requiring a common representation
across all games.

### Player

Identifies one of the participants in a match.

### Strategy

Selects a move from the legal moves using only the game's public contract.

### Match

Coordinates turns, strategies, termination and the match result.

## Dependencies

```text
Match
  |
  +-- Game
  |     |
  |     +-- GameState
  |     +-- Move
  |     +-- Player
  |
  +-- Strategy
        |
        +-- statistiche di ricerca
```

## Architecture rules

1. The engine does not import concrete game implementations.
2. Strategies do not know about Tre Sigilli or other specific games.
3. A state is not silently modified during a search.
4. Statistics are explicit results, not global variables.
5. Optimizations and caches must preserve observable behavior.
6. The CLI uses the domain, but the domain does not depend on the CLI.

## Minimum conceptual contract

A game must at least support:

- retrieving legal moves;
- applying a move;
- determining whether a state is terminal;
- evaluating a state from a player's perspective;
- producing an independent state suitable for search.

The final shape of the interfaces will be decided before implementing
the algorithms.
