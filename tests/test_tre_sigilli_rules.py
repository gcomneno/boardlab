"""Verify Tre Sigilli placement, maneuver, termination and transitions."""

import pytest

from boardlab.games.tre_sigilli import Place, Player, Slide, State, TreSigilli


def position(
    turn: int,
    north: tuple[int, ...] = (),
    south: tuple[int, ...] = (),
) -> State:
    """Build a structurally valid board for focused rule tests."""
    cells: list[Player | None] = [None] * 16

    for index in north:
        cells[index] = Player.NORTH

    for index in south:
        if cells[index] is not None:
            raise ValueError("Overlapping seals")
        cells[index] = Player.SOUTH

    state = State(tuple(cells), turn)
    TreSigilli._validate(state)
    return state


def play_placements(*indices: int) -> State:
    """Play placements through the public transition API."""
    game = TreSigilli()
    state = game.initial_state()

    for index in indices:
        state = game.apply_move(state, Place(index))

    return state


def test_initial_moves_are_ordered_and_north_starts() -> None:
    game = TreSigilli()
    state = game.initial_state()

    assert game.current_player(state) is Player.NORTH
    assert game.legal_moves(state) == tuple(Place(i) for i in range(16))
    assert not game.is_terminal(state)


def test_six_placements_alternate_and_enter_maneuver() -> None:
    game = TreSigilli()
    state = game.initial_state()

    for turn, index in enumerate((0, 5, 2, 6, 4, 8)):
        assert game.current_player(state) is (Player.NORTH if turn % 2 == 0 else Player.SOUTH)
        previous = state
        state = game.apply_move(state, Place(index))
        assert previous.turn == turn
        assert state.turn == turn + 1

    assert state == position(6, (0, 2, 4), (5, 6, 8))
    assert all(isinstance(move, Slide) for move in game.legal_moves(state))
    assert game.current_player(state) is Player.NORTH


def test_placement_order_and_occupied_destination() -> None:
    game = TreSigilli()
    state = game.apply_move(game.initial_state(), Place(4))

    assert game.legal_moves(state) == tuple(Place(i) for i in range(16) if i != 4)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Place(4))


def test_maneuver_order_is_origin_then_destination() -> None:
    game = TreSigilli()
    state = position(6, (0, 2, 4), (5, 6, 8))

    assert game.legal_moves(state) == (
        Slide(0, 1),
        Slide(2, 1),
        Slide(2, 3),
    )


@pytest.mark.parametrize(
    "move",
    [
        Place(16),
        Place(-1),
        Place(True),
        Place(1.0),  # type: ignore[arg-type]
        Slide(0, 1),
        None,
    ],
)
def test_illegal_placement_moves_are_rejected(move: object) -> None:
    game = TreSigilli()
    state = game.initial_state()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)  # type: ignore[arg-type]

    assert state == game.initial_state()


@pytest.mark.parametrize(
    "move",
    [
        Place(1),
        Slide(0, 4),
        Slide(0, 5),
        Slide(0, 15),
        Slide(5, 1),
        Slide(2, 6),
        Slide(0, 0),
        Slide(0, True),
        Slide(0, 1.0),  # type: ignore[arg-type]
        Slide(-1, 1),
        Slide(16, 1),
        None,
    ],
)
def test_illegal_maneuver_moves_are_rejected(move: object) -> None:
    game = TreSigilli()
    state = position(6, (0, 2, 4), (5, 6, 8))

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)  # type: ignore[arg-type]

    assert state == position(6, (0, 2, 4), (5, 6, 8))


def test_orthogonal_move_is_immutable_and_independent() -> None:
    game = TreSigilli()
    state = position(6, (0, 2, 4), (5, 6, 8))

    first = game.apply_move(state, Slide(2, 1))
    second = game.apply_move(state, Slide(2, 3))

    assert first == position(7, (0, 1, 4), (5, 6, 8))
    assert second == position(7, (0, 3, 4), (5, 6, 8))
    assert state == position(6, (0, 2, 4), (5, 6, 8))
    assert first.cells is not state.cells
    assert second.cells is not state.cells


def test_row_boundary_does_not_wrap() -> None:
    game = TreSigilli()
    state = position(6, (3, 9, 12), (0, 5, 14))

    assert Slide(3, 4) not in game.legal_moves(state)
    assert Slide(12, 11) not in game.legal_moves(state)


def test_reachable_block_at_turn_six() -> None:
    game = TreSigilli()
    state = play_placements(0, 2, 1, 5, 4, 8)

    assert state.turn == 6
    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()

    with pytest.raises(ValueError, match="Terminal state"):
        game.current_player(state)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Place(0))


def test_reachable_block_at_turn_seven() -> None:
    game = TreSigilli()
    state = play_placements(5, 2, 6, 3, 11, 7)

    assert state.turn == 6
    state = game.apply_move(state, Slide(5, 1))

    assert state.turn == 7
    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()

    with pytest.raises(ValueError, match="Terminal state"):
        game.current_player(state)


def test_reachable_block_at_turn_eight() -> None:
    game = TreSigilli()
    state = play_placements(0, 5, 2, 6, 4, 8)

    state = game.apply_move(state, Slide(2, 1))
    state = game.apply_move(state, Slide(6, 2))

    assert state.turn == 8
    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()


def test_turn_eighteen_is_terminal_even_if_moves_exist() -> None:
    game = TreSigilli()
    state = position(18, (0, 2, 4), (5, 6, 8))

    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, Slide(2, 1))


def test_invalid_state_rejected_by_public_rules() -> None:
    game = TreSigilli()
    invalid = State((None,) * 16, 6)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.is_terminal(invalid)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.legal_moves(invalid)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.current_player(invalid)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.apply_move(invalid, Place(0))


def test_legal_moves_are_deterministic_and_nonmutating() -> None:
    game = TreSigilli()
    state = position(6, (0, 2, 4), (5, 6, 8))
    original = state

    assert game.legal_moves(state) == game.legal_moves(state)
    assert state == original
