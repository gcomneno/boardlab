# Tre Sigilli — SHIP-02 Gaming UX & Art Direction

Status: Free visual baseline approved; implementation and QA pending

## Objective

Define an original, recognizable, accessible visual identity
consistent with the Free/Premium strategy.

## Candidate directions

### A — Imperial Seals
Midnight navy, satin gold, engraved geometric shapes.
Elegant collectible tabletop-game atmosphere.

### B — Essential Geometry
Ivory, graphite, terracotta.
Maximum legibility, simple forms, clean interactions.

### C — Nexus
Graphite, teal, violet.
Technological atmosphere and strategic laboratory.

## Gaming UX principles

- Seals distinguishable without color.
- Clear selection and legal-move feedback.
- Legible controlled, contested, and neutral territory.
- Explorable per-cell influence counts.
- Tutorial available before and during play.
- Visual explanation of the final result.
- Contrast and touch targets to be verified on Android.
- Subtle animations that do not slow down play.

## Free/Premium

Free: complete visual identity, original seals, base theme.
Premium: alternative themes and collections without game advantage.

## Skin strategy

- Essential Geometry: default Free skin.
- Imperial Seals: candidate Premium skin.
- Nexus: candidate Premium skin.
- Skins change presentation only.
- Rules, scoring, and AI remain unchanged.
- Every skin must meet accessibility and readability requirements.
- Premium unlocking remains subject to commercial validation.


## Free UX baseline — proposal pending validation

### Visual identity

- Default theme: Essential Geometry.
- Candidate palette: ivory #F5F0E6, graphite #33434B,
  terracotta #A65E49, sand #D7C7AD.
- North seal: four-point geometric star.
- South seal: hexagon with circular center.
- Symbols distinguishable independently of color.
- Contrast must be verified before implementation.

### Interaction

- Play mode: selection and legal moves.
- Explore territory mode: no moves executed;
  selected-cell North/South influence counts.
- Endgame: visible controlled territories and result explanation.
- Tutorial accessible without leaving the current match.
- Controls usable and legible on small screens.

### Technical boundaries

- Canonical game engine and rules unchanged.
- Presentation independent of the selected skin.
- No duplicated scoring logic in the interface.
- Exploration mode separate from game actions.
- State persistence and restoration require verification.

Visual choices remain candidates until mockup approval
and readability validation.

## Approved visual baseline — SHIP-02

### Identity

- Essential Geometry is the default Free skin.
- Palette: ivory #F5F0E6, graphite #33434B,
  terracotta #A65E49, sand #D7C7AD.
- North: graphite circular seal with a four-point star.
- South: terracotta circular seal with a hexagon and center dot.
- Symbols remain distinguishable without color.
- Imperial Seals and Nexus remain separate Premium candidates.

### Game layout

- Tre Sigilli title and BoardLab/GiadaWare identity.
- Territorial scores for both players.
- Turn, phase, and contextual instructions.
- Classic 4×4 board with adequate touch targets.
- Distinct Play and Show territories modes.
- Selected-cell detail panel.
- North/South influence counts and point explanation.

### Interaction contract

- Play mode taps retain canonical game semantics.
- Explore mode taps never modify game state.
- Territory data comes from the domain, with no duplicated
  influence rules in the user interface.
- Exploration does not change turns or trigger AI moves.
- Final results remain explainable.
- Match and interaction-mode restoration require dedicated tests.

### Implementation acceptance criteria

- No changes to Tre Sigilli v0.2 rules.
- Python/Kotlin parity preserved.
- No regressions in existing game modes.
- Accessible and distinguishable symbols, colors, and states.
- Legibility on small Android screens.
- Selection, exploration, and restoration tests.
- No Premium dependency for essential UX.
- No payment integration in SHIP-02.

## Next gate

Validate Essential Geometry as the default Free theme.
Only then design final assets and Compose changes.

No assets, pricing, or release are approved here.
