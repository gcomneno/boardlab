# M001-SBL-A — Sporting Balance Lab, measurement contract v1

Status: measurement infrastructure over qualified RPG baseline `b257091`.
MUST and MUST NOT are normative. Historical supplied regression: 37/37 CTest.
This increment does not change sporting rules or claim completed calibration.
The engineering, Match Flow v1 and Selector v2 contracts remain authoritative.

## Campaign inputs and assignment

The CLI MUST require exactly one of each option below; `--help` alone succeeds:

```
rmc-sporting-balance --seeds FILE --fixtures all|MATCH_ID \
  --selectors 1|2|1,2 --matrix balanced-v1 --half-time keep \
  --replay none|all --output PREFIX
```

No defaults, implicit seeds, range expansion or additional scenario flags exist.
Exact fixture/scenario overrides are deferred. A manifest contains 1–4096
unique decimal uint64 seeds, one per line, in recorded order. LF and CRLF and
an unterminated last line are accepted; empty lines, whitespace, signs, NUL,
nondecimal characters, >20 digits and overflow are rejected. Seed zero is legal.
Paths are at most 900 bytes. Options are case sensitive; duplicates, missing
values, unknown arguments and unknown persistent fixture IDs fail (exit 2).
Runtime, resource, verification and output errors exit 1; success/help exit 0.

Generator 1, rules 1, stream derivation 1 (PCG32 MATCH), Flow 1, selectors 1/2,
event schema 2, Shot model/action 1 and the existing version-1 action models
MUST be preserved and recorded. The production career foundation MUST be
generated and validated for every seed. Fixtures MUST come from its actual
56-entry calendar, in ascending canonical array index (matchday then slot).
A numeric selection searches actual MatchIds; IDs MUST NOT be synthesized.

For zero-based manifest ordinal `s` and outbound fixture index `p=0..27`:

```
k = (28*s + p) % 1152
opener_side = k % 2                 # 0 home, 1 away
 tactical_pair = (k / 2) % 64
formation_pair = k / 128
home_tactics = tactical_pair % 8; away_tactics = tactical_pair / 8
home_formation = formation_pair % 3; away_formation = formation_pair / 3
```

Formation ordering is 442, 433, 352. Tactical triple bits are bit 0 DIRECT,
bit 1 DRIBBLE_FIRST, bit 2 PRESS_HIGH; unset bits select PATIENT, CARRY_FIRST,
PRESS_NORMAL. Thus opener varies fastest, then outbound home tactics, outbound
away tactics, outbound home formation, outbound away formation. Return index
`p+28` MUST verify the reversed team identities, swap formation/tactic sides
and invert opener side, preserving the same opening team. Return MatchIds and
MATCH streams remain their own production identities. Single-fixture selection
MUST retain its canonical index and `s`, not renumber selected scenarios.

Lineups MUST use `rmc_roster_lineup` and authoritative roster ownership, then
production flow initialization; each eleven remains fixed. Both sides submit
the assigned triple before play and KEEP at half-time. For each fixture selectors
run in order 1 then 2, with identical inputs and independently initialized
canonical MATCH streams. Draw consumption MUST NOT be synchronized or overridden.
Worlds and matches run sequentially; no season results or player growth apply.

## Observer and provenance

The observer MUST read only accepted typed commands and their newly appended
event ranges. It MUST NOT mutate flow/world/RNG/history, retry rejected commands,
insert clock ticks or fabricate events. It MUST NOT repeatedly scan full prefixes;
causes resolve directly by checked consecutive EventId. Replay may traverse the
prefix for semantic verification. Production engines retain their own validation.

SHOT events count shots once; ACTION/500 is a chance, GOAL counts a goal once.
SHOT/501 is reconciled with GOAL and each side's final score, not added to it.
Shot bins preserve exact outcome 501..506, source sector/lane, integer chance
quality 0..100 (not xG), and latent trajectory 1..3, including blocked shots.
Every accepted timed command RECOVERY..SHOT counts one sporting operation;
other command kinds remain separate. Every command and event kind has a count.

PASS attempts use accepted PASS commands; only ACTION/COMPLETED credits its
actual recipient. Failed passes do not credit receipts or control. Tackle
attempts belong to command.team_id / the authoritative defender, irrespective
of event.team_id (which records resulting possession). Outcomes 301/302/303
mean retained/won/loose. Recovery outcome 1/2 means retained/turnover relative
to the causal possession; credit the event winner team. Source MUST be obtained
from `cause_event_id`: PASS loose, TACKLE loose, CROSS loose, or final SHOT
503/504/506, never guessed from the last selected command.

Controlled actions count one PASS passer, CARRY carrier, DRIBBLE dribbler,
CROSS crosser or SHOT shooter per accepted command. They exclude recovery,
tackle carrier and defender, opening/restart and duplicate chance/goal events.
Actual scorers come only from GOAL.primary_actor. Departments 0/1/2/3 are
GK/defender/midfielder/attacker resolved through world roster identity and team
ownership, never PlayerId arithmetic or formation-slot inference.

Participation is a declared role measure, not minutes or touches: each player
named in an accepted timed command counts at most once for that operation,
including explicit opponent/defender/goalkeeper roles; a PASS/CROSS recipient
counts only on completion. Opening/restart takers also count once. A failed
pass target alone does not constitute participation. All eleven selected players
MUST appear, including zero counts. Distinct/zero participation uses this measure.
Top-one/top-two concentration uses controlled actions, sorting counts descending
(ties do not affect the sum). No assists, xG or possession percentages exist.

All ratios MUST retain numerator and denominator. CSV uses separate `_num` and
`_den` columns, with `NA` in the numerator when denominator is zero and denominator
0. Summary ratios use `N/D` or `NA/0`. No floating point, locale-dependent decimal
formatting, implicit rounding or nondeterministic wall-clock timing is allowed.
Campaign additions MUST check uint64 overflow; per-match bounded counters cannot
exceed command/event capacities (role totals at most four per command).

## Storage, failures and replay

Reuse one heap event/journal pair across matches; allocate a second pair only
for `--replay all`. Capacities MUST use production defaults 36008/21616 with
checked actual `sizeof` multiplications; default operation limit is 21616.
Maximum event branch reservations remain production responsibilities (Shot up
to five events). No maximum pair is retained per fixture. Seed storage is bounded.

Stop at the first production/reducer/replay failure. Diagnostics identify seed,
fixture, selector, stage, error code and accepted command/event prefix. A failed
attempt row retains prefix sizes and status but sporting metric fields are NA;
no player/team/bin rows or completed aggregate contributions come from that
attempt. Already written complete rows remain a valid prefix. Replay failure
is an unverified attempt even if sporting execution reached the whistle.
Output errors propagate, including buffered close errors; files may contain a
partial final row or lack a final summary. Multi-file publication is not atomic.
Existing files at PREFIX are overwritten; use a fresh prefix to preserve runs.
A readable summary explicitly records complete/failed campaign and incomplete
attempts. Setup/unknown-fixture rejection may precede report creation.

Replay MUST initialize separate storage with the identical immutable world,
actual fixture, lineups, versions, opener, resource configuration and canonical
MATCH derivation. It MUST invoke only `rmc_match_flow_replay` on accepted typed
commands, split immediately after first-half closure and at completion.
Compare every event semantic field and cause, full Shot payload, command header
and active payload, fixture/lineups/state/possession/score/clocks/stoppage/terminal
reason, configuration, instructions, window flags, sporting/reset counters,
log/journal counts/capacities/MatchId, derived status, RNG state AND increment.
Half-time comparison occurs before KEEP. Pointers, padding and inactive union
fields MUST NOT define equality. No portable replay encoding or hash is claimed.

## Stable output schemas (version 1)

All CSVs use ASCII headers, comma delimiters, decimal integers, LF, no quoted
text fields, and no paths/timestamps. Each match key `K` expands exactly to
`world_ordinal,world_seed,match_id,selector_version`. Manifest order is input
order; other files use world, canonical fixture index, selector, home then away.
Player rows follow lineup slot order 0..10. Bin rows sort by their numeric keys
and omit zero counts. Team department suffix order is `gk,def,mid,att`.
The implementation's header strings below are normative; prefixes and groups
expand in the stated order, without literal braces:

- `PREFIX-manifest.csv`:
  `schema_version,world_ordinal,world_seed,generator_version,rules_version,flow_version,event_schema_version,shot_model_version,stream_version,selectors,matrix,half_time,replay,fixtures,command_capacity,event_capacity`.
  `selectors` is `1`, `2` or `1+2`; fixtures is `all` or the selected MatchId.
- `PREFIX-matches.csv`: K, then
  `fixture_index,matchday,scenario_k,leg,home_team_id,away_team_id,home_formation,away_formation,home_tactics,away_tactics,opener_team_id,status,stage,error_code,accepted_commands,sporting_commands,events,home_goals,away_goals,goal_events,shot_events,chance_events,pass_attempts,pass_completed,pass_failed,tackle_attempts,tackle_retained,tackle_won,tackle_loose,recoveries,half_time_commands,half_home_goals,half_away_goals,half_rng_state,half_rng_increment,final_rng_state,final_rng_increment,replay,period,mode,terminal_reason,elapsed_seconds`, then `cmd_0`..`cmd_11`, then `evt_0`..`evt_14`.
  Status is `complete` or `incomplete`; stage is `complete`, `init`, `next`,
  `select`, `apply`, `reduce`, `reconcile`, `replay`, or `guard`. Replay is
  `none`, `pass`, `fail` or `NA`. Incomplete sporting and half-time/RNG metrics
  use NA; prefix and current lifecycle fields remain observations.
- `PREFIX-team-matches.csv`: K, then
  `side,team_id,formation,tactics,goals,shots,shot_501,shot_502,shot_503,shot_504,shot_505,shot_506,pass_attempts,pass_completed,pass_failed,tackle_attempts,tackle_retained,tackle_won,tackle_loose,recoveries,participants_num,participants_den,zero_players,controlled_actions,top_one_num,top_one_den,top_two_num,top_two_den,pass_success_num,pass_success_den,conversion_num,conversion_den`, then `receipts_gk,receipts_def,receipts_mid,receipts_att`, then corresponding `controlled_*`, then `goals_*`.
  Side is 0 home/1 away; participants denominator is always 11.
- `PREFIX-players.csv`: K, then
  `side,team_id,lineup_index,player_id,slot,department,participations,controlled_actions,pass_receptions,goals,shots`. Includes exactly 22 rows per completed match.
- `PREFIX-shot-bins.csv`: K, then
  `side,team_id,outcome,sector,lane,quality,trajectory,count,shots_den`.
- `PREFIX-recovery-bins.csv`: K, then
  `side,team_id,source,outcome,count,recoveries_den`.
  Source codes 0 PASS, 1 TACKLE, 2 CROSS, 3 SHOT; denominator is that winner
  team's total recoveries. Shot denominator is that shooting team's total shots.
- `PREFIX-summary.txt`: deterministic labelled text, schema/version and campaign
  status, attempted/completed/incomplete, per-selector completed goals/shots/
  scoring-match/draw ratios and command/event totals, paired complete counts
  and v1/v2 paired goal/shot sums. Separate replay counts and memory sizes.
  No target acceptance decision is calculated.

Enums for slots, lifecycle, commands, events, trajectories and departments are
the existing public header values; formations are printed as 442/433/352.
The manifest is an audit copy of all admitted seeds, including worlds not yet
attempted after an earlier runtime failure. It is not evidence of completed runs.

## Exploratory and held-out campaigns; limits

The exploratory design is **448 executions**: four explicit seeds in this order
`0, 1, 20261114, 18446744073709551615`, all 56 fixtures, both selectors. This is
a proposed measurement campaign, not an assertion that it ran or calibrated the
engine. Smoke tests are smaller infrastructure evidence.

The proposed paired held-out design is **20160 executions**: 180 worlds x 56
fixtures x 2 selectors (10080 matched scenario pairs). Its disjoint explicit
seed manifest is below, in row-major order; an operator MUST write one value
per line and lock inputs and analysis before execution. These seeds are not
used in routine smoke, exploratory tuning or acceptance-threshold selection.

```
20270000 20270001 20270002 20270003 20270004 20270005 20270006 20270007 20270008 20270009
20270010 20270011 20270012 20270013 20270014 20270015 20270016 20270017 20270018 20270019
20270020 20270021 20270022 20270023 20270024 20270025 20270026 20270027 20270028 20270029
20270030 20270031 20270032 20270033 20270034 20270035 20270036 20270037 20270038 20270039
20270040 20270041 20270042 20270043 20270044 20270045 20270046 20270047 20270048 20270049
20270050 20270051 20270052 20270053 20270054 20270055 20270056 20270057 20270058 20270059
20270060 20270061 20270062 20270063 20270064 20270065 20270066 20270067 20270068 20270069
20270070 20270071 20270072 20270073 20270074 20270075 20270076 20270077 20270078 20270079
20270080 20270081 20270082 20270083 20270084 20270085 20270086 20270087 20270088 20270089
20270090 20270091 20270092 20270093 20270094 20270095 20270096 20270097 20270098 20270099
20270100 20270101 20270102 20270103 20270104 20270105 20270106 20270107 20270108 20270109
20270110 20270111 20270112 20270113 20270114 20270115 20270116 20270117 20270118 20270119
20270120 20270121 20270122 20270123 20270124 20270125 20270126 20270127 20270128 20270129
20270130 20270131 20270132 20270133 20270134 20270135 20270136 20270137 20270138 20270139
20270140 20270141 20270142 20270143 20270144 20270145 20270146 20270147 20270148 20270149
20270150 20270151 20270152 20270153 20270154 20270155 20270156 20270157 20270158 20270159
20270160 20270161 20270162 20270163 20270164 20270165 20270166 20270167 20270168 20270169
20270170 20270171 20270172 20270173 20270174 20270175 20270176 20270177 20270178 20270179
```

Frozen analysis MUST distinguish within-world/fixture/return dependencies and
paired selector inputs. Executions are not independent real-world observations;
shared initial streams do not imply equal subsequent draws. balanced-v1 is a
cyclic assignment, not randomization or a full factorial within each world.
180 worlds do not divide the 1152-cell cycle exactly. Reports provide exact
counts/ratios, not confidence intervals, causal tactical superiority or claims
about universal participation. No real-world football population is sampled.

Existing proposed M001 targets remain **unachieved/unqualified by this task**:
mean goals 1.8–4.0, mean shots 12–36, matches with goals 65–95%, draws 15–40%.
No new sporting thresholds are frozen from unmeasured results. Calibration,
held-out acceptance, broader determinism, portable replay/hash, lateral/CROSS
selection, missing rules, season persistence and statistical qualification are
outside this infrastructure increment. Reports MUST describe observations as
exploratory and MUST NOT label results realistic or balanced without evidence.

## Executed infrastructure evidence (2026-11-15)

Created the standalone driver and a compile-time-only self-check executable from
its same C11 translation unit. One CTest `rmc-sporting-balance-smoke` has a
120-second timeout. The public CLI has no test flags. The smoke uses seed
20261114, actual MatchIds 1 and 29 (teams 4/6 and 6/4), both selectors and KEEP;
repeated six CSV files and the summary compare byte-identically. It checks exact
schemas/scenarios, command/event/goal/shot reconciliation, department sums and
all eleven players. The independent event/command oracle observes successful
and failed passes, tackle attribution differing from resulting possession,
PASS/TACKLE/SHOT recovery causes and zero-participation players. Fixture 1
provides all six actual Shot outcomes. Pure reducer vectors cover CROSS causes
without submitting fabricated events to production. Semantic replay compares
half-time and final prefixes, including both RNG fields; comparator sensitivity
checks deliberately change read-only comparison copies. A separate production
run with operation limit 4 stops after four accepted commands/three events and
proves exclusion of the incomplete attempt from completed aggregates and detail
rows. Strict malformed-manifest/options and output-open/buffered-close errors
are subprocess checks; CSV streams close before summary publication status.

Actual commands (reconfiguration was needed for the new targets):

```
cmake -S src/rpg_manager_calcio -B /tmp/rmc-b1-match-state-build
cmake --build /tmp/rmc-b1-match-state-build --parallel 4
ctest --test-dir /tmp/rmc-b1-match-state-build --output-on-failure
```

Build exit 0 with `-std=c11 -Wall -Wextra -Wpedantic -Werror`.
Actual regression: **38/38 PASS**, zero failures, 91.59 seconds; the new smoke
passed in 6.85 seconds. These times are host observations, not canonical data.

A separate functional traversal used a one-line manifest `20261114`, fixtures
`all`, selectors `1,2`, balanced-v1, KEEP and replay `none`, output prefix
`/tmp/rmc-sbl-validation/all`. Actual: **112/112 complete**, 56 paired scenarios,
zero incomplete, all 56 actual MatchIds and return-team/opener mappings checked.
External elapsed time was 39.157 seconds; it is excluded from canonical files.
On this host event/command sizes are 144/72 bytes; one reusable default backing
pair occupies 5,185,152 + 1,556,352 = 6,741,504 bytes, plus bounded world/metrics/
seed storage and production scratch. Replay adds a second pair. A separate
selector-2-only fixture-29 check completed 2/2 worlds with manifest order seed 0
then UINT64_MAX, accepting CRLF and no final newline.

The 112-run traversal is functional evidence, not the declared 448-execution
exploratory campaign or proposed 20160-execution held-out campaign; neither full
campaign was run here. Observed goals/shots were outside the proposed target
windows (v1 totals 1293 goals/4022 shots over 56 matches; v2 1307/4024 over 56).
No sporting acceptance, completed calibration, causal tactical conclusion or
broader M001 gate is claimed. Only the three new authorized files and local
CMakeLists were edited; protected production components and concurrent work
were preserved. No Git history, branch, commit, push or worktree operation ran.
