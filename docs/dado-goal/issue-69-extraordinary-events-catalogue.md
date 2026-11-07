# Dado Goal! #69 — Extraordinary events catalogue

Status: design candidate — single-player first

## Invariants

- Extraordinary events are evaluated only after a completed hand.
- At most one auxiliary event may be selected for a completed hand.
- Extraordinary events do not alter dice outcomes or scoring unless an
  explicitly approved mechanical contract says otherwise.
- Narrative events never mutate match state.
- Mechanical events must have deterministic tests for prerequisites,
  state transition, limits and failure cases.
- Existing yellow/red/substitution behaviour remains authoritative until
  explicitly superseded.
- Existing five-substitution and active-player invariants remain authoritative.
- Every implemented event requires equivalent IT/EN presentation.
- Multiplayer parity is deferred until the single-player catalogue and
  mechanics are qualified.

## Classification

`NARRATIVE`
: Chronological match event and commentary only. No roster, score, dice,
  probability or match-flow mutation.

`MECHANICAL`
: May mutate explicitly listed match state. It must never implicitly alter
  scoring or dice resolution.

## Candidate matrix

| Event | Class | Prerequisites | Candidate probability | Effect | Required tests |
|---|---|---|---:|---|---|
| Light knock | NARRATIVE | active player | 0.6% | commentary only; player continues | deterministic selection; no state mutation |
| Serious injury | MECHANICAL | active player; legal replacement available | existing `injury` probability: 1% | existing forced substitution with `reason: "injury"`; no additional extraordinary-event draw | active player leaves; substitute enters; five-sub limit |
| Injury, no replacement | EXISTING FAILURE CASE | no legal replacement or substitution limit reached | existing `injury` branch | no event emitted and no roster mutation; no fabricated replacement or narrative outcome | roster unchanged; no incomplete event |
| Heavy rain | NARRATIVE | none | 0.25% | atmospheric commentary | no state mutation; per-match cap |
| Strong wind | NARRATIVE | none | 0.15% | atmospheric commentary | no state mutation; per-match cap |
| Snow / poor visibility | NARRATIVE | none | 0.05% | rare atmospheric commentary | no state mutation; rarity boundary |
| Pitch invasion | NARRATIVE | none | 0.03% | temporary narrative interruption | no extra hand; no clock/state mutation |
| Object on pitch | NARRATIVE | none | 0.04% | narrative interruption | no state mutation |
| Animal on pitch | NARRATIVE | none | 0.01% | rare ironic but plausible interruption | rarity; no state mutation |
| Floodlight failure | NARRATIVE | none | 0.02% | narrative suspension/resumption | no state mutation |
| Damaged goal net | NARRATIVE | none | 0.02% | narrative repair delay | no state mutation |
| VAR review | NARRATIVE | suitable preceding football action | 0.10% | commentary review only; original result stands | score/dice unchanged |
| Referee equipment problem | NARRATIVE | none | 0.03% | narrative interruption | no state mutation |

## Probability policy

The percentages above are design candidates, not yet gameplay constants.

They form a separate extraordinary-event channel. They must not be inserted
into the existing yellow/red/injury/substitution cumulative thresholds until
the selection architecture is approved.

The initial implementation should favour narrative events because they add
variety without changing competitive balance.

Serious injury is not a new extraordinary-event probability. Issue #69 adopts
the existing authoritative `injury` branch (`1%`) and its forced-substitution
mechanics. Adding a separate `0.4%` serious-injury draw would duplicate the
same football event and alter established gameplay probabilities.

Mechanical expansion beyond this existing injury mechanic is explicitly
deferred.

## Per-match limits

- Weather family: maximum 1 extraordinary weather event per match.
- External interference family: maximum 1 per match.
- Technical/arbitration family: maximum 1 per match.
- Animal on pitch: maximum 1 per match.
- Serious injury remains the existing `injury` event and is constrained by
  roster eligibility and the existing five-substitution limit.
- If the existing injury branch cannot perform a legal substitution, it emits
  no event and does not fabricate a replacement narrative.
- A failed mechanical prerequisite must not fabricate an alternative state
  transition.

## Minute policy

- Time-wasting remains governed by the existing disciplinary model.
- VAR requires a suitable preceding football action and is not a free random
  interruption.
- Weather may occur throughout the match.
- External interference and technical incidents may occur throughout the
  match but remain individually rare.
- No extraordinary event may advance the match clock independently.

## Single-player implementation order

1. Introduce an extraordinary-event catalogue as data.
2. Introduce deterministic eligibility and selection without UI changes.
3. Implement narrative-only events.
4. Add IT/EN descriptions and chronological rendering.
5. Qualify serious injury as the existing authoritative `injury` branch;
   do not add a second probability or state transition.
6. Run deterministic and statistical tests.
7. Road-test single-player.
8. Only then derive the authoritative multiplayer implementation.

## Explicitly deferred

- Score changes caused by VAR.
- Re-rolling or modifying dice because of weather.
- Abandoning or replaying matches.
- Persistent weather modifiers.
- Temporary numerical disadvantage requiring a new timed-player-state model.
- Any multiplayer persistence or D1 change.
