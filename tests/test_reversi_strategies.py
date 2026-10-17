"""Compatibility of generic search strategies with Reversi 6x6."""

from random import Random

import pytest

from boardlab.engine.game import Game
from boardlab.engine.strategy import Strategy
from boardlab.games.reversi import Move, Pass, Player, Reversi, State
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.mcts import MCTSStrategy
from boardlab.strategies.minimax import MinimaxStrategy


def disc_difference(
    game: Game[State, Move, Player],
    state: State,
    player: Player,
) -> float:
    """Evaluate material advantage from the requested perspective."""
    opponent = Player.WHITE if player is Player.BLACK else Player.BLACK
    return float(state.cells.count(player) - state.cells.count(opponent))


def forced_pass_state() -> State:
    """Reproduce a forced pass through a seeded legal game."""
    game = Reversi()
    rng = Random(1)
    state = game.initial_state()

    for _ in range(100):
        if game.is_terminal(state):
            pytest.fail("Game terminated before a forced pass")

        moves = game.legal_moves(state)

        if moves == (Pass(),):
            return state

        state = game.apply_move(state, rng.choice(moves))

    pytest.fail("No forced pass found within 100 actions")


@pytest.mark.parametrize("depth", [1, 2, 3])
def test_minimax_and_alpha_beta_agree(depth: int) -> None:
    game = Reversi()
    state = game.initial_state()

    minimax = MinimaxStrategy[State, Move, Player](disc_difference, depth)
    alpha_beta = AlphaBetaStrategy[State, Move, Player](disc_difference, depth)

    first = minimax.choose_move(game, state)
    second = alpha_beta.choose_move(game, state)

    assert first == second
    assert first in game.legal_moves(state)
    assert state == game.initial_state()

    assert alpha_beta.last_stats.nodes_visited <= minimax.last_stats.nodes_visited


@pytest.mark.parametrize("strategy_name", ["minimax", "alpha_beta", "mcts"])
def test_strategies_choose_forced_pass(strategy_name: str) -> None:
    game = Reversi()
    state = forced_pass_state()
    original = state

    strategy: Strategy[State, Move, Player]
    if strategy_name == "minimax":
        strategy = MinimaxStrategy[State, Move, Player](disc_difference, depth=2)
    elif strategy_name == "alpha_beta":
        strategy = AlphaBetaStrategy[State, Move, Player](disc_difference, depth=2)
    else:
        strategy = MCTSStrategy[State, Move, Player](16, rng=Random(42))

    assert game.legal_moves(state) == (Pass(),)
    assert strategy.choose_move(game, state) == Pass()
    assert state is original

    successor = game.apply_move(state, Pass())
    assert successor.cells == state.cells
    assert successor.active_player is not state.active_player


def test_mcts_is_seed_reproducible() -> None:
    game = Reversi()
    state = game.initial_state()

    first = MCTSStrategy[State, Move, Player](32, rng=Random(123))
    second = MCTSStrategy[State, Move, Player](32, rng=Random(123))

    assert first.choose_move(game, state) == second.choose_move(game, state)
    assert state == game.initial_state()


@pytest.mark.parametrize("strategy_name", ["minimax", "alpha_beta", "mcts"])
def test_strategies_choose_legal_opening(strategy_name: str) -> None:
    game = Reversi()
    state = game.initial_state()

    if strategy_name == "minimax":
        move = MinimaxStrategy[State, Move, Player](disc_difference, depth=2).choose_move(
            game, state
        )
    elif strategy_name == "alpha_beta":
        move = AlphaBetaStrategy[State, Move, Player](disc_difference, depth=2).choose_move(
            game, state
        )
    else:
        move = MCTSStrategy[State, Move, Player](16, rng=Random(42)).choose_move(game, state)

    assert move in game.legal_moves(state)
    assert state == game.initial_state()
