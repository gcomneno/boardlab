# BoardLab — Browser Internationalization Contract (M0)

[English](boardlab-i18n-contract.md) | [Italiano](boardlab-i18n-contract.it.md)

Status: PROPOSED — BoardLab #33

## 1. Purpose

Provide consistent English and Italian player-facing interfaces for
BoardLab browser games without changing gameplay semantics.

Initial targets:

- Tre Sigilli.
- Dado Goal.
- Dado Goal multiplayer laboratory interface.

## 2. Locales

Supported locale identifiers:

- `en`: canonical translation-key reference.
- `it`: Italian translation.

Translation keys use stable semantic names, independent of either
language's displayed wording.

Existing production interfaces currently use Italian. Migration must
preserve Italian as their initial presentation unless the user explicitly
selects another supported locale.

Unknown or unsupported locales resolve deterministically to `en`.

## 3. Selection and persistence

The interface exposes an accessible EN/IT language selector.

Changing language must:

- Update visible labels, instructions, status and accessibility text.
- Preserve the current game state and player choices.
- Avoid restarting engines, AI workers or multiplayer sessions.
- Work without network access.

Explicit selection takes precedence over the initial default.

Persistence, if implemented, must be optional, local, documented and
safe when storage access is unavailable.

## 4. Runtime localization

Use a lightweight, dependency-free localization module.

Requirements:

- Explicit dictionaries for `en` and `it`.
- Identical required translation-key sets.
- Parameterized messages for dynamic values.
- Escaped text rendering by default.
- No HTML injection through translated messages.
- Explicit missing-key detection during tests.
- Predictable English fallback at runtime.

Translation modules may be shared across browser games, but gameplay
engines must not depend on them.

## 5. Boundaries

Internationalization must not modify:

- Game rules, scoring or deterministic state transitions.
- AI algorithms, worker message protocols or search parameters.
- Network authorization and multiplayer session identity.
- Existing persistent game-state formats.

Internal protocol identifiers remain language-neutral.

## 6. Documentation

Existing Markdown conventions remain authoritative:

- Canonical English: `document.md`.
- Italian translation: `document.it.md`.
- Existing bilingual manifest and validator remain in use.

Runtime UI localization is a distinct contract and must not be
implemented by modifying the Markdown validator.

## 7. Verification

Required automated checks:

- Translation-key parity between `en` and `it`.
- Resolution of all referenced runtime translation keys.
- Correct parameter interpolation and missing-key handling.
- Locale switching without changes to game state.
- Equivalent behavior for Italian and English.
- Offline loading without additional dependencies.
- Accessibility labels and document language match selection.
- Existing game, AI and multiplayer regression suites pass.

## 8. Implementation sequence

M0: Inventory, contract and contract tests.

M1: Tre Sigilli localization.

M2: Dado Goal and multiplayer laboratory localization.

M3: Repository documentation and bilingual navigation.

M4: Public release metadata and itch.io bilingual publication.

No production language switch or release is authorized by M0 alone.
