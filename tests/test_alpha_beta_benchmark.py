"""Deterministic comparison of Minimax and Alpha-Beta on Tre Sigilli."""

import pytest

from boardlab.games.tre_sigilli import Move, Player, State, TreSigilli
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.minimax import MinimaxStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory


def sample_states(game: TreSigilli) -> tuple[State, ...]:
    """Build a reproducible sequence of legal, nonterminal states."""
    states = []
    state = game.initial_state()

    for turn in range(6):
        if game.is_terminal(state):
            break

        states.append(state)
        moves = game.legal_moves(state)
        state = game.apply_move(state, moves[turn % len(moves)])

    return tuple(states)


@pytest.mark.parametrize("depth", [1, 2, 3])
def test_alpha_beta_matches_minimax_and_never_expands_more(
    depth: int,
) -> None:
    game = TreSigilli()
    states = sample_states(game)

    assert len(states) == 6

    total_minimax_nodes = 0
    total_alpha_beta_nodes = 0

    for state in states:
        minimax = MinimaxStrategy[State, Move, Player](evaluate_territory, depth)
        alpha_beta = AlphaBetaStrategy[State, Move, Player](evaluate_territory, depth)

        assert alpha_beta.choose_move(game, state) == (minimax.choose_move(game, state))

        assert alpha_beta.last_stats.nodes_visited <= minimax.last_stats.nodes_visited
        assert alpha_beta.last_stats.evaluated_leaves <= minimax.last_stats.evaluated_leaves

        total_minimax_nodes += minimax.last_stats.nodes_visited
        total_alpha_beta_nodes += alpha_beta.last_stats.nodes_visited

    if depth >= 2:
        assert total_alpha_beta_nodes < total_minimax_nodes
