# Dado Goal — Remote Gateway Contract (M2)

Status: DESIGN ONLY — NOT INTERNET QUALIFIED

Tracking: https://github.com/gcomneno/boardlab/issues/27

## Goal

Enable two players on separate computers to share an authoritative
Dado Goal match through a single HTTPS browser origin.

Preserve the current Dado Goal gameplay baseline defined by `extended-rules-v1.md`, together with the lifecycle v4 transport and authority constraints.

## Architecture

Remote browsers
    |
    | HTTPS — canonical origin
    v
Qualified TLS ingress / gateway
    |
    | Explicit private upstream
    v
Loopback-only Dado Goal HTTP authority
    |
    v
SQLite lifecycle v4

The backend remains bound to 127.0.0.1.
The existing loopback Host/Origin guard must remain unchanged.
A remote gateway must not assume that changing forwarded headers
automatically makes the backend proxy-compatible.

## Trust and isolation

- Public and upstream origins have separate trust boundaries.
- The gateway must enforce an exact public Host and Origin policy.
- Forwarded headers are untrusted unless explicitly normalized.
- The upstream remains inaccessible directly from the Internet.
- Browser assets and API use the same canonical HTTPS origin.
- No wildcard CORS or bearer credentials in URLs or logs.
- Invalid configurations fail closed.
- No authentication bypass through proxy rewrites.

## Resource security

- Finite body and header limits.
- Explicit upstream deadlines and concurrency limits.
- Rate limiting with an abuse-isolation policy.
- Durable match expiry and cleanup.
- Credential confidentiality and server-side authorization.
- Defined persistence, restart and recovery guarantees.

## Acceptance gates

M2-A — Isolated gateway implementation and unit tests.
M2-B — Adversarial Host, Origin and forwarding tests.
M2-C — Local HTTPS integration with two browser sessions.
M2-D — Verify zero-cost remote connectivity feasibility.
M2-E — Explicit authorization for deployment.
M2-F — Real two-computer remote end-to-end acceptance.

Passing M2-A through M2-D does not authorize Internet exposure.

## Non-goals

- Modifying original Dado Goal rules.
- Replacing lifecycle v4.
- Changing existing localhost HTTP behavior.
- Public deployment before authorization.
- Purchasing services or introducing mandatory hosting costs.
- Automatic Git push.
