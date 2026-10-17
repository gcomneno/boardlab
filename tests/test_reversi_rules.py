"""Verify legal placements, captures, passing and termination in Reversi."""

import pytest

from boardlab.games.reversi import Pass, Place, Player, Reversi, State


def position(
    black: tuple[int, ...],
    white: tuple[int, ...],
    active: Player = Player.BLACK,
) -> State:
    """Construct a board position for isolated rule tests."""
    cells: list[Player | None] = [None] * 36

    for cell in black:
        if cells[cell] is not None:
            raise ValueError("Overlapping discs")
        cells[cell] = Player.BLACK

    for cell in white:
        if cells[cell] is not None:
            raise ValueError("Overlapping discs")
        cells[cell] = Player.WHITE

    return State(tuple(cells), active)


def test_opening_legal_moves_are_deterministic() -> None:
    game = Reversi()
    state = game.initial_state()

    assert game.legal_moves(state) == (
        Place(8),
        Place(13),
        Place(22),
        Place(27),
    )
    assert game.current_player(state) is Player.BLACK
    assert not game.is_terminal(state)


def test_opening_capture_and_immutable_transition() -> None:
    game = Reversi()
    original = game.initial_state()

    successor = game.apply_move(original, Place(8))

    assert successor.cells[8] is Player.BLACK
    assert successor.cells[14] is Player.BLACK
    assert successor.active_player is Player.WHITE
    assert original.cells[8] is None
    assert original.cells[14] is Player.WHITE


def test_capture_all_eight_directions() -> None:
    game = Reversi()
    center = 14

    neighbors = (7, 8, 9, 13, 15, 19, 20, 21)
    endpoints = (0, 2, 4, 12, 16, 24, 26, 28)

    state = position(black=endpoints, white=neighbors)

    assert Place(center) in game.legal_moves(state)

    successor = game.apply_move(state, Place(center))

    assert successor.cells[center] is Player.BLACK
    assert all(successor.cells[cell] is Player.BLACK for cell in neighbors)
    assert all(state.cells[cell] is Player.WHITE for cell in neighbors)


def test_no_capture_means_illegal_placement() -> None:
    game = Reversi()
    state = game.initial_state()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Place(0))


def test_pass_is_required_when_only_opponent_can_place() -> None:
    game = Reversi()
    state = position(
        black=(0,),
        white=(1,),
        active=Player.WHITE,
    )

    assert game.legal_moves(state) == (Pass(),)
    assert not game.is_terminal(state)

    successor = game.apply_move(state, Pass())

    assert successor.cells == state.cells
    assert successor.active_player is Player.BLACK
    assert game.legal_moves(successor) == (Place(2),)


def test_pass_is_illegal_when_placement_exists() -> None:
    game = Reversi()
    state = game.initial_state()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Pass())


def test_terminal_when_neither_player_can_place() -> None:
    game = Reversi()
    state = position(black=(0,), white=())

    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()

    with pytest.raises(ValueError, match="Terminal state"):
        game.current_player(state)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Pass())


@pytest.mark.parametrize("cell", [-1, 36, True, 3.5])
def test_malformed_placements_are_rejected(cell: object) -> None:
    game = Reversi()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(game.initial_state(), Place(cell))  # type: ignore[arg-type]
