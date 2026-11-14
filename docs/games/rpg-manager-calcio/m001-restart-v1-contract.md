# M001-B1.27-A — Restart Engine v1 contract

Status: approved normative target for subsequent implementation. This increment
creates this contract only; it does not implement the interface below.
Baseline: repository HEAD `7ab2dca`.

Requirements using **must**, **must not** or **requires** are normative.
Paragraphs marked **Implementation note** describe existing facilities or possible
implementation structure without adding sporting rules.

Sources: [M001 Engineering Contract](m001-engineering-contract.md),
[Shot Engine v1](m001-shot-v1-contract.md), and the existing
`src/rpg_manager_calcio/` definitions in `match_state.h`, `match_event_log.h/.c`,
`match_shot_action.h/.c`, `match_sequence.h`, `match_clock.h`, `match_recovery.h`,
`roster.h` and `world.h`. Timing also uses the existing `match_stoppage.h` policy.

## 1. Scope and exclusions

Restart v1 executes exactly two kinds of restart:

- `RMC_RESTART_KICKOFF` after a completed Shot v1 GOAL chain.
- `RMC_RESTART_GOAL_KICK` after a completed Shot v1 WIDE chain.

Opening-match kickoff, second-half opening kickoff, corners, throw-ins, fouls,
free kicks, penalties and all other restart categories are outside this contract.
`RMC_RESTART_NONE` is not an executable kind. A lifecycle state with mode
`RMC_MATCH_RESTART_PENDING` does not itself authorize either supported restart.
The wider M001 sporting scope remains unchanged.

The five approved conventions are explicit here:

| Convention | Normative decision |
|---|---|
| 1. Explicit taker | KICKOFF uses an awarded-team on-field outfield player; GOAL_KICK uses that team's current POR occupant, who must be a genuine goalkeeper. |
| 2. Initial phase | Both create a fresh `RMC_PHASE_BUILDUP` possession with taker as initiator and controller and both counters zero. |
| 3. Instantaneous execution | No clock advancement, additional stoppage registration, RNG consumption, duration/context parameters or possession action charge. |
| 4. Sporting coordinates | KICKOFF is `(2, RMC_LANE_CENTRE)`; GOAL_KICK is `(0, RMC_LANE_CENTRE)`, already in the awarded team's attacking frame; no inversion. |
| 5. Explicit event kind | One `RMC_EVENT_RESTART`, with outcome GOAL_KICK = 1 or KICKOFF = 2, directly references the source SHOT; no additional POSSESSION_START. |

## 2. Types and command interface

The following declarations are the normative target interface, following the
existing `rmc_match_*_apply` convention. They are documentation, not declarations
added to the current build:

```c
typedef struct {
    RmcRestartKind restart_kind;
    RmcTeamId awarded_team;
    RmcPlayerId taker;
    uint64_t source_shot_event_id;
} RmcRestartAction;

typedef struct {
    uint64_t restart_event_id;
    uint64_t source_shot_event_id;
    uint64_t source_goal_event_id; /* Zero for GOAL_KICK. */
} RmcRestartResult;

bool rmc_match_restart_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcRestartAction *action,
    RmcRestartResult *result
);
```

Reuse the existing `RmcRestartKind` from `match_event_log.h`:
`RMC_RESTART_NONE = 0`, `RMC_RESTART_GOAL_KICK = 1`,
`RMC_RESTART_KICKOFF = 2`. Do not introduce a competing restart enumeration.
The command's kind and awarded team are expectations to validate against the
authoritative SHOT payload, not overrides. The taker is explicit; there is no
automatic selection. There is no RNG, duration or context parameter.

Return `true` only after the complete transaction succeeds. Return `false` on
rejection with the preservation guarantees in section 8. Successful results
identify the appended RESTART, its source SHOT, and, for KICKOFF, the mandatory
adjacent source GOAL. Identifiers use existing `uint64_t` event IDs; zero denotes
absence only for `source_goal_event_id` on GOAL_KICK.

## 3. Eligibility

Require non-null arguments, a valid `RmcWorldCore`, fixture and `RmcMatchState`
under `rmc_match_state_valid`, valid current home and away lineups, and an active
`RMC_MATCH_FIRST_HALF` or `RMC_MATCH_SECOND_HALF`. Mode must be
`RMC_MATCH_RESTART_PENDING`; terminal and management states are ineligible.

Possession must be canonically cleared, with the field values of
`(RmcMatchPossession){0}`: zero `team_id`, `initiator`, `controller`,
`action_count` and `elapsed_seconds`, sector 0, `RMC_LANE_LEFT`,
`RMC_PHASE_BUILDUP`. This is a semantic field check, not a requirement on padding.
General state validity alone does not establish this stronger precondition.

The awarded team must be the fixture opponent of the SHOT's shooting team:
the conceding team for GOAL, the defending team for WIDE. Resolve identities
through authoritative `RmcWorldCore.teams`, `rosters`, `RmcRoster.team_id` and
`RmcPlayer.team_id`; array indices are not entity IDs.

| Kind | Eligible explicit taker |
|---|---|
| KICKOFF | Nonzero `RmcPlayerId` in the awarded team's current `RmcLineup.players`, assigned to a slot other than `RMC_SLOT_POR`, with department other than `RMC_DEPARTMENT_GOALKEEPER`. |
| GOAL_KICK | Nonzero `RmcPlayerId` in the awarded team's current `RmcLineup.players` at the corresponding `slots[i] == RMC_SLOT_POR`, with `department == RMC_DEPARTMENT_GOALKEEPER`. |

A bench player, another team's player, zero/unknown ID or an arbitrary roster
goalkeeper is insufficient. Use current slot assignments, not a presumed roster
position or a cached default goalkeeper. No emergency goalkeeper exception is
introduced. No extra taker attribute threshold or tactical preference is added.

## 4. Causal preconditions

Require a coherent, validated event-log prefix for the current fixture, ending
in exactly one of these completed Shot v1 chains, with no unrelated intervening
event. Let `S` be the command's nonzero source SHOT ID:

```text
WIDE: ACTION/SHOT_CHANCE -> SHOT/WIDE
      ID S-1              ID S (log tail)

GOAL: ACTION/SHOT_CHANCE -> SHOT/GOAL -> GOAL
      ID S-1              ID S         ID S+1 (log tail)
```

The ACTION is `RMC_EVENT_ACTION / RMC_SHOT_CHANCE` (500), SHOT is
`RMC_EVENT_SHOT / RMC_SHOT_WIDE` (505) or `/ RMC_SHOT_GOAL` (501), and GOAL is
`RMC_EVENT_GOAL / RMC_SHOT_GOAL` (501). SHOT's `cause_event_id` must identify
the immediately preceding ACTION. GOAL's `cause_event_id` must identify the
immediately preceding SHOT. A GOAL without the adjacent GOAL record is incomplete
and cannot authorize KICKOFF. ACTION's optional cause retains existing logger
semantics; this contract does not invent a new cause for it.

Validate all of the following before accepting the source:

- The source ID resolves to the actual SHOT at the required tail position;
  all IDs are consecutive and causal references resolve to real earlier records.
- Chain members and the log belong to `state->fixture.id`. Their timestamps
  agree with one another and equal `state->elapsed_seconds`.
- Chain envelopes agree on shooting `team_id`, shooter `primary_actor` and
  goalkeeper `secondary_actor`; secondary actor equals `shot.goalkeeper_id`.
  Shooter and defender are the proper on-field outfield actors and the opposing
  goalkeeper is the genuine POR occupant under existing Shot v1 validation.
- Payload `model_version == RMC_SHOT_MODEL_VERSION` (1). Validate the existing
  Shot v1 domain, source zone, positive shot duration, context, quality, talent
  mask and actor rules. ACTION and SHOT share the chance fields: model, quality,
  source zone, context, duration, defender, goalkeeper and applied talent mask.
  ACTION has neutral trajectory/restart. GOAL copies the complete final SHOT
  payload, including trajectory and restart fields. Compare fields, not padding.
- GOAL has `RMC_SHOT_TRAJECTORY_ON_TARGET`, `RMC_RESTART_KICKOFF`, opposing
  awarded team and zone `(2, RMC_LANE_CENTRE)`. WIDE has
  `RMC_SHOT_TRAJECTORY_WIDE`, `RMC_RESTART_GOAL_KICK`, opposing awarded team and
  zone `(0, RMC_LANE_CENTRE)`. Both require `RMC_RESTART_PENDING` disposition.
- Command `restart_kind` and `awarded_team` exactly match those authoritative
  fields. The source must belong to the current active half, including its shot
  duration fitting between the half's start and completion timestamp.
- The current clock is strictly before the effective deadline (section 7).

Validate the complete prefix, not only the last two or three records. Preserve
existing schema checks for envelopes, current on-field actors, payloads,
nondecreasing/nonfuture timestamps and causal consistency. A second-half prefix
may contain valid first-half shots: validate historical shots against their
applicable period deadline rather than treating them as second-half shots.
Historical records remain subject to the logger's current-lineup requirements.

Do not infer permission from `home_goals` or `away_goals`, reconstruct a cause
from score, or require a new score replay algorithm. A pending cause must be
explicit and currently at the log tail. Other shot outcomes, neutral or unknown
models, missing GOAL, wrong kind/team/zone, suppressed causes, stale half/clock,
corrupted prefixes and unrelated tails must reject.

**Implementation note:** the logger trusts an API-produced, unmodified prefix
and validates new appends. Existing Shot/recovery code revalidates a prefix in
temporary storage. Restart validation must establish prefix coherence before
commit; merely appending a generic RESTART does not establish these causal or
sporting preconditions. Temporary allocation, if used, may fail only before
caller-visible mutation and must reject atomically.

## 5. Restart event semantics

Append exactly one `RmcMatchEvent` through the existing event-log append
facilities, using event schema version 2:

| Field | Required value |
|---|---|
| `id` | `(uint64_t)log->count + 1`, after headroom validation |
| `match_id` | `state->fixture.id` |
| `at_second` | Current `state->elapsed_seconds` |
| `type` | `RMC_EVENT_RESTART` (existing numeric value 8) |
| `team_id` | Validated awarded team |
| `primary_actor` | Validated explicit taker |
| `secondary_actor` | Zero `RmcPlayerId` |
| `outcome_code` | `RMC_RESTART_GOAL_KICK` = 1 or `RMC_RESTART_KICKOFF` = 2 |
| `cause_event_id` | Source SHOT ID `S` for both kinds |
| `shot` | Neutral `RmcShotEventPayload`, every field zero/neutral |

In particular, neutral shot payload means model 0, sector 0/LEFT zones, zero
identities, numbers and mask, trajectory NONE, restart kind NONE and disposition
`RMC_RESTART_DISPOSITION_NONE`. A RESTART must not copy its cause's model-1
payload. The event kind is explicit in `outcome_code`.

For KICKOFF the preceding GOAL remains in the causal chain; RESTART still points
directly to SHOT, not GOAL or SHOT_CHANCE. Do not emit a
`RMC_EVENT_POSSESSION_START`, a second GOAL, an ACTION or any other extra event.

## 6. State transition matrix

All rows below require the complete validation above, in either active half:

| Source at current tail | Award and destination | Resulting possession | Resulting mode |
|---|---|---|---|
| SHOT/GOAL followed immediately by GOAL, PENDING KICKOFF | Conceding team; `(2, RMC_LANE_CENTRE)` | Fresh awarded-team possession, taker initiator/controller, BUILDUP, counters zero | `RMC_MATCH_BALL_IN_PLAY` |
| SHOT/WIDE, PENDING GOAL_KICK | Defending team; `(0, RMC_LANE_CENTRE)` | Fresh awarded-team possession, taker initiator/controller, BUILDUP, counters zero | `RMC_MATCH_BALL_IN_PLAY` |
| No matching complete cause, wrong mode/period/possession, or any failed check | No award | Unchanged | Unchanged; rejection |

On success assign `possession.team_id`, `zone`, `phase = RMC_PHASE_BUILDUP`,
`initiator = taker`, `controller = taker`, `action_count = 0` and
`elapsed_seconds = 0`. Restart coordinates already use the awarded team's
attacking frame. Do not apply the recovery turnover transformation `4-sector`
or exchange lanes, including for away awards and in the second half.

Keep the fixture, lineups, period, terminal reason, absolute clock,
`second_half_start_second`, both stoppage accumulators and both scores unchanged.
The goal was already scored by Shot v1. Restart charges no possession action.
The next ordinary action starts from the new window at count zero.

## 7. Timing and period boundaries

Use the effective deadline from the existing stoppage policy, with checked
arithmetic (`rmc_stoppage_deadline`):

```text
FIRST_HALF:  0 + 2700 + first_half_stoppage_seconds
SECOND_HALF: second_half_start_second + 2700 + second_half_stoppage_seconds
```

Require current match time strictly less than that deadline. A source shot may
complete during authorized stoppage before the deadline. Restart takes zero
seconds and registers no additional stoppage; it does not call a positive clock
advance or `rmc_stoppage_record` with an invented interruption duration.

A legal Shot v1 GOAL or WIDE exactly at the deadline has
`RMC_RESTART_SUPPRESSED_PERIOD_END`, never PENDING, and cannot authorize a
restart. Existing Shot period closure retains the sporting outcome and score,
then emits the ending events and enters HALF_TIME or FINISHED as applicable.
Restart must reject at or beyond the deadline even if a malformed state claims
RESTART_PENDING. It must not repair or extend the period.

Respect existing sequence precedence: END_PERIOD precedes RESET_ACTION_WINDOW,
which precedes ALLOW_ACTION. `rmc_match_sequence_decide` requires BALL_IN_PLAY;
RESTART_PENDING eligibility must not be obtained by bypassing its deadline rule.
A new possession resets both counters; this differs from RESET_ACTION_WINDOW,
which only resets the continuing possession's action count. Do not change that
operation, loose-ball causes or its event-free behavior. After a valid restart,
the unchanged predeadline clock and zero action count permit the next ordinary
action under the existing sequence policy.

## 8. Atomicity and rejection

Caller-owned objects must remain alive and accessible for the entire call.
The event buffer is caller-owned bounded storage, alive for the log's lifetime;
the engine takes no ownership and must not resize, replace or delete it. Require
exclusive access and nonoverlap of world, command, match state, log metadata,
the entire backing event storage and result, consistent with the existing Shot
action API. The low-level batch logger separately permits its input event array
to overlap storage; this does not relax the action API's object preconditions.

Before any caller-visible write:

1. Validate arguments, world/state/lineups, canonical possession, kind/team/taker,
   period/deadline, coherent prefix and complete currently pending cause.
2. Validate storage and arithmetic: non-null buffer, positive capacity,
   `count <= capacity`, `capacity <= SIZE_MAX / sizeof(RmcMatchEvent)`, room for
   one event, representable count/ID conversion and `count + 1` within both the
   storage/count domain and `UINT64_MAX`. Check bounds before source indexing,
   ID subtraction/addition or temporary-storage sizing. Reject deadline overflow.
3. Prepare candidate `RmcMatchState`, one event and `RmcRestartResult`; complete
   candidate validation before committing. Do not write result as scratch space.
4. Append through `rmc_match_event_log_append` or a one-record
   `rmc_match_event_log_append_batch`, with the valid active state. The logger
   finishes validation before its storage copy and count update.
5. After successful append, assign the prepared state and result. No fallible
   operation may remain after the append succeeds.

Any rejection must preserve byte-for-byte the match state, result, event-log
metadata and **entire backing event storage**, including unused slots and bytes
beyond the current count. No historical event may be mutated or deleted on
success either; only the next slot and count change in the log. Do not use an
unused caller storage slot for speculative validation. No RNG is consumed on
either success or rejection.

**Implementation note:** the existing logger can append this neutral-payload
event without schema changes, but does not enforce restart eligibility itself.
Action-level validation must supply those guarantees. There is no requirement
to allocate temporary storage if equivalent validation is available.

## 9. Single-use guarantee

Successful execution consumes the cause logically through both changes:
the log tail becomes RESTART and mode becomes BALL_IN_PLAY. The historical SHOT
and GOAL retain their original payloads, including PENDING disposition. They are
history, not a mutable pending token. Do not mark historical payloads consumed.

An identical second command must fail without mutation. Changing only the mode
back to RESTART_PENDING would still leave an ineligible RESTART tail; an old
source elsewhere in the prefix cannot be reused. No consumed-ID registry or
new persistent state field is required by this contract.

## 10. Determinism and versioning

For the same validated world, match/lineups, event prefix and command, successful
restart must produce the same semantic state, event and result. It consumes no
bounded draws or raw PCG32 outputs and changes no RNG stream identity or state.
Host clock, pointer addresses, scheduling and floating point must not affect it.
Replay comparisons/serialization compare explicit fields, not native padding,
enum width or host endianness. Byte-level rejection checks establish memory
preservation, not a portable replay hash format.

Preserve global RulesVersion 1, GeneratorVersion 1, shot model version 1
(`RMC_SHOT_MODEL_VERSION`), event schema version 2 (`RMC_EVENT_SCHEMA_VERSION`),
all existing event type numeric values, all frozen Shot v1 mathematics and
talent/draw rules, logistic table v1, recovery behavior, clock/lifecycle/sequence
semantics, stream derivation and previously qualified stream vectors. Do not
bump these versions or reinterpret old records to add Restart v1.

**Implementation note — consistency assessment:** existing `RmcRestartKind`,
pending/suppressed SHOT payloads and the schema-v2 envelope express the approved
rules without a data-layout change. The Shot contract's statements that shot
execution/recovery extensions were future work describe its original increment;
the current source implements them. This documentation-status drift is not a
sporting contradiction and is not corrected here. Wider M001 restart categories
remain future work relative to this narrow contract. No unresolved normative
contradiction was identified in the reviewed rules and interfaces.

## 11. Qualification tests

The subsequent implementation must add one dedicated CTest case and preserve
the existing 31/31 regression baseline, targeting **32/32 PASS**. This contract
increment does not run a build or CTest or claim implementation qualification.

Require production integration through `rmc_match_shot_apply`,
`rmc_match_restart_apply` and `rmc_match_pass_apply`, not only fabricated events:

```text
GOAL -> KICKOFF -> BALL_IN_PLAY -> PASS
WIDE -> GOAL_KICK -> BALL_IN_PLAY -> PASS
```

Qualification must cover:

- Home and away awards in both halves, correct taker/controller, zone and BUILDUP
  phase, direct SHOT reference, GOAL result ID when applicable, exactly one
  RESTART and no additional POSSESSION_START, unchanged score/clock/stoppage.
- Current POR eligibility, rejecting bench/other goalkeepers and outfield POR
  occupants; valid outfield KICKOFF takers and rejection of POR occupants,
  genuine goalkeepers, substitutes, opposing players and zero/unknown actors.
- Wrong/unsupported kind, wrong team, wrong taker and zero/wrong/stale source ID;
  malformed model, trajectory, award, zone or disposition; corrupted chance/shot/
  goal actors, payloads, IDs, match IDs, references, timestamps or adjacency;
  corrupted earlier prefix and valid mixed-half history.
- Duplicate restart rejection, missing causes, missing adjacent GOAL, unrelated
  tail events and SUPPRESSED_PERIOD_END causes. Opening-match and second-half
  lifecycle pending states alone must reject; score must not authorize them.
- Invalid world/fixture/lineups, inactive periods, wrong modes, active or
  noncanonical cleared possession, stale source half/time, and clock at or beyond
  each effective deadline. Include authorized stoppage and arithmetic overflow.
- Fresh action-window semantics: both possession counters reset to zero and the
  following PASS starts as the first action. Preserve existing 32-action reset
  behavior and period-end precedence, including a shot at the window limit.
- Atomic rejection snapshots of state, result, all log metadata and all backing
  storage, including sentinel-filled unused slots, for every rejection class.
  Include precommit resource failure if temporary allocation is used.
- Deterministic replay and unchanged RNG around restart on success and failure;
  following PASS must see the same stream state it would see without a restart
  draw. Preserve the existing qualified stream vectors.
- Exactly one free event slot succeeds; full log, invalid storage metadata,
  unsafe capacity/count arithmetic and exhausted event-ID headroom reject.

## 12. Deferred features

Opening-match and second-half opening kickoff, corners, throw-ins, fouls, free
kicks, penalties and any other restart category require a separate approved
contract. Automatic taker selection, timed/contextual or probabilistic restarts,
new restart mathematics, extra stoppage policy, schema migration and new replay
serialization are not introduced here. Existing recovery behavior remains intact.
Implementation, tests and unrelated documentation corrections are deferred from
this document-only task.
