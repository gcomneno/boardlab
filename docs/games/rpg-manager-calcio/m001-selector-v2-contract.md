# M001 — Selector v2-B: deterministic pass-recipient rotation

## 1. Status, authority and baseline

This is the normative contract for Selector v2 and its functional qualification.
**MUST**, **MUST NOT** and **REQUIRED** express binding requirements. Selector v2
is implemented and functionally qualified in the authorized files; executed
evidence and limits are recorded in section 11. Requirements below remain binding
after qualification.

The current RPG baseline is `28ced4f`; `a694c47` is its previous equivalent
before the legitimate Git history rewrite. No ancestry of `a694c47` is required.
Implementation started at `f5d35f0dae4104ff6057a632b985c2e4497f3973`.
The user supplied completed read-only audit gates
`RPG_EQUIVALENCE_GATE=PASS`, `RPG_CURRENT_BASELINE_GATE=PASS`,
`RPG_WORKTREE_GATE=PASS` and `SELECTOR_V2_CONTRACT_GATE=PASS`.
Those gates and the earlier **37/37 CTest PASS** are historical supplied evidence,
separate from the actual execution recorded below. No branch/history operation
was needed for this implementation.

The [Match Flow v1 contract](m001-match-flow-v1-contract.md), especially sections
6–11, 16 and 17, remains frozen. This separate contract adds the explicitly
versioned recipient policy below; it MUST NOT silently redefine Selector v1 or
rewrite the historical implementation/qualification evidence in that contract.
Its older CLI milestone status is historical; current CLI behavior is traced
to the production files in section 10 here.

## 2. Version compatibility and lifetime

| Flow version | Selector version | Required admission and meaning |
|---:|---:|---|
| 1 | 1 | Admit; existing qualified Selector v1 policy, unchanged. |
| 1 | 2 | Admit; same Flow v1 composition with only the pass-recipient policy in section 4 replaced. |
| All other combinations | — | Reject with `RMC_FLOW_UNSUPPORTED_VERSION`; no fallback or normalization. |

This matrix applies to initialization and the coordinator's version admission
checks used by classify, select, status, apply and replay. Both
`rmc_match_flow_init` and the internal metadata check admit exactly `(1,1)` and
`(1,2)`. Zero is not a selector default in
the public configuration.

`RMC_MATCH_FLOW_VERSION` MUST remain 1. Preserve
`RMC_MATCH_SELECTOR_VERSION` with its historical value and meaning of Selector
v1; it MUST NOT become an alias for the latest CLI default. Implementation
MUST provide separately named explicit constants
`RMC_MATCH_SELECTOR_V1_VERSION` (1) and `RMC_MATCH_SELECTOR_V2_VERSION` (2).
The historical constant continues to identify v1, including for existing callers.

Choose `RmcMatchFlowConfig.selector_version` before initialization and keep it
immutable for the match and its replay. No midmatch switch, tactical version
switch, mixed-version journal or reinterpretation of an existing v1 run as v2
is permitted. Public observation fields do not authorize external mutation.

No new RNG stream, RNG derivation or global rules version is introduced.
Preserve generator/rules version 1, event schema 2, Shot model 1 and all
qualified sporting model/action versions. CLI default selection is a separate
adapter decision, gated by section 7; explicit v1 MUST reproduce legacy policy.

## 3. Exact policy boundary

For the same valid current observation, v2 MUST select exactly the v1 command
except that, when its kind is `RMC_FLOW_PASS`, `data.pass.recipient` follows
section 4. Command metadata and all other active PASS fields remain governed
by v1. Different recipients can cause different subsequent controllers,
attributes, outcomes and RNG trajectories through production play; identical
v1 and v2 full-match results are not required.

The following MUST remain unchanged:

- Mandatory classification/order: resolve the home-then-away instruction window
  and opening at inactive boundaries; COMPLETE at finished; exact active
  BALL_IN_PLAY deadline closure; predeadline required action-window reset;
  eligible Shot restart in RESTART_PENDING; recoverable loose-ball recovery;
  then controlled sporting selection. Mode checks and cause validation remain.
- Sporting count starts at zero, increments only for accepted positive-duration
  operations including recovery, and persists across possession and half-time.
  It is distinct from possession action_count. Cadence remains
  `q = sporting_count % 5`.
- Genuine POR goalkeeper control distributes by PASS before any ordinary tackle,
  shot or progression choice. Outfield play chooses tackle at q=2, also q=1
  when the **defending** team has HIGH pressure; then the existing Shot row;
  then PASS at q=0; then instructed carry/dribble or the existing PASS fallback.
- Pressure belongs to the defending team; approach/progression belong to the
  possessing team. PATIENT shoots at sector 4; DIRECT also at sector >=3 on
  q=4. Higher-priority tackle still wins. The current eligible controller is
  the shooter; no department-based shooter override or shooting quota exists.
- Context remains 0 for every selected duel. Nominal seconds remain PASS 8,
  CARRY 10, DRIBBLE 7, TACKLE 5, SHOT 6 and RECOVERY 4. Selected timed actions
  require positive remaining time and use `min(nominal, remaining)`; production
  engines retain reject-on-overshoot behavior.
- Authoritative player eligibility and every other participant choice, including
  defenders, recovery participants, opening and restart takers, remain unchanged.
  Genuine goalkeeper means POR slot AND goalkeeper department; outfielder means
  non-POR slot AND non-goalkeeper department, with validated ownership/identity.
- Opening Kickoff, Shot, Recovery, Restart, period closure, reset and tactical
  submission semantics remain unchanged. Half openings and Shot restarts are
  distinct operations. Existing event schema, causal adjacency, cause IDs,
  probability models and sporting probabilities remain unchanged.

Automatic CROSS remains disabled. Do not fabricate positions, lanes, phases,
possession transfers, successful passes or sporting events to broaden player
participation. Formation slots are eligibility data, not physical coordinates.

## 4. Normative recipient rotation

This algorithm applies only after the unchanged action policy selects PASS
for possession team `T` with current controller `C`, including goalkeeper
distribution. It does not select a new action kind.

1. Resolve T through the fixture teams and authoritative world team/roster
   ownership. Resolve each candidate's nonzero `RmcPlayerId`, player team_id,
   membership in T's current validated lineup, lineup slot and department.
   Only valid on-field outfield teammates are eligible; bench players,
   opponents, unknown IDs, POR occupants and goalkeeper-department players are
   excluded. Do not infer identity or order from an array index.
2. Order eligible PlayerIds by their numerical `.value` in ascending order.
   Exclude `C` from recipient candidates.
3. Search backward from `flow->journal.count` through the actual accepted
   command prefix `[0, count)`. Find the latest command whose kind is
   `RMC_FLOW_PASS` and whose `data.pass.team_id` is T. Read its recorded
   `data.pass.recipient` as the previous recipient `P`. Only the active PASS
   member may be read; events and other command kinds do not define this history.
4. If P exists, choose the smallest eligible recipient with `PlayerId > P`
   after excluding C. The comparison is strictly greater, not greater-or-equal.
5. If no such successor exists, wrap to the smallest eligible PlayerId
   excluding C. No arithmetic increment of P is needed or allowed to wrap.
6. If there is no previous accepted PASS for T, choose the smallest eligible
   PlayerId excluding C.
7. If there is no eligible recipient excluding C, selection MUST return
   `RMC_FLOW_INVALID_STATE`, preserving the destination and all inputs.

History is per team, not per possession, passer, department, half or successful
pass. It persists across possession changes, recoveries, restarts, technical
action-window resets, tactical submissions and half-time. A fresh initialized
match has an empty history. Opposing-team passes never advance T's history.

Every accepted PASS counts, including `RMC_PASS_OUTCOME_LOOSE_BALL`. An apply
return of `RMC_FLOW_OK` is acceptance even when the sporting pass failed.
Rejected commands and merely selected but unapplied commands do not count;
selection does not reserve a rotation step. Unused or uncommitted backing
storage at indices >= count MUST be ignored, even if it resembles a newer PASS.

P remains the numeric anchor even when P equals C, is currently excluded, or
is no longer an eligible outfielder. A legal, noncanonical previously accepted
pass to an on-field goalkeeper MUST use that recorded recipient ID as P.
Do not skip it to find an older outfielder pass, reset history, or reject valid
history solely because its recipient is not a current outfield candidate.
This does not authorize changing fixed lineups or accepting malformed history.

For illustration, with eligible outfield IDs `{10,20,30,40}`:

| Previous T recipient | Controller C | Required recipient |
|---|---:|---:|
| None | 10 | 20 |
| 10 | 30 | 20 |
| 20 | 30 | 40 |
| 40 | 10 | 20 |
| 20 | 20 | 30 |
| 25 (currently ineligible) | 10 | 30 |
| 50 (previous goalkeeper recipient) | 50 | 10 |

These numbers explain ordering only; they are not generated-match evidence.
An intervening opponent PASS or a rejected/unapplied T PASS leaves each answer
unchanged. The same rule applies to the priority goalkeeper distribution PASS,
including the pass after an accepted noncanonical pass to that goalkeeper.

Only a completed production pass transfers control to its recipient through
`rmc_match_pass_apply`. A loose-ball pass leaves controller zero and follows
the existing mandatory recovery/closure path. Rotation advances its accepted
history without asserting that the recipient ever controlled the ball.

## 5. Pure selection, rejection and resources

Use the existing public signature unchanged:

```c
RmcFlowError rmc_match_flow_select(
    const RmcMatchFlow *flow, const RmcMatchFlowNext *next,
    RmcMatchFlowCommand *destination);
```

No new selector parameter is necessary: `RmcMatchFlow` already exposes const
world/state/config and the bounded accepted `RmcMatchCommandJournal`.
Rotation MUST use that journal, not infer a previous recipient from possession,
initiator, controller, action_count, sporting_count or the event tail.

Selection MUST NOT allocate, consume or preview RNG, apply speculative actions,
write state/journal/events, or maintain a mutable cursor/cache outside the flow.
It may write only destination on success. Repeated selection from identical
observations MUST produce semantically identical commands without advancing
history. An ascending minimum/successor scan suffices; numerical ordering does
not require an allocated or mutable sorted roster.

Retain `rmc_match_flow_next` validation before selection, its metadata/count
bounds before indexing, the observed-command-count stale guard and exact
classification comparison. Preserve `INVALID_ARGUMENT`, `STALE_COMMAND`,
`WRONG_OPERATION`, `INVALID_STATE`, `UNSUPPORTED_VERSION` and resource semantics.
Instructions and COMPLETE remain nonselectable (`WRONG_OPERATION`). Missing
eligible recipients use `INVALID_STATE`, not an invented fallback command.

Search is bounded by the accepted journal count and existing roster/lineup
limits. Preserve capacities 21,616 commands, 36,008 events, default operation
limit 21,616, checked arithmetic and existing maximum-branch reservations.
No additional unbounded storage, journal truncation, hidden history state or
resource-limit increase is justified by rotation. Selector allocation is
forbidden; existing production engine scratch allocation remains unchanged.

Every rejected single operation/read/selection preserves caller-visible flow,
RNG, log/journal metadata, entire backing arrays including unused storage, and
output. Earlier accepted operations remain committed. No retry loop may seek a
preferred sporting result or silently repair a rejected state.

## 6. Recorded replay and prefix continuation

`rmc_match_flow_replay` MUST continue to call `rmc_match_flow_apply` on recorded
commands without invoking `rmc_match_flow_select`. Apply validates legal
commands and causality; it MUST NOT reject a legal recorded command merely
because either selector would currently choose a different recipient.
Canonical selector fidelity is tested separately from command legality.

The replay bundle MUST retain the original flow and selector versions, full
accepted command prefix (including event-free instructions/resets), exact
recorded participants and active payloads, identical immutable world, fixture,
fixed lineups and configuration. Initialize with the same canonical
`rmc_stream_init(..., RMC_STREAM_MATCH, fixture->id.value)` derivation from the
original world identity, including initial MATCH state and increment. Use one
MATCH stream across both teams and halves; no reseeding at replay boundaries.

Replay into independent initialized destination storage; source records MUST
NOT overlap destination storage. Each successful apply appends its normalized
accepted record to the destination journal. Replay may consume a valid prefix
or chunks. If a later record rejects, retain the already accepted prefix;
there is no whole-batch rollback promise.

After replay of a valid v2 prefix, a fresh classify/select call MUST recover
both teams' rotation anchors from that **destination** journal. It requires no
source flow, external cursor or replay-only parameter. Selection and subsequent
legal production continuation must match an independently continued live v2
run from the same prefix, including after loose-ball recovery, restart or
half-time obligations. Prefix continuation retains the original selector
version; it is not an opportunity to upgrade a v1 match.

REQUIRED EVIDENCE compares semantic fields of every accepted command (header
and active payload), every event (IDs, timestamp, team, actors, outcome, cause
and complete Shot payload), state/fixture/lineups/possession, scores, clocks,
stoppage and terminal reason. Compare coordinator configuration/versions,
instructions, window flags, sporting/reset counters, log/journal counts and
metadata, status and exact RNG state **and increment**. Use equivalent resource
configuration and compare capacities in the qualification harness. Include the
half-time snapshot immediately after closure, prefix boundaries and completion.

C padding, pointers and inactive union members do not define semantic equality.
Byte snapshots remain appropriate for atomic rejection. This contract makes no
portable serialization, replay-file validation or canonical replay-hash claim,
and does not close the broader M001 determinism or MATCH_REPLAY obligations.

## 7. CLI version selection and acceptance gate

The CLI MUST support `--selector-version 1|2` in interactive and
`--auto` operation, with no new interactive version prompt. Parse it under the
existing exact-option rules; missing, duplicate, malformed or unsupported
values are invalid options, with exit code 2. Pass the explicit selected value
to `RmcMatchFlowConfig.selector_version` and preserve it for `--verify-replay`.

Default to v2 **only after implementation and successful functional
qualification against all requirements in section 8 have been accepted**.
The executed qualification in section 11 gates this adapter decision. The qualified
release MUST default to v2 when the flag is omitted; explicit
`--selector-version 1` MUST retain legacy selection and complete-match behavior.
Do not implement the default by changing the historical v1 constant.

Report the actual selected version, including explicit v1, in version output.
Interactive tactical explanations MUST describe the chosen policy: v1 uses the
smallest eligible outfield recipient excluding the controller; v2 rotates in
ascending PlayerId order from that team's latest accepted PASS recipient,
wrapping and excluding the controller. Retain the common cadence, priority and
defending-pressure explanations. Do not promise a completed pass or participation
by every player in every match.

Preserve existing flags, choices, numbered 1/2 tactical menus, pre-match defaults,
half-time KEEP/CHANGE and help behavior. Preserve EOF handling and exit codes:
0 completion/help, 1 runtime/resources/I/O failure, 2 invalid options/selection,
3 interactive EOF. `--auto` requires no stdin reads. Event-derived priority
commentary (including half starts, goals, restarts and whistles) MUST remain
intact under the existing routine-commentary suppression behavior.

## 8. Required functional qualification matrix

Every row is REQUIRED EVIDENCE. Section 11 records actual execution against
this matrix, with reproducible witnesses and explicit limits. Existing test assertions that v2 is unsupported must be updated to the
new matrix while retaining genuine unsupported-version rejection coverage.

| ID | Requirement | Required evidence |
|---|---|---|
| A | Legacy preservation | Explicit v1 selection, all tactical combinations and full matches retain legacy semantic commands, events, half-time/final state and RNG. Keep v1-specific smallest-recipient assertions; do not weaken them to accommodate v2. |
| B | Version admission | Explicit `(1,1)` and `(1,2)` admission; unsupported pairs, including zero and unsupported flow versions, reject atomically with `UNSUPPORTED_VERSION` for otherwise valid inputs. Historical constant still means 1. |
| C | Exact v2 rotation | Assert exact recipient IDs for no previous T pass, ordered successor, wraparound, controller exclusion (including P=C), opponent-only/interleaved history, accepted loose-ball pass, priority goalkeeper distribution, previous non-outfielder/goalkeeper anchor, currently excluded anchor, and rejected/selected/uncommitted commands. No eligible recipient rejects as `INVALID_STATE`. Verify both team histories and persistence through possession changes, recovery, restart, reset, tactical submissions and half-time. |
| D | Purity | Repeated classify/select/status calls preserve flow, both entire backing arrays and RNG state/increment; repeated selected commands compare semantically. Output is preserved on errors; no selector allocation, lookahead or mutable cursor. |
| E | Production full matches | Generate/validate career world, actual calendar fixture and legal lineups; run canonical v2 from init through both openings and halves to genuine FINISHED/COMPLETED with bounded progress, real engines and event-derived scores. Cover both fixture roles and tactical effects. |
| F | Successful recipients | Demonstrate completed-pass recipients in MIDFIELDER and ATTACKER departments for **each** team across the declared campaign. Resolve identity/department authoritatively and record command/event IDs, completed outcome and resulting control. Selection or loose-ball receipt alone is insufficient. |
| G | Subsequent control | Demonstrate subsequent accepted controlled actions by those midfielders and attackers for both teams, as passer, carrier in CARRY, dribbler or shooter; identify controller and active action role in state/command evidence. Being targeted by a tackle or acting only as an opposing defender is insufficient. Do not infer actions from a recipient list. |
| H | Shooting participation | Demonstrate real accepted shots by midfielders and attackers, covering both teams where genuinely reachable, across a declared bounded canonical v2 qualification campaign. Record actual shooter department/IDs and supporting commands/events. Preserve shot eligibility/cadence and report any unmet coverage; no fabricated control or success to close this row. |
| I | Full semantic replay | Replay actual v1 and v2 full-match journals through production apply, without selector invocation; compare all section 6 semantics at half-time and final, including causes and RNG state/increment. Legal noncanonical recorded recipients remain replayable. |
| J | Prefix and continuation | Replay valid v2 prefixes with both teams' histories into independent destinations; compare exact next selected commands and legal continuation to live runs. Include accepted failed passes, legal goalkeeper-recipient history and boundary obligations; no external history injection. |
| K | Atomicity/resources | Preserve error classes, stale/category checks, maximum branch reservations, full-buffer snapshots and checked capacity/count/ID/operation limits. Rejected PASS cannot advance rotation. A bad later replay record retains its accepted prefix. No new selector heap use or resource expansion. |
| L | CLI versions and replay | Test explicit v1/v2, invalid version inputs, actual version reporting, qualified v2 default, `--auto`, and `--verify-replay` under both versions. Omitted version and explicit v2 produce the same qualified semantics. Explicit v1 matches its legacy reference. |
| M | Interactive compatibility | Existing numbered menus, KEEP/CHANGE, invalid/long input, help, setup/half-time EOF, exit codes and priority commentary remain intact; flag-driven and equivalent interactive tactics agree for each version. No version prompt or extra stdin consumption. |
| N | Regression baseline | All existing 37 CTest cases continue passing with maintained coverage, alongside the new v2 assertions in existing tests. Report actual suite results; do not fabricate results, events or sporting outcomes. |

Before running the participation campaign, declare a finite maximum number of
generated matches, deterministic seed/fixture enumeration, formations, first
opener and pre-match/half-time instruction schedule. Report attempted/completed
counts and per-team/department coverage with reproducible witnesses. A bounded
search for genuine reachable outcomes is allowed; no midmatch reseeding, RNG
preview, attribute edits, injected positions/clocks/possessions or forced results.
Unmet required evidence remains open and blocks qualification/default promotion;
do not silently expand an exhausted campaign or claim impossible guarantees.

Canonical full-match participation evidence MUST use v2 selection and production
application. Separately labeled noncanonical legal-command tests may exercise
rotation anchors/replay edge cases through the same production apply path;
they cannot substitute for F–H. Malformed-state unit tests establish rejection
only and do not count as production full matches.

The broader **10,000-match sporting balance campaign is separate** from this
functional qualification. Functional acceptance neither requires claiming that
campaign was run nor guarantees realistic scorelines, calibrated scoring,
universal player participation or a particular result in any individual match.

## 9. Authorized implementation scope

The Selector v2-B task authorizes only the ten files below:

| File | Responsibility |
|---|---|
| `src/rpg_manager_calcio/match_flow.h` | Explicit v1/v2 constants and version documentation; preserve historical constant and public data/API shape. |
| `src/rpg_manager_calcio/match_flow.c` | Admit exactly the compatibility matrix in init and metadata validation; preserve apply/replay legality and transactions. |
| `src/rpg_manager_calcio/match_flow_selector.h` | Document versioned pure selection with the unchanged signature. |
| `src/rpg_manager_calcio/match_flow_selector.c` | Dispatch v1/v2 recipient choice only; bounded accepted-prefix scan and authoritative eligibility. |
| `src/rpg_manager_calcio/test_match_flow.c` | Keep v1 references; add version, rotation, purity, participation, full-match and rejection/resource qualification. |
| `src/rpg_manager_calcio/test_match_flow_replay.c` | Version-preserving semantic full/prefix replay and continued v2 selection. |
| `src/rpg_manager_calcio/match_cli.c` | Version option/config/reporting, qualified default and version-appropriate tactical explanations. |
| `src/rpg_manager_calcio/test_match_cli.c` | Version-specific references, input/menu/commentary compatibility and replay assertions. |
| `src/rpg_manager_calcio/test_match_cli_integration.cmake` | Existing subprocess cases extended for versions/default/auto/replay and preserved EOF/exit behavior. |
| `docs/games/rpg-manager-calcio/m001-selector-v2-contract.md` | This normative contract and explicitly identified executed qualification evidence. |

No CMake modifications are expected: extend existing targets/tests. No changes
to sporting engines, event logger, world generator, roster, probabilities, RNG,
Shot, Recovery, Restart or Opening Kickoff are in scope. Do not edit the frozen
Match Flow v1 contract or unrelated Dado Goal work. Unexpected need for excluded
changes is a contract/scope issue to resolve explicitly, not incidental authority.

## 10. Traceability and contract verification

Paths below link to the inspected interfaces and implemented policy boundary.

| Source | Existing semantics carried forward / normative destination |
|---|---|
| [Match Flow v1 §§6, 8, 16](m001-match-flow-v1-contract.md) | Frozen participant roles, cadence, tactical ownership, durations and precedence; only v2 PASS recipient changes in §§3–4 here. |
| [Match Flow v1 §§7, 9–11, 17](m001-match-flow-v1-contract.md) | Coordinator authority, typed accepted journal, atomicity, bounded resources and semantic replay; §§5–6 and 8 here extend qualification without rewriting old evidence. |
| [match_flow.h](../../../src/rpg_manager_calcio/match_flow.h) | `RmcMatchFlowConfig`, `RmcMatchFlowCommand.data.pass`, `RmcMatchCommandJournal`, const read APIs, apply and replay contracts; §§2, 4–6 here. |
| [match_flow.c](../../../src/rpg_manager_calcio/match_flow.c) | Init/metadata admit exactly v1/v2; next classifies; apply normalizes and journals only after acceptance; replay calls apply record by record; §§2, 4–6 here. |
| [match_flow_selector.h](../../../src/rpg_manager_calcio/match_flow_selector.h), [match_flow_selector.c](../../../src/rpg_manager_calcio/match_flow_selector.c) | Pure signature, authoritative role scan, unchanged priority/cadence, smallest-recipient v1 and journal-based v2; §§3–5 here define the sole v2 delta. |
| [roster.h](../../../src/rpg_manager_calcio/roster.h), [match_pass_action.h](../../../src/rpg_manager_calcio/match_pass_action.h), [match_pass_action.c](../../../src/rpg_manager_calcio/match_pass_action.c) | PlayerId/team/slot/department eligibility and production PASS: COMPLETED transfers control, LOOSE_BALL clears it; accepted inputs can legally include a goalkeeper recipient; §4 here distinguishes policy from legality. |
| [match_event_log.h](../../../src/rpg_manager_calcio/match_event_log.h) | Schema 2, consecutive IDs, bounded append-only events and preceding-event causes. PASS emits ACTION with passer/recipient actors; command inputs, not events alone, determine rotation/replay; §§3–6 here. |
| [test_match_flow.c](../../../src/rpg_manager_calcio/test_match_flow.c), [test_match_flow_replay.c](../../../src/rpg_manager_calcio/test_match_flow_replay.c) | Existing v1 smallest-recipient assertions, tactical matrix, noncanonical boundary policies, atomic snapshots and full/chunked semantic replay; §8 specifies the qualification requirements. |
| [match_cli.c](../../../src/rpg_manager_calcio/match_cli.c), [match_cli.h](../../../src/rpg_manager_calcio/match_cli.h), [test_match_cli.c](../../../src/rpg_manager_calcio/test_match_cli.c), [test_match_cli_integration.cmake](../../../src/rpg_manager_calcio/test_match_cli_integration.cmake) | Versioned config/reporting, numbered tactical menus, event commentary, EOF/status rules and auto/replay paths; §7 defines version options, qualified default and tactical explanations. |

Static contract verification MUST read this document and check exact v1/v2
compatibility, API consistency, rotation cases, replay/prefix continuation,
default qualification gate and matrix A–N. Check Markdown whitespace and keep
all changes within section 9, preserving concurrent work. No changes to the
frozen Match Flow v1 contract, CMake registrations, sporting engines or RNG are
authorized. No commit, push, branch/worktree change, reset, clean or restore was
performed. Executed implementation evidence follows.

## 11. Executed Selector v2-B qualification (2026-11-15)

### 11.1 Build and regression

The requested directory `/tmp/rmc-b1-match-state-build` was absent in this
session. It was configured at that same path without changing CMake sources:

```sh
cmake -S src/rpg_manager_calcio -B /tmp/rmc-b1-match-state-build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build /tmp/rmc-b1-match-state-build --parallel 4
ctest --test-dir /tmp/rmc-b1-match-state-build --output-on-failure
```

Compiler: GNU C 13.3.0. Existing `-Wall -Wextra -Wpedantic -Werror` settings and
test `-UNDEBUG` remain in force. **Build: exit 0, successful. Full regression:
37/37 PASS, zero failures, exit 0, 125.54 seconds.** These are actual results from
the commands above, independent of the historical baseline result. Targeted flow,
replay and CLI runs also exited successfully. No new CTest registrations exist.

### 11.2 Exact v1 compatibility and version admission (A–B)

The historical `RMC_MATCH_SELECTOR_VERSION` remains 1. Named v1/v2 constants
identify 1 and 2; init and metadata admit exactly Flow 1 with Selector 1 or 2.
The version matrix tests include zero, Flow 2, Selector 3 and `UINT32_MAX`.
Unsupported operations preserve outputs and storage. Configuration is copied at
initialization and remains read-only for a match; no switching API was added.

All existing v1 flow assertions and the 64 instruction-pair full-match matrix
remain active. The one old unsupported-selector assertion now uses 3.
Additionally, before editing production, the original selector was compiled to
`/tmp/rmc-selector-v1.o`, renaming only its exported function to
`rmc_legacy_selector_select`. A temporary harness
`/tmp/rmc-selector-v1-equivalence.c` used the existing semantic comparison
helpers and independently initialized live flows to compare old and new v1
selection for all 8x8 instruction pairs, in both windows, with alternating
opening sides. **64/64 complete matches and 53,874 commands matched exactly**,
as did every active event/cause payload, half-time/final state, instructions,
windows, counters, capacities and exact RNG state/increment.

The pre-edit executable `/tmp/rmc-selector-v1-cli` was also compared with the
current CLI using explicit `--selector-version 1`: **3/3 complete stdout/stderr
transcripts matched exactly**, including replay verification. Cases were the
default seed/settings; seed 0 with 352/442, away opener and DIRECT/DRIBBLE_FIRST/
PRESS_HIGH changing to PATIENT/CARRY_FIRST/PRESS_NORMAL at half-time; and seed
`UINT64_MAX` with 433/352 and a DIRECT/DRIBBLE_FIRST/PRESS_HIGH half-time change.
Temporary object/harness files are local qualification artifacts, not a portable
replay format or additional repository files.

### 11.3 Rotation, purity and resources (C–D, K)

The v2 oracle independently traverses accepted commands forwards, builds a
sorted list from actual lineup slots and authoritative roster departments, and
compares exact recipients against production's backward search/minimum scan.
It runs throughout canonical and separately labelled noncanonical matches.
Coverage includes initial selection, strictly numerical successor, wrap,
controller exclusion, P=C, opponent-only/interleaved history, accepted failed
passes, goalkeeper distribution and accepted noncanonical goalkeeper anchors.
The anchor remains numeric even when excluded from current candidates.

The legal noncanonical anchor test completed with observed selection cases:
118 successors, 16 wraps, 16 P=C cases, 2 goalkeeper-anchor cases, 13 goalkeeper
controllers, 79 opponent-interleaved histories and 75 accepted failed PASSes.
These are overlapping observations, not sporting success promises. Interior
outfielder, goalkeeper and largest-outfielder anchors are recorded through
production apply; no accepted prefix is fabricated or edited.

Repeated selection preserves flow, complete event and command backing arrays,
RNG state/increment and outputs. An allocator-failure sentinel confirms selection
does not allocate. Plausible PASSes at the first unused command and final backing
slot are ignored. Merely selected and rejected PASSes leave selection unchanged.
Malformed lineup/no-recipient rejection preserves the destination; a valid fixed
eleven always has eligible outfield alternatives, so this rejection is expressly
a malformed-state test, not a generated-match claim.

The original atomic rejection and maximum-branch/resource tests run for both
versions, retaining full-buffer snapshots, stale/category errors, capacity/count/
ID/operation limits and engine allocation-failure checks. Prefix replay failure
retains only the accepted prefix under both versions. Resource constants and
sporting engines are unchanged.

### 11.4 Bounded canonical participation (E–H)

The campaign was declared before execution: at most 128 matches, enumerating
seeds 20261114 through 20261129, eight instruction pairs per seed, first calendar
fixture, home 442 / away 433, and home opening iff `(seed_offset+h)%2 == 0`.
For h=0..7, home uses triple h and away uses `(3*h)%8`; bit 0 means DIRECT,
bit 1 DRIBBLE_FIRST, bit 2 PRESS_HIGH (unset bits select their v1 alternatives).
The same instructions are submitted at pre-match and half-time. Stop only after
a complete eight-match seed block once all 12 receipt/control/shot cells have
evidence. No preview, reseeding, outcome retry, state injection or attribute edit
is involved.

Actual result: **8 attempted, 8 completed**, all at seed **20261114**, generated
fixture **1**, home team **4**, away team **6**. Both opening sides and all eight
triples for each team were exercised. Each reached real FINISHED/COMPLETED at
5400 seconds with event-derived scores. The bound was not expanded.

| Side / authoritative department | Successful receipts | Subsequent accepted controlled actions | Accepted shots |
|---|---:|---:|---:|
| Home / MIDFIELDER | 186 | 146 | 5 |
| Home / ATTACKER | 95 | 103 | 8 |
| Away / MIDFIELDER | 49 | 45 | 1 |
| Away / ATTACKER | 43 | 36 | 2 |

Controlled-action counts include only PASS passer, CARRY carrier, DRIBBLE
actor or SHOT shooter who previously completed a receipt in that match; an
opposing tackle role never counts. Multiple later actions may follow a receipt.
Department resolution uses roster identity/ownership, never PlayerId ranges.

Reproducible first receipt/control witnesses below all use h=0, a=0, home opener
(PATIENT/CARRY_FIRST/PRESS_NORMAL for both teams). Command/event IDs are accepted
journal/event ordinals. Each receipt event is COMPLETED and the resulting state
controller equals the recorded recipient.

| Team / department | Player | Receipt command / event | Subsequent command / event | Controlled role |
|---|---:|---:|---:|---|
| 4 / MIDFIELDER | 62 | 45 / 48 | 46 / 49 | CARRY carrier |
| 4 / ATTACKER | 69 | 65 / 68 | 66 / 69 | CARRY carrier |
| 6 / MIDFIELDER | 99 | 148 / 156 | 149 / 157 | CARRY carrier |
| 6 / ATTACKER | 105 | 589 / 629 | 590 / 630 | PASS passer |

Accepted shooting witnesses, from the same seed and fixture:

| Team / department | h / a / opener | Shooter | Prior receipt command / event | SHOT command / SHOT event |
|---|---|---:|---:|---:|
| 4 / ATTACKER | 0 / 0 / home | 70 | 189 / 199 | 194 / 205 |
| 6 / ATTACKER | 0 / 0 / home | 107 | 673 / 719 | 676 / 723 |
| 6 / MIDFIELDER | 1 / 3 / away | 99 | 555 / 621 | 559 / 626 |
| 4 / MIDFIELDER | 1 / 3 / away | 64 | 798 / 892 | 802 / 897 |

For h=1/a=3, home uses DIRECT/CARRY_FIRST/PRESS_NORMAL and away uses
DIRECT/DRIBBLE_FIRST/PRESS_NORMAL in both windows. Every shot uses the actual
eligible controller and production Shot engine; no department override or
participation quota exists.

### 11.5 Replay and continuation (I–J)

Full replay covers canonical complete v1 and v2 matches with both opening sides,
plus retained boundary engineering policies, through production recorded apply.
Every active command/event field and cause, state/fixture/lineups/possession,
scores/clocks/stoppage/terminal reason, versions/config, instructions, windows,
counters, capacities/counts/status and exact RNG state/increment compare
semantically at half-time and completion. Incremental replay also compares next
selection after every nonterminal accepted prefix, including boundary obligations.
Replay itself never invokes selection.

Two additional v2 prefix tests reconstruct both teams' histories in independent
destination storage, then select and legally apply independently through full
completion, comparing complete semantics after each step:

| Prefix case | Accepted prefix | Final commands | Final RNG state / increment |
|---|---:|---:|---|
| Accepted failed PASS, both histories | 9 | 738 | `24736c35fde2c343 / 0463f86035e918e3` |
| Legal noncanonical goalkeeper receipt, both histories and prior failed PASS | 14 | 739 | `7b064945c832237c / 0463f86035e918e3` |

The goalkeeper prefix deliberately supplies legal recorded recipients that a
canonical selector would not choose. Replay preserves them; subsequent canonical
v2 selection derives its anchor solely from the destination accepted journal.

### 11.6 CLI and scope (L–N)

`--selector-version 1|2` works in auto and interactive modes; omitted version is
v2 after the functional qualification above. Default and explicit v2 transcripts
are identical. Explicit v1 preserves legacy match semantics. Actual version
reporting, policy-specific tactical help, numbered menus, KEEP/CHANGE, all eight
triples for both versions, invalid/long input, setup/half-time EOF, exit codes,
priority commentary and semantic replay are covered by the existing CLI test
executables and subprocess registration. Missing/duplicate/malformed/unsupported
version values return 2, including `0`, `3`, `-1`, `+1`, `01`, `1x`, `1.0` and
overflow. Auto mode consumes no input.

Only the ten files in section 9 were edited for this task. Concurrent Dado Goal
changes were neither edited nor reverted. Sporting engines, RNG, probabilities,
Shot model, causal validators, opening/restart participant policies, world,
roster/generator/calendar, CMake and unrelated projects remain outside this change.

### 11.7 Remaining limits

This is bounded functional qualification, not the separate 10,000-match sporting
balance campaign. It establishes genuine reachable participation for the declared
fixtures, not universal participation, calibrated scoring or realistic scorelines.
The participation block uses one generated fixture/seed; other seeds in the
predeclared bound were unnecessary and were not run. Temporary legacy comparison
artifacts are not permanent serialization/hash guarantees. Fixed lineups do not
permit legal midmatch eligibility changes; excluded numeric anchors are exercised
through actual controller and goalkeeper cases. Broader M001 determinism and
MATCH_REPLAY obligations remain outside this task.
