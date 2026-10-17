"""Verify terminal Reversi outcomes from each player's perspective."""

import pytest

from boardlab.engine.game import Outcome
from boardlab.games.reversi import Player, Reversi, State


def position(
    black: tuple[int, ...],
    white: tuple[int, ...],
) -> State:
    cells: list[Player | None] = [None] * 36

    for cell in black:
        assert cells[cell] is None
        cells[cell] = Player.BLACK

    for cell in white:
        assert cells[cell] is None
        cells[cell] = Player.WHITE

    return State(tuple(cells), Player.BLACK)


@pytest.mark.parametrize(
    ("black_count", "expected_black", "expected_white"),
    [
        (36, Outcome.WIN, Outcome.LOSS),
        (20, Outcome.WIN, Outcome.LOSS),
        (18, Outcome.DRAW, Outcome.DRAW),
        (16, Outcome.LOSS, Outcome.WIN),
        (0, Outcome.LOSS, Outcome.WIN),
    ],
)
def test_full_board_outcome_from_both_perspectives(
    black_count: int,
    expected_black: Outcome,
    expected_white: Outcome,
) -> None:
    game = Reversi()
    state = position(
        tuple(range(black_count)),
        tuple(range(black_count, 36)),
    )

    assert game.is_terminal(state)
    assert game.outcome(state, Player.BLACK) is expected_black
    assert game.outcome(state, Player.WHITE) is expected_white


def test_non_full_terminal_board_uses_disc_counts() -> None:
    game = Reversi()
    state = position((0, 1, 2), ())

    assert game.is_terminal(state)
    assert game.outcome(state, Player.BLACK) is Outcome.WIN
    assert game.outcome(state, Player.WHITE) is Outcome.LOSS


def test_non_full_terminal_draw() -> None:
    game = Reversi()
    state = position((0,), (35,))

    assert game.is_terminal(state)
    assert game.outcome(state, Player.BLACK) is Outcome.DRAW
    assert game.outcome(state, Player.WHITE) is Outcome.DRAW


def test_outcome_rejects_ongoing_state() -> None:
    game = Reversi()

    with pytest.raises(ValueError, match="terminal"):
        game.outcome(game.initial_state(), Player.BLACK)


@pytest.mark.parametrize("player", [None, "black", 0, True])
def test_outcome_rejects_invalid_player(player: object) -> None:
    game = Reversi()
    state = position((0,), ())

    with pytest.raises(ValueError, match="Invalid player"):
        game.outcome(state, player)  # type: ignore[arg-type]


def test_outcome_rejects_invalid_state() -> None:
    game = Reversi()
    invalid = State((None,) * 35, Player.BLACK)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.outcome(invalid, Player.BLACK)
