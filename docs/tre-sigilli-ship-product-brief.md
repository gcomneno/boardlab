# Tre Sigilli — SHIP-01 Product Brief

Version: 0.1 — product direction approved; hypotheses pending validation
Scope: product discovery and pre-publication validation

## Mission

Evaluate Tre Sigilli as an original, understandable, enjoyable,
and replayable mobile strategy game.

Commercial publication is not automatic.

## Existing evidence

- Original rules v0.2: 4×4 board, three seals per player.
- Placement, maneuver, and territorial control.
- Python engine and verified Kotlin domain parity.
- Playable Android Compose prototype.
- Android Alpha-Beta opponent at depth 2.
- Initial internal games played.

Positive early impressions do not demonstrate market demand,
balance, or replayability among external players.

## Audience — hypothesis to validate

Mobile players interested in abstract strategy,
short games, progressive learning, and offline play.

Evidence about the actual audience is not yet available.

## Candidate product promise

"Few moves. Many strategies."

Validate this promise through external player testing.

## Game design

- Preserve the classic 4×4 mode.
- Preserve canonical rules until changes are explicitly approved.
- Evaluate AI difficulty levels using implemented,
  measured strategies.
- Study 5×5 and 6×6 as separate variants: seal counts, duration,
  territory, balance, and AI time require validation.

## Gaming UX

- Interactive rules guide.
- Visual influence on a selected square.
- Explainable scoring and final result.
- Clear selection, valid-move, and invalid-move feedback.
- Accessibility: contrast, sizing, touch targets, and
  information that does not rely on color alone.

## Visual identity

- Replace N/S as the final seal representation.
- Design original symbols distinguishable without color.
- Define palette, typography, hierarchy, and microanimations.
- Verify readability on small Android screens.
- Verify third-party asset rights before adoption.

Art direction remains subject to approval.

## Product marketing

- Qualify target audience and market alternatives.
- Validate name, product promise, and presentation.
- Run external testing before planning user acquisition.
- Do not forecast downloads or revenue without evidence.
- Monetization and publication require separate decisions.

## Free/Premium model — proposal

### Free: complete experience

- Classic 4×4 mode with complete rules.
- Local two-player games.
- Accessible Android opponent and base difficulty.
- Tutorial, influence visualization, and result explanation.
- Original standard seals and visual theme.
- Offline play without intrusive advertising.

### Premium: customization and expansion

- Alternative seal collections and visual themes.
- Official 5×5 and 6×6 variants, only after validation.
- Laboratory mode with configurable board size, subject
  to technical feasibility and game balance.
- Advanced AI levels, calibrated with reproducible benchmarks.
- AI Laboratory mode with MCTS, subject to Android implementation,
  performance qualification, and gameplay validation.

### Candidate AI levels

| Level | Candidate strategy | Access |
| --- | --- | --- |
| Beginner | Random | Free |
| Normal | Calibrated Alpha-Beta | Free |
| Expert | Deeper-search Alpha-Beta | Premium |
| AI Laboratory | Configured and verified MCTS | Premium |

MCTS is not automatically harder or stronger than Alpha-Beta.
Levels require measurement before approval.
Minimax remains a potential experimental alternative.

Cosmetics do not change rules or grant competitive advantages.

### Assets and licenses

- Prefer original GiadaWare collections.
- External sticker packs require verified rights for
  modification, integration, and commercial distribution.
- Standard seals must not depend on external services.

### Monetization

- Hypothesis: one free app with permanent Premium unlock.
- Indicative price for testing: EUR 2.99–4.99.
- No subscription in the initial proposal.
- Payments, store, taxation, and purchase management
  require separate qualification and approval.
- No purchases, commercial accounts, or publication authorized.

The first release does not necessarily need Premium.
Undeveloped features must not be presented as available.

## Playtesting

Proposed first cycle: 5–10 external players.

Observe:
- Understanding of rules without assistance.
- Ability to explain final scoring.
- First-game completion.
- Spontaneous desire to play again.
- Perceived AI difficulty.
- Interaction and accessibility problems.

Record findings without inventing results.

## SHIP gate

SHIP-01 — Product Direction Gate: explicit approval of the hypothesized audience, promise, UX priorities, game design, and validation plan.

SHIP Release Gate — publication decision only with:
- An understandable and playable prototype.
- Identified and verified priority UX issues.
- Reproducible evidence for AI difficulty.
- Documented external feedback.
- A technical plan for release, signing, testing, distribution.
- Asset rights and cost review.

Store publication requires explicit approval.

## Out of scope for SHIP-01

- Changes to game engine or rules.
- Implementation of board variants.
- Final restyling and asset production.
- Monetization, store listing, and publication.
- Automatic commits or pushes.
