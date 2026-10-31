# ChronoOops! — A Deterministic C11 Time-Travel Laboratory

ChronoOops! is an experimental cooperative, turn-based game in
GiadaWare BoardLab.

Two characters inhabit different years of the same mutable timeline:

- **A (1985)** investigates and changes historical decisions.
- **B (2025)** observes their consequences and interacts with the
  resulting world.

The game's fictional events can be absurd. Its computational rules
must remain explicit, deterministic, and testable.

This directory contains the first playable *simulation engine* for
Mission 001, not a complete RPG or graphical game.

## 1. Learning objectives

By studying this example, you can learn to:

1. Represent a game world using C structures and enumerations.
2. Model actions as validated state transitions.
3. Enforce turn order, action points, and prerequisites.
4. Separate immediate local effects from deferred historical effects.
5. Reconstruct future state from approved historical decisions.
6. Observe causal transitions through before/after snapshots.
7. Test deterministic behavior and rejected operations.
8. Distinguish a successful test from a universal correctness proof.

No external libraries or Python runtime are required.

## 2. Project files

| File | Responsibility |
| --- | --- |
| `chrono.h` | Public types, state structures, and API declarations |
| `chrono.c` | Action validation, state transitions, and causal propagation |
| `test_chrono.c` | Executable scenarios, regression tests, and invariants |

The authoritative design specification is:

`../../docs/games/chrono-oops/mission-001.md`

This path is relative to the directory containing this README.

The specification describes the intended game rules. Tests describe
the behavior currently verified by the implementation.

## 3. Compile and run

From the BoardLab repository root:

```sh
cd src/chrono_oops

gcc -std=c11 -O0 -g \
    -Wall -Wextra -Wpedantic -Werror \
    chrono.c test_chrono.c -o test_chrono

./test_chrono
```

The executable is a local build artifact. It is not required to
understand or modify the source files.

To avoid placing binaries in the source directory, you may instead
compile into a temporary directory of your choice.

Compilation options:

- `-std=c11`: use the ISO C11 language standard.
- `-O0`: disable optimization during introductory debugging.
- `-g`: include debugging information.
- `-Wall -Wextra -Wpedantic`: request additional diagnostics.
- `-Werror`: reject the build if warnings occur.

The tests use `assert()`. Do not compile them with `-DNDEBUG`,
which disables their assertions.

## 4. The state-transition model

The engine exposes four primary functions:

```c
void chrono_init(ChronoState *state);

ChronoResult chrono_apply(
    ChronoState *state,
    ChronoAction action
);

ChronoResult chrono_end_turn(
    ChronoState *state,
    ChronoTrace *trace
);

bool chrono_is_victory(const ChronoState *state);
```

A typical simulation follows this sequence:

```text
INITIAL STATE
     |
     v
VALIDATE ACTION
     |
     +---- invalid ----> REJECT WITHOUT STATE CHANGE
     |
     v
APPLY LOCAL OR HISTORICAL DECISION
     |
     v
END TURN
     |
     v
PROPAGATE HISTORICAL CONSEQUENCES
     |
     v
CHECK VICTORY / DEADLINE
     |
     v
NEXT PLAYER (A <-> B)
```

A turn offers two action points. The players alternate A, B, A, B.
Mission 001 has eight turns in total, not eight turns per player.

Each listed action costs one action point. Communication through
the Cronofono and consultation of the Time Lens are design-level
free operations; a general implementation of those interfaces is
not part of this C11 prototype.

## 5. Why rejected actions are atomic

`chrono_apply()` validates an action while working with a local
copy of the game state.

Only a successful action replaces the original state.

Consequently, an invalid action must not consume action points,
move a character, change objects, or alter mission progress.

This resembles a small transaction:

```text
COPY -> VALIDATE -> MODIFY COPY -> COMMIT
                       |
                       +-- failure -> DISCARD COPY
```

Read `test_invalid_action_atomicity()` for executable examples.

## 6. Two kinds of consequences

### Local actions in 2025

B may inspect or prepare the Duck machine, cross the river, enter
the tower, or recover the capsule.

These actions update the corresponding gameplay state directly.

### Historical decisions in 1985

A may approve a revised bridge design or authorize construction
of the Duck machine.

Approving a plan does not immediately rebuild the future world.
The change becomes visible when `chrono_end_turn()` propagates
the historical decision.

Duck calibration follows the same principle:

1. A reads the historical manual.
2. A records a calibration decision.
3. `calibration_pending` becomes true.
4. The current turn ends.
5. The future machine becomes calibrated.
6. The pending flag is cleared.

The calibration must preserve preparation previously completed
by B in 2025, as required by Mission 001.

## 7. Observing causality

`ChronoTrace` records:

- The world state before propagation.
- The world state after propagation.
- A principal causal action.
- The turn in which propagation occurred.
- Whether the recorded world fields changed.

For example:

```text
TURN 3
Cause: APPROVE_BRIDGE
Before: bridge_exists = false
After:  bridge_exists = true
Changed: true
```

This is an illustrative representation of the values available
through the trace structure, not a built-in logging format.

The current trace is deliberately limited. It does not represent
a complete dependency graph or separately record every local
action performed during the turn.

World changes are compared field by field rather than using
`memcmp()` on C structures. Structure padding is not gameplay
data and should not influence semantic equality.

## 8. The two Mission 001 strategies

### Strategy A — Repaired Bridge

A obtains historical plans, B identifies the bridge defect, and
A approves the correction.

The repaired bridge becomes available after turn 3.

B reaches the tower and retrieves the capsule on turn 6.

Expected result:

- Mission victory on turn 6.
- Two turns remaining.
- No Tide Register.

### Strategy B — Pendulum Duck

A approves an improbable river-crossing machine.

B prepares it, A corrects its historical calibration, and B
subsequently crosses the river.

The onboard recorder automatically acquires the Tide Register.

Expected result:

- Mission victory on turn 8.
- Tide Register acquired.
- No turns remaining.

Neither strategy is universally better: one prioritizes speed,
while the other completes the optional objective.

## 9. Reading the tests

Recommended study order:

1. `run_route()` — first three turns of both strategies.
2. `test_complete_bridge_mission()` — complete bridge scenario.
3. `test_complete_duck_mission()` — complete Duck scenario.
4. `test_historical_trace()` — causal before/after evidence.
5. `test_invalid_action_atomicity()` — rejected transitions.
6. `test_deterministic_replay()` — equal inputs, equal outputs.
7. `test_mission_deadline()` — eight-turn deadline.

The test suite has demonstrated these representative behaviors.
It has not exhaustively explored every possible action sequence
or formally proved all possible game states correct.

## 10. Student experiments

Try these exercises in a separate working copy or branch:

**Exercise A — Action-point accounting**

Attempt a third action in a turn. Predict the returned result
and verify that the state remains unchanged.

**Exercise B — Missing historical information**

Attempt to select the repaired bridge before B identifies the
defect. Explain which prerequisite prevents the action.

**Exercise C — Delayed propagation**

Observe `bridge_exists` immediately before and immediately
after turn 3 ends. Explain why the values differ.

**Exercise D — Deterministic replay**

Run identical sequences from two independently initialized
states. Compare their meaningful fields and causal traces.

**Exercise E — Time Lens extension**

Design an additional trace representation that records local
actions without inventing causes or altering the outcome.

Preserve the existing tests when experimenting.

## 11. Current boundaries

Implemented and exercised:

- Deterministic C11 state transitions.
- Alternating turns and action-point limits.
- Action validation and rejection.
- Historical bridge and Duck creation.
- Deferred historical Duck calibration.
- Capsule recovery and Tide Register acquisition.
- Mission victory and deadline.
- Representative causal trace and replay tests.

Not implemented in this prototype:

- Graphical interface or complete RPG systems.
- Network multiplayer.
- AI-controlled cooperative companion.
- General inventory or geographical grid.
- Future-action debt mechanics.
- Automatic planning or search.
- Full causal dependency graph.
- Exhaustive state-space verification.

## 12. Engineering principle

The implementation should be understandable without requiring
the original author's explanations.

Prefer explicit rules, precise names, small functions,
focused tests, and comments that explain design decisions.

A game may be absurd. Its implementation must be explainable.
