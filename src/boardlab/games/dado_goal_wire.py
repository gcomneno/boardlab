"""Strict Dado Goal state serialization for authoritative services.

This module does not authenticate users, generate dice rolls,
persist matches, or implement network transport.
"""

from collections.abc import Mapping
from typing import Any

from boardlab.games.dado_goal import (
    Phase,
    State,
    _validate,
)

SCHEMA_VERSION = 1
MAX_WIRE_HANDS = 1000

FIELDS = frozenset(
    {
        "schemaVersion",
        "handsPerTeam",
        "player",
        "hands",
        "goals",
        "phase",
    }
)


def _pair(value: object, name: str) -> tuple[int, int]:
    if (
        type(value) is not list
        or len(value) != 2
        or any(type(item) is not int for item in value)
    ):
        raise ValueError(f"Invalid {name}")

    return (value[0], value[1])


def encode_state(state: State) -> dict[str, Any]:
    """Encode a validated mathematical state for JavaScript."""

    _validate(state)

    if state.hands_per_team > MAX_WIRE_HANDS:
        raise ValueError("Match length exceeds wire policy")

    return {
        "schemaVersion": SCHEMA_VERSION,
        "handsPerTeam": state.hands_per_team,
        "player": state.active_player,
        "hands": list(state.completed_hands),
        "goals": list(state.goals),
        "phase": state.phase.value,
    }


def decode_state(payload: Mapping[str, Any]) -> State:
    """Decode a snapshot. Never treat it as client authority."""

    if not isinstance(payload, dict):
        raise ValueError("Expected JSON object")

    if set(payload) != FIELDS:
        raise ValueError("Invalid snapshot fields")

    if (
        type(payload["schemaVersion"]) is not int
        or payload["schemaVersion"] != SCHEMA_VERSION
    ):
        raise ValueError("Unsupported schema version")

    n = payload["handsPerTeam"]
    player = payload["player"]
    phase_name = payload["phase"]

    if type(n) is not int or not 1 <= n <= MAX_WIRE_HANDS:
        raise ValueError("Invalid match length")

    if type(player) is not int or player not in (0, 1):
        raise ValueError("Invalid player")

    if type(phase_name) is not str:
        raise ValueError("Invalid phase")

    try:
        phase = Phase(phase_name)
    except ValueError as exc:
        raise ValueError("Invalid phase") from exc

    state = State(
        hands_per_team=n,
        active_player=player,
        completed_hands=_pair(payload["hands"], "hands"),
        goals=_pair(payload["goals"], "goals"),
        phase=phase,
    )

    _validate(state)
    return state
