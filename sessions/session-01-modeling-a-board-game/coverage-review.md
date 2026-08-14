# Session 01 — Coverage review

[English](coverage-review.md) | [Italiano](coverage-review.it.md)

## Purpose

This review checks whether Session 01 content preparation is complete before
the repository is allowed to mark the session as **Prepared**.

Content completeness and repository readiness are separate gates.

This document may conclude that content preparation is complete while the
session remains in **Preparation in progress** until repository-wide readiness
validation has passed.

## Session objective

Session 01 prepares the computational model needed to reason about a board game
before implementing a concrete game or search algorithm.

The intended conceptual scope is:

- game state;
- legal moves;
- state transitions;
- terminal states and outcome;
- player identity;
- strategy behavior;
- match coordination;
- responsibility and dependency boundaries.

## Objective-to-unit coverage

| Objective | Primary unit | Coverage |
|---|---|---|
| Complete and valid game state | Unit 01 | Covered |
| Legal moves and rule authority | Unit 02 | Covered |
| State transitions and successor independence | Unit 03 | Covered |
| Terminality, outcome, and evaluation distinction | Unit 04 | Covered |
| Player identity versus strategy behavior | Unit 05 | Covered |
| Match coordination and responsibility boundaries | Unit 06 | Covered |

All planned conceptual objectives have one primary teaching unit.

## Unit-to-assessment coverage

| Unit | Assessment | Direct questions | Answer key |
|---|---|---:|---|
| Unit 01 — Game state | Quiz 01 | 2 | Present |
| Unit 02 — Legal moves | Quiz 01 | 2 | Present |
| Unit 03 — State transitions | Quiz 01 | 2 | Present |
| Unit 04 — Terminal states and outcome | Quiz 02 | 2 | Present |
| Unit 05 — Player and strategy | Quiz 02 | 2 | Present |
| Unit 06 — Match coordination | Quiz 02 | 2 | Present |

Every unit has direct assessment coverage.

Quiz 01 contains 6 questions.

Quiz 02 contains 6 questions.

The complete Session 01 assessment therefore contains 12 questions.

## Misconception coverage

### State

Covered misconceptions:

- visible board representation is automatically a complete state;
- hidden global gameplay context is acceptable;
- hypothetical states may share mutable gameplay data safely.

Covered by:

- Unit 01;
- Quiz 01 questions 1 and 2.

### Move legality

Covered misconceptions:

- any representable action is legal;
- strategies should implement concrete legality rules themselves.

Covered by:

- Unit 02;
- Quiz 01 questions 3 and 4.

### State transitions

Covered misconceptions:

- a legal move automatically guarantees a valid successor;
- hypothetical search branches may mutate the same current state.

Covered by:

- Unit 03;
- Quiz 01 questions 5 and 6.

### Terminality and evaluation

Covered misconceptions:

- a strongly evaluated state must be terminal;
- one terminal outcome has identical meaning for every player.

Covered by:

- Unit 04;
- Quiz 02 questions 1 and 2.

### Player and strategy

Covered misconceptions:

- player identity and strategy behavior are one responsibility;
- generic strategies should contain concrete game rules.

Covered by:

- Unit 05;
- Quiz 02 questions 3 and 4.

### Match coordination

Covered misconceptions:

- `Match` should own game-specific rules or decision algorithms;
- adding a new game should normally require changes to generic orchestration.

Covered by:

- Unit 06;
- Quiz 02 questions 5 and 6.

All misconception groups defined by the quiz plan receive direct assessment.

## Answer-key completeness

Each of the 12 quiz questions has separate review material.

For every question, the answer key includes:

- expected answer;
- reasoning;
- typical misconception;
- practical takeaway.

Answer keys are stored separately from quiz files.

They are intended for review after an actual learner attempt.

## Quiz separation

Quiz files contain questions and answer options only.

They must remain free of:

- correct-answer markers;
- answer-key sections;
- solution sections;
- direct explanatory leakage.

This separation must be enforced automatically before the session becomes
**Prepared**.

## Source coverage

The session currently relies on BoardLab repository sources already identified
in the source coverage map.

Primary sources:

- `docs/architecture/overview.md`;
- `README.md`.

Supporting sources:

- `docs/roadmap.md`;
- `docs/architecture/adr/0001-python-toolchain.md`.

No external commercial rulebook, protected manual, textbook, or commercial
game material is required by the prepared teaching content.

Original examples such as Three Stones are BoardLab teaching artifacts.

## Publication boundary

The session does not require publication of:

- commercial rulebook PDFs;
- scans;
- protected artwork;
- substantial card-text collections;
- copied commercial rule sets;
- source transcripts;
- private study corpora.

Private material, if ever needed later, belongs under `sources/private/` and
must remain outside Git history.

The current Session 01 content does not depend on private source files.

## Deliberately deferred material

The following remain outside Session 01:

- final Python interfaces;
- concrete domain implementation;
- Tre Sigilli rules and implementation;
- random strategy implementation;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- performance benchmarking;
- UI or graphical representation;
- advanced game-specific mechanics.

No prepared unit or assessment requires these topics.

## Canonical navigation

The intended navigation path is:

Root README
→ sessions index
→ Session 01 README
→ source coverage map
→ study map
→ learning units
→ quizzes
→ answer keys after an actual attempt
→ coverage review

No competing active Session 01 learning path has been identified.

`docs/roadmap.md` remains the software roadmap.

`docs/progress.md` remains the learner and teaching-material progress tracker.

## Preparation versus study

Repository preparation and learner activity remain separate.

Current learner state:

- units studied: 0 of 6;
- quizzes attempted: 0 of 2;
- answer reviews completed: 0 of 2.

Creating or publishing teaching material must not change those values.

The Session 01 study checkbox therefore remains incomplete.

## Interview material decision

A dedicated interview bank is not required for this foundational session.

Session 01 is focused on establishing BoardLab's domain-model mental model and
responsibility boundaries.

Interview-oriented material may be introduced later when the learning scope
includes architecture review, algorithm trade-offs, or system-design-style
discussion.

Its absence is deliberate rather than an incomplete preparation artifact.

## Content preparation result

The planned source map, study map, six learning units, quiz plan, two quizzes,
two separate answer keys, and this coverage review are present.

Objectives, misconception coverage, assessment coverage, publication boundary,
deferred material, and canonical navigation are accounted for.

**Content preparation complete.**

This statement does not mark the session **Prepared**.

The Prepared status still requires final repository-wide readiness validation,
inventory checks, navigation checks, quiz-leakage enforcement, publication
checks, whitespace validation, Git-state inspection, and successful revalidation
after the status change.
