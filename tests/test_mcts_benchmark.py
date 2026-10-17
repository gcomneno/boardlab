"""Reproducible MCTS comparisons against established strategies."""

from collections import Counter
from random import Random

import pytest

from boardlab.engine.game import Outcome
from boardlab.engine.match import Match
from boardlab.engine.strategy import Strategy
from boardlab.games.tre_sigilli import Move, Player, State, TreSigilli
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.mcts import MCTSStrategy
from boardlab.strategies.minimax import MinimaxStrategy
from boardlab.strategies.random import RandomStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory

type TreSigilliStrategy = Strategy[State, Move, Player]


def opponent(name: str, seed: int) -> TreSigilliStrategy:
    """Construct an opponent with explicit, reproducible parameters."""
    if name == "random":
        return RandomStrategy[State, Move, Player](Random(seed))
    if name == "minimax":
        return MinimaxStrategy[State, Move, Player](evaluate_territory, depth=1)
    if name == "alpha_beta":
        return AlphaBetaStrategy[State, Move, Player](evaluate_territory, depth=1)
    raise ValueError(f"Unknown opponent: {name}")


def play(
    name: str,
    seed: int,
    mcts_player: Player,
    simulations: int = 40,
) -> tuple[Outcome, int]:
    """Play a complete match with the MCTS side explicitly selected."""
    game = TreSigilli()
    match: Match[State, Move, Player] = Match(game)

    mcts: TreSigilliStrategy = MCTSStrategy[State, Move, Player](simulations, rng=Random(seed))
    rival = opponent(name, seed + 10_000)

    while not game.is_terminal(match.state):
        active = game.current_player(match.state)
        match.play_strategy(mcts if active is mcts_player else rival)

    return game.outcome(match.state, mcts_player), match.state.turn


@pytest.mark.parametrize("name", ["random", "minimax", "alpha_beta"])
def test_mcts_benchmark_reproducibility(name: str) -> None:
    """Repeat paired matches with identical seeds and both player roles."""
    first = tuple(
        play(name, seed, player) for seed in range(5) for player in (Player.NORTH, Player.SOUTH)
    )
    second = tuple(
        play(name, seed, player) for seed in range(5) for player in (Player.NORTH, Player.SOUTH)
    )

    assert first == second
    assert len(first) == 10
    assert all(6 <= turns <= 18 for _, turns in first)

    counts = Counter(outcome for outcome, _ in first)
    assert sum(counts.values()) == 10
    assert set(counts).issubset(set(Outcome))
