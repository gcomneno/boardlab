"""Tests for the generic random strategy."""

from random import Random

import pytest

from boardlab.engine.match import Match
from boardlab.engine.strategy import Strategy
from boardlab.games.three_stones import Player, State, ThreeStones
from boardlab.strategies.random import RandomStrategy


def test_random_strategy_satisfies_protocol() -> None:
    strategy: Strategy[State, int, Player] = RandomStrategy(Random(7))
    game = ThreeStones()

    assert strategy.choose_move(game, game.initial_state()) in (0, 1, 2)


def test_random_strategy_selects_only_legal_moves() -> None:
    game = ThreeStones()
    strategy = RandomStrategy[State, int, Player](Random(11))
    state = game.apply_move(game.initial_state(), 0)

    for _ in range(30):
        assert strategy.choose_move(game, state) in game.legal_moves(state)


def test_random_strategy_is_reproducible_with_same_seed() -> None:
    game = ThreeStones()
    state = game.initial_state()

    first = RandomStrategy[State, int, Player](Random(42))
    second = RandomStrategy[State, int, Player](Random(42))

    first_moves = [first.choose_move(game, state) for _ in range(20)]
    second_moves = [second.choose_move(game, state) for _ in range(20)]

    assert first_moves == second_moves


def test_random_strategy_preserves_input_state() -> None:
    game = ThreeStones()
    strategy = RandomStrategy[State, int, Player](Random(3))
    state = game.initial_state()

    strategy.choose_move(game, state)

    assert state == game.initial_state()


def test_random_strategy_rejects_terminal_state() -> None:
    game = ThreeStones()
    strategy = RandomStrategy[State, int, Player](Random(5))
    state = game.initial_state()

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    assert game.is_terminal(state)

    with pytest.raises(ValueError, match="No legal moves"):
        strategy.choose_move(game, state)


def test_random_strategy_integrates_with_match() -> None:
    game = ThreeStones()
    match = Match(game)
    strategy = RandomStrategy[State, int, Player](Random(9))

    while not game.is_terminal(match.state):
        previous = match.state
        legal = game.legal_moves(previous)

        successor = match.play_strategy(strategy)

        assert successor is match.state
        assert successor != previous
        assert len(game.legal_moves(successor)) == len(legal) - 1

    assert game.is_terminal(match.state)
