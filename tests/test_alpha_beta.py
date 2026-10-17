"""Behavioral and adversarial tests for Alpha-Beta pruning."""

from math import inf, nan

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.engine.strategy import Strategy
from boardlab.games.three_stones import Player, State, ThreeStones
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.minimax import MinimaxStrategy
from test_minimax_adversarial import TreeGame, TreeState, evaluate


def neutral(
    game: Game[State, int, Player],
    state: State,
    player: Player,
) -> float:
    return 0.0


def test_strategy_contract_and_equivalence() -> None:
    game = ThreeStones()
    alpha_beta: Strategy[State, int, Player] = AlphaBetaStrategy(neutral, depth=3)
    minimax = MinimaxStrategy[State, int, Player](neutral, depth=3)

    assert alpha_beta.choose_move(game, game.initial_state()) == (
        minimax.choose_move(game, game.initial_state())
    )


@pytest.mark.parametrize("depth", [0, -1, True])
def test_invalid_depth(depth: int) -> None:
    with pytest.raises(ValueError, match="Depth"):
        AlphaBetaStrategy[State, int, Player](neutral, depth)


def test_no_legal_moves() -> None:
    game = ThreeStones()
    state = game.initial_state()

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    strategy = AlphaBetaStrategy[State, int, Player](neutral, depth=3)

    with pytest.raises(ValueError, match="No legal moves"):
        strategy.choose_move(game, state)


def test_first_legal_move_wins_ties() -> None:
    game = ThreeStones()
    strategy = AlphaBetaStrategy[State, int, Player](neutral, depth=1)

    assert strategy.choose_move(game, game.initial_state()) == 0


def test_adversarial_equivalence_and_pruning() -> None:
    game = TreeGame(
        {
            "root": ("safe", "trap"),
            "safe": ("safe_a", "safe_b"),
            "trap": ("trap_bad", "trap_good"),
        }
    )
    minimax = MinimaxStrategy[TreeState, str, str](evaluate, depth=2)
    alpha_beta = AlphaBetaStrategy[TreeState, str, str](evaluate, depth=2)

    expected = minimax.choose_move(game, game.initial_state())
    actual = alpha_beta.choose_move(game, game.initial_state())

    assert actual == expected == "safe"
    assert alpha_beta.last_stats.nodes_visited < minimax.last_stats.nodes_visited
    assert alpha_beta.last_stats.evaluated_leaves < minimax.last_stats.evaluated_leaves


def test_terminal_infinities_preserve_first_optimal_move() -> None:
    game = TreeGame(
        {"root": ("win_a", "win_b")},
        {"win_a": Outcome.WIN, "win_b": Outcome.WIN},
    )
    strategy = AlphaBetaStrategy[TreeState, str, str](evaluate, depth=1)

    assert strategy.choose_move(game, game.initial_state()) == "win_a"


@pytest.mark.parametrize("invalid", [nan, inf, -inf])
def test_nonfinite_evaluator_is_rejected(invalid: float) -> None:
    game = ThreeStones()

    def invalid_evaluator(
        game: Game[State, int, Player],
        state: State,
        player: Player,
    ) -> float:
        return invalid

    strategy = AlphaBetaStrategy[State, int, Player](invalid_evaluator, depth=1)
    previous = strategy.last_stats

    with pytest.raises(ValueError, match="finite"):
        strategy.choose_move(game, game.initial_state())

    assert strategy.last_stats == previous


def test_input_state_is_preserved() -> None:
    game = ThreeStones()
    state = game.initial_state()
    strategy = AlphaBetaStrategy[State, int, Player](neutral, depth=3)

    strategy.choose_move(game, state)

    assert state == game.initial_state()
