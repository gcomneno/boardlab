# Session 01 — Quiz plan

[English](quiz-plan.md) | [Italiano](quiz-plan.it.md)

## Purpose

This plan defines the assessment strategy for Session 01 before quiz questions
and answer keys are written.

The quizzes must measure whether the learner can reason about BoardLab's
abstract game model and responsibility boundaries, not merely repeat
definitions.

Quiz files and answer keys must remain strictly separated.

## Assessment goals

The assessment should verify that the learner can:

- identify what belongs in a complete `GameState`;
- distinguish representable actions from legal `Move` values;
- reason about state transitions and independent successor states;
- distinguish terminality, outcome, and strategic evaluation;
- distinguish `Player` identity from `Strategy` behavior;
- explain the responsibility of `Match`;
- identify leakage of concrete game rules into generic components;
- reason about dependency direction between game, engine, and strategy.

## Assessment structure

Session 01 will use two quizzes.

### Quiz 01 — State, legality, and transitions

Coverage:

- Unit 01 — Game state;
- Unit 02 — Legal moves;
- Unit 03 — State transitions.

Question count:

6 questions.

Primary measurement targets:

- state completeness;
- state validity;
- hidden mutable context;
- move representation versus legality;
- authoritative location of game rules;
- transition correctness;
- successor-state independence.

### Quiz 02 — Termination, strategy, and coordination

Coverage:

- Unit 04 — Terminal states and outcome;
- Unit 05 — Player and strategy;
- Unit 06 — Match coordination and responsibility boundaries.

Question count:

6 questions.

Primary measurement targets:

- terminality versus evaluation;
- player-relative outcome;
- player identity versus strategy behavior;
- strategy interchangeability;
- generic versus game-specific responsibility;
- `Match` orchestration boundaries;
- dependency direction.

## Question styles

The quizzes should mix:

- conceptual multiple-choice questions;
- short scenario analysis;
- responsibility-placement questions;
- misconception detection;
- small architecture reasoning questions.

Pure vocabulary recall should be limited.

At least half of the questions across the two quizzes should require reasoning
about a concrete scenario rather than recalling a definition.

## Misconceptions to target

### State misconceptions

- visible board position alone is always a complete state;
- hidden global context is acceptable if the program can access it;
- hypothetical search states may safely share mutable gameplay data;
- UI representation belongs automatically in the domain state.

### Move and legality misconceptions

- any representable move is legal;
- strategies should independently implement move legality;
- the generic engine should know concrete board rules;
- legality can be determined without all relevant state information.

### Transition misconceptions

- a move and its successor state are the same concept;
- a legal move automatically guarantees a correct successor state;
- mutating the original state is harmless during hypothetical search;
- strategies should directly perform game-specific state transitions.

### Terminal-state misconceptions

- no legal moves universally means the game is over;
- every evaluated state is terminal;
- terminal detection and strategic evaluation are equivalent;
- outcome has one universal meaning independent of player perspective.

### Player and strategy misconceptions

- player identity and strategy are one responsibility;
- strategies should contain concrete game rules;
- changing strategy requires changing the game domain;
- a generic strategy may assume one fixed player perspective.

### Match misconceptions

- `Match` should contain concrete move rules because it controls turns;
- `Match` should choose moves itself;
- the CLI may be part of the game domain;
- adding a new game should normally require changes to generic match logic.

## Coverage matrix

| Unit | Quiz | Planned questions |
|---|---|---:|
| Unit 01 — Game state | Quiz 01 | 2 |
| Unit 02 — Legal moves | Quiz 01 | 2 |
| Unit 03 — State transitions | Quiz 01 | 2 |
| Unit 04 — Terminal states and outcome | Quiz 02 | 2 |
| Unit 05 — Player and strategy | Quiz 02 | 2 |
| Unit 06 — Match coordination | Quiz 02 | 2 |

Each unit therefore receives direct assessment coverage.

Questions may combine concepts from earlier units when useful, but no question
should require material deliberately deferred beyond Session 01.

## Quiz publication rules

Quiz files must not contain:

- the correct answer;
- answer keys;
- solution sections;
- explanations that directly reveal the expected answer.

The repository validator must eventually enforce this separation
automatically.

## Answer-key requirements

Each quiz must have a separate answer key.

For every question, the answer key should explain at least:

- expected answer or reasoning;
- why that reasoning is defensible from Session 01 material;
- the typical misconception being tested;
- the practical takeaway.

Answer keys are review material and should be consulted only after an actual
quiz attempt during active study.

## Language policy

Every quiz and answer key must be published as an English canonical document
with an Italian `.it.md` counterpart.

Question numbering, technical identifiers, answer structure, and scenario
meaning must remain synchronized across each pair.

## Assessment boundary

The quizzes must not require:

- final Python interfaces;
- Tre Sigilli rules;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- performance analysis;
- knowledge of commercial board games.

The assessment must remain within the source coverage and study map of
Session 01.

## Preparation status

This plan defines the assessment structure only.

No quiz question or answer key has been written yet.

No learner assessment has been attempted.
