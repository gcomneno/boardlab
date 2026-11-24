# Dado GOAL! — SP-02C: Global Save Cup + Archive contract

Status: normative technical contract for SP-02; documentation only.
Authority: [issue #131](https://github.com/gcomneno/boardlab/issues/131).
Baseline: `76a8400f00f5be3fc38ae85fb8977624e0578be7`; SP02_B design audit PASS.
Inspected on 2026-11-22. No applicable `AGENTS.md` was found in the repository,
its ancestor directories, or the destination path. Unrelated working-tree edits
are outside this contract's evidence baseline and MUST be preserved.

MUST/MUST NOT are acceptance requirements; implementation interfaces and internal
keys introduced here are requirements for subsequent work, not existing APIs.
This document alone neither implements SP-02 nor qualifies its runtime behavior.

## 1. Authority and existing V1 boundaries

| Baseline source | Authority and boundary |
| --- | --- |
| [app.mjs](../../../web/dado-goal/app.mjs), `globalSaveDocument`, `validateGlobalSave`, `restoreGlobalRuntime`, `importGlobalSave` | Global Save `schemaVersion: 1`; configuration, locale, persistent management/Manager/championship and ordinary-match runtime. Cup, archive and Cup recovery are omitted. Existing rollback is an in-process best effort, not crash recovery. |
| [team-management-store.mjs](../../../web/dado-goal/team-management-store.mjs) | `management.matches` is the general canonical match history; management V1 import alone validates arrays, not the complete reference graph. |
| [manager-store.mjs](../../../web/dado-goal/manager-store.mjs), [manager-next-match.mjs](../../../web/dado-goal/manager-next-match.mjs) | Manager V1 contains club, career management and optional `nextMatch`; career history is a projection, not another fixture owner. |
| [tournament-store.mjs](../../../web/dado-goal/tournament-store.mjs), [tournament.mjs](../../../web/dado-goal/tournament.mjs) | Championship V1 canonical schedule and fixture associations; regular-match recovery uses `{ championshipId, fixtureId, matchId }`. |
| [cup-store.mjs](../../../web/dado-goal/cup-store.mjs), [cup.mjs](../../../web/dado-goal/cup.mjs) | Cup `schemaVersion: 1`, current document, completed archive, regular-match recovery `{ cupId, fixtureId, matchId }`, verified progression and session validation. Archive append and current removal are separate baseline writes. |
| [cup-shootout.mjs](../../../web/dado-goal/cup-shootout.mjs), [SP-01 contract](cup-shootout-contract.md) | Deterministic session replay, stable attempt identities, retained failed-save candidate, exclusive Cup ownership and one-document attempt commit. Historical pending qualification notes in the SP-01 document do not override this mission's qualified baseline. |
| [roll-event.mjs](../../../web/dado-goal/roll-event.mjs), [event/statistics contract](extended-event-statistics-contract.md), [Extended v1 rules](extended-rules-v1.md) | Historical roll normalization, canonical match statistics and Extended v1 penalty probability 3/6. |

SP-02 retains Global Save V1 and Cup V1. It extends the portable snapshot and
replaces the global import and archive persistence protocol. It MUST NOT change
sporting outcomes, canonical schedules or existing SP-01 draw semantics.
Cup shape-only import is insufficient: the complete snapshot requires verified
Cup progression, shootouts, canonical references and cross-document validation.

## 2. Additive portable schema and compatibility

New exports MUST include all existing supported V1 fields plus:

| Field | Required value in a new export |
| --- | --- |
| `persistent.cup` | Fully verified current Cup V1 document, including a completed but unarchived Cup, or `null`. |
| `persistent.cupArchive` | Array of fully verified completed Cup V1 documents, possibly `[]`, retaining archive order. |
| `runtime.competitionRef` | Typed fixture reference below, or `null` for ordinary/unassociated runtime. |

The exact reference alternatives are:

```json
{ "kind": "cup", "cupId": "cup-123", "fixtureId": "r1-p1-l1" }
```

```json
{ "kind": "tournament", "championshipId": "championship-123", "fixtureId": "m1-f1" }
```

These are schema fragments; fixture strings MUST resolve to the owning document's
actual canonical fixture. The tournament example does not prescribe its ID format.
No other `kind`, missing identity, empty ID or ambiguous owner is allowed.
`runtime.activeTournamentFixtureId` remains a compatibility projection; when
non-null it MUST equal `competitionRef.fixtureId`. It does not identify the owner.
The new reference MUST resolve to current Cup or championship, never an archive.

Presence is determined by own-property existence, never truthiness:

| Cup field presence | Reader decision |
| --- | --- |
| Both absent | Legacy Global Save V1. Apply the legacy rules below. |
| Both present | Complete snapshot; validate both, even when values are `null` and `[]`. |
| Exactly one present | Reject before mutation. |
| Both present but wrong type, unsupported version or malformed content | Reject; never downgrade to legacy. |

New exports always include `competitionRef`. Missing reference in an older V1
snapshot is normalized on a copy: a non-null legacy fixture ID may infer a
`tournament` reference only when the incoming championship uniquely resolves it
and runtime agrees. A legacy snapshot cannot imply a Cup reference. An explicit
`null` with a non-null fixture ID is contradictory and rejected. Missing reference
in a complete snapshot may infer a uniquely valid current owner; ambiguity or an
unverifiable runtime is rejected, never guessed from UI selection or team names.

Legacy import MUST block if a current Cup exists in memory or storage, including
a completed unarchived Cup. A load error or unreadable current key is blocking,
not evidence of absence. A local archive alone may remain only if every entry,
identity and association verifies against the incoming canonical match history.
The retained archive is included in the prepared destination and is not merged
with incoming Cup data. Invalid local Cup/archive or unresolved regular recovery
MUST NOT be deleted to make a legacy import admissible.

For a complete import, explicit `null` and `[]` replace existing current Cup and
archive. The operation is snapshot replacement, never append, merge or replay.
Valid local state may be replaced only after ownership checks; malformed local
Cup/archive and unresolved journals block replacement rather than being erased.
Reimport yields the same domain snapshot: no duplicate match, Cup, archive entry,
shootout or bracket advance. Transaction IDs and fencing epochs are internal and
may change without violating domain idempotence.

Preserve existing compatibility defaults: absent `configuration.randomHands`
means `false`; absent `manager.nextMatch` means `null`; historical runtime
`rollEvents` pass through `normalizeRollEvents` before commit. Existing semantic
`kind` remains authoritative where that normalizer permits it; do not derive a
new outcome from a translated face. Absent `shootoutSessions` means legacy Cup
history, not fabricated attempts. Preserve valid optional Cup metadata and IDs.
Malformed present values MUST NOT be silently defaulted.
Compatibility is new reader → old backups. Old application versions are not safe
writers for the additive snapshot or concurrently shared storage.

## 3. Pure complete preparation before mutation

The planned [global-save.mjs](../../../web/dado-goal/global-save.mjs) boundary
MUST parse, normalize and validate detached data without storage writes, DOM,
randomness, clock-dependent identities, timers, network access or mutation of
caller input. Its result is an immutable prepared domain/runtime plan, or an
error. Storage and browser capabilities are coordinator concerns.

Preparation MUST validate the entire envelope, supported versions, locale and
configuration values, management and Manager documents, championship, current
Cup, every archive entry, all shootout sessions and all runtime consumed during
hydration. Validate nested shapes, two-seat arrays, state phases, safe numeric
ranges, booleans, event streams, rosters, referee `yellowed` arrays and
extraordinary-state arrays before converting arrays to Sets on detached copies.
Controls and catalogue resolution MUST have deterministic accepted values; a
valid historical snapshot MUST NOT require a network fetch to restore.
A null match state prepares the existing idle state without drawing randomness.

The prepared runtime MUST include normalized events, compatibility projections,
validated fixture ownership and all hydration inputs. Moving validation into
post-write rendering is forbidden. Existing shallow store validators remain
useful boundaries but do not discharge this complete validation requirement.

After file reading and pure preparation, acquire the gate, run priority recovery,
read authoritative local state, and recheck generation, ownership, pending draws,
legacy retention and reference assumptions. If inputs changed, reprepare on
copies or reject stale work. Serialize every intended destination value and the
journal before the first transaction write. Serialization errors leave domain
storage and live UI unchanged. No timer, render, match recording, random draw or
async simulator may run between verification and commit.

## 4. Canonical match references and ownership

Build an exact-ID index from incoming `persistent.management.matches`. Reject
empty or duplicate `matchId` values before building a Map; identical duplicate
records are still invalid. Never search by score, timestamp, order or team names.
Do not use pre-import local matches to repair a portable snapshot.

Every non-null fixture `matchId` in championship, current Cup and archives MUST
resolve exactly once to a completed canonical record. Verify home/away provider
team IDs in fixture orientation and nonnegative safe integer regular scores at
`record.score.home` / `record.score.away`. Check aggregate arithmetic for overflow,
leg ordering, canonical schedule, round progression, result and champion.
Unplayed fixtures have `matchId: null`; no dangling or speculative association.

Cup fixture IDs remain `r{round}-p{pair}-l{leg}`, tie keys `r{round}-p{pair}`.
Validate one- and two-legged Cups, 4/8/16/32 participants and reversed return-leg
orientation through existing Cup authority. Preserve IDs, fixture order, team
snapshots, regular results, metadata, events and sessions exactly except documented
legacy normalization. Validation may reconstruct a comparison bracket, but import
MUST NOT regenerate IDs, create a new schedule or advance the saved bracket.

Across current Cup and archive, `cupId` values MUST be unique; current/archive
identity collisions and duplicate archived identities are rejected. A canonical
match may belong to at most one fixture owner across championship, current Cup
and archive. A shootout's `fixtureRefs` points back to that same ownership, not a
second claim. Manager history may mirror a canonical match for career accounting;
matching IDs MUST agree on sporting identity/results and MUST NOT be appended a
second time or treated as separate competition ownership. Career-only historical
records do not supply missing canonical fixture matches.

Current championship and current Cup MUST NOT coexist in an incompatible active
configuration: the baseline has one current competition slot, so simultaneous
non-null current documents are rejected. A Manager career may coexist with a
compatible competition. Configuration and runtime teams MUST agree with the
selected fixture; recorded runtime must agree with its canonical finished record,
and unrecorded runtime cannot claim an already completed fixture. An idle or
ordinary match uses a null reference. A pending shootout is Cup state, not an
ordinary runtime match that may be recorded again after restore.

## 5. SP-01 sessions and zero replay draws

Retain `shootoutSessions`, `shootouts`, rules/version identifiers, `fixtureRefs`,
`shootoutId = JSON.stringify([cupId, tieKey])` and contiguous
`attemptId = JSON.stringify([cupId, tieKey, index])` unchanged.
Validate every session, including completed earlier rounds and archived Cups,
against canonical matches and replayed status/results. Reject invalid face/kind,
team order, revision, future attempt, impossible prefix, multiple unfinished
sessions, unknown rules/version or mismatched final `shootouts` totals.
Manual legacy results without sessions stay manual; do not invent a history.

SP-01 remains five attempts each with early clinch, alternating home/away and
paired sudden death. Shootout goals never enter regular scores, roll streams,
statistics, scorers, possession, Manager totals or championship standings.
Import, export, validation, replay, recovery, hydration and reload consume exactly
zero random draws. A newly authorized attempt consumes exactly one canonical
Extended v1 green-die draw, conversion probability 3/6. Invalid, duplicate and
retry commands consume zero new draws.

A failed-save prepared attempt remains the SP-01 in-memory candidate. While it
exists, import/export and destructive transitions are blocked; retry persists
the same face with no redraw. Stale generation or changed predecessor blocks
retry and requires explicit reconciliation; never silently discard or overwrite
that candidate to permit import. Crash recovery only promises persisted attempts;
an unpersisted in-memory draw cannot survive browser process loss.

Each successful attempt, including terminal session, final totals and resulting
Cup advancement, remains a single verified Cup-document `setItem`. SP-02 MUST NOT
add a global multi-key transaction or separately persisted outcome per attempt.

## 6. Complete export and import

Export takes the shared persistence gate and captures a coherent persistent and
runtime snapshot. Read every relevant document; do not trust stale UI caches.
Validate current and archived Cups, Manager, championship and references together.
Resolve recoverable regular Cup/championship association journals on detached
copies using exact match ID, matching owner and fixture teams. A matching already
linked record is a no-op; conflicting/missing matches, competing ownership or an
unresolved journal reject export. Do not infer a different match to make it pass.
Export MUST NOT write these reconciled copies back or clear source journals.

A pending global transaction is different: priority recovery MUST finish before
snapshot capture. If that prerequisite cannot complete, export is blocked. The
subsequent export projection is read-only. The portable JSON contains domain
state and runtime, never regular recovery journals, global transaction journal,
lock ownership, generation, failed-save candidates, timers or animation state.
Validate the final portable projection through the same pure reader before
serializing it; no partial download on validation failure.

Import prepares the complete destination, serializes it, commits through §7–8,
then hydrates the UI from the accepted plan. Clear regular recovery keys only as
part of the journaled destination after their local state has been resolved and
validated. Full import replaces all included single-player state, including
clearing absent current Cup/Manager/championship via explicit null semantics.
Legacy archive retention is the only local-data exception defined in §2.

Hydration MUST perform no persistence writes or new draws, rebuild Sets from the
prepared arrays, restore the typed fixture projection, refresh Cup/archive,
Manager and championship views, and cancel automatic rolls/delays. No automatic
activity resumes merely because a file was imported. Postcommit render failure
reports a committed-save/UI error and permits rehydration/reload; it MUST NOT
roll back the persistent decision or report that no import occurred.

## 7. Recoverable before/after journal

The planned [global-save-transaction.mjs](../../../web/dado-goal/global-save-transaction.mjs)
coordinator owns `dado-goal.global-save-transaction.v1`. localStorage has **no
native multi-key atomic transaction**. Only an individual key replacement has
its storage API atomicity; physical mixed keys can exist during interruption.
The journal plus mandatory gate/recovery makes such a mixture inadmissible to
cooperating readers and writers. It is not filesystem or database durability.

The transaction's explicit key allowlist is:

| Key | Role |
| --- | --- |
| `dado-goal.team-management.v1` | Canonical management history and ratings. |
| `dado-goal.manager.v1` | Career and its management projection. |
| `dado-goal.tournament.v1` | Current championship. |
| `dado-goal.tournament-recovery.v1` | Internal ordinary championship association journal. |
| `dado-goal.cup.v1` | Current Cup. |
| `dado-goal.cup-archive.v1` | Completed Cup archive. |
| `dado-goal.cup-recovery.v1` | Internal ordinary Cup association journal. |
| `dado-goal.global-save-generation.v1` | Internal replacement fencing epoch. |

The generation key is new. The transaction journal cannot name itself as a
destination. Multiplayer keys, including `dado-goal.multiplayer-seat.v1`, locale
preferences outside this snapshot, unrelated app keys and arbitrary imported
key names are excluded. Configuration/runtime remain portable and in-memory;
this contract introduces no automatic durable ordinary-runtime key.

Journal V1 MUST contain `schemaVersion: 1`, a nonempty transaction ID, operation
(`import`, `archive` or coordinated `reset`), `phase` (`prepared` or `committed`),
and complete `before` / `after` maps for exactly the allowlist. Every image is
an exact raw string or `null` meaning key absence. Preserve whitespace and bytes
in before-images; never reconstruct them from parsed objects. Unchanged keys
have identical images. Reject missing/extra keys, unknown versions/operations,
invalid phases, malformed images, impossible generation transitions or invalid
intended domain state. Do not let portable JSON provide a journal or storage map.

Before-images come from the gated, validated local snapshot; after-images come
from the fully serialized prepared plan. Initial absent generation denotes epoch
0; a transaction stores the next nonnegative safe integer epoch. Overflow blocks
the operation. Local malformed data blocks before journal preparation; no repair
by replacing it with null. Historical recovery metadata in an image must itself
be valid and recoverable, not an excuse to import unresolved state.

## 8. Commit and recovery state machine

Under the exclusive global gate, after all preconditions and serialization:

1. Write and read back the complete `prepared` journal. No domain write precedes
   this verified write. If creation fails, leave every allowlisted key unchanged.
2. Apply `after` images in allowlist order (`null` uses `removeItem`), then read
   back every image and verify the complete intended domain snapshot.
3. Replace the same journal with `phase: committed`, preserving transaction ID
   and images, and verify it. This is the durable logical commit decision.
4. Remove the journal only after all after-images and committed decision verify.
   Hydrate/announce success after the decision; cleanup failure is recoverable
   committed state and MUST NOT cause rollback or a second logical operation.

| Persisted state observed under gate | Required recovery |
| --- | --- |
| No journal | Validate/load normal state; no transaction to replay. |
| Valid `prepared`, including all after-images already written | Restore every exact before-image, verify all, then remove journal. No commit decision exists. |
| Valid `committed`, including partially restored/missing after-images | Reapply every after-image, verify all and semantic validity, then remove journal. Never choose before-images. |
| Malformed, unsupported or ambiguous journal | Fail closed without any recovery write or deletion; preserve bytes for diagnosis. |
| Valid journal but any recovery read/write/verification/cleanup fails | Fail closed; retain journal and images, block loaders and operations, permit retry. |

Recovery MUST run before ordinary loaders, commands, export capture and every
writer, including on reload and bfcache restoration. It uses the same gate and
ownership discipline. It does not depend on initialized UI or random sources.
Validate the entire journal and selected target before any recovery write; do
not require the physically interrupted live keys to form a valid domain snapshot.
A partially applied recovery is not admitted. Repeating recovery any number of
times produces the selected image with no duplicate append, match or advancement.

On an exception before a verified committed marker, reread the journal under the
gate: prepared selects before, committed selects after. A write that succeeded
but whose readback failed is an uncertain result, not proof of rollback. If the
decision cannot be established, retain evidence and block. Do not overwrite a
journal with another transaction, clear it in a generic catch, or use in-memory
rollback as a substitute for this state machine. Recovery failures preserve the
journal; already applied valid image writes remain recoverable, not silently
rewritten to guessed defaults.

## 9. Archive atomic lifecycle

Archiving requires the current verified completed Cup, all canonical matches,
no pending attempt, resolved association recovery and valid archive. Hold the
global gate and required Cup ownership. Prepare one transaction with archive
append, current Cup removal, Cup recovery removal and new generation. Other
allowlisted keys retain their exact images. Session history and metadata survive.

A prepared interruption restores the completed current Cup and original archive;
a committed interruption yields exactly one archived Cup and no current Cup or
Cup journal. Never expose an archived/current duplicate as admitted state.
Normal duplicate archive IDs remain errors. A retry of the same completed
transaction may report already archived only when identity and full document
agree and current Cup is absent; it MUST NOT append again. A mismatched duplicate
is corruption, not a successful retry. Global recovery precedes any retry test.
Archive validation and transaction use no random draws or extra bracket advance.

## 10. Multi-tab gate, lock ordering and generation

Use one origin-scoped exclusive Web Lock named `dado-goal.single-player.persistence`
for all affected persistence operations and consistent snapshot reads. It covers
ordinary match completion, management and Manager changes, championship creation,
progression and recovery, all Cup writers, shootout retry, import, export capture,
global recovery, archive and reset. A raw store call bypassing the gate is not a
supported app writer. Keep regular association-journal protocols under the gate.

Retain SP-01 editor-lifetime `dado-goal.cup:${cupId}` ownership. Creation also
respects the existing `dado-goal.cup.create` namespace. Lock ordering is explicit:
a Cup owner may wait for the global gate while retaining its lifetime ownership;
a global-gate holder MUST NEVER wait for another tab's Cup ownership. It may
reuse proven ownership or request required external ownership with
`ifAvailable: true`; unavailable means release temporary locks/gate and report
busy. For several identities, request in deterministic sorted order, all
nonblocking. Cup creation locks follow the same nonblocking rule under the gate.
Never reacquire one's own non-reentrant lock. This prevents the owner-waits-gate /
gate-waits-owner cycle. External Cup ownership blocks snapshot replacement,
archive, recovery touching that Cup and destructive reset.

Persist and compare the generation epoch under the gate before every affected
write and **before drawing** the next shootout face. Import, archive and reset
advance it in their transaction. Ordinary single-document updates compare the
epoch plus authoritative document identity/revision or exact predecessor bytes;
they need not introduce a second key write per SP-01 attempt. An epoch alone is
not a document revision. A stale tab MUST reload/revalidate before a new command
and MUST NOT flush pre-import Manager, match, tournament or Cup state. Successful
replacement invalidates old coordinators, queued commands and cached snapshots.
Recovery selects the journal's epoch image with the same before/after decision;
no command is admitted during that selection.

Storage events invalidate views, dirty snapshot tokens and caches; handle clear
notifications as well as individual keys. On focus/pageshow/bfcache resume,
recheck recovery and generation. Events are advisory, never the correctness
barrier: recheck at the actual write slot even if no event arrived. No required
Web Locks support means fail closed for transactions and affected concurrent
writers/export capture; do not substitute a localStorage flag, timeout lease,
revision-only test or best-effort unlocked write.

## 11. Fail closed, quota, dirty state and reset

Malformed Cup/archive, failed storage reads, unresolved journals, invalid runtime,
missing lock support, foreign ownership and generation conflicts leave the app
blocked for the affected operation with actionable IT/EN status. Stored errors
are not converted to empty Cup state. Preserve diagnostic bytes; do not clear
storage, truncate sessions or delete archive history automatically.

Quota/security failures during serialization or prepared-journal storage MUST
precede domain mutation. Journal space includes both images and may exceed the
snapshot size. A later quota failure follows the persisted decision: prepared
rolls back, committed rolls forward. If quota also prevents recovery, retain the
journal and fail closed until retry can complete; never claim immediate recovery
is guaranteed under exhausted storage. Failed journal phase replacement leaves
the existing decision authoritative. Failed cleanup keeps a recoverable journal.

Dirty-state tracks meaningful differences in the exportable snapshot: Cup create,
association, session start/attempt/finalization, progression, archive, Manager,
championship, ordinary match/runtime and configuration edits. Duplicate/no-op
commands and failed writes do not create false dirty-state; a failed attempt is
instead an explicit pending-save blocker. A persisted successful mutation stays
dirty even if rendering subsequently fails. Recovery cannot imply a backup was
exported and MUST invalidate any obsolete clean token.

An export captures generation plus a local snapshot revision/token. Clear dirty
only after successful serialization/download initiation and only if the live
snapshot still equals that captured token; a newer change remains dirty. Export
failure never clears dirty. A successful import may establish its hydrated
snapshot as clean; subsequent changes are dirty. Download initiation cannot
prove the user saved bytes to disk, and the UI MUST not imply that guarantee.

Reset MUST use the same gate, fencing, ownership and recovery preconditions. It
MUST NOT delete canonical histories while leaving current/archived Cups orphaned.
A reset retaining Cups must retain all their canonical inputs; a full single-player
reset must remove dependencies together through a journaled valid empty snapshot.
Do not remove the recovery journal to bypass an error. Multiplayer storage is
outside this contract's reset/transaction allowlist; the baseline reset list's
multiplayer-seat entry is not authority to include it in a global-save transaction.

## 12. Regression and fault-injection matrix

Each row is required evidence for the later implementation/qualification gates.
Assertions include exact preserved IDs/content, zero replay draws and unchanged
regular statistics where applicable, not merely a successful UI notification.

| Scenario | Required observation |
| --- | --- |
| Legacy V1, absent `randomHands`, absent Manager `nextMatch`, old rolls without `kind` | Defined defaults/normalization; ordinary runtime, Manager and championship round-trip. |
| Both Cup fields absent / present / partial; malformed null/array/version | Correct legacy/complete routing; partial/malformed rejected before any write or UI mutation. |
| Legacy with current Cup, archive-only, malformed local Cup/archive, unresolved journals | Current blocks; archive-only retained only against incoming history; no silent deletion. |
| Current active/completed Cup, archived Cup, one/two legs, 4/8/16/32 teams | Portable round-trip preserves complete snapshot, metadata, fixture orientation, champion and archive order. |
| Missing/duplicate match ID, wrong teams/score, duplicate fixture ownership, Cup/archive collision | Complete graph rejected before transaction creation; no matching by score or name. |
| Typed reference missing/inferable/ambiguous/wrong kind/wrong owner; recorded and unrecorded runtime | Only valid compatibility inference; restored fixture cannot be recorded twice. |
| Incompatible current Cup/championship; Manager career mirrors | Reject conflicting active slot; preserve legitimate career projection without duplicate accounting. |
| Shootout 0, 5, 10 attempts; odd/even sudden death; early clinch; complete earlier round; legacy manual result | Replay exact sessions and results with random spy count zero; no invented attempts or statistics. |
| Corrupt session kind/face/ID/rules/fixtureRefs/status/final result | Reject current and archive uniformly; no partial normalization into valid results. |
| Pending failed-save attempt; duplicate confirmation and retry | Import/export blocked; candidate retained; retry/duplicate consume zero additional draws. |
| Reimport same complete snapshot; repeated archive/recovery | Same domain state, one association/entry, no bracket advance or random draw. |
| Export regular Cup/championship journal, exact linked/missing/conflicting record | Reconcile only recoverable detached copies; source bytes/journals unchanged. |
| Inject before/after prepared journal write and readback | No domain mutation without durable valid prepared journal; ambiguous decision blocks. |
| Inject before/after every allowlisted set/remove and verification | Reload selects before when prepared, after when committed; no mixed admitted state. |
| Inject before/after committed marker replacement and readback | Persisted phase alone decides; no rollback after committed. |
| Inject before/after cleanup, then repeat interrupted recovery at every write | Cleanup retriable; selected images stable; no duplicate action or premature loader. |
| Malformed/truncated/unknown-version journal, extra key, invalid image/generation | Fail closed without guessed recovery or journal deletion. |
| Quota at journal creation, destination write, phase marker, rollback/roll-forward and cleanup | Preserve decision/evidence, correct blocker and retry; no false success or dirty reset. |
| Crash during archive append/current removal/Cup-journal removal | Before restores original current+archive; after yields one archive and no current/journal. |
| Two tabs: import versus Cup attempt, Manager, match, championship, archive, another import and reset | Gate serializes operations; foreign ownership blocks; stale epoch/predecessor never overwrites. |
| Delayed/lost storage event, queued command, bfcache, unsupported Web Locks | Write-slot fencing or safe blocker; no unlocked fallback/deadlock. |
| Dirty export with concurrent newer change; no-op/failure; import render failure | Only exported token becomes clean; failed/no-op writes do not manufacture dirty; committed data survives UI failure. |
| Reset with current/archive dependencies, corrupt state or pending recovery | No orphan history; coherent journaled removal or blocked reset. |
| Ordinary match, Manager, championship, SP-01 rules and statistics; multiplayer bytes | Existing behavior/regressions pass; no multiplayer key touched. |

Fault injection MUST cover exceptions and simulated process termination before
and after every storage boundary, including recovery itself, not just ordinary
catch/rollback behavior. Assert raw keys and journal phase after reload, and
assert loaders/commands remain disabled until recovery verification completes.

## 13. SP-02C/D/E/F acceptance gates

| Gate | Required deliverable and PASS evidence |
| --- | --- |
| `SP02_C_CONTRACT_GATE` | This one formal contract covers every issue #131 decision, source terminology, native localStorage limitation, recovery, ownership, compatibility and matrix; only this file changed by the contract task; content, scope and whitespace verified. |
| SP-02C — contract and pure model | Contract gate plus complete pure V1/legacy preparation and Node cases for runtime, canonical references, archive and session validation. Contract-only PASS does not close the whole SP-02C milestone. |
| SP-02D — recoverable transaction | Allowlisted before/after journal, priority idempotent recovery, generation fencing and atomic archive lifecycle; fault injection at every preparation/write/decision/cleanup/recovery boundary proves before-or-after admission. |
| SP-02E — application integration | Complete import/export and write-free hydration; all affected writers use shared gate and ownership rules; two-tab fencing, pending-draw blocking, reset/dirty invariants, save indicator and IT/EN errors; no SP-01 or other-mode regression. |
| SP-02F — qualification | Focused Node suite and real single-player browser PASS for all matrix rows, shootout checkpoints and zero replay draws; diff/scope audit and selective staging in a separately authorized implementation workflow; commit/push distinct from public deploy. Combined multiplayer smoke is not mandatory. |

Final implementation criterion: no interrupted import or archive operation can
make mixed persistent state admissible; every verifiable persisted Cup remains
recoverable, and qualified SP-01 regressions remain PASS. Gates require actual
evidence; this document makes no runtime-test or deployment claim.

## 14. Out of scope and known limitations

No new football mechanics, probability changes, extra time, away-goals rule,
shootout simulator, backend/cloud service, destructive schema migration, V2
promotion, multiplayer assets, Worker/D1 changes or implicit deployment.
The future implementation targets `global-save.mjs`,
`global-save-transaction.mjs` and focused app/store/UI/test integration described
by #131; this contract task creates or updates only this document.

The protocol protects cooperating same-origin versions using the defined locks
and gate. Old tabs, external scripts, developer tools or other writers bypassing
the protocol are outside its concurrency guarantee. Lock availability is a real
browser capability requirement; storage events alone cannot provide exclusion.

localStorage has no native multi-key atomic transaction, no cross-device backup
and no guarantee against browser deletion, origin eviction or hardware loss.
Journal recovery needs intact stored images and enough writable storage; full
quota or malformed evidence can require user intervention while remaining
blocked. A downloaded file is not confirmation of a physical disk write.
Portable runtime is restored from the file; this work does not add continuous
autosave of an ordinary match or recovery of an unpersisted shootout face.

---

## Appendix A — SP-02E2B local implementation evidence (2026-11-24)

This appendix records subsequent implementation evidence. Sections 1–14
remain the normative SP-02 contract; their original baseline and acceptance
criteria are not retroactively reclassified as completed.

Implementation tranche SP-02E2B, locally verified without staging, commit,
push or deployment:

| Increment | Local evidence |
| --- | --- |
| 7B — Ownership forwarding | Cup ownership forwarded to coordinated read/write boundaries; focused Node and browser checks PASS. |
| 7C — Dirty/export fencing | Revision and snapshot revalidation with dirty-state regression checks PASS. |
| 7D — Shootout binding | Coordinator-bound shootout write/retry checks PASS. |
| 7E — Championship startup recovery | Browser recovery and injected persistence-failure retry PASS. |
| 7F — Championship lifecycle | Creation, abandonment and stale-tab protection PASS. |
| 7G — Canonical Manager history | Focused Manager regressions and stale-history concurrency PASS. |
| 7H — Transactional reset | Browser deletion/isolation and committed-journal cleanup recovery PASS. |
| 7I — Documentation and qualification | Bilingual registry and contract appendix aligned; 550/550 Node and 14/14 integrated browser tests PASS; selective Git scope audit PASS. |

The reset uses the journaled `reset` operation for the seven single-player
domain documents, preserving multiplayer and unrelated storage keys. The
generation epoch and transaction journal remain controlled by the engine.

These are focused local qualification results, not an assertion that every
row of the §12 fault-injection matrix has been independently rerun or that
the SP-02F overall acceptance gate has passed. The final local verification reports **550/550 Node tests PASS** and **14/14 integrated browser tests PASS**, with zero failures. The selective Git audit identified mixed application changes and a divergent shared `main` branch; isolation, staging, publication and overall SP-02F closure remain separate authorization gates.
