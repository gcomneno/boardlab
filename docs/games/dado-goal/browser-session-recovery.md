# Dado Goal — Optional Browser Session Recovery

Status: DESIGN CONTRACT — implementation not authorized
Tracking: https://github.com/gcomneno/boardlab/issues/27

## 1. Purpose

Allow a player to recover an authorized multiplayer seat after
refreshing or reopening the same browser.

Server-side persistence and browser-side credential retention
are independent capabilities.

No changes to Dado Goal Original rules are permitted.

## 2. Consent model

- Remembering a seat is disabled by default.
- The player must explicitly select "Ricorda questa postazione".
- No credential is persisted before consent.
- Refusing consent preserves the current in-memory behavior.
- Consent may be withdrawn using "Dimentica questa postazione".
- Consent applies to one browser origin and one local seat.
- Replacing a remembered seat requires explicit handling.
- The UI must warn about shared devices.

## 3. Proposed local storage contract

Storage candidate: same-origin localStorage.

Record fields:

- formatVersion
- matchId
- seat
- seatToken

Do not persist invitation tokens, snapshots, request receipts
or other players' credentials.

A stored seat token is a bearer credential, not an identity proof.

The storage key, schema and validation logic must be qualified
before implementation.

No cross-origin storage or synchronization is authorized.

## 4. Recovery sequence

1. Load and validate the stored record locally.
2. Reject malformed or unsupported records.
3. Request GET /api/v1/matches/{matchId}
   with Authorization: Bearer <seatToken>.
4. Verify that the returned seat matches the stored seat.
5. Restore the latest authoritative snapshot and revision.
6. Re-enable normal polling and permitted controls.
7. Never infer authoritative state from stored client data.

Invalid or rejected credentials must not restore a session.

Distinguish authentication rejection from temporary network
failure: network errors must not silently destroy valid credentials.

## 5. Forget and leave

"Forget this device" removes the locally retained credential
and clears the active browser session.

A separate decision is required on whether the existing
"Leave" action should also revoke server-side authorization.

Deleting local storage does not revoke an existing server token.

## 6. Security requirements

- Do not place seat tokens in URLs.
- Do not expose tokens in UI messages or application logs.
- Use same-origin requests and no-store responses.
- Preserve strict CSP and reject untrusted script execution.
- Browser JavaScript with origin access can read localStorage.
- Shared-device users may access retained credentials.
- HTTPS is mandatory before public Internet exposure.
- Do not claim protection against XSS or local device compromise.
- Do not implement implicit cross-device credential recovery.

Production token expiry, revocation, rate limiting and session
retention require separate qualification.

## 7. Required tests

- Default behavior does not persist credentials.
- Explicit consent persists only permitted fields.
- Refresh restores a valid seat through the server.
- Host and guest records remain isolated by browser context.
- Malformed records fail safely.
- Invalid tokens cannot recover a session.
- Wrong-seat server responses cannot recover a session.
- Temporary network failure retains recoverability.
- Forget removes the retained credential.
- Disabled or unavailable storage does not break local play.
- Invitation tokens never become seat credentials.
- Existing multiplayer backend and browser tests still pass.

## 8. Acceptance boundaries

This contract does not authorize:

- Implementing browser credential persistence.
- Public hosting or exposing the localhost HTTP server.
- Modifying original game rules.
- New paid services or accounts.
- Changing server authentication semantics.
- Closing issue #27.

Next gate: separate approval for scoped browser implementation
and security-oriented tests.
