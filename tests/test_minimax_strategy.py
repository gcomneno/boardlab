"""Behavioral contract for deterministic depth-limited Minimax."""

from math import inf, nan

import pytest

from boardlab.engine.game import Game
from boardlab.engine.strategy import Strategy
from boardlab.games.three_stones import Player, State, ThreeStones
from boardlab.strategies.minimax import MinimaxStrategy


def neutral_evaluator(
    game: Game[State, int, Player],
    state: State,
    player: Player,
) -> float:
    """Provide a neutral score for nonterminal cutoffs."""
    return 0.0


def test_minimax_satisfies_strategy_contract() -> None:
    game = ThreeStones()
    strategy: Strategy[State, int, Player] = MinimaxStrategy[State, int, Player](
        neutral_evaluator, depth=3
    )
    assert strategy.choose_move(game, game.initial_state()) in (0, 1, 2)


@pytest.mark.parametrize("depth", [0, -1])
def test_rejects_invalid_depth(depth: int) -> None:
    with pytest.raises(ValueError):
        MinimaxStrategy[State, int, Player](neutral_evaluator, depth=depth)


def test_rejects_terminal_state() -> None:
    game = ThreeStones()
    strategy = MinimaxStrategy[State, int, Player](neutral_evaluator, depth=3)
    state = game.initial_state()

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    with pytest.raises(ValueError, match="No legal moves"):
        strategy.choose_move(game, state)


def test_tie_breaking_uses_first_legal_move() -> None:
    game = ThreeStones()
    strategy = MinimaxStrategy[State, int, Player](neutral_evaluator, depth=1)

    assert strategy.choose_move(game, game.initial_state()) == 0


def test_search_preserves_input_state() -> None:
    game = ThreeStones()
    state = game.initial_state()
    strategy = MinimaxStrategy[State, int, Player](neutral_evaluator, depth=3)

    strategy.choose_move(game, state)

    assert state == game.initial_state()


def test_search_reports_deterministic_metrics() -> None:
    game = ThreeStones()
    strategy = MinimaxStrategy[State, int, Player](neutral_evaluator, depth=3)

    strategy.choose_move(game, game.initial_state())

    assert strategy.last_stats.nodes_visited == 16
    assert strategy.last_stats.evaluated_leaves == 6


@pytest.mark.parametrize("invalid", [nan, inf, -inf])
def test_rejects_nonfinite_heuristic(invalid: float) -> None:
    game = ThreeStones()

    def invalid_evaluator(
        game: Game[State, int, Player],
        state: State,
        player: Player,
    ) -> float:
        return invalid

    strategy = MinimaxStrategy[State, int, Player](invalid_evaluator, depth=1)

    with pytest.raises(ValueError):
        strategy.choose_move(game, game.initial_state())


def test_terminal_outcomes_are_not_heuristically_evaluated() -> None:
    game = ThreeStones()

    def forbidden_evaluator(
        game: Game[State, int, Player],
        state: State,
        player: Player,
    ) -> float:
        raise AssertionError("Terminal states must use game.outcome")

    strategy = MinimaxStrategy[State, int, Player](forbidden_evaluator, depth=3)

    assert strategy.choose_move(game, game.initial_state()) == 0
