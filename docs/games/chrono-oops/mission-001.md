# ChronoOops! — Mission 001

[English](mission-001.md) | [Italiano](mission-001.it.md)

## The Bridge That Wasn't There. Then It Was. Then It Was a Duck.

- **Project:** GiadaWare BoardLab
- **Document:** Game Design Specification M001
- **Version:** 1.0
- **Status:** design baseline; software validation not performed
- **Game title:** provisional

## 1. Identity

ChronoOops! is a cooperative, turn-based tactical RPG set in a
single mutable timeline.

Two protagonists operate in different eras of the same world.
Their decisions change events, environments, and objects, producing
observable causal consequences.

The game combines strategy, humorous science fiction, logical
paradoxes, and algorithmic education.

**Guiding principle:** the world may become absurd, but the rules
governing it must be rigorous.

## 2. Cooperation

Supported configurations:

- Two human players.
- One human player and an AI-controlled companion.

World rules are identical in both configurations.

- **A — 1985:** changes historical decisions and causes.
- **B — 2025:** observes consequences, gathers information, and acts
  in the present.

Turns alternate: A → B → A → B.

Both players pursue a shared objective. Their different causal
capabilities form part of the cooperative strategy.

## 3. Laws of Time

**L1 — Single timeline.** One mutable timeline, without
parallel universes.

**L2 — Explicit causality.** Effects depend on identifiable causes.

**L3 — Conservation of anticipated actions.** Actions borrowed
from the future generate debts that must be repaid.
This mechanic is not active in the pilot mission.

**L4 — Determinism.** Identical initial conditions and decisions
produce identical results.

**L5 — Consistency.** Causal incompatibilities follow explicit
rules rather than arbitrary effects.

**L6 — Immediate propagation.** At the end of every turn, the world
is updated before the next character acts.

**L7 — Geographic persistence.** A character retains their
coordinates while the environment is reconstructed.

**L8 — Object evolution.** Objects may persist, change state or
technology, undergo functional replacement, or disappear.
Functional equivalence is not guaranteed.

## 4. Scenario

**Location:** Sottosopra, a fictional town.

In 2025, a scientific capsule is stored in the Clock Tower
across the river.

The tower will be demolished at the mission deadline.
The bridge was demolished after its supports deteriorated.

B must recover the capsule.
A can modify the river-crossing design in 1985.

The protagonists communicate through the **Chronophone**.

### Objectives

- **Mandatory:** recover the capsule by turn 8.
- **Optional:** obtain the Tide Register.

## 5. Operational Rules

- Each turn grants **2 action points (AP)**.
- Actions listed in the routes cost **1 AP** each.
- Moving between connected logical locations costs **1 AP**.
- Interactions require presence at the relevant location.
- Chronophone communication is free.
- Consulting the Time Lens is free.
- Communication does not directly change the world.
- Causal transformations apply at the end of the turn.
- The mission limit is **8 total turns**, not 8 per player.

The map is initially abstract; no geographical grid
has been implemented yet.

## 6. Initial State

**A — 1985:** at the town hall, adjacent to the archive.
A has alternative designs but does not know why the bridge failed.

**B — 2025:** on the western bank, near the bridge remains.
B can investigate them but cannot reach the tower directly.

**World:** bridge absent; alternative vehicle absent;
capsule stored in the tower.

B can discover the defect observed in the future.
A can change the design in the past.

## 7. Route A — Corrected Bridge

| Turn | Player | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Reaches the archive | Examines the design |
| 2 | B | Examines the remains | Identifies the defect |
| 3 | A | Selects the correction | Approves the design |
| 4 | B | Crosses the bridge | Reaches the tower |
| 5 | A | Verifies the design | None |
| 6 | B | Enters the tower | Recovers the capsule |

On turn 2, B communicates the defect to A for free.
At the end of turn 3, the 2025 world is updated.

**Nominal result:** victory on turn 6.

**Advantage:** two spare turns.

**Disadvantage:** the Tide Register is not acquired.

**Scenario rule:** the corrected design eliminates the defect
that would have led to demolition of the bridge.

## 8. Route B — Pendulum Duck

A duck-shaped river vehicle uses counterweights and
a mechanical propulsion system.

| Turn | Player | Action 1 | Action 2 |
|---|---|---|---|
| 1 | A | Reaches the archive | Examines alternative designs |
| 2 | B | Examines the remains | Surveys river conditions |
| 3 | A | Selects the Duck | Approves construction |
| 4 | B | Examines the vehicle | Prepares the mechanism |
| 5 | A | Consults the historical manual | Updates calibration |
| 6 | B | Starts the vehicle | Crosses the river |
| 7 | A | Verifies the intervention | None |
| 8 | B | Enters the tower | Recovers the capsule |

B communicates the river data to A for free.
The world is updated after turns 3 and 5.

During the crossing, the onboard recorder automatically acquires
the **Tide Register**, without any AP cost.

**Nominal result:** victory on turn 8, including the optional objective.

**Advantage:** acquisition of the Tide Register.

**Disadvantage:** no spare turns.

**Scenario rule:** the new calibration does not invalidate
the mechanism preparation already completed by B.

## 9. Strategic Comparison

| Criterion | Corrected Bridge | Pendulum Duck |
|---|---|---|
| Turns | 6 | 8 |
| Capsule | Recovered | Recovered |
| Tide Register | No | Yes |
| Spare turns | 2 | 0 |
| Cooperation | Required | Required |
| Educational theme | Causal graph | Multi-objective planning |

Neither strategy is universally better:
the comparison depends on the objectives.

## 10. Victory, Defeat, and Deadlocks

- **Victory:** capsule acquired by turn 8.
- **Bonus victory:** capsule and Tide Register acquired.
- **Defeat:** capsule not acquired by the end of turn 8.
- **Deadlock:** a plan may make the objective unreachable;
  the engine must recognize and explain it.

## 11. Time Lens

The Lens does not directly modify game state.

It must make the following observable:

1. Initial decision or event.
2. Affected causal dependencies.
3. World state before and after the change.
4. Changes to objects, locations, and available actions.
5. Satisfied or violated constraints.
6. Comparison between strategies, when available.

Modes:

- **Adventure:** optional explanations.
- **Apprentice:** guided reasoning.
- **Researcher:** comparison of scenarios and hypotheses.

Explanations must derive from transitions actually calculated
by the engine, never from invented justifications.

## 12. Candidate Algorithmic Model

- **Causal Graph:** dependencies between events and states.
- **Timeline Recalculation:** propagation of changes.
- **State Transition Engine:** turns, positions, objects, and resources.
- **Constraint Validator:** prerequisites, AP, and victory conditions.
- **Planning / Search:** exploration of alternative sequences.
- **Explainability Layer:** verifiable traces.
- **Companion AI:** reasoned actions without privileged knowledge.

Candidate techniques include directed graphs, topological sorting
where applicable, incremental propagation, state-space search,
CSP, backtracking, A*, Dijkstra, and Utility AI.

Not all of these belong in the initial implementation.

## 13. Future Verification Contract

The engine must verify:

- A/B turn alternation.
- Maximum 2 AP per turn.
- Information available only after acquisition or communication.
- Causal propagation before the next turn.
- Bridge presence only after the relevant modification.
- Duck presence only after approval.
- Preparation validity after calibration.
- Nominal victory on turns 6 and 8 for the respective routes.
- Tide Register acquisition under the specified conditions.
- Deterministic reproducibility of states.
- Correspondence between transitions and Lens explanations.

## 14. Pilot Exclusions

The following are not yet included:

- Tactical combat.
- Character progression and full inventory.
- Playable debts of future actions.
- Complete spatial maps.
- Network multiplayer.
- Implemented companion AI.
- Procedural generation.

This mission is a **conceptual vertical slice of causal
cooperation**, not the complete RPG.

## 15. Qualification

- **DESIGN_GATE:** PASS — explicit design contract.
- **PAPER_FEASIBILITY:** PASS — nominal sequences of 6 and 8 turns.
- **STRATEGY_BALANCE:** PROVISIONAL.
- **ENGINE_VALIDATION:** NOT_RUN.
- **IMPLEMENTATION:** NOT_STARTED.
- **PUBLICATION:** NOT_STARTED.

Software verification and the concrete value of the Tide Register
remain future activities.
