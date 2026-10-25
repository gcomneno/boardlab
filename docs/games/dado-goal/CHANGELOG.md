# Dado Goal — Changelog

Original football-themed dice game invented by
Giancarlo Cicellyn Comneno during childhood.

Format: https://keepachangelog.com/en/1.1.0/

## [Unreleased]

### Added — Handmade dice visual identity (#28)

- Integrated a generated illustration inspired by the four
  original handmade Dado Goal dice.
- Added a responsive visual theme to the multiplayer page.
- Preserved the original reference photograph separately.
- Kept game rules, mathematical engine and multiplayer
  JavaScript unchanged.
- Qualified image delivery and the same-origin CSP policy.
- Completed Chromium visual acceptance on 2026-10-25.
- Further framing and contrast refinements remain optional.


### Added — Online multiplayer laboratory (#27)

- Python authoritative match manager with distinct player seats,
  private invitations, validated turns and secure server-side dice.
- Versioned JSON wire schema with strict state validation.
- HTTP API for match creation, joining, snapshots and rolls.
- Revision-based synchronization and idempotent roll requests.
- HTTP framing and input-validation regression tests.
- Same-origin multiplayer browser interface using HTTP polling.
- Successful manual acceptance: two Chromium windows completed
  a match with synchronized authoritative state.
- Python/JavaScript parity verified across 800 transitions.

### Multiplayer limitations

- Currently verified only on localhost.
- Matches are stored in process memory and do not survive restart.
- Browser refresh loses the current seat credential.
- Session expiry, rate limiting and durable recovery are pending.
- Multiplayer is not publicly deployed.
- GitHub issue #27 remains open until remote-player acceptance
  and operational requirements are satisfied.


### Added — Historical reconstruction

- Document the original game and its four handmade dice.
- Distinguish original, digital and evolution variants.
- Reconstruct the deterministic state and action model.
- Implement exact probability calculations.
- Introduce model and probability tests.
- Add English and Italian game documentation.

Reference: `f96fbe2` (2026-10-24).

### Browser release

Dado Goal 1.0 — Playable MVP was published on GitHub Pages
on 2026-10-24 and subsequently verified publicly.

- Two local players on the same device.
- Original confirmed dice rules and match scoring.
- Animated dice and configurable match length.
- Free browser access without registration.
- Public URL: https://gcomneno.github.io/boardlab/dado-goal/
- Deployment commit: `44854a7` on `gh-pages`.
- Deployment workflow completed successfully.
- HTTP 200 verified for the published game.
- Browser source integrated locally into `main`
  at commit `9358399` (not pushed as part of M0).

This release does not include online multiplayer.

## Historical evidence

### 2026-10-24

- `f96fbe2`: original rules and mathematical model.
- `44854a7`: playable browser MVP published to GitHub Pages;
  deployment and public HTTP availability verified.
- `9358399`: published browser source integrated into local `main`.

## Maintenance policy

Game-specific rules, mathematical models, probability
analysis, Web interfaces, tournaments and versions
belong here.

Shared BoardLab infrastructure belongs in the root
`CHANGELOG.md`.
