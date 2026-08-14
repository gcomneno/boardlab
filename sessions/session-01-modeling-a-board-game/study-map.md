# Session 01 — Study map

[English](study-map.md) | [Italiano](study-map.it.md)

## Purpose

This map defines the teaching sequence for Session 01 before the individual
learning units are written.

The sequence moves from the representation of a game position to the
coordination of a complete match.

Each unit depends only on concepts introduced earlier in the sequence.

## Learning sequence

### Unit 01 — Game state

Primary question:

What information must exist to describe one complete game situation?

Learning focus:

- distinguish the game itself from one particular game position;
- identify the information that belongs in a complete state;
- reason about validity and independence of states;
- understand why search algorithms need explicit states rather than hidden
  mutable context.

Why this comes first:

Every later concept refers to a state. Legal moves, transitions, terminal
conditions, and strategy decisions have no precise meaning until the current
game situation can be represented explicitly.

### Unit 02 — Legal moves

Primary question:

Given a state, which actions are allowed?

Learning focus:

- distinguish possible actions from legal actions;
- understand that legality depends on the current state and game rules;
- separate move representation from rule enforcement;
- recognize why strategies must choose only among legal moves.

Why this comes second:

A move can only be judged relative to an already defined state.

### Unit 03 — State transitions

Primary question:

What does it mean to apply a legal move?

Learning focus:

- model gameplay as transitions from one state to another;
- distinguish the move from the resulting state;
- reason about deterministic state changes at the domain-model level;
- understand why search requires independent successor states.

Why this comes third:

Once states and legal moves exist, the next dependency is the operation that
connects them.

### Unit 04 — Terminal states and outcome

Primary question:

When has the game ended, and what does that state mean?

Learning focus:

- distinguish ongoing and terminal states;
- identify terminal conditions as game rules;
- separate termination from strategic evaluation;
- reason about outcomes from a player's point of view.

Why this comes fourth:

Termination is a property of states reached through transitions. Introducing
it earlier would require concepts that have not yet been established.

### Unit 05 — Player and strategy

Primary question:

Who participates in the game, and what chooses a move?

Learning focus:

- distinguish a player identity from the algorithm that selects actions;
- understand `Strategy` as a consumer of the game's public contract;
- prevent game-specific rules from leaking into generic decision algorithms;
- reason about interchangeable strategies.

Why this comes fifth:

A strategy needs states and legal moves before its responsibility can be
defined precisely.

### Unit 06 — Match coordination and responsibility boundaries

Primary question:

What coordinates a complete game without owning game-specific rules or
decision logic?

Learning focus:

- understand the responsibility of `Match`;
- connect players, strategies, states, moves, and termination;
- preserve dependency direction between engine, game, and strategy;
- recognize architectural boundary violations;
- form the complete Session 01 mental model.

Why this comes last:

Match coordination composes all earlier concepts. It is the first point where
the complete abstract model can be reasoned about as a system.

## Dependency chain

The intended conceptual dependency is:

Game state
→ legal moves
→ state transition
→ terminal condition and outcome
→ player and strategy
→ match coordination

Later units may revisit earlier concepts but should not require knowledge from
future units.

## Teaching pattern for each unit

When appropriate, each unit should contain:

- learning objectives;
- source references;
- intuitive mental model;
- technical explanation;
- one original concrete example;
- common modeling mistakes;
- failure or problem scenarios;
- practical takeaway;
- one learner question or exercise.

Exercises must not automatically include their solution.

## Deferred implementation

The study sequence prepares the conceptual model only.

It does not yet require:

- final Python protocols or abstract base classes;
- Tre Sigilli implementation;
- search algorithm implementation;
- benchmarks.

Those activities remain outside the Session 01 preparation boundary defined by
the source coverage map.

## Preparation status

This study map defines the planned teaching order.

The individual units have not yet been written and no learner activity has
been completed.
