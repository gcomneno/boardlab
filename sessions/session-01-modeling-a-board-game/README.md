# Session 01 — Modeling a board game as a state-transition system

[English](README.md) | [Italiano](README.it.md)

## Status

Repository preparation: **Prepared**.

Active study: **not studied**.

## Purpose

This session builds the computational model needed to reason about board games
before implementing a concrete game or search algorithm.

The central idea is to describe gameplay in terms of states, legal moves,
state transitions, terminal conditions, players, strategies, and match
coordination.

## Relationship with BoardLab

The session prepares the concepts already introduced by BoardLab's architecture:

- `Game`;
- `GameState`;
- `Move`;
- `Player`;
- `Strategy`;
- `Match`.

It does not define their final Python interfaces and does not start active
implementation work.

## Preparation and orientation

Start with the documents that define scope and teaching order:

1. [Source coverage map](source-coverage-map.md)
2. [Study map](study-map.md)
3. [Quiz plan](quiz-plan.md)

The source coverage map defines what the session may claim and what remains
deferred.

The study map defines the conceptual dependency between units.

The quiz plan defines assessment coverage before learner assessment begins.

## Learning units

Study the units in order:

1. [Unit 01 — Game state](units/unit-01-game-state.md)
2. [Unit 02 — Legal moves](units/unit-02-legal-moves.md)
3. [Unit 03 — State transitions](units/unit-03-state-transitions.md)
4. [Unit 04 — Terminal states and outcome](units/unit-04-terminal-states-and-outcome.md)
5. [Unit 05 — Player and strategy](units/unit-05-player-and-strategy.md)
6. [Unit 06 — Match coordination and responsibility boundaries](units/unit-06-match-coordination.md)

The presence of these units means the teaching material has been prepared.

It does not mean they have been studied.

## Assessments

The session provides two learner-facing quizzes:

1. [Quiz 01 — State, legality, and transitions](quizzes/quiz-01-state-legality-transitions.md)
2. [Quiz 02 — Termination, strategy, and coordination](quizzes/quiz-02-termination-strategy-coordination.md)

Quiz files intentionally do not contain their answers.

## Review material

Consult answer keys only after an actual quiz attempt:

1. [Quiz 01 answer key](answer-keys/quiz-01-state-legality-transitions.md)
2. [Quiz 02 answer key](answer-keys/quiz-02-termination-strategy-coordination.md)

Creating or publishing answer keys does not count as learner review.

## Coverage review

The preparation audit is documented in:

- [Coverage review](coverage-review.md)

The coverage review currently concludes that content preparation is complete.

That conclusion is distinct from the final repository **Prepared** status.

## Deferred material

Tre Sigilli and concrete search algorithms remain deliberately deferred beyond
this foundational session.

The session does not yet introduce:

- final Python domain interfaces;
- concrete game implementation;
- random strategy implementation;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- performance benchmarking.

## Study state

No exercise, quiz, game analysis, answer review, or other learner activity has
been completed for this session yet.

Repository preparation and learner study remain separate states.
