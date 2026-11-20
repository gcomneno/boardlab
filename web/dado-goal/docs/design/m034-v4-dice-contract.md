# M034-V4 — Dice semantics and balance contract

Status: APPROVED CANDIDATE BASELINE — NOT IMPLEMENTED
Scope: Dado Goal single-player, multiplayer, Manager and 3D lab.

## Invariants

- Five physical dice: yellow, blue, pink, red, green.
- Six equally probable physical faces per die.
- Yellow and blue have equivalent rules for opposing teams.
- The game engine remains authoritative for outcomes.
- Animation and artwork never determine gameplay.
- No change to the canonical engine until implementation approval.

## Candidate matrix

| Face | Yellow / Blue | Pink | Red | Green |
|---|---|---|---|---|
| 1 | Affondo offensivo | Tiro potente | GOAL! | GOAL! |
| 2 | Tiro dalla distanza | Mezza rovesciata | Parato! | GOAL! |
| 3 | Passaggio intercettato | Rovesciata | FUORI! | GOAL! |
| 4 | Palla persa | Colpo di testa | PALO/TRAVERSA | GOAL! |
| 5 | Cross | Tiro murato | Respinto! | GOAL! |
| 6 | Passaggio riuscito | Palla sul fondo | Deviazione fuori | SBAGLIATO! |

The matrix contains 30 physical faces (yellow and blue separately).

## Transition contract

Yellow/Blue:
- Faces 1, 2, 5 advance to pink.
- Faces 3, 4 end the attacking hand without a goal.
- Face 6 repeats the team-die phase without ending the hand.

Pink:
- Faces 1–4 advance to final resolution.
- Faces 5–6 end the attacking hand without a goal.

Red:
- Face 1 ends the hand with a goal.
- Faces 2–6 end the hand without a goal.
- No red face awards a penalty.

Green:
- Faces 1–5 end the hand with a goal.
- Face 6 ends the hand without a goal.
- Face 6 is "SBAGLIATO!": an unsuccessful penalty (save, post, crossbar or wide).

## Penalty award model

- A penalty may be awarded only after reaching final resolution.
- Award probability given final resolution: 2.5%.
- The award check occurs before the ordinary red outcome.
- If awarded: record the event, skip the red roll, enter green phase.
- Otherwise: proceed with the red roll.
- No additional hidden decision is made after a physical die result.
- The random source must be injectable for deterministic testing.
- Award and resolution must be recorded for replay and statistics.
- A penalty award is separate from disciplinary events.

## Analytical balance

Under equally probable independent dice and the stated award rule:

- Reach pink: 60%.
- Reach final resolution: 40%.
- Penalty awards per hand: 40% × 2.5% = 1%.
- Penalty conversions: 5/6 = 83.333...%.
- Goals per hand:
  0.40 × (0.975 × 1/6 + 0.025 × 5/6)
  = 0.073333... (7.333...%).
- Expected goals per 20-hand match: 1.466666...

Previous canonical baseline:
- Penalties per hand: 5%.
- Penalty conversion: 50%.
- Goals per hand: 7.5%.
- Expected goals per 20-hand match: 1.5.

## Implementation boundaries

- A single canonical source must define face IDs, labels and transitions.
- Sync game state, roll event schema, statistics and commentary.
- Respect single-player and multiplayer differences.
- Preserve stored-match compatibility or provide explicit migration.
- Use localized display strings without changing stable event IDs.
- Keep artwork and 3D presentation separate from rule authority.
- The GOAL-only stadium celebration uses the resolved match event,
  not a visual guess based on the artwork.
- Road tests and deterministic regressions are mandatory before release.

## Open questions

1. Is the proposed face wording understandable and entertaining?
2. Should a failed penalty distinguish save, post and wide via narration,
   or should the physical green die itself represent those outcomes?
3. How should penalty awards appear in event logs and commentary?
4. Which actions should use realistic cutout illustrations?
5. How should historical saves and statistics identify rule versions?

No implementation or deployment is authorized by this document alone.


## M034-V15 — Versioned ruleset architecture

Status: DESIGN CANDIDATE — NOT IMPLEMENTED.

### Version boundaries

- `schemaVersion` describes serialized state shape.
- `rulesVersion` identifies the immutable gameplay contract.
- Historical matches lacking `rulesVersion` use V1.
- New M034 matches will use V2 only after an explicit rollout.
- Server authority assigns the rules version at match creation.
- Durable match identity retains that version throughout the match.
- Snapshots and roll receipts must remain version-consistent.
- V1 outcomes, statistics and replay semantics must be preserved.

### Authoritative execution

- `authority.mjs`: assign and expose rules version.
- `lifecycle-d1.mjs`: persist the assigned rules version.
- `roll-d1.mjs`: dispatch version-specific rolls and events.
- `game.mjs`: validate and execute version-specific transitions.
- V2 explicitly supports the penalty phase.
- Penalty award occurs before the ordinary red result roll.
- Award and penalty resolution are distinguishable events.
- Retry idempotency and optimistic concurrency remain mandatory.
- Migration and rollback behavior require dedicated tests.

### Future Editor Edition

- Extend immutable rule identity to `rulesetId` and `rulesetRevision`.
- Allow saved presets with variable dice count and face count.
- Define per-face outcomes, probabilities and state transitions.
- Repeated equal outcomes determine weighted probabilities.
- Validate complete rulesets before accepting new matches.
- Freeze the selected ruleset for each started match.
- Keep user-created rulesets separate from canonical defaults.

This section is architectural design only.
No database migration, game implementation or deployment is authorized.
