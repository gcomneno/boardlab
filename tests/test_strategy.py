"""Verify the generic Strategy contract with a test-only implementation."""

import pytest

from boardlab.engine.game import Game
from boardlab.engine.strategy import Strategy
from boardlab.games.three_stones import Player, State, ThreeStones


class FirstLegalMove:
    """Deterministic test double, not a production strategy."""

    def choose_move(
        self,
        game: Game[State, int, Player],
        state: State,
    ) -> int:
        moves = game.legal_moves(state)

        if not moves:
            raise ValueError("No legal moves")

        return moves[0]


def strategy_fixture() -> Strategy[State, int, Player]:
    """Check structural compatibility with the public Strategy protocol."""
    return FirstLegalMove()


def test_strategy_selects_a_legal_move() -> None:
    game = ThreeStones()
    strategy = strategy_fixture()
    state = game.initial_state()

    move = strategy.choose_move(game, state)

    assert move == 0
    assert move in game.legal_moves(state)


def test_strategy_does_not_modify_state() -> None:
    game = ThreeStones()
    strategy = strategy_fixture()
    original = game.initial_state()

    strategy.choose_move(game, original)

    assert original == game.initial_state()


def test_strategy_respects_available_moves() -> None:
    game = ThreeStones()
    strategy = strategy_fixture()
    state = game.apply_move(game.initial_state(), 0)

    move = strategy.choose_move(game, state)

    assert move == 1
    assert move in game.legal_moves(state)


def test_strategy_rejects_terminal_state() -> None:
    game = ThreeStones()
    strategy = strategy_fixture()
    state = game.initial_state()

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    assert game.is_terminal(state)

    with pytest.raises(ValueError, match="No legal moves"):
        strategy.choose_move(game, state)


def test_strategy_selection_does_not_advance_match() -> None:
    from boardlab.engine.match import Match

    game = ThreeStones()
    match = Match(game)
    strategy = strategy_fixture()
    initial = match.state

    move = strategy.choose_move(game, match.state)

    assert move == 0
    assert match.state is initial

    successor = match.play(move)

    assert successor != initial
    assert match.state is successor
