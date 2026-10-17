# Learning & Product Map

[English](learning-product-map.md) | [Italiano](learning-product-map.it.md)

## Mission

BoardLab is an open-source laboratory for learning to design,
implement, test and evaluate original board games and game AI.

Promising games may become mobile products. Commercial publication
is optional and does not determine learning completion.

## LEARN — Understand

Acquire transferable knowledge of game states, legal moves,
transitions, termination, architecture, strategies, Minimax,
Alpha-Beta pruning, Monte Carlo Tree Search and experimentation.

Evidence includes completed exercises, reviewed assessments,
explanations and application to unfamiliar problems.

Prepared does not mean Studied.

## BUILD — Implement

Build a generic engine, original games, interchangeable strategies,
search algorithms and reproducible benchmarks.

Evidence includes contracts, tests, architectural conformance,
measured behavior and reproducible results.

Passing software checks does not imply learning completion.

## SHIP — Validate and potentially publish

Evaluate whether original games are understandable, enjoyable,
replayable and suitable for mobile distribution.

Evidence includes external playtesting, usability findings,
gameplay metrics, technical feasibility and economic assessment.

Publication and revenue are opportunities, not obligations.

## Traceability

| Phase | LEARN | BUILD | SHIP |
| --- | --- | --- | --- |
| 0. Foundations | States, actions and contracts | Generic engine | Playability criteria |
| 1. Three Seals | Game modeling and design | Game, random strategy and match | Playable prototype |
| 2. Minimax | Adversarial search | Deterministic strategy | Computer opponent |
| 3. Alpha-Beta | Complexity and pruning | Optimized search and benchmarks | Responsiveness |
| 4. MCTS | Statistical tree search | Alternative strategy | Gameplay relevance |
| 5. Scalability | Experimental methodology | Additional game and comparisons | External playtesting |
| 6. Distribution (proposed) | UX, packaging and metrics | Mobile integration if justified | Optional Android publication |

Phase 6 is a proposed extension, not a change to the approved
software roadmap.

## Independent completion gates

- LEARN_GATE: actual study, exercises and reviewed assessments.
- BUILD_GATE: correct, tested and reproducible implementation.
- SHIP_GATE: original rules, replayability, external feedback,
  usability and a plausible economic case.

No gate substitutes for another.

## Architectural boundaries

- Python and ADR 0001 remain the initial baseline.
- The CLI precedes graphical interfaces.
- The engine remains independent of games and user interfaces.
- Mobile technology is not selected prematurely.
- Code reuse and behavioral reuse are evaluated separately.
- Product assets, presentation and monetization remain separate.
- Public content follows repository hygiene and licensing rules.

## Tracking ownership

- Learning progress belongs to the existing progress tracker.
- Software milestones belong to the existing roadmap.
- This map connects learning, implementation and product opportunities.
- Product validation never changes learning status automatically.

## First candidate

Three Seals is the initial playable prototype candidate,
not an automatically approved commercial product.

Its rules, strategic depth and replayability require validation.
