# BoardLab — Lessons Learned

[English](README.md) | [Italiano](README.it.md)

## Purpose

This directory preserves engineering lessons demonstrated during BoardLab's
Python engine, Kotlin parity, Android application, and CI development.

These are project retrospectives, not completed learner exercises.

## Lessons

1. **Evidence before implementation.** Inspect the repository, establish
   constraints, and define observable PASS/FAIL gates before changing code.
2. **Separate domain and presentation.** Game rules and state transitions
   must remain independent of the CLI, Android UI, and platform lifecycle.
3. **Use executable parity contracts.** Shared fixtures and transition
   expectations are more reliable than visually comparing implementations.
4. **Keep strategies interchangeable.** Deterministic baselines, explicit
   budgets, and reproducible benchmarks enable meaningful AI comparisons.
5. **Validate progressively.** Static checks, unit tests, parity checks,
   platform builds, CI artifacts, and real-device testing answer different
   questions; none replaces the others.
6. **Treat CI artifacts as evidence.** Check archive integrity, application
   metadata, signing status, and provenance rather than trusting a green job.
7. **Distinguish debug from release.** A successful debug APK does not prove
   release readiness. Release Lint and unsigned release packaging require
   independent validation.
8. **Do not bypass safety checks to make builds pass.** A missing offline
   dependency is not evidence of defective source code. Diagnose dependency
   resolution before disabling release checks.
9. **Preserve project boundaries.** Keep SDKs, build outputs, private study
   sources, and publication-safe documentation in their appropriate places.
10. **Distinguish demonstrated from planned.** A successful real-device match
    demonstrates basic usability, not production readiness, accessibility,
    release signing, or comprehensive lifecycle testing.

## Verified milestones

- Python engine and strategy validation.
- Kotlin domain parity and platform-independent game controller.
- Android Compose application and successful real-device match.
- GitHub Actions debug APK and unsigned release APK.
- Release Lint and unsigned APK verification.

## Remaining production gates

Automated Android tests, release signing and key management, lifecycle and
accessibility validation, distribution policy, and publication readiness
remain separate future work.
