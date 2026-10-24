# Tre Sigilli — SHIP-03 · Android Release Readiness

Languages: [English](tre-sigilli-ship03-release-contract.md) | [Italiano](tre-sigilli-ship03-release-contract.it.md)

Status: APPROVED — scope authorized; signing key generation requires separate authorization.

## Objective

Qualify the first signed, installable, offline-verified Free Android release,
without automatic publication on the Play Store.

SHIP-02 remains the approved functional and visual baseline.

## M1 — Release baseline

- Verify applicationId, manifest, versionCode and versionName.
- Verify the `assembleRelease` CI job and its unsigned artifact.
- Verify the Android tools required for release.
- Confirm that no signing credentials or private keys are tracked.

Gate: M1_BASELINE=PASS.

## M2 — Secure local signing

- Define a key storage location outside the repository.
- Protect sensitive paths with Git rules before creating any key.
- Create or select a signing identity only after explicit authorization.
- Keep passwords out of shell history, source files, logs and GitHub Actions.
- Sign and verify the APK using official Android tools.
- Define key recovery and backup before distribution.

Gate: M2_SIGNING=PASS.

## M3 — Release validation

- Install the signed release APK on the qualified AVD.
- Verify launch, complete games, offline operation, AI and reset.
- Verify session persistence and restoration.
- Preserve the SHIP-02 Android test suite as the regression baseline.
- Safely unmount the Samsung storage after testing.

Gate: M3_RUNTIME=PASS.

## M4 — Packaging

- Prepare the qualified APK, SHA-256 checksum and artifact manifest.
- Document installation, compatibility and known limitations.
- Keep distributable artifacts separate from signing material.
- Do not publish artifacts without explicit authorization.

Gate: M4_PACKAGE=PASS.

## M5 — Final seal

- Verify build, test, signing and artifact integrity results.
- Review the scope of changes.
- Record the commit and release status.
- Push only after explicit authorization.

Gate: SHIP_03_SEAL=PASS.

## Binding exclusions

- No changes to the Free skin approved in SHIP-02.
- No changes to game rules or AI.
- No Premium features, in-app purchases, advertising or telemetry.
- No accounts, cloud services or connectivity requirements.
- No Play Store publication.
- No key generation during the documentation phase.

## Verified initial state

- applicationId: com.giadaware.tresigilli
- minSdk: 26
- targetSdk and compileSdk: 36
- versionCode: 1
- versionName: 0.1.0
- CI: existing unsigned release job
- SHIP-02 tests: 8/8 PASS
- Release signing: not yet configured
- GitHub repository: public

## Authorizations

The SHIP-03 M1–M5 scope was approved on October 24, 2026.
Planning and milestone execution are authorized, subject to their
respective gates and binding exclusions.

Signing key generation or selection requires separate authorization.
Publication and distribution require explicit authorization.
