"""Exercise the generic contract using the Session 01 Three Stones example."""

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.games.three_stones import Player, State, ThreeStones


def game_fixture() -> Game[State, int, Player]:
    """Static typing checks structural compatibility with the public contract."""
    return ThreeStones()


def test_transition_preserves_original_and_independent_branches() -> None:
    game = game_fixture()
    original = game.initial_state()

    assert game.current_player(original) is Player.NORTH
    assert game.legal_moves(original) == (0, 1, 2)
    assert not game.is_terminal(original)

    left = game.apply_move(original, 0)
    right = game.apply_move(original, 1)

    assert original == State((None, None, None), Player.NORTH)
    assert left == State((Player.NORTH, None, None), Player.SOUTH)
    assert right == State((None, Player.NORTH, None), Player.SOUTH)
    assert game.current_player(left) is Player.SOUTH
    assert game.legal_moves(left) == (1, 2)

    continued = game.apply_move(left, 1)
    assert continued == State((Player.NORTH, Player.SOUTH, None), Player.NORTH)
    assert left == State((Player.NORTH, None, None), Player.SOUTH)
    assert right == State((None, Player.NORTH, None), Player.SOUTH)


@pytest.mark.parametrize("move", [-1, 0, 3])
def test_illegal_move_is_rejected_without_changing_state(move: int) -> None:
    game = game_fixture()
    state = game.apply_move(game.initial_state(), 0)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)

    assert state == State((Player.NORTH, None, None), Player.SOUTH)


def test_terminal_result_uses_player_perspective() -> None:
    game = game_fixture()
    state = game.initial_state()

    with pytest.raises(ValueError, match="terminal"):
        game.outcome(state, Player.NORTH)

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    assert state == State((Player.NORTH, Player.SOUTH, Player.NORTH), Player.SOUTH)
    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()
    assert game.outcome(state, Player.NORTH) is Outcome.WIN
    assert game.outcome(state, Player.SOUTH) is Outcome.LOSS

    with pytest.raises(ValueError, match="active player"):
        game.current_player(state)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, 0)


@pytest.mark.parametrize(
    ("state", "message"),
    [
        (State((Player.SOUTH, None, None), Player.NORTH), "Invalid game state"),
        (State((Player.NORTH, None, None), Player.NORTH), "Invalid game state"),
        (
            State((Player.NORTH, Player.NORTH, None), Player.SOUTH),
            "Invalid game state",
        ),
        (
            State((Player.NORTH, Player.SOUTH, None), Player.SOUTH),
            "Invalid game state",
        ),
    ],
)
def test_invalid_states_are_rejected(state: State, message: str) -> None:
    game = game_fixture()

    with pytest.raises(ValueError, match=message):
        game.legal_moves(state)

    with pytest.raises(ValueError, match=message):
        game.is_terminal(state)


@pytest.mark.parametrize("move", [True, 1.0, "1", None])
def test_non_integer_moves_are_rejected(move: object) -> None:
    game = ThreeStones()
    state = game.initial_state()

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)  # type: ignore[arg-type]

    assert state == game.initial_state()


def test_successor_is_independent_of_original_state() -> None:
    game = game_fixture()
    original = game.initial_state()

    first = game.apply_move(original, 0)
    second = game.apply_move(original, 2)

    assert original == game.initial_state()
    assert first != second
    assert first.cells is not original.cells
    assert second.cells is not original.cells
