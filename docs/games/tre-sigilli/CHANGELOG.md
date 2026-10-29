# Tre Sigilli — Changelog

Original game instance developed within BoardLab.

Format: https://keepachangelog.com/en/1.1.0/

## [Unreleased]

### Added — AI vs AI (#35)

- Autonomous computer-versus-computer matches.
- Independent strategy selection for Ivory and Terracotta.
- Automatic alternating turns using the existing Web Worker.
- Safe interruption through a new game or mode change.
- Italian and English interface support.
- Browser acceptance and CI qualification completed.

Reference: PR #49, merge `51a949c` (2026-10-29).

### Added — Guided AI move explanations (#36)

- Explanations of the computer's actual moves in guided play.
- Territorial score changes calculated from the pre-move state.
- Localized explanations in Italian and English.
- Previous move analysis cleared when appropriate.
- Browser acceptance and CI qualification completed.

Reference: PR #47, merge `aff0561c` (2026-10-29).

### Added — Strategy Lab M4

- Configurable Random, Minimax, Alpha-Beta and MCTS
  opponents in the standalone browser application.
- Algorithm-specific parameters: seed, search depth,
  simulation budget and UCT exploration.
- Isolated asynchronous Web Worker for AI computation.
- Cross-runtime validation of deterministic strategies.
- Browser acceptance and regression qualification.

Reference: `fc23d56` (2026-10-24).

### Added — Guided Play M5

- Optional interactive mode: "Prova il gioco".
- Alpha-Beta depth-2 suggested moves.
- Explanations based on actual territorial changes.
- Optional execution of recommended moves.
- Suggestion highlighting and independent player choice.
- Protection against obsolete asynchronous responses.
- Automated testing and browser acceptance.

Reference: `ba99b25` (2026-10-25).

### Release status

M4 and M5 were committed locally. Their public deployment
is not established by these development commits.

## Historical public release

### Tre Sigilli Free v0.1.0

- Verified GitHub release tag:
  `tre-sigilli-free-v0.1.0`.
- Public Android edition.
- Standalone browser edition released separately.
- Original browser opponent: Alpha-Beta, depth 2.

The exact release publication date is not asserted here.

## Development milestones

### 2026-10-24 — Standalone browser game

- `a8b17c3`: offline browser game and parity tests.
- `5748ca3`: explain territorial scoring and pace AI turns.
- `5c03d9c`: free skin and territory explorer.

### 2026-10-20 — Android identity and readiness

- Android identity, launcher icon and state restoration.
- Emulator screenshots and visual baseline qualification.

### 2026-10-18 to 2026-10-19 — Android foundations

- Kotlin domain parity and independent game controller.
- Jetpack Compose interface and Android CI.
- Contextual move highlights and territorial scores.
- Deterministic offline single-player opponent.
- Emulator tests and latency measurements.

### 2026-10-17 — Game foundation

- `574962b`: Tre Sigilli game model on the generic engine.
- Territorial score validation and strategy testing.

## Maintenance policy

Game rules, UX, Android and Web builds, tutorials,
game-specific benchmarks and releases belong here.

Shared BoardLab infrastructure belongs in the root
`CHANGELOG.md`.
