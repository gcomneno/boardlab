# M001-B1.28-A — Match Flow v1 contract

Status: normative architecture contract for the next implementation increments;
no implementation or new qualification result is delivered here. Baseline source
is HEAD `4d06e0b277bb845c737e471a85289a15e40414f5`. The supplied qualified
baseline is **32/32 CTest PASS**; this document-only task does not rerun it.

**EXISTING** means behavior inspected in that source. **APPROVED** identifies
the seven task decisions. **PROPOSED** identifies concrete new API, policy or
envelope conventions specified here for implementation. **DEFERRED** identifies
work outside these increments. Requirements using must, must not and requires
are binding architectural constraints. Proposed numerical policy choices remain
**CANDIDATE VALUES REQUIRING NORMATIVE FREEZE**, as collected in section 16;
they are not previously approved sporting rules or qualified calibration.

## 1. Purpose and scope

Provide the minimum deterministic ISO C11 composition that can execute:

```text
NOT_STARTED -> FIRST_HALF opening -> actual sporting actions
            -> HALF_TIME -> SECOND_HALF opening -> actual sporting actions
            -> FINISHED / COMPLETED
```

The player remains a manager. A deterministic external selector chooses ordinary
actions from current participants under bounded instructions; existing action
engines validate and resolve them. The coordinator orders operations and owns
resources. This is neither a replacement match engine nor a generic framework.

Sources and normative parents are [Concept v1](concept-v1.md),
[M001 Engineering Contract](m001-engineering-contract.md),
[Shot v1](m001-shot-v1-contract.md),
[Restart v1](m001-restart-v1-contract.md) and
[Determinism v1](determinism-v1.md). The wider M001 season, decisions/AI, RPG,
persistence and qualification obligations remain in force; this narrow first
playable match does not claim their completion. Earlier contracts' original
increment-status wording is not corrected by this task.

## 2. Existing foundations

All source paths in this table are relative to `src/rpg_manager_calcio/`.

| EXISTING interface | Actual responsibility and limit |
|---|---|
| `career.h`, `rmc_career_setup_generate`, `rmc_career_setup_valid` | Generate/validate `RmcCareerSetup`: foundation world/calendar and three candidate references; no playable season loop. |
| `calendar.h`, `RmcFixture`, `RmcCalendar` | Scheduled MatchId, explicit home/away TeamIds, matchday; 56 fixtures in canonical matchday order. A standalone match-state validator does not prove membership in that calendar. |
| `roster.h`, `RmcLineup`, `rmc_roster_lineup` | Eleven distinct legal players in canonical formation slots for 442, 433 or 352. `RmcPlayer.department`, `team_id` and slot assignments establish participant eligibility. |
| `match_state.h/.c`, `rmc_match_state_init`, `rmc_match_state_valid` | Fixture, fixed lineup references, period/mode, absolute clock, stoppage accumulators, score, possession. General validity is weaker than complete flow/causal validity. |
| `match_transitions.h/.c`, `rmc_match_transition_apply` | START and SECOND_HALF enter RESTART_PENDING; HALF_TIME and FINISH clear possession and enter PERIOD_ENDED. No events, time or RNG. |
| `match_clock.h/.c`, `rmc_match_clock_advance` | Positive seconds in active BALL_IN_PLAY only; accepts an exact deadline, rejects overshoot. Advances possession elapsed time when present. |
| `match_stoppage.h`, `rmc_stoppage_deadline` | 2700 regulation seconds per half, independently capped stoppage of 900 seconds; checked deadline addition. |
| `match_interruption.h`, `rmc_match_interruption_record` | Existing explicit interruption accounting in RESTART_PENDING or MANAGEMENT_WINDOW; no clock advance. Flow v1 generates no such interruptions. |
| `match_sequence.h/.c`, `rmc_match_sequence_decide` | In active BALL_IN_PLAY: END_PERIOD before RESET_ACTION_WINDOW before ALLOW_ACTION; cap is `RMC_POSSESSION_MAX_ACTIONS == 32`. |
| `match_sequence_apply.h/.c` | Technical reset changes only action_count, emits no event, consumes no time/RNG. Bare END_PERIOD is unlogged and insufficient for the coordinator. |
| `match_period_commit.h/.c` | `rmc_match_period_plan` prepares deadline closure; `rmc_match_period_commit` atomically appends PERIOD_END, plus MATCH_END in half two, and commits the lifecycle transition. |
| `match_event_log.h/.c` | Caller-owned bounded append-only `RmcMatchEventLog`, consecutive uint64 IDs from 1, schema 2; batch append validates before copying. It trusts an unmodified API-produced prefix. |
| `match_pass_action`, `match_carry_action`, `match_dribble_action`, `match_tackle_action`, `match_cross_action` (`.h/.c`) | Qualified production `rmc_match_*_apply` operations, positive duration, context in [-20,20], one ACTION event and one bounded MATCH draw each. |
| `match_shot_action.h/.c`, `rmc_match_shot_apply` | Frozen Shot v1, 2–4 bounded draws; ACTION/SHOT_CHANCE, SHOT, optional GOAL and atomic exact-deadline closure. |
| `match_recovery.h/.c`, `rmc_match_recovery_apply` | Positive-duration contested recovery, one bounded draw, one causally linked RECOVERY; recognizes pass/tackle/cross and Shot loose-ball causes. |
| `match_restart_action.h/.c`, `rmc_match_restart_apply` | Frozen Shot-caused GOAL/KICKOFF or WIDE/GOAL_KICK only; one neutral-payload RESTART, no time/RNG/action charge. Does not open a half. |
| `streams.h/.c`, `rmc_stream_init` | Canonical MATCH PCG32 derivation from world identity and nonzero fixture MatchId. Shared generator implementation is `src/boardlab_c11/rng.c`. |

Carry/dribble advance sector by one on success and preserve lane **and phase**;
contained outcomes retain the ball and sector. Pass changes controller or makes
the ball loose, without moving zone/phase. Tackle WON and recovery TURNOVER
invert sector as `4-sector`, retain lane and create a fresh TRANSITION possession.
Cross requires sector 2/3 and LEFT/RIGHT, targets next sector/CENTRE and sets
ATTACKING_DEVELOPMENT or CHANCE on completed/loose outcomes. Shot requires sectors
2..4, any lane, and does not require `RMC_PHASE_CHANCE`.

## 3. Normative decisions

| APPROVED decision | Required implementation consequence |
|---|---|
| 1. Opening kickoffs | Separate atomic operation; explicit first opening team and on-field outfield taker; opponent opens half two; `(2,CENTRE)`, BUILDUP; PERIOD_START then RESTART/KICKOFF, no synthetic SHOT/GOAL; zero RNG/time/action charge; once per half. See section 5. |
| 2. Managerial control | Bounded instructions at pre-match and half-time boundaries, separate from match rules. Ordinary action choice belongs to selector. Any direct-action test driver is explicitly noncanonical. See sections 6 and 12. |
| 3. External selector | Deterministic participant/action choice from validated current lineups, including defenders; context 0; strictly positive, remaining-time-bounded sporting durations; no MATCH draws to choose candidates or personality AI. |
| 4. Single authority | One `RmcMatchState`, one live MATCH `RmcPcg32`, one log and one bounded accepted typed-command journal; immutable world and fixed lineups. CLI/selector cannot mutate sporting state. |
| 5. Progress | Mandatory opening, exact closure, eligible technical reset, Shot restart, recoverable loose ball, then ordinary sporting action, with mode-aware eligibility. No inserted sporting events, deadline overshoot, zero-time loops or unbounded retry. See sections 4 and 8. |
| 6. Atomicity/resources | Each command is one transaction. Reserve journal slot and maximum event branch before production operation. Rejections preserve all caller-visible data. No promise to roll back earlier accepted commands. See sections 9–10. |
| 7. Replay | Same inputs, initial stream and accepted typed commands reproduce semantic events/causes, half-time/final states, scores, reason and RNG. No canonical-hash, portable replay-file or global MATCH_REPLAY closure claim. See section 11. |

Preserve `RMC_RULES_VERSION == 1`, `RMC_GENERATOR_VERSION == 1`,
`RMC_EVENT_SCHEMA_VERSION == 2`, `RMC_SHOT_MODEL_VERSION == 1`, frozen
logistic/probability models, Shot/Restart/recovery behavior, sequence/clock/
lifecycle semantics and current PCG32 derivation/vectors. A new flow-policy
version identifies composition choices without redefining those versions.

## 4. Full lifecycle/state machine

The actual `RmcMatchPeriod` identifiers are `RMC_MATCH_NOT_STARTED`,
`RMC_MATCH_FIRST_HALF`, `RMC_MATCH_HALF_TIME`, `RMC_MATCH_SECOND_HALF` and
`RMC_MATCH_FINISHED`. Period, mode and possession phase are distinct dimensions.
In particular `RMC_MATCH_PERIOD_ENDED`, `RMC_MATCH_RESTART_PENDING` and
`RMC_MATCH_BALL_IN_PLAY` are **modes**, not period names.

| Source / required condition | Next accepted operation | Required postcondition |
|---|---|---|
| NOT_STARTED, PERIOD_ENDED, clock/score/stoppage zero, cleared possession and empty log; managerial window resolved | Coordinator first opening: private START, then Opening Kickoff Engine | FIRST_HALF, BALL_IN_PLAY, opening possession; two opening events. |
| Active half, BALL_IN_PLAY, controlled possession, predeadline and ALLOW_ACTION | Selector's eligible positive-duration action | Existing engine outcome, time advances; counters/events/RNG exactly as that engine defines. |
| Active half, BALL_IN_PLAY, controller zero, nonzero possession team and recoverable log tail; predeadline and ALLOW_ACTION | Recovery only | Controller assigned or turnover, one linked RECOVERY, positive time; no invented restart. |
| Active half, BALL_IN_PLAY, predeadline, action_count >= 32 | Technical reset | Only action_count becomes zero; controller may remain zero; cause stays at log tail. No event. |
| Active half, RESTART_PENDING, canonical cleared possession, predeadline, complete current Shot GOAL/WIDE tail | Existing Restart v1 | BALL_IN_PLAY, fresh awarded possession, one linked RESTART; score/time/RNG unchanged. |
| Active half, BALL_IN_PLAY, clock equals effective deadline, with controlled, loose or no possession | Logged period closure | First half: HALF_TIME/PERIOD_ENDED, cleared possession, second_half_start_second set, PERIOD_END. Second half: FINISHED/PERIOD_ENDED/COMPLETED, PERIOD_END then MATCH_END. |
| HALF_TIME, PERIOD_ENDED, cleared possession, valid first-half termination history, managerial window resolved | Coordinator second opening: private SECOND_HALF, then Opening Kickoff Engine | SECOND_HALF/BALL_IN_PLAY, opponent of first opener controls opening possession at unchanged clock; two opening events. |
| FINISHED with `RMC_MATCH_TERMINAL_COMPLETED` | Report completion only | No further command, event or draw. |

**PROPOSED:** opening-pending is a coordinator classification at NOT_STARTED or
HALF_TIME after its managerial window resolves. It is not a new state enum or
mutable possession flag. The table describes the future Match Flow Coordinator's
transaction, not the directly callable Opening Kickoff Engine's entry state.
The coordinator copies the inactive state and privately applies
`RMC_TRANSITION_START` or `RMC_TRANSITION_SECOND_HALF` through
`rmc_match_transition_apply`, obtaining an active FIRST_HALF or SECOND_HALF
candidate in RESTART_PENDING. The Opening Kickoff Engine validates that candidate,
atomically appends PERIOD_START -> RESTART and establishes fresh opening
possession. It requires active RESTART_PENDING and does not itself invoke START
or SECOND_HALF. The coordinator publishes the accepted candidate only after
complete success; failure must preserve the caller-visible inactive state and
must not expose a partially applied lifecycle transition. It must not expose an
accepted bare START followed by a separate fallible kickoff. Legacy externally
created active RESTART_PENDING with no Shot cause is rejected by flow admission, not silently
treated as either kind of restart. This keeps single-use proof unambiguous.

An active PERIOD_ENDED or MANAGEMENT_WINDOW, BALL_IN_PLAY without a possession
before the deadline, missing loose-ball cause, RESTART_PENDING at the deadline,
or clock beyond the deadline is not reachable through this coordinator. Some
are allowed by weaker general state validation. Report invalid flow state;
do not fabricate a recovery, reset, restart, interruption or clock repair.
Insufficient-player suspension has an existing terminal reason but is not
generated by fixed-lineup flow v1; it must never be reported as COMPLETED.

## 5. Opening kickoffs and causality

### 5.1 Separate operation

**PROPOSED** C11 public interface for B1.28-B (new declarations, not existing API):

```c
typedef struct {
    RmcMatchPeriod opening_period; /* FIRST_HALF or SECOND_HALF expectation */
    RmcTeamId opening_team;
    RmcPlayerId taker;
} RmcOpeningKickoffAction;

typedef struct {
    uint64_t period_start_event_id;
    uint64_t restart_event_id;
} RmcOpeningKickoffResult;

bool rmc_match_opening_kickoff_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcOpeningKickoffAction *action,
    RmcOpeningKickoffResult *result);
```

No RNG, context, duration or source_shot_event_id argument is present. Do not
expand `RmcRestartAction` or weaken `rmc_match_restart_apply` eligibility.
For half one, `opening_team` explicitly selects either fixture team. For half
two it is an expectation that must equal the fixture opponent of the first
PERIOD_START's opening team. The coordinator's immutable configuration also
records the first opening team and must agree with that history.

Resolve nonzero taker through world teams, roster ownership, player.team_id and
the opening team's current lineup; require slot != `RMC_SLOT_POR` and department
!= `RMC_DEPARTMENT_GOALKEEPER`. Reject bench, opposing, POR, genuine goalkeeper,
unknown and zero actors. Do not infer identity from array indices. Automatic
choice by the external selector still produces an explicit validated command.

### 5.2 Source and history requirements

Require non-null, accessible, nonoverlapping arguments and storage, exclusive
access, valid world/state/lineups and matching log MatchId. Possession must have
the semantic field values of `(RmcMatchPossession){0}`, including sector 0,
LEFT and BUILDUP, not merely zero team/controller. Do not compare padding.

At the coordinator level, first opening starts from the initialized
NOT_STARTED/PERIOD_ENDED state; second opening starts from HALF_TIME/PERIOD_ENDED
after logged first-half closure. The coordinator privately stages the respective
lifecycle transition as specified in section 4 before calling the engine.

The directly callable `rmc_match_opening_kickoff_apply` requires an already active
FIRST_HALF or SECOND_HALF state in RESTART_PENDING, with `opening_period` equal
to that active period and terminal reason NONE. It rejects NOT_STARTED and
HALF_TIME inputs and does not apply START or SECOND_HALF itself.

First opening at the engine boundary requires FIRST_HALF/RESTART_PENDING,
clock zero, zero scores/stoppage and `log->count == 0`.
No prior PERIOD_START, SHOT or artificial POSSESSION_START is permitted.

Second opening at the engine boundary requires SECOND_HALF/RESTART_PENDING,
unchanged half-time clock equal to `second_half_start_second` and the checked first
deadline, second-half stoppage zero, and a completely coherent prefix ending
in the first-half PERIOD_END. That ending event has neutral team/actors/outcome/
Shot payload, cause 0, and timestamp equal to that deadline. The prefix contains
exactly the first opening pair, valid first-half action/causal chains and that
single PERIOD_END; no second PERIOD_START, MATCH_END or second opening. Require
the first opener to be a fixture team and its opening RESTART to reference it.
Historical Shot events must be validated against the first-half deadline.
Validate semantic lifecycle order as well as schema envelopes. A manufactured
HALF_TIME state staged into SECOND_HALF or unrelated tail cannot substitute for
termination history.
This does not require the low-level opening operation to recompute the score
from every historical event; the coordinator admits only its own accepted prefix.

### 5.3 Exact proposed envelopes

Let `M = state->fixture.id`, `C = log->count`, `T = state->elapsed_seconds`,
`O = validated opening_team`, `P = validated taker`. Let `E` be the tail
first-half PERIOD_END ID for half two. Validate all additions before evaluating
IDs. The complete operation appends exactly this batch in this order:

| `RmcMatchEvent` field | PERIOD_START | Opening RESTART |
|---|---|---|
| `id` | `C + 1` | `C + 2` |
| `match_id` | M | M |
| `at_second` | T (0 for first; actual first deadline for second) | T |
| `type` | `RMC_EVENT_PERIOD_START` (0) | `RMC_EVENT_RESTART` (8) |
| `team_id` | O | O |
| `primary_actor` | zero PlayerId | P |
| `secondary_actor` | zero PlayerId | zero PlayerId |
| `outcome_code` | 0, neutral period-family outcome | `RMC_RESTART_KICKOFF` (2) |
| `cause_event_id` | 0 in half one; E in half two | `C + 1`, the adjacent PERIOD_START |
| `shot` | Fully neutral | Fully neutral |

Neutral payload means model/quality/context/duration/mask and all player/team
IDs zero, both zones `(0,RMC_LANE_LEFT)`, trajectory NONE, restart kind NONE,
disposition `RMC_RESTART_DISPOSITION_NONE`. The envelope's outcome 2 carries
KICKOFF; it does not put a pending Shot restart in the payload. No half number
is packed into an outcome code. The validated lifecycle/history and typed
opening command identify the half. No source SHOT exists or is manufactured.

After successful append and commit, the active period is the expected half,
mode BALL_IN_PLAY and terminal reason NONE. Fresh possession has team O,
initiator/controller P, zone `(2,RMC_LANE_CENTRE)`, phase `RMC_PHASE_BUILDUP`,
action_count 0 and possession elapsed_seconds 0. Preserve fixture, lineups,
absolute clock, half-start field, scores and stoppage accumulators. No coordinate
inversion is applied for away or second-half openings. Emit no POSSESSION_START,
ACTION, SHOT or GOAL. Success charges no time, possession action or RNG.

### 5.4 Schema compatibility and transaction

**EXISTING inspection:** `rmc_match_event_log_append_batch` requires an active
FIRST_HALF/SECOND_HALF validation state. It cannot append against NOT_STARTED
or HALF_TIME themselves. The coordinator prepares the transitioned active
RESTART_PENDING candidate and passes it to the Opening Kickoff Engine; the engine
validates and prepares the BALL_IN_PLAY candidate used by the logger.
Both proposed records have codes outside 500..506, so `shot_valid` accepts model 0
with all-neutral fields. Match-team and
on-field-actor checks accept O, P and absent actors; consecutive IDs, equal
timestamps and the real earlier/batch causes satisfy `batch_cause` checks.
The second PERIOD_START references the real tail PERIOD_END at the same clock.

**Finding:** the complete proposed opening chain is representable and accepted
by the current schema-v2 envelope validator without changes to layout, event
codes or validator. Existing Shot/Recovery/Restart prefix revalidators can also
accept it: they split history at the first deadline. The neutral second opening
events at that shared timestamp may be checked with the first-half validation
copy; generic envelope validation does not attach a period to them. Positive
second-half actions finish after that timestamp and use the second-half checks.
This is source inspection, not an executed compatibility test.

**Precise future dependency:** generic logging does not enforce PERIOD_START
uniqueness, opening-team alternation, PERIOD_END-to-PERIOD_START lifecycle order,
taker's team/outfield eligibility or RESTART-to-PERIOD_START opening causality.
B1.28-B must supply these checks in the new opening operation, including coherent
prefix validation. This is an operation-validation gap, not a schema conflict.
Appending the pair alone would not implement this contract.

Within the engine, prepare next state, both events and result privately from the
active RESTART_PENDING input; do not invoke a lifecycle transition. Preflight two
slots and all ID/count/size arithmetic; validate the complete candidate and history before
writing. Append the pair in **one** batch against the active candidate, then
assign state/result with no remaining fallible call. Rejection preserves state,
result, all log metadata and every byte of the entire backing storage. Prefix
validation allocation failure, if allocation is used, occurs before commit.
Engine success moves the active candidate from RESTART_PENDING to BALL_IN_PLAY
and records the opening pair; a repeated command rejects. The coordinator calls
the engine with its private staged state, the live log and a private result.
After engine success, publish that state and the prepared journal/metadata/result
using only infallible copies, with no remaining validation or allocation. On any
failure, discard the candidate: the authoritative state remains NOT_STARTED or
HALF_TIME, and the log, its entire backing storage, journal and caller result
remain unchanged. Exclusive access covers this complete enclosing transaction.
No extra mutable consumed-opening registry is needed.

## 6. Action-selection policy

### 6.1 Managerial inputs, separate from sporting rules

**PROPOSED:** expose only these bounded instructions for the first match:

| Input | Allowed values | Selector effect only |
|---|---|---|
| Approach | PATIENT, DIRECT | Prefer later shooting at sector 4 or earlier shooting from sector 3 on the scheduled finishing slot. |
| Progression | CARRY, DRIBBLE | Select the corresponding existing forward action. |
| Pressure | NORMAL, HIGH | Increase scheduled defender challenge frequency. |
| KEEP | No parameter | Retain the existing instruction triple and close that team's decision submission for the window. |

Default triple is PATIENT/CARRY/NORMAL for both teams. Accept one complete
instruction submission or KEEP per team in each of two windows: NOT_STARTED
before opening and HALF_TIME after logged closure. A submission closes that
team's window; no endless zero-time resubmission or extra confirmation command.
Once both teams submit, next classification is opening. The human submits only
for the selected managed fixture team; the other team's default/KEEP uses the
same validator. Noninteractive mode supplies explicit deterministic submissions.
Decision metadata includes MatchId, team, expected period/window, sequential
command ID and observed accepted-command count (section 7).

No instructions edit attributes, probability tables, phases, scores or context.
Context is **0** for every selected duel. No personality utility model, hidden
information, situational quota, active MANAGEMENT_WINDOW, substitutions or
formation changes during the match are introduced. Pre-match legal formation
selection remains available; lineups then freeze. This is the approved narrow
managerial subset, not closure of the parent decisions/AI feature set.

### 6.2 Participants and bounded search

Read only the observable current state, immutable world, instructions and
accepted-operation counters; do not inspect or sample MATCH RNG or preview
outcomes. Validate current lineups and resolve team/player ownership first.
Use ascending nonzero PlayerId to order eligible players, independent of storage
addresses. For a given role select the smallest eligible ID, except a pass/cross
recipient must differ from the current controller. The exact selection rule is
PROPOSED, deterministic and bounded by the existing 11-player lineups/18-player
rosters; it is not a new probability model or an optimal-player claim.

The controller is always the passer/carrier/dribbler/crosser/shooter. Select an
opposing on-field outfielder as pass opponent, carry/dribble opponent, cross
defender, Shot defender and tackle defender; tackle.team_id is that defender's
team. Shot goalkeeper is the opposing genuine goalkeeper at the current POR
slot. Opening and post-goal takers are awarded-team outfielders. Goal-kick taker
is the awarded team's genuine POR occupant. Recovery chooses an outfield
incumbent for the suspended possession team and an outfield challenger for its
opponent. The action engines remain the final validators.

A goalkeeper controlling after SAVED_HELD or GOAL_KICK must first pass to the
smallest eligible outfield teammate, against a selected outfield opponent. Do
not select a Shot or force a goalkeeper into an outfield role. Ordinary pass
recipients are always outfield players. This avoids later selection of an
ineligible goalkeeper shooter while retaining qualified goalkeeper behavior.

### 6.3 Concrete candidate policy table

The following schedules, thresholds, priorities within ordinary play and nominal
seconds are **CANDIDATE VALUES REQUIRING NORMATIVE FREEZE**. Mandatory operation
precedence in section 8 is already approved and overrides this table. Define
`n` as the total accepted positive-duration sporting operations, including
recovery, since initialization; it never resets with possession or half. Let
`q = n % 5`. Let `R = checked_deadline - elapsed_seconds`.
Every timed row uses `duration_seconds = min(nominal_seconds, R)` and requires
`R > 0`; clipping is selector preparation before submission, not engine truncation.

| Priority | Eligible situation | Selected production operation | Nominal seconds |
|---:|---|---|---:|
| Mandatory | Recoverable loose ball, ALLOW_ACTION | `rmc_match_recovery_apply` | 4 |
| 1 | Current controller is genuine POR goalkeeper | `rmc_match_pass_apply` to outfielder | 8 |
| 2 | Outfield control; q == 2; or HIGH pressure and q == 1 | Opponent's `rmc_match_tackle_apply` | 5 |
| 3 | Outfield control at sector 4; or q == 4 and sector >= shot threshold (PATIENT: 4, DIRECT: 3) | `rmc_match_shot_apply` | 6 |
| 4 | Outfield control, sector 2/3 and LEFT/RIGHT, q == 4 | `rmc_match_cross_apply` | 8 |
| 5 | Outfield control, q == 0 | `rmc_match_pass_apply` to distinct outfielder | 8 |
| 6 | Outfield control, sector < 4 | `rmc_match_carry_apply` or `rmc_match_dribble_apply` per instruction | 10 / 7 |
| 7 | Remaining eligible controlled case | Legal outfield-to-outfield PASS | 8 |

Evaluate the finite rows once, first eligible row wins. A defender challenge
can precede a sector-4 shot on its scheduled slot; it advances time and n, so
it cannot suppress finishing indefinitely by a zero-time cycle. A contained
advance can repeat on later slots but still advances time. There is no promise
that every match reaches every sector or scores; progress to the whistle does
not depend on a successful carry, pass, tackle, recovery or shot.

Pass/tackle/recovery are legal in all existing possession phases subject to
their actual state/role preconditions. Carry/dribble require sector < 4 and
preserve the current phase; Shot requires sectors 2..4, not CHANCE phase.
Cross requires its actual lateral source zone. Do not rewrite BUILDUP into
PROGRESSION or CHANCE merely because sector advanced. Turnovers and Shot/cross
outcomes establish only the phases those engines already implement.

Normal centre-start play cannot reach lateral cross zones: openings and Shot
restarts are CENTRE, passes do not move zones, carries/dribbles preserve lane,
tackle/recovery turnover preserves lane, and Shot destinations retain CENTRE
or the already central source. Cross **requires** LEFT/RIGHT; it cannot create
its own lateral source. Thus the cross row is unreachable in ordinary v1 flow.
Keep qualified cross behavior for existing isolated tests; do not inject a
lateral possession, lateral movement operation or extra event to exercise it
in a full-match campaign. Tactical width and lateral movement are DEFERRED.

If no row has valid participants/preconditions, return a selector diagnostic
without a command or mutation. Do not consume RNG, attempt arbitrary engines
until one accepts, or retry a rejected candidate indefinitely. A correct valid
fixed-lineup flow has an eligible timed row whenever ordinary selection is
requested. Engineering direct-action drivers may submit typed legal actions
for targeted tests but must be labeled **noncanonical test policy**, use the
same production operations and never become the managerial CLI default.

## 7. Coordinator architecture

**PROPOSED:** a small stateful `match_flow.h/.c` owns the live match composition;
`match_flow_selector.h/.c` implements section 6 externally. No rule engine is
embedded in the selector or duplicated in the coordinator. The following is a
concrete proposed public C11 shape; referenced sporting types already exist,
except the opening types in section 5. All enum names prefixed FLOW below are
new proposals, not claims about existing headers.

```c
typedef enum {
    RMC_FLOW_OK = 0,
    RMC_FLOW_INVALID_ARGUMENT,
    RMC_FLOW_INVALID_STATE,
    RMC_FLOW_WRONG_OPERATION,
    RMC_FLOW_STALE_COMMAND,
    RMC_FLOW_RESOURCE_EXHAUSTED,
    RMC_FLOW_OPERATION_REJECTED,
    RMC_FLOW_UNSUPPORTED_VERSION
} RmcFlowError;

typedef enum {
    RMC_FLOW_INSTRUCTION = 0,
    RMC_FLOW_OPENING,
    RMC_FLOW_CLOSE_PERIOD,
    RMC_FLOW_RESET_WINDOW,
    RMC_FLOW_RESTART,
    RMC_FLOW_RECOVERY,
    RMC_FLOW_PASS,
    RMC_FLOW_CARRY,
    RMC_FLOW_DRIBBLE,
    RMC_FLOW_TACKLE,
    RMC_FLOW_CROSS,
    RMC_FLOW_SHOT
} RmcFlowCommandKind;

typedef enum { RMC_FLOW_PATIENT, RMC_FLOW_DIRECT } RmcFlowApproach;
typedef enum { RMC_FLOW_CARRY_FIRST, RMC_FLOW_DRIBBLE_FIRST } RmcFlowProgression;
typedef enum { RMC_FLOW_PRESS_NORMAL, RMC_FLOW_PRESS_HIGH } RmcFlowPressure;

typedef struct {
    RmcFlowApproach approach;
    RmcFlowProgression progression;
    RmcFlowPressure pressure;
} RmcFlowInstructions;

typedef struct {
    RmcTeamId team_id;
    RmcMatchPeriod window; /* NOT_STARTED or HALF_TIME */
    bool keep;
    RmcFlowInstructions instructions; /* neutral zero fields when keep */
} RmcFlowInstructionCommand;

typedef struct {
    uint32_t flow_version;
    uint32_t selector_version;
    RmcTeamId first_opening_team;
} RmcMatchFlowConfig;

typedef struct {
    uint64_t command_id;             /* next accepted ordinal, starting at 1 */
    uint64_t observed_command_count; /* stale-state guard */
    RmcMatchId match_id;
    RmcMatchPeriod expected_period;
    RmcFlowCommandKind kind;
    union {
        RmcFlowInstructionCommand instruction;
        RmcOpeningKickoffAction opening;
        RmcRestartAction restart;
        RmcRecoveryAction recovery;
        RmcPassAction pass;
        RmcCarryAction carry;
        RmcDribbleAction dribble;
        RmcTackleAction tackle;
        RmcCrossAction cross;
        RmcShotAction shot;
    } data; /* no payload for close/reset; inactive members ignored */
} RmcMatchFlowCommand;

typedef struct {
    RmcMatchFlowCommand *commands;
    size_t count;
    size_t capacity;
} RmcMatchCommandJournal;

typedef struct {
    const RmcWorldCore *world;
    RmcMatchFlowConfig config;
    RmcMatchState state;
    RmcPcg32 match_rng;
    RmcMatchEventLog log;
    RmcMatchCommandJournal journal;
    RmcFlowInstructions home_instructions;
    RmcFlowInstructions away_instructions;
    bool home_window_submitted;
    bool away_window_submitted;
    uint64_t sporting_count;
    uint64_t reset_count;
} RmcMatchFlow;

typedef enum {
    RMC_FLOW_NEED_INSTRUCTION = 0,
    RMC_FLOW_NEED_OPENING,
    RMC_FLOW_NEED_CLOSURE,
    RMC_FLOW_NEED_RESET,
    RMC_FLOW_NEED_RESTART,
    RMC_FLOW_NEED_RECOVERY,
    RMC_FLOW_NEED_SPORTING_ACTION,
    RMC_FLOW_COMPLETE
} RmcFlowNextKind;

typedef struct {
    RmcFlowNextKind kind;
    RmcTeamId team_id; /* pending instruction/opening/award, else zero */
    uint32_t deadline_second; /* zero before start; active/half-time deadline */
    uint32_t remaining_seconds;
    uint64_t observed_command_count;
} RmcMatchFlowNext;

typedef struct {
    RmcMatchPeriod period;
    RmcMatchMode mode;
    RmcMatchTerminalReason terminal_reason;
    uint32_t at_second;
    uint16_t home_goals;
    uint16_t away_goals;
    uint64_t accepted_commands;
    uint64_t sporting_operations;
    size_t event_count;
    bool complete;
    bool needs_instruction;
} RmcMatchFlowStatus;

typedef struct {
    uint64_t accepted_command_id;
    size_t first_event_index; /* old log count, including event-free commands */
    size_t appended_event_count;
    bool entered_half_time;
    RmcMatchFlowStatus status;
} RmcMatchFlowStepResult;

RmcFlowError rmc_match_flow_init(
    const RmcWorldCore *world, const RmcFixture *fixture,
    const RmcLineup *home, const RmcLineup *away,
    const RmcMatchFlowConfig *config,
    RmcMatchEvent *event_storage, size_t event_capacity,
    RmcMatchFlowCommand *command_storage, size_t command_capacity,
    RmcMatchFlow *destination);

RmcFlowError rmc_match_flow_next(
    const RmcMatchFlow *flow, RmcMatchFlowNext *destination);

RmcFlowError rmc_match_flow_apply(
    RmcMatchFlow *flow, const RmcMatchFlowCommand *command,
    RmcMatchFlowStepResult *destination);

RmcFlowError rmc_match_flow_status(
    const RmcMatchFlow *flow, RmcMatchFlowStatus *destination);

RmcFlowError rmc_match_flow_select(
    const RmcMatchFlow *flow, const RmcMatchFlowNext *next,
    RmcMatchFlowCommand *destination);
```

Initialization validates configuration versions and first opening team, calls
`rmc_match_state_init` and `rmc_match_event_log_init` on local candidates,
derives the single stream with `rmc_stream_init(..., RMC_STREAM_MATCH,
fixture->id.value)`, initializes instructions/counters/windows, then commits.
Initial state/log/journal counts are zero; storage contents remain untouched.
The CLI must separately prove the fixture belongs to the validated generated
calendar; low-level init accepts a valid fixture for isolated integration tests.

World and both caller-supplied backing arrays must outlive flow; init borrows
them, never frees, resizes or replaces them. The live state/lineups/config are
copied into flow; caller inputs are not live mutable aliases. The world remains
const for the match. Require nonoverlap between world, flow, configuration,
input command/result and entire storage buffers, and exclusive access during
application. Read APIs and selector are pure and preserve outputs on error.
Public structure fields are inspectable but application-owned mutation is
forbidden. A shallow copy of initialized flow aliases buffers and must not be
used as another executor or rollback snapshot. Read-only semantic snapshots for
tests/reports/replay comparison are observations, not second authorities.

`next` classifies without running an operation. It requests instructions in a
fixed home-then-away order at eligible windows, or the mandatory/ordinary
category in section 8. `select` supplies a deterministic opening/restart/recovery
or ordinary command for the classified category; instruction commands come from
the manager/default adapter. Pure close/reset commands have no payload. Every
command validates MatchId, expected period, next ordinal and observed count,
including commands whose operation does not emit an event. Reject stale copies.

`apply` reclassifies and accepts exactly one compatible operation; it does not
run until the next decision point. It dispatches to the existing public engine,
the new opening operation on a privately transitioned active candidate (sections
4 and 5.4), `rmc_match_period_commit`, or reset through
`rmc_match_sequence_apply(..., RMC_SEQUENCE_RESET_ACTION_WINDOW)`. An ordinary
command must fit the eligible category; canonical selector fidelity is checked
separately in qualification. Update selector counters only on acceptance.
After first-half closure, clear the two submission flags exactly once as part
of that accepted command, including when Shot performs closure. Do not clear
them repeatedly on `next` calls. Second opening requires both resolved flags.

No hidden multi-operation auto-run is part of this interface. A thin run loop
repeats classify/select/apply, returning at an instruction request or completion.
Completion is true only for FINISHED/COMPLETED; status and next distinguish a
managerial pause from completion. Once complete, `next` returns COMPLETE and
`apply` rejects. An error is returned to the caller rather than changing the
sporting terminal reason or storing a sticky failure in the authoritative state.

Errors are consistent: malformed arguments/enum/storage metadata ->
INVALID_ARGUMENT; unsupported versions -> UNSUPPORTED_VERSION; incoherent flow/
history -> INVALID_STATE; wrong eligible category -> WRONG_OPERATION; ordinal/
observed-count mismatch -> STALE_COMMAND; known capacity/headroom failure ->
RESOURCE_EXHAUSTED; otherwise a qualified boolean engine refusal ->
OPERATION_REJECTED. Existing boolean APIs cannot distinguish allocation failure
from every other refusal; do not invent a more precise underlying cause. Return
classification error before attempting mutation. All non-OK returns preserve
the complete flow, both backing arrays and output object; return codes carry
diagnostics without modifying those objects.

## 8. Time, progress and termination

Use checked `rmc_stoppage_deadline` with start 0 in half one, and
`second_half_start_second` in half two. Flow v1 starts with both stoppage totals
zero and adds none: openings, Shot restarts and real-world managerial pauses
consume no game time and do not register stoppage. The existing cap/accumulator
semantics must nevertheless be preserved and checked. Half two starts at the
actual first-half end; there is no wall-clock half-time offset.

Apply this mode-aware precedence, never a blind call to sequence policy in an
ineligible mode:

1. At NOT_STARTED or HALF_TIME resolve the bounded managerial window, then
   require one coordinator opening transaction: privately stage START or
   SECOND_HALF, call the Opening Kickoff Engine on active RESTART_PENDING, then
   publish only after complete success. It creates active play; it is not a
   Shot restart. FINISHED classifies COMPLETE.
2. In an active BALL_IN_PLAY half at the exact deadline, require logged period
   closure before reset, recovery or another action. At or beyond the deadline
   in RESTART_PENDING, or beyond it in any mode, reject incoherent flow.
3. In active predeadline BALL_IN_PLAY, apply a required 32-action reset before
   any further sporting action, including loose-ball recovery. Do not call the
   reset in RESTART_PENDING: its sequence-policy precondition is not met.
4. In active predeadline RESTART_PENDING with the valid current Shot cause,
   require Restart v1. New possession counters are already zero afterward.
5. In active predeadline BALL_IN_PLAY with a recoverable loose ball and an
   available action window, require recovery.
6. Otherwise select one eligible positive-duration controlled sporting action.

No unrelated sporting or tactical event may intervene between a cause and its
required recovery/restart. An event-free technical reset may precede recovery;
deadline closure supersedes recovery and consumes no recovery draw. Do not
schedule a managerial window there. No event is appended for technical reset.

A normal non-Shot action, including recovery, may finish **exactly** at the
deadline. Its accepted event/state stands; the next accepted operation is
`rmc_match_period_commit`, even if the last result is a loose ball or the action
count is 32. That is a second transaction. Shot can include the same closure
atomically: GOAL/WIDE at the deadline has SUPPRESSED_PERIOD_END, never a pending
restart. Do not emit duplicate PERIOD_END or MATCH_END afterward.

Every timed candidate has integer duration in `[1,R]`. The selector clips its
nominal duration before constructing the command; engines retain their existing
reject-on-overshoot semantics. No empty time-tick operation, artificial whistle
delay or direct CLI clock advance is needed. Positive time means even contained
actions and repeated failures exhaust the finite remaining seconds.

Each accepted zero-time category has a decreasing finite obligation: an opening
consumes its half's boundary, a reset clears a nonzero capped count, a restart
consumes the current cause/mode, closure exits its half, and an instruction
consumes one team/window submission. None can enable itself again at unchanged
state/time. A run loop must stop on the first non-OK selection/application,
report its intact prefix and return control; no internal unbounded rejection
loop or RNG reseeding to obtain a preferred result is permitted. User correction
of input or a host-chosen later retry after a resource issue is outside the
automatic simulation loop and does not roll back earlier accepted operations.

## 9. Events and atomicity

One accepted journal command corresponds to one public operation transaction.
It records the command's typed inputs, not merely an event label; events omit
some ordinary duel inputs and cannot substitute for the journal. Journal reset
and instruction commands too, even though they append no events. In this minimal
flow, instructions are audit/configuration changes recorded in the journal;
no new TACTICAL_DECISION event convention is invented at inactive boundaries.
Live sporting commentary derives solely from committed match events.

Before dispatch, validate category, command and complete flow invariants;
reserve one journal slot/ordinal and maximum event slots/IDs for the operation.
Prepare the normalized journal record, result metadata, counter increments,
instruction changes and window updates privately. Never use spare caller log/
journal slots as speculative scratch. The production engine retains its own
atomic validation/sampling/append boundary. After it returns true, append the
already reserved journal record and assign prepared coordinator metadata and
result with only infallible copies. Do not validate, allocate, format text, write
files or call a fallible callback after sporting success but before journal
commit. Calculate final status from validated outcome fields with total,
infallible assignments. A failed boolean operation must leave coordinator
metadata and journal untouched as well as preserving its own sporting outputs.

| Operation | Event reservation before invocation | Existing/required commit |
|---|---:|---|
| Instruction / technical reset | 0 | Validated instruction+submission flag, or qualified counter reset, plus journal record. |
| Opening | 2 | Coordinator privately stages lifecycle; engine commits PERIOD_START + RESTART as one batch and fresh possession on the active candidate; coordinator publishes only on complete success. |
| PASS / CARRY / DRIBBLE / TACKLE / CROSS / RECOVERY | 1 | Existing one-event atomic engine. |
| Shot before deadline | 3 | Reserve optional GOAL even if outcome will not score. |
| Shot ending half one / half two | 4 / 5 | Existing maximum branch, including optional GOAL and endings. |
| Shot-caused Restart | 1 | Existing RESTART and fresh possession. |
| Standalone closure, half one / half two | 1 / 2 | Existing PERIOD_END, plus MATCH_END referencing it for half two. |

Do not reserve only a predicted Shot branch by sampling or reading future RNG.
Do not combine a successful ordinary action and its later whistle/restart into
a fictitious rollback transaction. If the next operation exhausts resources,
the previously accepted action, events and journal record remain accepted.
Whole-match preallocation is the way to avoid predictable mid-match exhaustion,
not a cross-operation rollback promise.

Rejection snapshots cover state, MATCH state/increment, log metadata, entire
event backing storage (including unused bytes), journal metadata and entire
command backing storage, instructions, counters/flags and result. Successful
operations preserve historical events/commands; only new reserved slots change.
Compare semantic fields for replay; byte preservation is a rejection property.

## 10. Bounded resources

Require positive event/journal capacities, accessible backing arrays, count <=
capacity and capacities <= `SIZE_MAX / sizeof(element)`. Before any operation
check `required <= capacity-count`, count/required conversion to uint64, and
`required <= UINT64_MAX-count`; then form IDs. Apply equivalent checks to
journal ordinals, all counter increments and temporary prefix-buffer sizes.
Reject unsafe metadata before indexing the prefix or subtracting cause IDs.
Preserve score headroom checks before Shot and elapsed/possession/deadline
headroom checks in the existing engines. No wraparound is a fallback.

For a deterministic completion budget let
`T = 2 * (RMC_PERIOD_REGULATION_SECONDS + RMC_STOPPAGE_MAX_SECONDS) = 7200`.
Since each sporting operation consumes at least one second, `N <= T`. The
zero-stoppage v1 path has the tighter `N <= 5400`. The reset count is at most
`floor(N/32)`: every reset needs 32 intervening charged sporting actions;
turnovers/new possessions can only reduce its frequency. Shot-caused restarts
are at most N. Openings <= 2, standalone closures <= 2 and instruction
submissions <= 4. Hence accepted commands are bounded by
`N + floor(N/32) + N + 8`, conservatively by `3*T + 16`.

Each timed operation emits at most three sporting events; Shot's ending events
belong to the two half closures, not an independent unlimited source. Restart
events <= N, opening events <= 4, and half/final ending events total 3.
Thus events <= `4*T + 7`; the convenient conservative allocation `5*T + 8`
also covers maximum-branch reservation slack before sampling. A separate maximum
branch reservation remains mandatory even with a total budget.

**PROPOSED allocation defaults, CANDIDATE VALUES REQUIRING NORMATIVE FREEZE:**
21,616 command slots (`3*T+16`) and 36,008 event slots (`5*T+8`); bound total
accepted operations by 21,616. These are memory/budget choices, not sporting
limits, and may be tightened with a reviewed proof. They must fit target
`size_t`, element-size multiplication and event/command ID domains. Allocate
large buffers with checked host allocation before play, not unbounded stack
arrays; allocation failure must produce a setup diagnostic and intact state.
The API may accept smaller positive buffers for resource tests or constrained
callers; it cannot then promise full-match completion without exhaustion.

Production Shot/Recovery/Restart currently allocate temporary prefix validation
storage. Flow does not eliminate that dependency or guarantee allocation cannot
fail after earlier commands succeeded. Bound scratch by checked current event
count; rejection remains atomic. No buffer expansion, event deletion, journal
truncation or overwrite is permitted to conceal exhaustion. Status must
distinguish incomplete/resource-stopped execution from a valid final result.

The operation/event bounds are not a false fixed raw-RNG-output bound. Ordinary
duels/recovery request one bounded draw; Shot requests 2–4; PCG32 bounded draws
use rejection sampling, so raw-output consumption varies. Preserve the qualified
generator rather than adding a sampling retry cap that changes its semantics.

## 11. Determinism and replay

Initialize exactly one live MATCH stream for this fixture; use it for both teams
and every sporting operation throughout both halves. Never reinitialize it at
half-time, restart, recovery, turnover or technical reset. Selector, managerial
input, openings, resets, restarts and whistles consume no draws. Engine-local
candidate copies used for atomicity are not independent authoritative streams.
Do not introduce AI-domain randomness for this deterministic selector.

The replay input bundle contains the identical immutable world snapshot
(identities, attributes, energy, morale, fitness, talents and relevant roster
data), fixture, fixed lineups, all rules/model/schema/sequence/stoppage/flow/
selector versions, flow configuration and initial MATCH state+increment from
the unchanged canonical derivation, and the full accepted typed-command journal.
Initialization is the common starting point, not a manually injected live
possession. Managerial commands, explicit actors, context, chosen durations,
opening expectations and Restart source IDs are journaled. Source zones are
reconstructed from authoritative state, not overridden by a command.

Replay applies each recorded command through the same coordinator/production
operations and requires acceptance in the same order. It does not resample
candidate selection. Separately qualify selector determinism by regenerating
candidate commands from the same observations and comparing semantic inputs.
Rejected user input/commands and commentary wording are not replay inputs;
rejection must leave future accepted behavior unchanged.

Compare every explicit event field, including IDs, MatchId, timestamp, actors,
team, outcome, cause and full Shot payload; compare the semantic HALF_TIME state
immediately after first closure and before instructions/opening, and FINISHED
state with scores, terminal reason, clocks, stoppage, lineups and possession.
Compare MATCH RNG state and increment at half-time and completion, journal
semantic fields and coordinator counters/instructions/windows. Ignore C padding,
inactive union members, pointer values and host enum representation. Do not
derive equality from identical commentary or final score alone.

This establishes **semantic replay equality only**. Canonical result hashing,
portable replay-file encoding/validation and persistence qualification are
DEFERRED. Parent M001 requires canonical equality, at least 1,000 match replays
and the wider determinism campaign, including 100 repeated universes, 100
repeated seasons and 100 continuous-versus-save/resume comparisons. Scoped
B1.28-C full-match equality is not sufficient evidence to declare global
`MATCH_REPLAY=PASS`, the parent determinism gate or full M001 closed. Record
actual campaign scope and remaining obligations explicitly.

## 12. Managerial CLI

**PROPOSED B1.28-D:** a thin C11 executable supports the first actual generated
fixture, not a synthetic demonstration which installs a possession. It must:

1. Accept a checked uint64 deterministic seed (decimal or a documented numeric
   form); reject overflow/trailing junk. Print seed and versions. Text-seed
   hashing or wall-clock-derived default randomness is not introduced here.
2. Generate and validate `RmcCareerSetup` through production setup operations;
   display the three existing candidate teams and the selector's fallback flag.
   Let the manager choose a candidate; show roster and scheduled fixtures for
   that team. Fixture selection resolves an actual calendar MatchId, preserving
   its home/away roles, not a newly fabricated pairing.
3. Offer legal 442/433/352 pre-match lineups through `rmc_roster_lineup` or the
   existing qualified strength selection interface. Display starters and POR;
   validate the complete chosen lineup with match initialization. Freeze both
   lineups/world for the match. The unselected side receives a deterministic
   legal lineup under the same validation.
4. Explicitly choose and display the first opening team from the two fixture
   teams; noninteractive input must specify it or use a printed deterministic
   default such as home. Convert the selector's eligible taker into the explicit
   opening command. Never infer opening team from a coin-toss draw.
5. Present the bounded instruction triple/KEEP pre-match; show current choices
   and accept a complete submission. Run classify/select/apply until half-time,
   printing commentary only from newly committed event ranges. At half-time
   show event-derived score and offer one new instruction submission/KEEP, then
   continue with the opposing opening team to the final whistle.
6. Show final score and COMPLETED reason with event-derived shot/goal/recovery
   summary. Do not invent assists, possession percentages, lateral play or
   missing statistics whose source data are not sufficient. Setup/progress/error
   messages are UI diagnostics, not fabricated sporting commentary.
7. Provide noninteractive mode with explicit seed, candidate/team, scheduled
   fixture, legal formations, first opener and pre-match/half-time instructions
   or documented defaults. It must require no terminal input, complete the same
   match through the same coordinator, and support in-process journal replay
   verification without claiming a portable replay-file format.

Exact executable name/flags and transcript formatting are PROPOSED for D freeze.
The CLI offers no canonical pass/shot/defender-by-defender prompt. It cannot
call raw state mutation, bare lifecycle transitions, clock advance or the
action engines around the coordinator. Automatic matches use the same path.

**PROPOSED input behavior:** blank instruction input means KEEP; malformed enum,
unknown ID, illegal formation/fixture or oversized input reports a diagnostic
and reprompts without submitting a command, changing state or consuming RNG.
Use a finite line buffer and drain an overlong line before reprompting. EOF at
an interactive setup/decision prompt exits cleanly as user cancellation, leaving
any accepted match prefix intact and reporting no official completed result;
it does not spin, auto-score or silently start a different mode. Noninteractive
invalid/missing required arguments fail before play with nonzero exit status;
runtime engine/resource errors return nonzero with an intact-prefix diagnostic.
Normal completion and documented interactive cancellation have distinct status
messages; exact numeric exit codes/buffer size require D freeze. I/O failure
cannot roll back accepted sporting operations.

Waiting for human input can last indefinitely in wall-clock time. It is an
intentional paused UI state, consumes no simulation operations/time/RNG and
must not be classified as a simulation deadlock. Automated no-deadlock tests
drive all required submissions or test EOF explicitly; they do not wait for
unspecified human responses. There is no match save/resume promise in this CLI
increment and no full-season playability claim.

## 13. Qualification campaign

No test is executed or declared PASS by B1.28-A. The baseline's 32/32 PASS is
supplied context. Existing test sources establish which narrow facilities can
be exercised today, not fresh evidence from this document review.

| Acceptance area | Achievable with current production operations | Required future increment/evidence |
|---|---|---|
| Lifecycle, clock, stoppage, 32-action reset, logged exact closure | Isolated operations in `test_match_transitions.c`, `test_match_clock.c`, `test_match_stoppage.c`, `test_match_sequence*.c`, `test_match_period_commit.c`. | C proves these in uninterrupted public full-match flow, including deadline precedence over reset/recovery. |
| Shot outcomes/recovery/Restart | Existing `test_match_shot_action.c`, `test_match_recovery.c`, `test_match_restart_action.c` cover production local chains and rejection. | C reaches GOAL -> KICKOFF -> continued timed play and WIDE -> GOAL_KICK -> continued timed play after a production opening, in complete fixtures. |
| First and second opening | Schema inspection can assess proposed envelopes; no production opening API exists today. | B tests both openings, alternating teams, pair atomicity/history/single-use and schema acceptance; C integrates both in a complete match. |
| Full first and second half, half-time/final whistle | Not reachable as the required complete playable opening/action path today. | C uses init -> first opening -> timed actions -> logged half-time -> second opening -> timed actions -> FINISHED/COMPLETED, including Shot and non-Shot ending branches. |
| One MATCH stream | Existing generator/stream vectors and local action tests. | C captures initial, half-time and final state+increment; both teams share the one stream; no selector/opening/restart/reset/closure draws. |
| Bounds and no deadlocks | Individual capacity/arithmetic tests exist for relevant operations. | C asserts derived operation/event bounds, positive durations, finite zero-time obligations, no automatic rejected-command retry and no deadline overshoot for every policy/instruction combination. |
| Generated fixtures, roles and replay | Setup/calendar/lineup APIs exist; full journal/coordinator does not. | C completes at least one actual generated scheduled fixture, repeats it through journal replay, and covers manager/opener/restart awards in both home and away roles. |
| Atomic rejection/resources | Existing per-engine tests can exercise their preservation guarantees. | B covers the opening's entire storage/result; C covers combined journal+flow+RNG+log preservation and resource refusal before sporting success. |
| Interactive/noninteractive CLI | No CLI implemented by this task. | D scripted stdin and terminal-independent automated runs prove the behavior in section 12. |

B opening acceptance must include empty first log, exact event fields/order/
causes, half-two shared boundary timestamp, proper first-half termination
history (including a Shot-produced PERIOD_END), valid historical Shot chains,
correct fresh possession and unchanged RNG/clock/score/stoppage. Exercise each
fixture side as first opener, genuine outfield takers, wrong expected half/team,
invalid/bench/POR taker, malformed world/lineup/prefix, duplicate openings,
missing ending, second opening before half-time, stale timestamps and any
fabricated source SHOT. Test exactly two free event slots, one/full/invalid
storage, exhausted count/ID headroom and allocation failure if used. Snapshot
result, state, log metadata and sentinel-filled **entire** backing buffer.

C full-match tests must start from generated setup and legal lineups, then use
only public production init/opening/action/recovery/Restart/reset/period-commit
operations through the coordinator. Do not inject a controlled possession,
clock, score, mode or artificial historical event to make the match progress.
Choose documented deterministic seeds/configurations or bounded fixture searches
outside execution to obtain branch coverage; once selected, run/replay the whole
fixture from its canonical MATCH initialization. Never reseed during a match,
patch the world to force a goal or bypass production probability sampling.
Targeted malformed-state/resource unit tests may construct invalid inputs;
they do not count as full-match acceptance.

Require complete paths that cover a loose ball followed immediately by recovery
(allowing only an event-free reset), a naturally reached 32-action window under
a declared noncanonical legal-action test policy if necessary, and normal
non-Shot completion exactly at each deadline followed by standalone logged
closure. Require Shot exact-deadline closure with suppression and no duplicate
whistle/restart/recovery. A bounded test-policy schedule may choose legal passes
to keep a possession and durations to reach the boundary, but must journal the
real production commands and outcomes, not inject successful results. Test
default canonical selector separately for both instruction windows, all bounded
instruction combinations, goalkeeper distribution, defended actions and repeat
selection purity. Cross/lateral coverage remains an isolated existing-engine
test because normal centre flow cannot reach its source zone.

Resource tests must exhaust journal capacity **before** a scoring/clock-changing
command, event capacity for every maximum branch (especially Shot 3/4/5), event
and command IDs/counters, invalid size multiplications and prefix allocation.
Verify a failure after earlier accepted commands preserves that accepted prefix
without claiming rollback. Include pending cause at count 32, loose ball at the
deadline, duplicate/stale/wrong commands, full journal with an event-free reset,
and repeated read/classify/select/status calls that cannot advance anything.

C replay compares all section 11 fields and half-time snapshots, not just final
score, for canonical selector runs and declared engineering policies. Record
actual seeds, fixtures, instructions, command/event counts and evidence; there
is no invented future PASS count. Carry forward all existing regressions.

D acceptance includes one noninteractive complete generated fixture and replay
check, equivalent scripted interactive submissions yielding the same semantic
match, half-time change/KEEP, invalid and overlong input, seed overflow, wrong
fixture/team/formation, EOF at setup and at half-time, output/resource errors and
no direct-action human control. A delayed human response is tested as an idle
input boundary, not against the simulation's accepted-operation budget.

## 14. Implementation milestones

Paths below are likely minimal implementation surfaces, not permission for this
document-only task to write them. Preserve existing engines and other games.

| Increment | Required inputs and dependencies | Likely minimal files | Acceptance and completion gate |
|---|---|---|---|
| **B1.28-A** | Supplied seven decisions; concept/engineering/Shot/Restart contracts; actual source at `4d06e0b`. | Only this contract. | Static decision/schema/state/API/resource review, whitespace and single-file scope verification; no builds or CTest. Gate: precise B implementation target with no unresolved opening-schema conflict. |
| **B1.28-B** | Section 5 envelopes/API/history/atomicity; valid world/fixture/lineups and event prefix. Depends only on qualified state/transitions/logger and current sporting history validation. | New `src/rpg_manager_calcio/match_opening_kickoff.h/.c`, `test_match_opening_kickoff.c`; minimal CMake target/test registration in that directory. Local prefix checks may reuse existing validation patterns; no sporting-engine redesign. | Both explicit openings and all B negatives/capacity tests in section 13; complete regression campaign preserving the supplied baseline; record actual results. Gate: separate atomic single-use opening production API, schema 2 unchanged, Restart v1 unchanged. |
| **B1.28-C** | B opening API; frozen selector/version/config and numerical choices; immutable generated setup, lineups, canonical MATCH stream and storage budgets. | New `match_flow.h/.c`, `match_flow_selector.h/.c`, `test_match_flow.c`, `test_match_flow_replay.c`; minimal CMake wiring. Journal can reside in flow files rather than a new subsystem. | Full-match/branch/role/progress/resource/replay campaign in section 13, through production APIs; no injected possessions/clocks/scores. Gate: bounded autonomous complete fixture and semantic full-match replay, documented scoped evidence; global MATCH_REPLAY remains unclosed until its complete campaign. |
| **B1.28-D** | C production coordinator/selector/journal and qualified generated setup/lineup operations; frozen CLI inputs/EOF/defaults. | New `match_cli.c` (or one narrowly scoped C11 CLI source), minimal CMake executable/test wiring, focused CLI transcript/integration tests. | Interactive managerial path and reproducible no-input path in section 13; event-derived live commentary, half-time and final summary, EOF/error checks. Gate: actual playable generated fixture through the same coordinator; no complete-season/persistence/AI claim. |

B does not depend on the candidate action durations, selector cycle or CLI
syntax. C must freeze them explicitly before implementation qualification. D
does not gain authority to mutate world/lineups in-match or implement deferred
features as incidental CLI work. Any later discovery that the complete schema
chain is rejected must be isolated with an exact failing case; it is a blocking
dependency to resolve explicitly, not permission to synthesize a Shot or alter
frozen Restart v1.

## 15. Explicit exclusions

**DEFERRED:** new sporting actions, lateral movement, corners, throw-ins, fouls,
free kicks, penalties, offside, cards, substitutions, active formation/width
changes, exceptional goalkeepers, fatigue updates, personality AI, RPG rewards,
season standings/result commit, persistent replay files, canonical replay hashes,
save/resume and wider season CLI. These remain parent M001 work where required;
their absence limits the first playable match qualification, not the parent
scope. No replacement probability model, Shot calibration, scoring shortcut,
new RNG derivation, global version bump, event schema migration, hidden
controller, duplicated authoritative state or generic orchestration framework
is authorized. Earlier unrelated documentation drift is outside this change.

This increment creates only this Markdown file. It performs no builds, CTest,
implementation edits, commits, pushes or new worktrees. Dado Goal, Cloudflare,
browser-test and `.wrangler` changes are unrelated and must be preserved.

## 16. Open numerical/normative decisions

| PROPOSED choice requiring later explicit freeze | Candidate in this contract | Blocks |
|---|---|---|
| Selector version and ordinary priorities | Deterministic smallest-eligible-ID participants and ordered rows in section 6; no RNG selection | C qualification |
| Action timing | PASS 8 s, CARRY 10 s, DRIBBLE 7 s, TACKLE 5 s, CROSS 8 s, SHOT 6 s, RECOVERY 4 s; clip to positive remaining time | C qualification |
| Action cycle and pressure | Cycle length 5; ordinary challenge slot 2, extra HIGH slot 1; pass slot 0; opportunistic finishing/cross slot 4 | C qualification |
| Shot preference threshold | PATIENT sector 4; DIRECT sector 3 on finishing slot; sector 4 otherwise; preserve actual Shot eligibility 2..4 | C qualification |
| Minimal instruction protocol | PATIENT/CARRY/NORMAL defaults; one submission/KEEP per team at pre-match and half-time, home-then-away; no TACTICAL_DECISION event at inactive boundaries | C qualification |
| Flow/selector version constants | Initial proposed version 1 for each; explicit supported-version validation | C qualification |
| Allocation and guard defaults | T=7200 conservative proof domain; 21,616 commands/operation guard, 36,008 events with checked allocation | C qualification |
| CLI surface | Exact executable/flags, explicit or printed home-opener default, input-line capacity, numeric exit codes and deterministic noninteractive defaults | D qualification |

The first two time constants (2700 regulation and 900 maximum stoppage),
32-action window, context 0, centre sector 2 opening, BUILDUP, zero-cost opening
and alternating opening teams are EXISTING/APPROVED, not candidate calibrations.
The duration/cycle/resource numbers above do not change frozen probability
coefficients or promise statistically plausible match rates.

The opening API names and the exact neutral PERIOD_START/RESTART envelope and
causal conventions in section 5 are explicit PROPOSED implementation targets
under the approved separate-opening decision. There is no unresolved technical
conflict preventing B from implementing and testing that target. B must still
implement the missing opening-level history/eligibility validator; the existing
generic logger is intentionally insufficient to supply those rules by itself.
The single-operation journal commit/preflight protocol and default instruction
subset likewise require the specified C implementation and tests, not redesign
of existing engines. No evidence here closes global MATCH_REPLAY or a future
qualification campaign.

**B1.28-B readiness verdict: READY.** This is readiness to implement the
specified separate atomic opening operation, not a validation PASS claim.
