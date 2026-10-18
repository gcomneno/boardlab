# ADR 0002 — Android behavioral parity contract

- Status: accepted for Kotlin domain parity (SHIP-M2) and Jetpack Compose UI (SHIP-M4)
- Scope: BoardLab SHIP-M1
- Target: Tre Sigilli, Android mobile-first

## Context

BoardLab uses Python 3.12+ as its initial implementation
language and educational reference (ADR 0001).

SHIP evaluates whether Tre Sigilli can become a playable
Android application without compromising the existing
Python laboratory.

## Proposed architecture

- Preserve Python as the behavioral reference.
- Use Kotlin for the parallel Tre Sigilli domain implementation; Jetpack Compose remains under evaluation for Android UI.
- Kotlin domain implementation explicitly approved for SHIP-M2; Python remains the canonical reference and permanent CLI.
- Keep Android UI separate from game rules and AI logic.
- Avoid introducing Android dependencies into the Python core.

Kotlin domain parity was approved for SHIP-M2. Jetpack Compose with a separate Kotlin GameController was explicitly approved for SHIP-M4. Python remains canonical.

## Versioned parity contract

SHIP-M1 introduces language-neutral JSON fixtures covering:

- Board occupancy, turn and active player.
- Ordered legal moves and deterministic transitions.
- Territorial scores, terminal states and outcomes.
- Early immobilization and the eighteen-turn limit.
- Illegal actions and rejection without state mutation.
- Alpha-Beta depth-two decisions and deterministic tie-breaking.

The contract must specify field types, validation rules,
error representation and ordering before being frozen.

Fixture values must be reviewed against the existing Python
tests and independent behavioral expectations. Tests must
not silently regenerate their own expected values.

## Acceptance gates

1. Document the JSON v1 schema and fixture provenance.
2. Review representative and boundary-case fixtures.
3. Validate fixture integrity and Python behavioral parity.
4. Run the existing Python quality gates.
5. Obtain explicit approval before any Kotlin implementation.

## Non-goals

- Android application implementation.
- Android SDK or Gradle installation.
- Store publication.
- Python engine refactoring.
- Automatic commit or push.

## Compatibility

ADR 0001 remains authoritative for the Python laboratory.
This proposal does not replace its toolchain or alter
the existing LEARN and BUILD tracks.

## JSON v1 wire contract

The canonical fixture file is
`tests/fixtures/tre_sigilli/v1.json`.

The top-level JSON object contains:

- `schema_version`: integer, exactly `1`.
- `game`: string, exactly `"tre_sigilli"`.
- `cases`: ordered array of independently identified test cases.

Each case contains:

- `id`: unique, stable string.
- `state`: object containing `turn` and `cells`.
- `active_player`: `"north"`, `"south"` or `null`.
- `terminal`: boolean.
- `legal_moves`: ordered array of move objects.
- `territory_scores`: object with integer `north` and `south`.
- `outcomes`: `null` or an object mapping both players to
  `"win"`, `"loss"` or `"draw"`.
- `ai_depth_2_move`: a move object or `null`.
- `transitions`: ordered array of transition expectations.
- `invalid_moves`: ordered array of rejected-action expectations.

The `state.cells` array contains exactly sixteen entries,
each `null`, `"north"` or `"south"`, indexed row-major from
zero through fifteen. `state.turn` is an integer from zero
through eighteen and counts completed actions.

Move representations:

- Placement: `{"type": "place", "cell": 0}`.
- Maneuver: `{"type": "slide", "origin": 0, "destination": 1}`.

A transition expectation contains `move` and `next_state`.
An invalid-action expectation contains `move` and
`error: "illegal_move"`.

A terminal case has `active_player: null`, an empty
`legal_moves` array, non-null `outcomes`, and
`ai_depth_2_move: null`. A nonterminal case has
`outcomes: null` and a non-null AI move.

Move ordering is semantically significant. Placement
moves follow ascending cell indices. Maneuver moves
follow ascending `(origin, destination)` order.

The depth-two AI uses territorial score difference
from the root player's perspective, terminal outcomes
as positive infinity, negative infinity or zero, and
the first optimal move on ties.

Malformed states and invalid player arguments are
separate validation concerns; they must not be
misclassified as `illegal_move`.

JSON fixtures are reviewed, versioned reference data.
Runtime tests compare against these values without
rewriting them.
