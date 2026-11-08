# BoardLab — Changelog

BoardLab is a reusable laboratory and platform for board-game engines,
strategies, research and tooling.

This changelog records shared platform capabilities and repository-level
changes. Each game maintains its own independent changelog.

Format: https://keepachangelog.com/en/1.1.0/

## [Unreleased]

### Architecture

- Maintain a strict distinction between shared platform functionality
  and game-specific implementations.
- Keep game histories and releases in dedicated changelogs.
- Keep generic puzzle/CSP research independent from individual games.

### Research

- Recover the qualified PZ-001F findings on CSP semantic diversity,
  equivalence and operational solver effort.

## Historical milestones

### 2026-11-07 — Repository recovery

- Recovered the released Arca di Darwin source and tests.
- Recovered generic CSP research modules and tests.
- Recovered the standalone topology-puzzle engine and tests.
- Recovered the Reversi game changelog.
- Preserved game-specific histories independently from platform history.

### 2026-10-17 — Shared engine foundation

- `574962b`: introduce the generic game engine,
  initially exercised through Tre Sigilli.
- `510e70a`: introduce Reversi 6x6.
- The platform supports independently modeled games through common
  game and strategy contracts.

### 2026-07-18 — Toolchain foundation

- `8eb49f2`: establish the Python development toolchain.

## Game changelogs

- [Tre Sigilli](docs/games/tre-sigilli/CHANGELOG.md)
- [Dado Goal](docs/games/dado-goal/CHANGELOG.md)
- [Reversi](docs/games/reversi/CHANGELOG.md)

Arca di Darwin and ChronoOops! currently remain documented through
their dedicated source, game documentation and release materials.

## Maintenance policy

- Record only shared engine, strategy infrastructure, platform APIs,
  tooling, CI, architecture and cross-game research here.
- Record gameplay, game-specific UI, ports and releases in the
  corresponding game's changelog.
- Distinguish committed development from public releases.
- Do not infer release dates from commit timestamps alone.
- Preserve qualified historical and research evidence without
  reintroducing obsolete implementation state.
