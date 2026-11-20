# M034 V2 local qualification — M034-RT06-A

Date: 20 November 2026. Status: **NOT DEPLOYED**.

## Scope and environment

This report records the completed M034 local V2 qualification from the
established regression, RT01–RT04 automated evidence and RT05 manual road test.
The local environment used a real HTTP server at `127.0.0.1:8787`, temporary
SQLite and a test-only SQLite/D1 adapter and V2 authority.

The V2 rules use five D6 dice, a one-in-forty penalty award opportunity and
five green scoring faces out of six for penalty resolution. `rulesVersion`
and `schemaVersion` are distinct; canonical V2 event history and persistent
version immutability are part of the qualification scope.

## Qualification evidence

| Stage | Established result |
| --- | --- |
| Original M034 regression | 38/38 Node tests and 9/9 SQLite tests passed. |
| RT01 | Test-only SQLite/D1 adapter qualified. |
| RT02 | Test-only V2 authority qualified with deterministic draws, authorized seats and receipt-first idempotency. |
| RT03 | Real local HTTP server; static endpoint checks returned HTTP 200; clean shutdown released port 8787. |
| RT04 | Dedicated Playwright end-to-end road test: 2 tests passed in 26.0 seconds, using two independent Chromium browser sessions, real HTTP and temporary SQLite. |

RT04 exercised a V2 match with 14 gameplay rolls, final revision 15 and a
2–0 score. It covered penalty award as a separate event, penalty goal
`faceIndex: 4`, penalty miss `faceIndex: 5`, network reconnection, idempotent
retries, canonical event replay and V1 compatibility. These are automated
results; they are not assertions of manual verification.

## Manual road-test outcome

RT05 created a local multiplayer V2 match and exercised the two-player
workflow, real team selection and invitation. The Monza versus Roma match
reached completion, confirmed by the user. Visual evidence includes a
penalty miss and match statistics. This establishes the manual outcome
without extending it to the behaviors covered only by Playwright.

## Defects and fixes

- The test authority accepts two hands per team, while the UI initially
  defaults to 20. This configuration mismatch was discovered during manual
  testing; no correction is claimed here.
- The invitation copy button remained disabled after match creation. This
  was corrected in both `multiplayer-play.mjs` copies
  (`web/dado-goal/` and `cloudflare/dado-goal/public/`).
- Clipboard feedback was absent; accessible success/failure messaging was
  added.
- A compact invitation panel and layout refinement remain a separate UI
  improvement.

## Remaining limitations

- Manual reconnection while a penalty is pending was not verified.
- Full-page reload recovery was not qualified.
- The local road test does not qualify the complete Cloudflare production
  gateway. The authority and SQLite/D1 adapter used here are test-only.

## Explicit production activation boundary

Public V1 remains active and unchanged in deployment. This qualification
includes no deployment, no remote D1 migrations and no authorization to
activate V2 in production. V2 is not publicly released; any production
activation requires separate authorization and qualification of the
production environment.

## Final verdict

**LOCAL QUALIFICATION PASS** — M034 V2 is qualified within the local scope
and evidence above, subject to the stated limitations and production
activation boundary.
