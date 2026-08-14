# Session 01 — Source coverage map

[English](source-coverage-map.md) | [Italiano](source-coverage-map.it.md)

## Purpose

This map defines the source boundary for Session 01 before learning units,
examples, quizzes, or answer keys are written.

It identifies what the session may derive from existing BoardLab material,
what material provides supporting context, what is deliberately deferred, and
where original teaching interpretation will be required.

This document guides preparation. It does not replace any source document.

## Session scope

Session 01 prepares the computational model needed to reason about a board game
before implementing a concrete game or search algorithm.

The planned conceptual boundary is:

- game state;
- legal moves;
- state transitions;
- terminal states;
- players;
- strategies;
- match coordination;
- separation between game rules and decision algorithms.

## Primary repository sources

### `docs/architecture/overview.md`

Role: primary architectural source.

Material relevant to this session:

- responsibility of `Game`;
- responsibility of `GameState`;
- responsibility of `Move`;
- responsibility of `Player`;
- responsibility of `Strategy`;
- responsibility of `Match`;
- separation between game-specific rules and generic engine behavior;
- requirement that search must not clandestinely mutate state;
- conceptual minimum contract for legal moves, move application, terminal-state
  detection, evaluation, and independent search state.

Coverage boundary:

The document defines architectural responsibilities and constraints but does not
yet define final Python interfaces or a complete formal model.

### `README.md`

Role: primary project-level source.

Material relevant to this session:

- BoardLab is a laboratory for game engines and algorithms rather than a
  collection of games;
- the engine must not know specific games;
- algorithms depend on abstract contracts;
- engine, game, and strategy remain separated;
- readability and testability take precedence over premature optimization;
- Tre Sigilli is planned as the first original concrete game after the generic
  engine foundation.

Coverage boundary:

The README provides project principles and direction, not detailed teaching
content.

## Supporting repository sources

### `docs/roadmap.md`

Role: sequencing and project context.

Relevant contribution:

- establishes the generic engine before Tre Sigilli;
- places Minimax, Alpha-Beta Pruning, and Monte Carlo Tree Search after the
  initial domain foundation.

The roadmap supports the teaching sequence but is not a source for the internal
mechanics of those algorithms in this session.

### `docs/architecture/adr/0001-python-toolchain.md`

Role: implementation-context support.

Relevant contribution:

- explains why Python was selected for readable algorithms, explicit contracts,
  testing, and reproducibility;
- establishes type checking and testing constraints that future implementation
  work must respect.

This ADR does not define board-game domain semantics and is not used as a source
for game-model concepts.

## Material deliberately deferred

Session 01 does not attempt to teach or implement:

- Tre Sigilli rules or gameplay;
- concrete Python interfaces for the domain contracts;
- random strategy implementation;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- performance benchmarking;
- advanced game-specific mechanics;
- UI or graphical representation.

These topics require later sessions or implementation phases.

## External sources

No external textbook, commercial rulebook, protected manual, or commercial game
material has been selected as a source for Session 01 at this preparation stage.

Future external sources may be added only deliberately, with bibliographic
references and a clear publication boundary.

Their absence must not be silently compensated for by copied or reconstructed
protected material.

## Original BoardLab interpretation

The teaching material for this session will necessarily add original
explanations around the existing architectural statements.

Original material may include:

- mental models for representing gameplay as state transitions;
- invented minimal game-state examples;
- original diagrams;
- failure and modeling scenarios;
- distinctions between rules, state, strategy, and match coordination;
- original exercises and quiz questions.

These artifacts must remain identifiable as BoardLab teaching material rather
than reproductions of an external source.

## Copyright and publication boundary

The session may reference concepts or specific sections from future external
sources, but the public repository must not contain source substitutes such as:

- commercial rulebook PDFs;
- scans;
- protected artwork;
- substantial collections of card text;
- copied rule sets;
- substantial translations;
- source transcripts.

Private material, if ever required for study, belongs under
`sources/private/` and must remain outside Git history.

## Preparation consequence

The source coverage currently supports preparation of a foundational session
about BoardLab's abstract game model.

It does not yet support claims about specific commercial games or detailed
search algorithms.

Any later expansion beyond this boundary requires an explicit source update
before the corresponding teaching material is added.
