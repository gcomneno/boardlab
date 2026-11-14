# M001-B1.26-A — Shot Engine v1 contract

Status: mathematical freeze and supporting infrastructure. Shot resolution is
not implemented by this increment. All arithmetic and event requirements below
are normative for its subsequent implementation. Coefficients are approved
candidate rules, not a statistically calibrated football model.

Freeze provenance: the exact mathematics supplied for M001-B1.26-A is recorded
below. The earlier design document was not supplied or found in this checkout;
equivalence of the explicit sporting-coordinate and envelope-actor conventions
below to that earlier approval remains unverified.

Global RulesVersion remains **1**, GeneratorVersion remains **1**, logistic table
version remains **1**. Event schema version is **2**; shot model version is **1**.
Existing stream identities, derivation constants and reference vectors remain
unchanged. The parent contracts are [M001](m001-engineering-contract.md) and
[Determinism v1](determinism-v1.md).

## Preconditions and numerical domain

Use ISO C11 integer arithmetic only. Attributes, energy and morale are integers
in [0,100]. Context is an explicit signed integer in [-20,20]. Duration is an
explicit positive integer number of seconds. Tactical coordinates are relative
to the shooting team's attacking direction. Legal source sectors are **2..4**;
legal lanes are **LEFT, CENTRE, RIGHT**. No positional interpolation is implied.

A shot requires a valid active FIRST_HALF or SECOND_HALF state, BALL_IN_PLAY,
a live possession, and its current controller as shooter. The shooter is an
on-field outfield player of the possession team. The defender is an on-field
outfield player of the opposing team. The goalkeeper is the opposing lineup's
player assigned to the **POR slot**, and has goalkeeper department. An arbitrary
roster goalkeeper, a substitute, or the shooting team's goalkeeper is invalid.
Shooter and defender cannot be goalkeepers or POR occupants.

The existing sequence policy must permit ALLOW_ACTION before accepting a shot.
Validate identities, roles, possession, zone, context, duration, world/state,
model support, clock/counter/score integer headroom and maximum possible event
capacity before drawing. Advance a candidate clock with existing clock semantics;
a duration crossing the authorized deadline is rejected, never truncated. A
shot may finish exactly at the deadline. No draw or caller-visible mutation is
permitted on rejection. The event logger validates records, not these complete
action-engine preconditions, and does not recalculate probabilities.

## Frozen mathematics

`floor` is mathematical floor, `clamp(x,l,u) = min(u,max(l,x))`. Positive
numerators can use C integer division. Do not move floors across sums, replace
the staged floors, or apply signed truncation where mathematical floor is needed.
Use signed intermediates for subtractions, then clamp before unsigned conversion.
The following expressions define the exact model:

```text
N = 10000

E(p,x) = floor((80*x + 10*p.energy + 10*p.morale)/100)

S = E(shooter, TIRO)
J = E(shooter, POSIZIONAMENTO)

D_base = floor((40*defender.CONTRASTO
              + 35*defender.MARCATURA
              + 25*defender.POSIZIONAMENTO)/100)
D = E(defender,D_base)

G_base = floor((65*goalkeeper.RIFLESSI
              + 35*goalkeeper.GK_POSIZIONAMENTO)/100)
G = E(goalkeeper,G_base)
H = E(goalkeeper,PRESA)
```

The base B is selected from the source tactical zone:

| Sector | LEFT | CENTRE | RIGHT |
|---|---:|---:|---:|
| 2 | 10 | 20 | 10 |
| 3 | 30 | 45 | 30 |
| 4 | 55 | 75 | 55 |

```text
Q = clamp(B + floor(J/5) - floor(D/10) + context, 0, 100)

T = clamp(2000 + 40*S + 30*Q, 2000, 9000)
K = 300
W = 9700 - T

b = clamp(500 + 25*D - 10*Q - 5*S, 200, 3000)

F = floor((S+Q)/2)
g0 = frozen_logistic(F-G)

g = active ? min(9999,g0+floor(g0/10)) : g0

h = clamp(2000 + 65*H - 20*Q - 10*S, 1000,8500)
```

`frozen_logistic` means `rmc_match_logistic_lookup`, canonical table v1,
with reference values `round_half_up(10000/(1+exp(-d/16)))` for d in [-100,100].
Use the existing integer table, never runtime floating point or a new table.
F-G already lies in that domain. Q is chance quality, not a goal probability.

COMPOSURE is active only when the shooter has **consolidated COMPOSURE**, the
source sector is **4**, and **Q >= 60**. Matured or candidate talents do not
qualify. The bonus is a relative ten percent of g0, floored, with cap 9999;
it is not an additive 1000-point bonus. No other talent modifies Shot v1.
The applied mask uses bit `RMC_TALENT_COMPOSURE` from the existing catalog.

## Conditional distributions and canonical draws

Every individual distribution sums to **10000**:

| Stage | Ordered alternatives and integer weights |
|---|---|
| Trajectory | ON_TARGET: T; WOODWORK: K; WIDE: W |
| Defensive opposition | BLOCK: b; NO_BLOCK: N-b |
| Unblocked ON_TARGET | GOAL: g; SAVE: N-g |
| SAVE | HELD: h; REBOUND: N-h |

For each stage draw with existing bounded `PCG32(10000)`, uniform in [0,9999].
Use half-open cumulative intervals and strict `<` comparisons. The mandatory
order is trajectory, defensive block, goal versus save if applicable, held
versus rebound if applicable. The defensive block draw occurs for **every**
latent trajectory, including WIDE and WOODWORK. A block overrides that trajectory
and ends resolution. Without a block, WIDE and WOODWORK are final; ON_TARGET
continues to the goalkeeper stages. No later stage is drawn after resolution.
Thus a successful shot requests two, three or four bounded draws according to
the branch. There is **no fixed count of raw PCG32 outputs**: bounded sampling
uses rejection sampling and may consume more raw outputs.

The six final outcomes are GOAL, SAVED_HELD, SAVED_REBOUND, BLOCKED, WIDE and
WOODWORK. No direct scoring path bypasses SHOT. Unconditional outcome weights
are products of the applicable conditional probabilities, never a competing
flat six-way distribution; their sum is exactly one before any presentation
rounding. Retain the originally drawn trajectory even when BLOCKED overrides it.

## Sporting effects before period closure

All outcomes use the candidate clock after duration. Only GOAL changes score,
by exactly one for the shooting team, with score headroom checked beforehand.
The following coordinates are in the current/new possession team's frame.
A new possession resets action_count and possession elapsed_seconds to zero;
a continuing possession increments action_count once and includes the duration
in its possession clock. Quality/goal/closure events do not count as actions.

| Final outcome | Possession, controller and zone | Mode and counter | Restart |
|---|---|---|---|
| GOAL | Clear active possession | RESTART_PENDING; cleared counters | KICKOFF to conceding team at (2,CENTRE) |
| SAVED_HELD | Fresh goalkeeper-team possession, goalkeeper initiator/controller at (0,CENTRE), TRANSITION phase | BALL_IN_PLAY; new counters zero | NONE |
| SAVED_REBOUND | Preserve shooting possession and initiator; loose ball at sector 4, RMC_LANE_CENTRE in the shooting team's attacking frame; clear controller | BALL_IN_PLAY; increment once, CHANCE phase | NONE |
| BLOCKED | Preserve shooting possession, initiator and source zone; clear controller | BALL_IN_PLAY; increment once, preserve source phase | NONE |
| WIDE | Clear active possession | RESTART_PENDING; cleared counters | GOAL_KICK to defending team at (0,CENTRE) |
| WOODWORK | Preserve shooting possession and initiator; loose ball at sector 4, RMC_LANE_CENTRE in the shooting team's attacking frame; clear controller | BALL_IN_PLAY; increment once, CHANCE phase | NONE |

No corner, throw-in, foul, penalty or immediate recovery is invented. Clearing a
possession uses the neutral lifecycle representation, not a fictitious controller.
The restart cause remains explicit in the final SHOT payload: kind, awarded team,
tactical zone and disposition. Do not hide it in an identity, action counter,
outcome number, phase or unused state field. No RESTART event is emitted until
an actual future restart is applied. NONE requires zero awarded team, zero zone
(sector 0, LEFT) and NONE disposition. Sporting destinations above describe Shot
v1 conventions; the earlier design text was not available in this checkout for
independent provenance comparison.

## Event schema v2 and causal transaction

All v1 event types retain their numeric values (PERIOD_START=0 through
MATCH_END=14). Every existing envelope field retains its name, type and order.
Append `RmcShotEventPayload shot`; its explicit `model_version` is the tag.
Zero denotes absence and requires every payload field to be neutral. Unrelated
families retain their existing outcome semantics and neutral shot payloads.
Nonzero unknown model versions are rejected. Zero-initialized legacy producers
remain source compatible; the struct size has changed, so raw v1 struct dumps
are not v2 records and require an explicit reader/conversion.

| Event type / outcome | Code | Causal requirement |
|---|---:|---|
| ACTION / SHOT_CHANCE | 500 | Optional existing action cause; neutral trajectory/restart |
| SHOT / GOAL | 501 | Immediately preceding ACTION / SHOT_CHANCE |
| SHOT / SAVED_HELD | 502 | Immediately preceding ACTION / SHOT_CHANCE |
| SHOT / SAVED_REBOUND | 503 | Immediately preceding ACTION / SHOT_CHANCE |
| SHOT / BLOCKED | 504 | Immediately preceding ACTION / SHOT_CHANCE |
| SHOT / WIDE | 505 | Immediately preceding ACTION / SHOT_CHANCE |
| SHOT / WOODWORK | 506 | Immediately preceding ACTION / SHOT_CHANCE |
| GOAL / GOAL | 501 | Immediately preceding SHOT / GOAL only |

Shot v1 envelopes identify the shooting team, primary actor shooter and secondary
actor goalkeeper. All chain members share match, completion timestamp and chance
fields (version, quality, source zone, context, duration, defender, goalkeeper,
applied mask). ACTION / SHOT_CHANCE records quality before the final SHOT in
log order, with trajectory NONE and neutral restart. Final SHOT records the
latent trajectory and explicit restart. GOAL is emitted only for a goal, copies
the complete final payload and references that SHOT. The adjacent causal chain
prevents double resolution of one chance. The logger permits causal references
into the existing prefix, including individually appended chain members; the
future Shot Engine must submit its complete transaction atomically.

Trajectory codes: NONE=0, ON_TARGET=1, WIDE=2, WOODWORK=3.
Restart kinds: NONE=0, GOAL_KICK=1, KICKOFF=2.
Restart dispositions: NONE=0, PENDING=1, SUPPRESSED_PERIOD_END=2.
Restart destinations and awarded team are present even when suppressed.
The applied talent mask cannot contain an unrelated bit or claim an
unconsolidated/ineligible COMPOSURE effect.

`rmc_match_event_log_append_batch` validates the entire nonempty batch before
writing: bounded storage/ID headroom, consecutive IDs from count+1, match IDs,
known event types, match teams, current on-field actors, nonfuture/nondecreasing
timestamps, and causal references to actual prefix or earlier batch members.
It additionally validates the tagged shot payload, roles, outcome/trajectory and
restart consistency and adjacent shot chain identity. No allocation is used.
Rejection preserves metadata and **all bytes of the backing storage**, including
unused slots. Success performs one overlap-safe copy and one count update;
there is no fallible operation after the first write. The prefix is trusted to
be API-produced and unmodified. Exclusive access and nonoverlap of storage with
state/world/log metadata are caller preconditions; batch input may overlap storage.
Single append uses the same validation with a one-event batch.

The future action transaction covers state, match RNG, event metadata, complete
storage and result. Work in candidates; validate/reserve the maximum branch
(including optional GOAL and ending events) before draws; append the full event
batch once, then assign candidates without further fallible calls. Capacity or
other failure must not leave an orphan quality event, partial goal or advanced RNG.

## Loose-ball recovery and the 32-action window

SAVED_REBOUND, BLOCKED and WOODWORK are explicit loose-ball causes. A future
recovery extension must accept the immediately preceding final SHOT with one
of these codes, matching match and suspended shooting possession, with coherent
ID/timestamp, and resolve it once. The loose-ball zone is sector 4,
RMC_LANE_CENTRE for SAVED_REBOUND and WOODWORK; BLOCKED preserves the original
shot source zone and phase. Loose-ball zones use the shooting team's attacking
frame; a recovery turnover inverts sector as `4-sector`, retaining lane, with a fresh
TRANSITION possession. Retained recovery continues possession. GOAL, WIDE and
SAVED_HELD are not loose-ball causes. SHOT_CHANCE is never a recovery cause.
This increment does **not** modify the qualified recovery implementation, which
currently recognizes only the existing pass/tackle/cross ACTION causes.

The shot itself consumes one action in a continuing possession; it may be the
32nd. Before a subsequent action/recovery, the existing RESET_ACTION_WINDOW
operation resets only action_count, preserving the loose ball, possession and
latest causal event. It emits no artificial restart, turnover or action event.
At the period deadline END_PERIOD always precedes the window reset, including
when action_count is 32. New goalkeeper possessions begin with counter zero.

## Period-boundary resolution

Resolve a shot that legally finishes exactly at the deadline, including a goal,
then close the period in the **same** candidate transaction. Do not allow another
recovery or restart at that deadline. Preserve sporting score and final outcome;
clear possession and enter PERIOD_ENDED through the existing lifecycle rules.
For GOAL or WIDE retain restart kind/team/zone but set disposition to
SUPPRESSED_PERIOD_END, never PENDING. Away from the deadline use PENDING for
these restarts. Other outcomes have neutral restart fields even at the boundary.

`rmc_match_period_plan` accepts the validated, timed, active BALL_IN_PLAY
candidate and the count of events that will precede its ending events. It
requires the exact existing deadline (2700 + first-half stoppage; or
second-half start + 2700 + second-half stoppage). It delegates END_PERIOD to
the existing sequence/lifecycle semantics and returns a next state plus:

- FIRST_HALF: PERIOD_END, then HALF_TIME state.
- SECOND_HALF: PERIOD_END, MATCH_END, then FINISHED/COMPLETED state.

MATCH_END references PERIOD_END; IDs follow the supplied preceding count and
both timestamps equal the boundary. Events carry neutral shot payloads. It is
a pure plan: no input mutation, log write, time advancement, RNG or arbitrary
restart. It rejects invalid states/modes, nondeadline clocks and ID headroom
failure without modifying any output. Call it while the timed candidate is
still BALL_IN_PLAY, before applying a provisional restart-pending mode; lifecycle
closure supersedes provisional sporting possession effects. Append events in
order: SHOT_CHANCE, SHOT, GOAL if applicable, PERIOD_END, MATCH_END if applicable.
The existing period-commit wrapper reuses the planner and atomic batch append.

## Replay, compatibility and qualification limits

Deterministic replay requires the same world snapshot (including energy, morale,
attributes and consolidated talents), match/lineups, initial state, decision
sequence (actors, context, duration and source zone), match stream state/identity,
RulesVersion, shot model, event schema, logistic table and sequence/lifecycle
versions. Preserve exact integer rounding, draw order and branch skipping.
Neither host clock, iteration scheduling, pointer addresses nor floating point
may affect results. Compare/serialize fields explicitly, never native struct
padding, host enum width or endianness. Byte equality in failure tests establishes
storage immutability, not a portable serialization or canonical replay hash.

Readers must distinguish schema from model version and reject unsupported
nonzero versions explicitly. A schema-v1 event can be imported with a neutral
shot payload; it cannot be reinterpreted as a resolved v1 shot. Future coefficient,
trajectory, talent, effect or draw-order changes require a new shot model and an
explicit compatibility/replay policy; do not silently redefine model 1 or alter
RulesVersion 1 stream vectors. A future global rules revision must be explicit.

The coefficients, zone bases, fixed woodwork weight and relative COMPOSURE boost
are frozen **candidate calibration**, not empirically qualified scoring rates.
This increment qualifies event infrastructure and closure only. It implements no
shot resolver, Shot RNG simulation, calibration suite, restart executor, new
recovery causes, save-file migration or complete match replay/hash.
