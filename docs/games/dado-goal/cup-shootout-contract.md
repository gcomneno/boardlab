# Dado Goal — SP-01A: single-player Cup shootout contract

Status: authoritative contract for SP-01 implementation; feature not implemented
by this document. The full SP-01 milestone is already authorized. SP-01A writes
only this contract and does not require another implementation authorization.

Evidence baseline: local working tree inspected on 2026-11-21, including existing
uncommitted single-player changes. No applicable `AGENTS.md` was found in the
repository or its ancestor directories. Existing unrelated edits are preserved.

**Authority:** the gameplay requirements explicitly marked **Approved** below
come from the mission. Items marked **Proposed implementation** specify the
technical design for the subsequent implementation; they are not additional
author-approved gameplay rules. MUST/MUST NOT express contract requirements.

## 1. Scope and explicit exclusions

**Approved:** make tied Cup eliminations playable through penalty attempts in
single-player only, for both single-leg and two-legged Cups. There are five
attempts per team in the initial series, alternating after each attempt, with
early termination when recovery becomes impossible. A tie after five each goes
to sudden death, decided only after equal attempts. Each attempt consumes exactly
one canonical Extended v1 green-die draw, with conversion probability 3/6.

No tactical modifiers, additional randomness, goalkeeper/taker selection,
player-rating effects, extra time, away-goals rule, randomized starting team or
alternative penalty rules are introduced. Shootout goals MUST NOT change regular
match scores, goalscorers, events, statistics, possession, Manager totals or
championship standings. A human operates the attempts for both teams; a separate
AI or automatic shootout simulator is unnecessary for SP-01.

M035-B, multiplayer clients, shared multiplayer components, authority, transports,
Worker/D1 persistence and multiplayer assets are excluded. This task does not
implement gameplay, modify tests or the changelog, install packages, commit,
push, deploy or access remote D1. Existing local changes MUST remain untouched.

## 2. Existing source-of-truth interfaces and invariants

The following are observed source facts, not proposed APIs:

| Source | Existing contract relevant to SP-01 |
| --- | --- |
| [Extended rules v1](extended-rules-v1.md), §3 | Bottle-green penalty die: three `GOAL!` faces, then `PALO/TRAVERSA`, `PARATO/RESPINTO`, `FUORI`; 50% conversion; no recursive penalty. |
| [Canonical event/statistics contract](extended-event-statistics-contract.md) | Match statistics derive from canonical match streams. Adding shootout rolls to those streams would incorrectly add goals, penalties and possession. |
| [Team management contract](issue-29-team-management-contract.md) | Local-first, offline single-player; descriptive provider data cannot affect outcomes. Official team/player names remain unchanged across locales. |
| [cup.mjs](../../../web/dado-goal/cup.mjs) | `createCup`, `completeCupFixture`, `resolveCupTie`, `advanceCup`, `advanceCupIfReady`, `validateCupProgression` define bracket and aggregate authority. |
| [cup-store.mjs](../../../web/dado-goal/cup-store.mjs) | Cup schema/version, JSON validation, active/archive storage, match-association recovery and manual shootout recording. |
| [engine.mjs](../../../web/dado-goal/engine.mjs) | `FACES.penalty` and `rollFace("penalty", random)` are the canonical Extended v1 draw. `applyRoll` advances a regular match and is unsuitable for shootout attempts. |
| [roll-event.mjs](../../../web/dado-goal/roll-event.mjs) | `rollEventKind("penalty", face)` supplies stable semantic kinds without producing match statistics. |
| [cup-view.mjs](../../../web/dado-goal/cup-view.mjs) | `cupFixturePresentation` displays regular score, aggregate, winner and `pending-penalties`, using `cup.shootouts`. |
| [app.mjs](../../../web/dado-goal/app.mjs) and [index.html](../../../web/dado-goal/index.html) | Single-player integration, current manual score form, match completion, startup recovery and Cup dashboard. |

Cup participants number 4, 8, 16 or 32, with `legs` equal to 1 or 2. Fixtures have
canonical IDs `r{round}-p{pair}-l{leg}`. Tie result keys are
`r{round}-p{pair}`; both coordinates are positive integers. Team IDs are canonical
strings, not names, shirts or current page selections. Completed fixtures link
to unique canonical `managementState.matches` records through `matchId`.

`resolveCupTie({ fixtures, matches, penalties })` requires completed, correctly
oriented match records with nonnegative safe integer scores. It sums goals in
first-leg home/away orientation. A non-tie is decided by goals and rejects
penalties. A tie without penalties returns `decidedBy: "pending-penalties"`.
A decisive penalty result has unequal nonnegative safe integer `home`/`away`
totals and returns `decidedBy: "penalties"` with the winner's team ID.

`advanceCupIfReady(cup, matches, shootouts)` returns `pending-matches` if any
fixture in the current round has `matchId === null`. Only after all current-round
matches exist does it return the first unresolved `pending-penalties` target,
`advanced`, or `completed` for an already crowned Cup. It advances a whole round,
not an individual winner. `validateCupProgression` reconstructs the canonical
bracket from match records and penalty totals; this remains authoritative.

### Confirmed manual-validation defect

In the inspected `confirmCupShootout()` in `app.mjs`, the literal is
`/^\\d+$/` for both trimmed inputs. **The source confirms the suspected defect.**
This JavaScript regex matches a literal backslash followed by one or more `d`
characters, not decimal digits. Source-extracted evaluation rejects `"4"` and
`"0"`, and accepts the two-character string backslash-plus-`d`; that string then
fails `Number.isSafeInteger(Number(value))`. Ordinary numeric input therefore
cannot pass the combined validation. This is not merely Markdown escaping or
a hypothetical report. The intended decimal regex is `/^\d+$/`.

The handler also guards against missing Cup, active championship and hidden
panel, reads numeric round/pair from the panel, rejects unsafe/equal totals,
then calls `recordCupShootout`, `advanceCupIfReady`, and `saveVerifiedCup` before
replacing `cupState`. It currently renders an Italian validation string and
raw caught `failure.message`. It has no attempt history or playable turn state.
The eventual manual path MUST fix the regex and explicitly check nonnegativity;
HTML `min="0"` is not domain validation.

The recovery regex in `validateCupRecovery()` is instead
`/^r[1-9]\d*-p[1-9]\d*-l[12]$/` and is correct in the inspected source. A direct
call to `saveCupRecovery` accepts `r1-p1-l1`. Do not report or fix that regex as
part of the confirmed manual-input defect.

### Existing recording and storage behavior

`recordCupShootout(document, matches, round, pair, penalties)` verifies the Cup,
requires an unfinished Cup, positive safe integer target coordinates, the active
round, an absent result at the tie key, exactly the required legs, and a completed
aggregate tie. It resolves and validates the submitted totals, clones the Cup,
adds only `shootouts[key] = { home, away }`, then verifies again. It neither saves
to storage nor advances the bracket. It rejects duplicate decisions. It does
not validate whether a manual total is achievable under a five-attempt history,
nor does it fabricate attempts. SP-01 MUST preserve that legacy distinction.

Current storage uses synchronous `localStorage` adapters:

| Key / interface | Observed behavior |
| --- | --- |
| `dado-goal.cup.v1` / `CUP_STORAGE_KEY` | One entire active Cup JSON document. `CUP_SCHEMA_VERSION` is 1. |
| `saveCup` / `loadCup` | Shape validation and JSON serialization/parsing; not full progression verification. Missing active key loads as `null`; malformed JSON/version throws. |
| `saveVerifiedCup` / `loadVerifiedCup` | Validate document and reconstruct progression against canonical matches, using `document.shootouts ?? {}`; save with one `setItem`. |
| `importCupJson` / `exportCupJson` | Shape-validated Cup JSON round trip. Unknown top-level fields survive `structuredClone`; optional `shootouts` currently receives only object-shape validation here. |
| `verifyCupDocument` / `importVerifiedCupJson` | Full progression validation; their default external `shootouts` argument is `{}`, so callers must currently pass document results explicitly. |
| `dado-goal.cup-archive.v1` | Array of completed, verified Cups. Archive checks unique Cup IDs and rejects re-archiving; active removal is a separate write. |
| `dado-goal.cup-recovery.v1` | A journal containing only `{ cupId, fixtureId, matchId }`, for linking a separately persisted regular match to its Cup fixture. It is not a shootout journal. |

On startup the app loads management matches before the verified Cup. A Cup load
exception currently sets in-memory `cupState = null` without deleting the stored
bytes. Match recovery checks Cup identity, fixture and canonical teams, completes
the association, computes readiness, saves, then clears the journal. Match
completion writes recovery intent, management data, then Cup association and
advancement, retaining the journal if reconciliation cannot finish. Shootout
attempts do not write management data and need no second journal/key.

**Model finding:** the existing schema has no meaningful representation of an
in-progress shootout. `shootouts[key]` is a final score consumed as an immediate
decision and MUST NOT hold partial scores. However, optional top-level fields
survive current serialization/cloning. A versioned optional session map can be
added without changing Cup schema v1 or storage keys. This is additive storage
compatibility, not proof that old clients understand or safely edit new sessions.

The app's `globalSaveDocument`, `validateGlobalSave` and `restoreGlobalRuntime`
currently include management, Manager and championship data, but omit Cup and
its archive/recovery. The existing global import rollback list and local reset
list also omit Cup keys. Thus global save is currently **not a portable Cup
backup**. These are integration limitations, not shootout persistence guarantees.

The historical [M034 dice candidate](../../../web/dado-goal/docs/design/m034-v4-dice-contract.md)
contains a 5/6 green matrix and a versioned V2 proposal. It is not the SP-01
baseline. SP-01 MUST use Extended v1's 3/6 matrix from `engine.mjs` and MUST NOT
dispatch through `engine-v2.mjs` or change either engine.

## 3. State machine and allowed transitions

**Proposed implementation:** use a small pure single-player shootout reducer,
separate from regular match state. The authoritative persisted states are:

| State | Meaning | Allowed next step |
| --- | --- | --- |
| Absent / eligible | Readiness identifies an undecided tied pair; no session or final result exists. | Start: create and save an empty `initial` session, with no draw. Alternatively record a manual legacy result before starting. |
| `initial` | Initial series remains undecided; zero to nine attempts are normally present. | Confirm one attempt; remain `initial`, enter `sudden-death`, or finish `complete`. |
| `sudden-death` | Five each ended level; subsequent pairs remain unresolved. | Confirm one attempt; remain here until an equal-attempt decisive pair produces `complete`. |
| `complete` | Terminal attempt history and matching final totals exist. | Read-only display, ordinary Cup scheduling, or archive when the whole Cup completes. |
| Legacy decided | Final manual totals exist with no session. | Read-only display and existing progression; never create attempts for it. |

At ten initial attempts, unequal totals are already terminal; equal totals enter
`sudden-death`. A terminal initial series can have fewer than ten attempts.
`complete` may belong to an earlier round after advancement. There is no persisted
`rolling`, `saving`, `save-failed`, `winner-pending` or `admitted` phase: those UI
conditions do not independently determine a sporting result.

Disallow reset, undo, replay, changing the starting team, switching to manual
totals after session start, confirming out of turn, and attempts after completion.
Leaving/reloading pauses by retaining the committed session. No automatic draw
is triggered by startup, rendering, localization, animation or resuming.

Preserve the current readiness gate: SP-01 starts only the pending pair returned
after all current-round matches are recorded. It does not change Cup scheduling.
Allow at most one unfinished session in a Cup; resolve multiple tied pairs in
readiness order. Completed histories remain in the session map.

## 4. State schema, versioning and stable identifiers

**Proposed implementation:** retain `CupDocument.schemaVersion: 1` and all
existing fields. Add optional `shootoutSessions`, a plain object keyed by the
existing tie key. Absence means no recorded playable history. Example of a
persisted session before its first attempt:

```json
{
  "shootoutSessions": {
    "r1-p1": {
      "schemaVersion": 1,
      "rulesVersion": "extended-v1",
      "shootoutId": "[\"cup-123\",\"r1-p1\"]",
      "round": 1,
      "pair": 1,
      "homeProviderTeamId": "A",
      "awayProviderTeamId": "B",
      "fixtureRefs": [
        { "fixtureId": "r1-p1-l1", "matchId": "match-123" }
      ],
      "firstTeam": "home",
      "status": "initial",
      "attempts": []
    }
  }
}
```

This is an extension fragment, not an independently valid Cup document.
`rulesVersion: "extended-v1"` identifies this session's immutable rules; it does
not migrate or add a rules version to historical regular matches. Session schema
version and rules version have different meanings. Unknown versions fail closed
with stored source retained; never reinterpret them as empty v1 sessions.

`shootoutId = JSON.stringify([cupId, tieKey])`. An attempt has:

```json
{
  "attemptId": "[\"cup-123\",\"r1-p1\",1]",
  "index": 1,
  "team": "home",
  "face": "GOAL!",
  "kind": "penalty-goal"
}
```

`attemptId = JSON.stringify([cupId, tieKey, index])`, with contiguous one-based
safe integer indices. Tuple encoding avoids delimiter ambiguity and needs no
random identifier. `fixtureRefs` lists all legs in leg order and freezes their
canonical match associations. Team IDs are those of leg 1 and never change when
the return leg reverses venue. `firstTeam: "home"` is a **proposed deterministic
ordering convention**, not a new approved coin-toss rule.

The four allowed kinds are `penalty-goal`, `penalty-woodwork`, `penalty-saved`,
`penalty-wide`, mapped by `rollEventKind`. Store canonical engine faces, never
translated labels. Three identical goal faces need not be distinguished by a
physical face index: `rollFace` returns their common value without losing any
gameplay information.

Derive revision (`attempts.length`), goals, counts, next team, stage, round within
the series and winner by replay. Persist only the listed session status as a
checked projection; do not persist independently mutable totals/cursors or an
`admitted` boolean. At completion, `cup.shootouts[tieKey]` retains its existing
exact `{ home, away }` final-result shape, equal to the replayed goals. Keep the
completed session for recovery and inspection. No timestamps, seeds, player
statistics or locale are needed for outcome authority.

## 5. Turn ordering, early termination and sudden death

**Approved rules; proposed first-team convention as identified above:** for
attempt index `i`, odd indices belong to tie-home, even indices to tie-away.
Every attempt, including a miss, consumes that team's opportunity and alternates
the turn unless the shootout has ended. The initial allocation is five each;
early termination can leave either team's allocation unused.

After each initial attempt, derive attempts `aH`, `aA`, goals `gH`, `gA` and
remaining opportunities `rH = 5 - aH`, `rA = 5 - aA`:

- Home wins immediately if `gH > gA + rA`.
- Away wins immediately if `gA > gH + rH`.
- Otherwise continue the initial series while either count is below five.
- At five each, equal goals enter sudden death; unequal goals are terminal.

Strict `>` matters: being able to tie still permits recovery. Evaluate after
every prefix, including unequal-attempt prefixes. For example, three home goals
and three away misses finish at six total attempts, 3–0. Home outcomes
goal/goal/miss/goal interleaved with three away misses finish at seven attempts,
also 3–0; requiring equal attempts here would be incorrect.

Sudden death begins with home, continues normal alternation, and grants pairs of
attempts. After the first attempt of a pair, no winner exists even if the score
differs. After the second attempt, differing goals decide the winner; equal goals
require another pair. Thus home goal/away miss and home miss/away goal decide
opposite winners, while two goals or two misses continue. No fixed sudden-death
round cap, replacement coin toss or hidden tiebreak is permitted. Storage or
numeric limits pause with an error; they never select a winner.

## 6. Deterministic injectable draw semantics

**Proposed interfaces** in a small `web/dado-goal/cup-shootout.mjs` module;
these names do not describe exports that already exist:

```text
createCupShootout(document, matches, round, pair) -> session
replayCupShootout(session) -> { status, attemptsByTeam, goals, nextTeam, winner }
applyCupShootoutFace(session, { attemptId, expectedRevision, face }) -> session
```

Creation validates eligibility through existing Cup interfaces. Replay and apply
are pure, immutable operations with no storage, DOM, clock, random source or
regular-match mutation. The replay result uses tie-relative home/away IDs and
`winner: null | "home" | "away"`; `nextTeam` is null when complete.

The single-player persistence coordinator accepts an injected
`random = Math.random`. After checking command identity, revision, current turn
and lifecycle, it invokes **exactly** `rollFace("penalty", random)` once for a new
attempt, derives the kind using `rollEventKind`, and applies that returned face.
`rollFace` itself calls `random()` once, accepts only finite numbers in `[0, 1)`,
and indexes `FACES.penalty` by `floor(value * 6)`.

| Draw interval | Canonical face | Kind | Shootout goal |
| --- | --- | --- | --- |
| `[0, 3/6)` | `GOAL!` (three slots) | `penalty-goal` | 1 |
| `[3/6, 4/6)` | `PALO/TRAVERSA` | `penalty-woodwork` | 0 |
| `[4/6, 5/6)` | `PARATO/RESPINTO` | `penalty-saved` | 0 |
| `[5/6, 1)` | `FUORI` | `penalty-wide` | 0 |

Never use a separate success coin, penalty-award roll, result die, rebound roll,
taker draw, tactical adjustment or animation outcome. Draw no replacement after
an invalid random result. Invalid commands, duplicate requests, retries of the
same prepared candidate and recovery use zero new draws. UI animation displays
the already committed outcome and cannot call the gameplay random source.

The frozen test source can supply interval midpoints for all six slots. A known
face sequence plus the same starting session MUST produce the same history,
termination point, totals and winner. Replay never invokes `applyRoll`.

## 7. Atomic persistence after every confirmed attempt

**Proposed implementation:** keep the session and final decision in the same Cup
document and existing active key. Define a confirmed attempt as one whose entire
new Cup document has been successfully written. Clicking the button requests
confirmation; persistence success is the acceptance boundary.

Proposed coordinator entry points (under the exclusive editor guard below):

```text
startCupShootout(storage, matches, { cupId, round, pair }) -> CupDocument
confirmCupShootoutAttempt(storage, matches,
  { cupId, shootoutId, attemptId, expectedRevision }, { random = Math.random })
  -> { cup, attempt, disposition: "committed" | "already-committed" }
```

These helpers return only accepted documents, never an unsaved candidate as a
successful result. A typed save failure retains the prepared candidate/command
in the coordinator for `retryCupShootoutSave()`; retry has no draw parameter.
The app owns that coordinator instance for the active session. Start on an
already matching empty/unfinished session returns it unchanged; a conflicting
target or decided tie fails. The pure reducer remains independently injectable
by face, while the coordinator is injectable by random source and storage.

Start persists the empty session before enabling the first attempt. For every
new attempt, the coordinator MUST:

1. Acquire exclusive local ownership of the Cup mutation, disable repeated
   activation, and load/verify the latest stored Cup against canonical matches.
2. Check Cup/session/attempt identity and `expectedRevision = attempts.length`.
   An already stored identical command returns its recorded outcome; stale or
   conflicting input fails before any draw. A command carries the rendered
   attempt ID/revision; queued clicks MUST NOT invent the next command on entry.
3. Draw once and build a candidate Cup on clones, appending exactly one attempt.
4. If terminal, derive final totals, record the final result and compute ordinary
   readiness/advancement on that candidate as described in §9.
5. Validate the entire candidate, including history and progression, then call
   `saveVerifiedCup(storage, candidate, matches)` once. This serializes session,
   final totals and any new fixtures/champion in a single `setItem`.
6. Only after success replace `cupState`, publish the result and next turn,
   release controls and render. Rendering MUST NOT advance again.

One-key replacement is the local atomic boundary; there is no claim of a
transaction across localStorage keys or of remote durability. Match records are
read-only inputs. Attempt persistence MUST NOT write management/Manager state,
archive data, the ordinary-match runtime or the match-association journal.

If validation/serialization/write fails, preserve the previous committed Cup
and its turn. After a draw has produced a valid candidate, retain that exact
candidate and command in memory, block another attempt and offer **Retry save**.
Retry saves the same candidate without drawing or advancing again. Reload before
successful persistence restores the previous committed revision: the unsaved
candidate was never a confirmed attempt and cannot be promised durable. Never
show it as saved or allow further attempts from it. After a successful write,
a presentation failure MUST recover from storage rather than treat it as a
failed attempt.

Before a save retry, re-read storage: if the exact candidate is already present,
return it as committed; if the predecessor is unchanged, retry its write; if
another document replaced it, reject as stale and preserve that newer document.
Do not overwrite a changed Cup with a retained candidate.

For storage access failure before a draw, draw nothing. For an invalid random
value, append nothing and show a resolution error; no automatic replacement draw
occurs. Guard every counter increment against safe-integer overflow.

**Local concurrency boundary:** localStorage read/compare/write is not CAS.
Use a feature-detected, exclusive Web Lock named by the Cup ID for the active
shootout editor, held while it owns that session; another current-version tab
is read-only. On reload/ownership transfer, re-read storage before enabling
commands. Current-version Cup mutations must respect that ownership, including
manual results and fixture writes. If locking is unavailable, fail closed for
the playable editor with a localized message; do not claim multi-tab safety
from a revision check alone. Legacy result viewing remains available. Supporting
additional locking fallbacks is outside the minimum implementation. Simultaneous
writes by older app versions or external storage tools are unsupported.

## 8. Reload recovery without reroll or double advancement

Load canonical management matches, reconcile any existing regular-match recovery
journal, then validate and replay the Cup's sessions. Re-enable only the next
unconfirmed attempt of the selected unfinished session. Re-render recorded
outcomes and totals directly from history, including a half-completed sudden-death
pair. Acquiring ownership or drawing the panel does not create a new session.

| Interruption point | Required recovery |
| --- | --- |
| Before session-start write | No session exists; Start is available without a draw. |
| After start, before first attempt | Empty initial session resumes at attempt 1. |
| Before an attempt's `setItem` succeeds | Previous document is authoritative; no confirmed attempt was added. |
| After `setItem`, before memory update, animation or next render | Replay the new document; show the recorded outcome and correct next turn, with zero draws. |
| After the first sudden-death attempt of a pair | Resume the other team's attempt; no winner is inferred from the temporary lead. |
| After a terminal-attempt write | Session, final result and any ready advancement already coexist; show completion without recording or advancing again. |

An old manual result with no history remains a legacy result. A terminal session
without final totals, or final totals conflicting with history, is invalid new
data, not an invitation to reroll or guess a winner. Do not repair malformed
history by dropping its last attempt, padding misses, overwriting results or
clearing the Cup.

New load failures MUST preserve the original stored bytes, show a localized
recovery error and block Cup mutations. In particular, the existing catch-to-null
startup behavior MUST NOT expose Create Cup as a way to silently overwrite an
unreadable Cup. A session with an unsupported version is retained similarly.

## 9. Winner admission exactly once into Cup progression

The terminal attempt's coordinator builds one candidate containing:

- The complete session, whose replay has exactly one winner.
- Exactly one `shootouts[tieKey] = { home, away }`, in first-leg orientation.
- The result of one `advanceCupIfReady(candidate, matches, candidate.shootouts)`
  evaluation: the same round if other pairs are undecided, otherwise the next
  canonical round or final champion.

Only the final map entry is supplied to `resolveCupTie`/Cup progression; partial
shootout totals never are. Reuse `recordCupShootout` for final-total validation
and duplicate-result protection. The public manual-recording path MUST reject
overriding a started session. A small internal finalization adapter can reuse
`recordCupShootout` on a private clone with only the target session temporarily
omitted, attach the complete session, then advance and fully verify the final
candidate. No intermediate clone is saved, returned as accepted state or exposed
to rendering. This avoids making persisted validators accept half-finalized
documents or adding another public bypass flag.

Exactly once is enforced by the stable tie/result key, deterministic attempt
IDs, revision checks, exclusive editor ownership and one document commit, not
by a separate mutable admission flag. When retrying a terminal command, load
the latest Cup: if the recorded attempt/history/result already match, return
that accepted state with no draw, record call or advancement. Conflicts fail.

Do not assume `advanceCup` is idempotent, or rerun it on a stale pre-advance
snapshot. A render, refresh, duplicate click or save retry cannot append another
next-round fixture or crown another champion. A winner is admitted once to the
pair's decision; the bracket materializes only when all round winners exist.

Archive only through the existing completed-Cup workflow. Archive writes and
active-key removal are separate operations today; SP-01 does not claim they are
atomic or rewrite the archive lifecycle. Completed session history survives
normal archive serialization; duplicate archive IDs retain existing protection.

## 10. Two-legged aggregate handling

Both legs MUST have valid canonical match associations before a shootout is
eligible. Use `resolveCupTie` on fixtures sorted by leg, not the return leg's
displayed score. No shootout after leg 1; no shootout after a drawn return leg
when the aggregate is decisive; an aggregate tie can require penalties even
when neither individual leg was drawn.

For leg 1 A–B 2–0 and leg 2 B–A 2–0, the tie is 2–2. Tie-home is A throughout:
`shootouts[key].home` and odd shootout attempts refer to A, even though B hosts
the return leg. For A–B 2–0 and B–A 1–0, A advances 2–1 without penalties.
For A–B 2–0 and B–A 0–0, a drawn second leg still does not authorize penalties.

`fixtureRefs` binds both match IDs; a stale UI target or swapped team mapping
must fail validation. Show team names next to shootout totals, and distinguish
regular scores, aggregate and shootout totals. Never add shootout goals to either
leg, the aggregate or canonical match statistics. No away-goals criterion or
extra-time match is inferred from real-world football rules.

## 11. Compatibility with old Cups and manual shootout results

**Proposed compatibility requirements:** missing `shootoutSessions` is valid
Cup v1 data and requires no rewrite. Missing `shootouts` is likewise valid.
Existing active Cups, advanced rounds, champions and archives MUST retain their
IDs, fixtures, canonical matches, results and unknown preserved metadata.

A legacy `shootouts[key]` without a session is an authoritative manually
recorded decision. Keep its unequal nonnegative safe integer totals exactly as
recorded, even if those totals cannot be explained by the new playable sequence.
Do not reconstruct a history, consume randomness or make that tie playable.
Manual imports/results remain supported. A user can still choose manual entry
for an eligible tie before starting a playable session, with the repaired numeric
validation; once the empty session is persisted, that choice is locked out.

New playable results MUST include complete history and satisfy all replay rules.
Unfinished sessions MUST NOT have final totals. Completed sessions MUST have
matching final totals. Metadata is additive; do not repurpose `shootouts` values
as mixed partial/final objects or increment `CUP_SCHEMA_VERSION` unnecessarily.

Existing writers preserve an unknown session field but do not enforce it. Opening
and editing a new in-progress Cup with an old app is unsupported and could create
a conflicting manual result. Do not advertise downgrade-safe editing. Detect any
conflict on new-client load without silently preferring either representation.

Cup-specific JSON APIs MUST round-trip sessions and validate them at import/export
and verified-storage boundaries. For verified import, the document's shootouts
are authoritative: omitted external arguments must use document totals, and an
explicit conflicting external map must fail rather than validate a different
bracket. Preserve the existing call signatures for callers supplying matching
maps. Shape-only Cup imports cannot prove canonical match existence; full
verification against matches is mandatory before admission to active play.

The minimum SP-01 recovery promise is same-origin Cup local storage and the
Cup-specific JSON contract, not a new global backup format. Keep the global
format unchanged in the minimal implementation. Until Cup-aware global backup
is separately implemented, visibly explain its omission in IT/EN and block global
import while a stored Cup exists: replacing management history could otherwise
orphan canonical match references. Do not silently use global import as a Cup
restore, erase Cup keys or alter the archive/reset lifecycle as an incidental fix.

## 12. Italian and English UI states and errors

**Proposed UI:** reuse the single-player Cup panel and localization system. Show
both team names, regular score/aggregate, a separate shootout score, per-attempt
goal/miss history, attempts taken and current team. Use explicit textual outcomes
in addition to color or icons. The roll control confirms one attempt and then
saves it; no extra confirmation dialog is required. A saved result is announced
before focus moves to the next enabled action. Keyboard activation and an
accessible live status/alert region are required.

Suggested stable message keys and required equivalent wording:

| Key / condition | Italiano | English |
| --- | --- | --- |
| `cupShootout.title` | Rigori di spareggio | Penalty shootout |
| `cupShootout.ready` | Parità: servono i rigori tra {home} e {away}. | Scores level: {home} and {away} need a shootout. |
| `cupShootout.start` | Gioca i rigori | Play shootout |
| `cupShootout.manual` | Inserisci risultato manuale | Enter manual result |
| `cupShootout.initial` | Serie iniziale · {team} · rigore {attempt} di 5 | Initial series · {team} · attempt {attempt} of 5 |
| `cupShootout.suddenDeath` | Oltranza · coppia {pair} · tira {team} | Sudden death · pair {pair} · {team} to shoot |
| `cupShootout.confirm` | Tira il rigore di {team} | Take {team}'s penalty |
| `cupShootout.saving` | Salvataggio del rigore… | Saving penalty… |
| `cupShootout.saved` | Rigore salvato. | Penalty saved. |
| `cupShootout.goal` | Goal | Goal |
| `cupShootout.woodwork` | Palo/traversa · sbagliato | Post/crossbar · missed |
| `cupShootout.savedByKeeper` | Parato/respinto · sbagliato | Saved/blocked · missed |
| `cupShootout.wide` | Fuori · sbagliato | Wide · missed |
| `cupShootout.awaitEqual` | Il vantaggio è provvisorio: deve tirare {team}. | The lead is provisional: {team} must take a penalty. |
| `cupShootout.resume` | Rigori ripresi dall'ultimo tentativo salvato. | Shootout resumed from the last saved attempt. |
| `cupShootout.winner` | {team} vince ai rigori {home}–{away}. | {team} wins the shootout {home}–{away}. |
| `cupShootout.legacy` | Risultato rigori registrato manualmente: {home}–{away}. | Manually recorded shootout: {home}–{away}. |
| `cupShootout.retrySave` | Riprova salvataggio | Retry save |
| `cupShootout.globalOmission` | Il salvataggio globale non include la Coppa. | The global save does not include the Cup. |

Domain/coordinator errors use stable codes with localized display, not raw
exceptions. Suggested minimum mapping:

| Code | Italiano | English |
| --- | --- | --- |
| `CUP_SHOOTOUT_NOT_ELIGIBLE` | Rigori non disponibili per questa eliminatoria. | A shootout is not available for this tie. |
| `CUP_SHOOTOUT_STALE` | Lo stato della Coppa è cambiato. Ricarica i dati salvati. | The Cup state changed. Reload the saved data. |
| `CUP_SHOOTOUT_DECIDED` | Questa eliminatoria è già decisa. | This tie is already decided. |
| `CUP_SHOOTOUT_MANUAL_LOCKED` | I rigori sono già iniziati: riprendi i tentativi salvati. | The shootout has started: resume the saved attempts. |
| `CUP_SHOOTOUT_INVALID_SCORE` | Inserisci due punteggi interi, non negativi e diversi. | Enter two different nonnegative whole-number scores. |
| `CUP_SHOOTOUT_DRAW_INVALID` | Impossibile risolvere il rigore. Nessun tentativo salvato. | Unable to resolve the penalty. No attempt was saved. |
| `CUP_SHOOTOUT_SAVE_FAILED` | Rigore non salvato. Riprova il salvataggio prima di continuare. | Penalty not saved. Retry saving before continuing. |
| `CUP_SHOOTOUT_STORAGE_UNAVAILABLE` | Salvataggio non disponibile: impossibile iniziare i rigori. | Saving is unavailable: the shootout cannot start. |
| `CUP_SHOOTOUT_INVALID_STATE` | Dati della Coppa non validi. I dati salvati sono stati conservati. | Invalid Cup data. The saved data has been preserved. |
| `CUP_SHOOTOUT_UNSUPPORTED_VERSION` | Versione dei rigori non supportata. I dati salvati sono stati conservati. | Unsupported shootout version. The saved data has been preserved. |
| `CUP_SHOOTOUT_OTHER_TAB` | Rigori aperti in un'altra scheda. Questa vista è in sola lettura. | The shootout is open in another tab. This view is read-only. |
| `CUP_SHOOTOUT_LOCK_UNAVAILABLE` | Questo browser non supporta l'accesso esclusivo ai rigori. | This browser does not support exclusive shootout editing. |
| `CUP_GLOBAL_IMPORT_BLOCKED` | Impossibile importare un salvataggio globale mentre è presente una Coppa. | A global save cannot be imported while a Cup is stored. |

Disable attempt controls during saving/animation, on storage errors, after
completion and without ownership. Expose Retry save only when an unchanged
candidate exists. Do not clear an error merely because the panel re-renders.
Locale switches preserve commands/history and consume no randomness. Canonical
face strings and official team names are not translated in storage; display
translations never determine outcomes. Localize the complete shootout path,
including manual validation, restored-state errors and final-result detail.

## 13. Validation rules for imported and persisted states

**Proposed implementation:** extend Cup shape validation for the optional session
map, then perform match/progression checks at the verified boundary. Keep these
checks in store/domain code, not only in the browser form.

1. Require plain JSON object/map/session/attempt shapes (no arrays where maps or
   objects are expected), supported versions and exact types. Numeric counters
   are safe integers; IDs are nonempty strings. Reject reserved/noncanonical
   map keys through canonical tie-key reconstruction and own-property access.
2. A session map key, its round/pair, tuple ID and leg-1 team IDs must identify
   the same real Cup pairing. `fixtureRefs` must list exactly `cup.legs` canonical
   fixtures in leg order, with unique match associations matching that pairing.
   Cross-Cup identities and home/away reversals fail.
3. Verified load requires each referenced canonical match exactly once, with
   correct teams and safe nonnegative scores; the pre-penalty aggregate must be
   tied. An unfinished session belongs to the current round, without champion,
   after all its current-round matches are complete, and is the selected pending
   tie. At most one unfinished session exists.
4. Require `firstTeam === "home"`, supported status and an attempts array. IDs
   and indices are unique and contiguous from 1. Team alternation follows index
   parity. Every face belongs to `FACES.penalty`; every kind must equal
   `rollEventKind("penalty", face)` even though legacy match normalization can
   otherwise preserve a supplied kind. A valid kind paired with the wrong face
   is invalid shootout data.
5. Replay from zero through every prefix. Enforce initial allocation, early
   termination, equal-attempt sudden-death decisions and no attempts after the
   first terminal prefix. Reject truncated/extra/misordered histories and any
   status that differs from replay. A nonterminal half-pair lead is valid.
6. A replay-complete session requires its exact final `shootouts[key]` totals and
   matching `resolveCupTie` winner. A nonterminal session forbids that map entry.
   Legacy final entries without sessions retain existing nonnegative safe integer,
   unequal-score validation, without new reachability restrictions. New writes
   must not leave a complete session half-finalized.
7. Validate all final-result entries against actual completed, tied pairings,
   including current-round and archived entries not yet consumed by bracket
   reconstruction. Existing progression validation alone can overlook unused
   current-round or extraneous map entries. Reject orphan keys and conflicting
   explicit shootout maps. Do not silently delete suspect legacy data; preserve
   it and surface the reason when it cannot be admitted safely.
8. Run `validateCupProgression` with canonical final totals. Later-round fixtures
   and champion must be exactly reproducible from earlier decisions. Completed
   histories in older rounds are allowed; unfinished histories in older rounds
   or a champion while history remains unfinished are not.
9. Import/load/export/save must preserve existing unrelated fields. Failed
   verification/import never replaces active storage, alters match history or
   normalizes a malformed result into a different winner. Document validation
   is local consistency checking, not cryptographic proof against save editing.

The raw import/export layer can validate session replay and internal identities
without external match data, but MUST NOT claim that it verified actual match
records. Active admission always uses the verified path. Preserve valid historical
Cup v1 data; unsupported or inconsistent data remains available in storage for
recovery instead of being migrated destructively.

## 14. Focused unit, persistence and browser tests

These are **future implementation requirements**, not tests added by SP-01A.
Use injected deterministic sequences and local fixtures; no remote D1 or package
installation is necessary. Test observable rules and recovery, not copies of
the reducer's implementation.

### Unit and integration coverage

- Cover all six draw slots, especially 0, the 3/6 success boundary, each miss
  boundary and a value just below 1. Reject negative, 1, NaN and infinite draws;
  assert exact random-call counts. Freeze the 3/6 matrix independently of V2.
- Verify five-attempt allocation, strict alternation, both early-win directions,
  clinches at six and seven attempts, and a recoverable equality boundary that
  MUST continue. Check initial decisions at ten attempts and ties at both 0–0
  and 5–5.
- Exercise repeated sudden-death ties, either-team winners and no decision at
  an odd total attempt count after ten. No arbitrary maximum or fallback winner.
- Replay a known history and JSON round trip identically. Reject wrong team,
  identity, revision, face/kind combination, extra terminal attempts and session
  status. Assert immutable inputs and safe counter handling.
- Exercise single-leg ties, both legs required, reverse venue mapping, decisive
  aggregates without penalties and tied aggregates from decisive individual legs.
- Cover repaired manual input: accept `4`/`3`, reject empty, negative, fractional,
  exponent-form, nonnumeric, unsafe and equal totals; retain legacy score
  reachability permissiveness. Assert session-start/manual exclusivity.
- Repeated terminal commands and two tied pairs produce exactly one result per
  tie, one next bracket and one champion. No draw or progression on rendering.
- Snapshot canonical matches, score, events, rollEvents, statistics, goalscorers,
  Manager/player totals and ordinary runtime before/after a complete shootout;
  assert deep equality. Shootout history belongs only to the Cup.

### Persistence and compatibility coverage

- Round-trip old Cup v1 documents with absent optional maps, manual results,
  progressed brackets and champions/archives without changing their data. Verify
  active sessions and completed histories survive save/load/export/import/archive.
- Inject storage failure before start, before draw, on a nonterminal attempt and
  on a terminal attempt. Assert unchanged committed bytes, no partial admission,
  locked controls and Retry save reusing the same candidate with zero new draws.
- Simulate interruption after commit but before memory/render updates. Reload at
  zero attempts, in the initial series, at ten tied, during a sudden-death half-pair
  and after the terminal write. Assert no reroll, missing attempt or double advance.
- Reject conflicting totals/history, missing canonical matches, swapped legs,
  orphan result/session keys, fabricated future fixtures, unsupported versions
  and invalid histories. Retain the original bytes on every failed load/import.
- Verify the existing regular-match recovery journal still reconciles before
  shootout admission and is never repurposed or cleared by attempt writes.
- Preserve unrelated storage and metadata. Check explicit/default verified-import
  result-map behavior, global-import blocking, lock handover and stale commands.

### Browser coverage

Use a focused single-player Cup spec under `tests/dado-goal-browser`, seeded with
canonical local matches and deterministic draws. Do not rely on random complete
matches or reuse/modify multiplayer components to reach a penalty scenario.

- Play a tied Cup elimination from Start through its saved winner, with visible
  alternating team names, separate regular/aggregate/shootout scores and history.
- Cover an early finish and sudden death, including the temporary lead after the
  first sudden-death attempt. Verify the next bracket and final champion.
- Refresh mid-series, mid sudden-death pair and immediately after the terminal
  commit; compare localStorage history/revision and bracket, not just page text.
- Rapid double-click/keyboard activation and duplicate command delivery must not
  consume a second draw or skip a turn. Retry after failed storage must not reroll.
- Test all shootout states and errors in IT and EN, locale switching during a
  session, manual input regression, keyboard access and status announcements.
- Test malformed/unsupported saved Cup visibility without replacement, blocked
  global import, two-tab ownership transfer and unavailable-lock messaging.

Existing focused baseline command (already run during SP-01A):

```bash
node --test web/dado-goal/cup.test.mjs web/dado-goal/cup-store.test.mjs web/dado-goal/cup-view.test.mjs
```

Observed baseline: **18 passed, 0 failed**. These tests do not establish playable
shootout correctness: current coverage has no attempt reducer, terminal atomic
commit, playable reload or manual-form regex regression. Future implementation
must add that coverage and run the focused Cup unit/store/view suite plus the
single-player Cup browser spec using available local tooling. Include relevant
engine/event/statistics regressions when their integration is affected. Do not
run the combined release smoke script merely for this task: it also exercises
the separate multiplayer environment.

## 15. Explicit acceptance and closure gates

### SP-01A contract gate

- [x] Applicable instructions searched and existing Cup, persistence, dice,
  event/statistics contracts and relevant implementations inspected.
- [x] Approved gameplay distinguished from implementation choices; all fifteen
  required subjects covered with concrete interfaces, state and recovery rules.
- [x] Numeric regex defect confirmed precisely; correct recovery regex
  distinguished; manual recording/storage behavior and additive schema feasibility
  documented with known compatibility limits.
- [x] Existing focused Cup baseline passes (18/18); no feature/test/changelog or
  multiplayer edits are part of this documentation task.

SP-01A closes when this single-file contract is delivered and its scope/format
checks pass. That closure does not mean SP-01 gameplay has been implemented.

### SP-01 implementation closure gates — pending

- [ ] Small single-player reducer and coordinator implement the approved rules
  with exactly one canonical v1 green draw per accepted new attempt.
- [ ] The additive session schema and all import/save/load validation preserve
  valid Cup v1 and legacy manual results, with localized non-destructive recovery.
- [ ] Each attempt commits atomically; terminal history, totals and ready Cup
  advancement commit together. Retries/reloads/duplicate commands cannot reroll,
  double advance, swap team orientation or alter any match/statistics stream.
- [ ] IT/EN playable and manual flows work, including exclusive editing, errors,
  save retry, accessibility and separate scores. Known global-backup omission is
  disclosed and unsafe global import is blocked while a Cup is stored.
- [ ] Focused unit, persistence and single-player browser scenarios in §14 pass
  against the final implementation with evidence of draw counts, failure recovery,
  single/two-leg winners and unchanged match statistics.
- [ ] Local operational road test plays both a single-leg and two-legged Cup,
  exercises reload and sudden death, and inspects history and bracket outcomes.
  Scope review confirms no M035-B, shared multiplayer or unrelated local changes.
- [ ] Implementation evidence and remaining limitations are reported. Any later
  changelog/release work follows its own step; no deployment or release is implied
  by either SP-01A delivery or passing local tests.

## Implementation scope

The minimal follow-up changes are one pure single-player shootout module,
optional session validation/finalization in `cup-store.mjs`, the single-player
Cup panel/controller, localized labels/errors and focused Cup tests. Extend
`cup-view.mjs` only to expose separate shootout presentation if needed. Reuse
`cup.mjs` progression and `engine.mjs` dice without broad engine changes. Add
only the narrow startup-error/global-import guards required to preserve Cup data.
Do not synchronize changes into multiplayer/public mirrors as an incidental step.

## Identified risks and limits

- The confirmed numeric regex makes today's manual form unusable; fixing just
  that form does not deliver playable shootouts.
- Generic schema v1 cloning preserves session fields but does not validate them.
  New validation is essential; old clients cannot safely edit active sessions.
- Atomic localStorage replacement covers one document only. Failed, unconfirmed
  draws are retained only in memory; local browser deletion is not recoverable.
  Archive cleanup remains a separate legacy multi-key lifecycle.
- A revision check alone cannot prevent concurrent tabs from overwriting state.
  Exclusive ownership adds a browser capability requirement; unsupported browsers
  must show the defined error. Older app tabs and external editors are outside
  the supported concurrent-write model.
- Global saves currently omit Cup state and can replace its canonical match
  inputs. Disclosure and import blocking are required until a separate portable
  Cup backup change is designed and qualified.
- Current load errors are hidden by setting `cupState` to null. Leaving that
  behavior unchanged could enable accidental overwrite of a recoverable Cup.
- Two-legged home/away reversal, odd sudden-death prefixes and the 5/6 M034
  candidate are specific sources of incorrect winners/probabilities; regression
  tests must pin the v1 rules and tie-relative identities.
- Sudden death has no sporting limit. A pathological long history may exhaust
  storage; pause and retry without changing the winner rule or discarding history.

## 16. SP-01C-FINAL implementation inspection — 2026-11-21

The current reducer, persistence coordinator and SP-01C1 app integration were
reviewed against this contract by source inspection. The implementation covers
single- and two-legged eligibility, alternating v1 attempts, initial clinches,
paired sudden death, verified session persistence, retained save candidates,
exclusive Web Lock ownership, terminal admission and ordinary Cup advancement.
Legacy results and the numeric manual-input correction remain in place. Startup
errors preserve stored bytes and block Cup creation; global import remains
blocked while a Cup is stored, with the backup omission disclosed in IT/EN.

This final integration pass completes keyboard auto-repeat suppression, explicit
attempt counts, localized Cup progression/archive messages, committed outcome
announcements across consecutive tied pairs, and accessible manual-result success
feedback. Regular-match association recovery now exposes failures for retry,
clears the active fixture after successful reconciliation, and blocks unresolved
canonical associations. No shootout path writes regular match scores or statistics.

No tests, game execution or browser sessions were run in this pass. Only source
inspection, syntax-only checks and diff/whitespace checks are authorized. The
previously reported 61 passing tests are historical evidence, not qualification
of these final edits. **Runtime/browser qualification: PENDING.** The unit,
persistence, browser and operational acceptance gates above remain open until
the separate final-code test campaign supplies evidence; this inspection does
not close the full SP-01 qualification milestone.
