# ChronoOops! — Mission 005

[Italiano](mission-005.it.md) | [English](mission-005.md)

## The Minute That Never Existed

## 1. Narrative premise

In 2025, the Archive of Anomalies collects evidence from
the previous four temporal incidents.

Two historical events, called SIGNAL and RESPONSE, appear
in the wrong order: the RESPONSE seems to precede the
SIGNAL that caused it.

In 1985, the reference clock had lost sixty seconds.
The real events did not change order: their recorded
timestamps became unreliable.

The final mission does not require inventing a new
chronology. It requires proving what actually happened.

## 2. Objective

A operates in 1985 and B in 2025.

A must identify the clock defect, preserve independent
evidence of the event order, and certify the provenance
of the original historical source.

B must identify the archival anomaly, reconstruct the
real order using independent evidence, and validate
the chronology.

Victory requires:

- proving the real SIGNAL → RESPONSE order;
- making the independent evidence available and verifying it;
- certifying the historical source;
- validating the reconstruction without changing events;
- completing the campaign by turn 8.

## 3. Preserved rules

- A single modifiable timeline.
- A operates in 1985; B operates in 2025.
- Alternating A/B turns, starting with A.
- Two action points (AP) per turn.
- Eight turns maximum.
- Historical effects propagate at the end of A's turn.
- Rejected actions consume no AP and do not change state.
- No new temporal engine or generalized backward time travel.

## 4. Causal model

The real sequence is unchanged:

1. The SIGNAL is emitted.
2. The RESPONSE occurs as a consequence of the SIGNAL.

The defective clock introduces a sixty-second discrepancy
into the records. The archive may therefore display the
RESPONSE before the SIGNAL.

The defect concerns time measurement, not event causality.

Independent evidence preserves the observed order without
relying on the defective clock.

Neither SIGNAL nor RESPONSE needs to be modified.

## 5. Two strategies

### A. Superficial correction

The player may attempt to edit the timestamps directly
in the 2025 archive.

This correction can make the timestamps appear ordered,
but it does not prove the real event sequence.

It creates no paradox. It produces an insufficient
reconstruction that may be reviewed.

### B. Evidence-based reconstruction

A preserves independent evidence of the
SIGNAL → RESPONSE sequence.

B can access that evidence only after propagation at
the end of the historical turn.

B compares the evidence against the original archive,
reconstructs the real order, and verifies that the two
events remain distinct and correctly causally connected.

A certifies the provenance of the historical evidence.
B validates the final reconstruction.

## 6. Revision and reversibility

A superficial correction does not block the mission.

B may recognize that it is insufficient and return to
the original records and independent evidence.

New historical evidence propagated by A invalidates
checks that depend on a previous reconstruction.

Revision neither deletes events nor creates new timelines.

Evidence that has not yet propagated cannot be
verified prematurely by B.

## 7. Nominal sequence

| Turn | Agent | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Inspect clock | Identify sixty-second loss |
| 2 | B | Consult archive | Detect apparent inversion |
| 3 | A | Observe real order | Preserve independent evidence |
| 4 | B | Compare evidence | Reconstruct real order |
| 5 | A | Certify original source | Record historical provenance |
| 6 | B | Validate chronology | Complete campaign |

The evidence propagates at the end of turn 3.

Nominal victory occurs on turn 6.

## 8. Alternative path

On turn 2, B may choose a superficial archive correction
instead of correctly identifying the anomaly.

The apparent correction is insufficient for victory.

On a later turn, B may recognize the insufficiency,
restore reliance on the original records, and complete
the evidence-based reconstruction.

The alternative path must allow victory by turn 8
when the player's actions are compatible.

Victory is not guaranteed if the player runs out of turns.

## 9. Information and evidence

The engine explicitly distinguishes:

- the real event order;
- the order suggested by defective timestamps;
- the presence of independent evidence;
- propagation of that evidence;
- verification of the evidence;
- superficial correction state;
- certified historical provenance;
- validated final reconstruction.

Reordering timestamps alone does not change the real
sequence and cannot satisfy victory requirements.

Checks must refer to evidence that is actually available.

## 10. Victory, defeat, and rejected actions

Victory jointly requires:

- propagated independent evidence;
- verified SIGNAL → RESPONSE order;
- certified historical provenance;
- validated final chronology;
- completion by turn 8.

Defeat occurs at the end of turn 8 if any requirement
remains unmet.

The following actions are rejected atomically:

- actions assigned to the wrong character;
- actions attempted without AP;
- verification of evidence that has not propagated;
- certification without an independent source;
- validation based only on edited timestamps;
- completion without validation and provenance.

A superficial correction is an ineffective choice,
not a causal paradox.

## 11. Narrative finale

When B completes validation, the Archive of Anomalies
finally preserves a coherent chronology of the events
experienced by A and B.

None of the previous four cases is rewritten or modified.

The apparent final contradiction disappears when the
difference between what happened and what the clock
recorded is understood.

Final line:

> Time had made no mistake.
> The clocks were telling tall tales.

## 12. Verification contract

The C11 implementation must verify:

- determinism and the eight-turn limit;
- at most two AP per turn;
- unchanged real event order;
- historical propagation only at the end of A's turn;
- inability to verify unpropagated evidence;
- no victory through superficial correction alone;
- atomic rejection with unchanged state;
- invalidation of obsolete checks;
- nominal victory on turn 6;
- recovery from the alternative path by turn 8;
- no regression in Missions 001–004;
- campaign completion through the unified CLI.

## 13. Implementation boundaries

- Standalone C11 engine.
- Dedicated Mission 005 API and tests.
- Text-based terminal consistent with previous missions.
- No modifications to the M001–M004 engines.
- No framework, database, or external dependency.
- No graphical interface in this phase.
- Completion of the fifth mission does not automatically
  authorize a public release.
- The completed campaign still requires an overall
  playtest and multiplatform distribution preparation.

## 14. Qualification

This document defines the candidate contract for
the campaign finale.

Qualification requires Italian/English semantic parity,
verification of causal rules, and a separate subsequent
C11 implementation.

No implementation is included in this phase.
