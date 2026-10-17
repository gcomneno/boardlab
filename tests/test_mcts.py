"""Behavioral tests for Monte Carlo Tree Search."""

from random import Random

import pytest

from boardlab.engine.strategy import Strategy
from boardlab.games.three_stones import (
    Player as StonesPlayer,
)
from boardlab.games.three_stones import (
    State as StonesState,
)
from boardlab.games.three_stones import (
    ThreeStones,
)
from boardlab.games.tre_sigilli import (
    Move,
    Player,
    State,
    TreSigilli,
)
from boardlab.strategies.mcts import MCTSStrategy


@pytest.mark.parametrize("simulations", [0, -1, True, 1.5])
def test_invalid_simulation_budget(simulations: int) -> None:
    with pytest.raises(ValueError, match="Simulations"):
        MCTSStrategy[StonesState, int, StonesPlayer](simulations)


@pytest.mark.parametrize("exploration", [-1.0, float("nan"), float("inf")])
def test_invalid_exploration(exploration: float) -> None:
    with pytest.raises(ValueError, match="Exploration"):
        MCTSStrategy[StonesState, int, StonesPlayer](10, exploration=exploration)


def test_strategy_contract() -> None:
    game = ThreeStones()
    strategy: Strategy[StonesState, int, StonesPlayer] = MCTSStrategy(30, rng=Random(42))

    move = strategy.choose_move(game, game.initial_state())

    assert move in game.legal_moves(game.initial_state())


def test_reproducible_with_seed() -> None:
    game = ThreeStones()
    state = game.initial_state()

    first = MCTSStrategy[StonesState, int, StonesPlayer](50, rng=Random(123))
    second = MCTSStrategy[StonesState, int, StonesPlayer](50, rng=Random(123))

    assert first.choose_move(game, state) == second.choose_move(game, state)


def test_single_simulation_returns_legal_move() -> None:
    game = ThreeStones()
    strategy = MCTSStrategy[StonesState, int, StonesPlayer](1, rng=Random(7))

    assert strategy.choose_move(game, game.initial_state()) in (0, 1, 2)


def test_terminal_state_rejected() -> None:
    game = ThreeStones()
    state = game.initial_state()

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    strategy = MCTSStrategy[StonesState, int, StonesPlayer](10, rng=Random(1))

    with pytest.raises(ValueError, match="No legal moves"):
        strategy.choose_move(game, state)


def test_tre_sigilli_legal_move_and_immutable_state() -> None:
    game = TreSigilli()
    state = game.initial_state()
    original = state

    strategy = MCTSStrategy[State, Move, Player](40, rng=Random(99))

    move = strategy.choose_move(game, state)

    assert move in game.legal_moves(state)
    assert state == original
