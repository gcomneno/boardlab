"""Verify Tre Sigilli against immutable JSON v1 reference fixtures."""

import json
from pathlib import Path
from typing import Any

import pytest

from boardlab.games.tre_sigilli import (
    Place,
    Player,
    Slide,
    State,
    TreSigilli,
)
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory

FIXTURES = Path(__file__).parent / "fixtures" / "tre_sigilli" / "v1.json"


def load_cases() -> list[dict[str, Any]]:
    document = json.loads(FIXTURES.read_text(encoding="utf-8"))
    assert document["schema_version"] == 1
    assert document["game"] == "tre_sigilli"
    cases: list[dict[str, Any]] = document["cases"]
    assert len(cases) == 5
    assert len({case["id"] for case in cases}) == len(cases)
    return cases


def decode_state(data: dict[str, Any]) -> State:
    return State(
        tuple(None if cell is None else Player(cell) for cell in data["cells"]),
        data["turn"],
    )


def encode_state(state: State) -> dict[str, Any]:
    return {
        "turn": state.turn,
        "cells": [None if cell is None else cell.value for cell in state.cells],
    }


def decode_move(data: dict[str, Any]) -> Place | Slide:
    if data["type"] == "place":
        return Place(data["cell"])
    if data["type"] == "slide":
        return Slide(data["origin"], data["destination"])
    raise ValueError("Unsupported fixture move")


def encode_move(move: Place | Slide) -> dict[str, Any]:
    if isinstance(move, Place):
        return {"type": "place", "cell": move.cell}
    return {
        "type": "slide",
        "origin": move.origin,
        "destination": move.destination,
    }


@pytest.mark.parametrize(
    "case",
    load_cases(),
    ids=lambda case: str(case["id"]),
)
def test_json_v1_parity(case: dict[str, Any]) -> None:
    game = TreSigilli()
    state = decode_state(case["state"])
    original = state

    assert encode_state(state) == case["state"]

    terminal = game.is_terminal(state)
    assert terminal is case["terminal"]

    assert (None if terminal else game.current_player(state).value) == case["active_player"]

    moves = game.legal_moves(state)
    assert [encode_move(move) for move in moves] == case["legal_moves"]

    north, south = game.territory_scores(state)
    assert {"north": north, "south": south} == case["territory_scores"]

    if terminal:
        assert {player.value: game.outcome(state, player).value for player in Player} == case[
            "outcomes"
        ]

        with pytest.raises(ValueError):
            game.current_player(state)

        with pytest.raises(ValueError):
            AlphaBetaStrategy(evaluate_territory, depth=2).choose_move(game, state)
    else:
        assert case["outcomes"] is None
        ai = AlphaBetaStrategy(evaluate_territory, depth=2)
        assert encode_move(ai.choose_move(game, state)) == (case["ai_depth_2_move"])

    assert len(case["transitions"]) == len(moves)

    for expected in case["transitions"]:
        move = decode_move(expected["move"])
        successor = game.apply_move(state, move)
        assert encode_state(successor) == expected["next_state"]

    for expected in case["invalid_moves"]:
        assert expected["error"] == "illegal_move"
        with pytest.raises(ValueError, match="Illegal move"):
            game.apply_move(state, decode_move(expected["move"]))

    assert state == original


def test_fixture_coverage() -> None:
    cases = load_cases()
    by_id = {case["id"]: case for case in cases}

    assert set(by_id) == {
        "initial",
        "placement",
        "maneuver",
        "turn_limit",
        "early_immobilization",
    }

    assert by_id["turn_limit"]["state"]["turn"] == 18
    assert by_id["turn_limit"]["terminal"] is True
    assert by_id["early_immobilization"]["state"]["turn"] < 18
    assert by_id["early_immobilization"]["terminal"] is True
