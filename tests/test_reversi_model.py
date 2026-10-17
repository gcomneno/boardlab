"""Verify the Reversi 6x6 immutable state and action model."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.games.reversi import Pass, Place, Player, Reversi, State


def test_initial_state() -> None:
    game = Reversi()
    state = game.initial_state()

    assert len(state.cells) == 36
    assert state.active_player is Player.BLACK
    assert state.cells[14] is Player.WHITE
    assert state.cells[15] is Player.BLACK
    assert state.cells[20] is Player.BLACK
    assert state.cells[21] is Player.WHITE
    assert state.cells.count(None) == 32


def test_initial_states_are_independent() -> None:
    game = Reversi()
    first = game.initial_state()
    second = game.initial_state()

    assert first == second
    assert first is not second
    assert first.cells is not second.cells


@pytest.mark.parametrize(
    "state",
    [
        State((), Player.BLACK),
        State((None,) * 35, Player.BLACK),
        State((None,) * 37, Player.BLACK),
        State([None] * 36, Player.BLACK),  # type: ignore[arg-type]
        State((0,) + (None,) * 35, Player.BLACK),  # type: ignore[arg-type]
        State((None,) * 36, None),  # type: ignore[arg-type]
    ],
)
def test_invalid_state_rejected(state: State) -> None:
    with pytest.raises(ValueError, match="Invalid game state"):
        Reversi._validate(state)


def test_invalid_state_type_rejected() -> None:
    with pytest.raises(ValueError, match="Invalid game state"):
        Reversi._validate(None)  # type: ignore[arg-type]


def test_state_is_frozen() -> None:
    state = Reversi().initial_state()

    with pytest.raises(FrozenInstanceError):
        state.active_player = Player.WHITE  # type: ignore[misc]


def test_actions_are_distinct_frozen_value_types() -> None:
    assert Place(7) == Place(7)
    assert Place(7) != Place(8)
    assert Pass() == Pass()

    with pytest.raises(FrozenInstanceError):
        Place(7).cell = 8  # type: ignore[misc]
