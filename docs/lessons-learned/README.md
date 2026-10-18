# BoardLab — Architectural Lessons Learned

[English](README.md) | [Italiano](README.it.md)

## Purpose

This collection captures reusable architecture and software-design decisions
demonstrated by BoardLab. It is a source for future, individually qualified
LeLe Manager entries, not a knowledge vault or a record of completed study.

## Architectural lessons

### 1. Behavioral contracts can bridge independent implementations

- **Problem:** A Python engine and a native Kotlin application must implement
  the same game semantics without sharing runtime code.
- **Trade-off:** Independent implementations duplicate domain logic, but avoid
  embedding Python in Android or coupling the native application to the CLI.
- **Decision:** Keep Python canonical and specify versioned, executable parity
  fixtures covering states, legal actions, transitions and outcomes.
- **Evidence:** [Android parity ADR](../architecture/adr/0002-android-parity.md);
  Kotlin domain parity and its CI checks.
- **Reusable when:** Migrating a domain across languages or platforms while
  preserving observable behavior.

### 2. Keep the functional domain independent of delivery mechanisms

- **Problem:** Game rules must work in the CLI, strategy evaluation and Android
  without depending on interface state or platform lifecycle.
- **Trade-off:** Explicit boundaries add coordination code but prevent UI
  concerns from contaminating domain invariants.
- **Decision:** Separate immutable game state and deterministic transitions
  from match coordination, the Kotlin controller and Jetpack Compose.
- **Evidence:** [Tre Sigilli contract](../games/tre-sigilli.md);
  platform-independent Kotlin GameController and Android application.
- **Reusable when:** The same domain needs multiple frontends, adapters or
  execution environments.

### 3. Stable abstractions let new games reuse existing algorithms

- **Problem:** Adding a second game could require rewriting match coordination
  and every AI strategy.
- **Trade-off:** A generic game contract constrains implementations but makes
  strategy and orchestration reuse testable.
- **Decision:** Implement Tre Sigilli and Reversi behind the same game contract;
  retain interchangeable Random, Minimax, Alpha-Beta and MCTS strategies.
- **Evidence:** [Reversi contract](../games/reversi.md);
  generic strategy compatibility and integration tests.
- **Reusable when:** Building extensible engines, rule systems or pluggable
  decision algorithms.

### 4. Determinism is an architectural capability, not only a test trick

- **Problem:** Search strategies and cross-language behavior are difficult to
  compare when outcomes depend on uncontrolled randomness or ordering.
- **Trade-off:** Explicit seeds, budgets and tie-breaking constrain execution
  but make results explainable and reproducible.
- **Decision:** Define deterministic legal-action ordering and transition
  contracts; use seeded tests and reproducible strategy benchmarks.
- **Evidence:** [Tre Sigilli rules](../games/tre-sigilli.md);
  [Reversi rules](../games/reversi.md) and benchmark documentation.
- **Reusable when:** Debugging algorithms, comparing implementations or
  validating regressions across environments.

### 5. Architecture boundaries should have executable evidence

- **Problem:** Diagrams and architectural intentions can drift from the
  behavior actually shipped.
- **Trade-off:** Contract fixtures and separate CI gates require maintenance,
  but reveal violations at the boundary where they matter.
- **Decision:** Validate Python semantics, Kotlin parity, Android debug builds
  and unsigned release builds independently; inspect release artifacts rather
  than treating CI success as proof of production readiness.
- **Evidence:** [Android parity ADR](../architecture/adr/0002-android-parity.md);
  SHIP-M2 through SHIP-M7 CI milestones and real-device validation.
- **Reusable when:** An architecture spans runtimes, platforms or release
  pipelines with distinct failure modes.

## Verified milestones

- Python game engine and reusable strategy implementations.
- Kotlin domain parity and platform-independent game controller.
- Android Compose application tested in a complete real-device match.
- CI-produced debug and unsigned release APKs; release Lint executed.

## Remaining production gates

Automated Android tests, release signing and key management, lifecycle and
accessibility validation, distribution policy and publication readiness
remain future work. These lessons do not establish production readiness.
