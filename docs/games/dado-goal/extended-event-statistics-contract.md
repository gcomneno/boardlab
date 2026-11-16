# Dado Goal Extended — canonical event/statistics contract

Status: qualification contract for the current Extended ruleset.

## Principle

If the engine knows a gameplay consequence structurally and that consequence
matters after the match, completed-match history preserves that meaning as
structured data. Statistics must not parse commentary, translations, DOM text,
CSS state, or localized die labels.

Two event streams exist intentionally:

- `rollEvents`: every authoritative dice outcome, with stable semantic `kind`;
- `events`: football/player/narrative consequences such as goals, cards,
  substitutions and extraordinary events.

`face` remains part of a roll event for presentation, replay compatibility and
historical inspection. `kind` is the machine-readable statistical contract.

Legacy runtime/save data created before `kind` is normalized once at the
compatibility boundary from the historical die face. New producers always
write `kind`.

## Canonical roll-event matrix

| Die | Semantic kind | Derived statistic | Attribution |
| --- | --- | --- | --- |
| team | `shot` | shots | rolling team |
| team | `intercepted-pass` | interceptions | rolling team |
| team | `corner` | corners | rolling team |
| team | `pass` | passes | rolling team |
| pink | `ball-out` | ball out | rolling team |
| pink | `free-kick` | free kicks | rolling team |
| pink | `over-bar` | over bar | rolling team |
| pink | `ball-intercepted` | ball intercepted | rolling team |
| pink | `bicycle-kick` | bicycle kicks | rolling team |
| pink | `header` | headers | rolling team |
| result | `blocked` | blocked shots | rolling team |
| result | `goal` | goals | rolling team |
| result | `wide` | wide | rolling team |
| result | `woodwork` | woodwork | rolling team |
| result | `penalty-awarded` | penalties | rolling team |
| result | `save` | saves | defending team |
| penalty | `penalty-goal` | penalty goals + goals | rolling team |
| penalty | `penalty-woodwork` | penalty woodwork | rolling team |
| penalty | `penalty-saved` | penalty saves | defending team |
| penalty | `penalty-wide` | penalty wide | rolling team |

Possession is derived from canonical roll-event counts per team. It is not
inferred from commentary or rendered labels.

## Structured match-event matrix

| Consequence | Canonical representation | Persistent | Player identity |
| --- | --- | --- | --- |
| ordinary goal | `type: "goal"` | yes | scorer when known |
| penalty attempt | `type: "penalty"` + `outcome` | yes | taker when known |
| yellow card | `type: "yellow"` + `reason` | yes | player when known |
| red card | `type: "red"` + `reason` | yes | player when known |
| substitution | `type: "substitution"` + `out`/`in` + `reason` | yes | both identities when known |
| injury substitution | substitution with `reason: "injury"` | yes | both identities when known |
| extraordinary event | stable extraordinary `type`, `classification`, family/context | yes | when event structurally has a player |

Per-player statistics derive exclusively from these structured match events.

## Completed-match persistence

A canonical completed-match record stores:

- immutable team snapshots and score;
- structured `events`;
- structured `rollEvents`;
- a statistics snapshot;
- ratings where supported.

Because `rollEvents` are persisted, team statistics can be recomputed after a
JSON round-trip and checked against the stored snapshot. Tournament and Manager
records reuse the same canonical match record rather than reconstructing
statistics from UI text.

Schema version 1 remains backward-readable: older records without `rollEvents`
load with an empty roll-event list. No historical information is fabricated.

## Explicitly unsupported data

The current Extended engine does not invent or infer:

- long-term injury duration or recovery;
- fatigue;
- morale;
- contracts, wages or transfer economics;
- tactical/positional metrics not structurally produced by the engine;
- expected goals or derived analytics requiring unavailable spatial data.

These remain unsupported rather than approximated from commentary.

## Invariants

1. Current roll producers write a semantic `kind`.
2. Statistics process semantic `kind`, not face text.
3. Commentary and localization never participate in statistics.
4. Player statistics process structured match events only.
5. Supported structured events survive JSON persistence unchanged.
6. Stored team statistics are reproducible whenever persisted `rollEvents`
   are available.
7. Legacy data is migrated only at a defined compatibility boundary.
