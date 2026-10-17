"""Adversarial tests for Reversi move generation and transitions."""

import pytest

from boardlab.games.reversi import Pass, Place, Player, Reversi, State


def position(
    black: tuple[int, ...],
    white: tuple[int, ...],
    active: Player = Player.BLACK,
) -> State:
    cells: list[Player | None] = [None] * 36

    for index in black:
        assert cells[index] is None
        cells[index] = Player.BLACK

    for index in white:
        assert cells[index] is None
        cells[index] = Player.WHITE

    return State(tuple(cells), active)


def test_simultaneous_eight_direction_capture() -> None:
    game = Reversi()
    neighbors = (7, 8, 9, 13, 15, 19, 20, 21)
    endpoints = (0, 2, 4, 12, 16, 24, 26, 28)
    state = position(endpoints, neighbors)

    successor = game.apply_move(state, Place(14))

    assert all(successor.cells[index] is Player.BLACK for index in neighbors)
    assert successor.cells.count(Player.BLACK) == 17
    assert state.cells.count(Player.WHITE) == 8


def test_horizontal_capture_does_not_wrap_rows() -> None:
    game = Reversi()
    state = position(black=(5,), white=(6,))

    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()


def test_placement_does_not_capture_unbounded_discs() -> None:
    game = Reversi()
    state = position(black=(0,), white=(1, 2))

    successor = game.apply_move(state, Place(3))

    assert successor.cells[1] is Player.BLACK
    assert successor.cells[2] is Player.BLACK
    assert successor.cells[3] is Player.BLACK


def test_pass_does_not_change_board_or_original_state() -> None:
    game = Reversi()
    state = position(black=(0,), white=(1,), active=Player.WHITE)

    successor = game.apply_move(state, Pass())

    assert successor is not state
    assert successor.cells == state.cells
    assert successor.active_player is Player.BLACK
    assert state.active_player is Player.WHITE


@pytest.mark.parametrize(
    "move",
    [
        Place(-1),
        Place(36),
        Place(True),
        Place(1.5),  # type: ignore[arg-type]
        Pass(),
    ],
)
def test_invalid_opening_actions_preserve_state(move: Place | Pass) -> None:
    game = Reversi()
    state = game.initial_state()
    snapshot = state

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)

    assert state == snapshot
    assert state.active_player is Player.BLACK


def test_terminal_board_rejects_every_action() -> None:
    game = Reversi()
    state = position(black=tuple(range(36)), white=())

    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()

    for move in (Pass(), Place(0)):
        with pytest.raises(ValueError, match="Illegal move"):
            game.apply_move(state, move)
