"""Reproducible full-match experiments for territorial Minimax."""

from collections import Counter
from random import Random

from boardlab.engine.game import Outcome
from boardlab.engine.match import Match
from boardlab.games.tre_sigilli import Move, Player, State, TreSigilli
from boardlab.strategies.minimax import MinimaxStrategy
from boardlab.strategies.random import RandomStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory


def play(seed: int, minimax_player: Player) -> tuple[Outcome, int]:
    game = TreSigilli()
    match: Match[State, Move, Player] = Match(game)

    minimax = MinimaxStrategy[State, Move, Player](evaluate_territory, depth=1)
    random = RandomStrategy[State, Move, Player](Random(seed))

    while not game.is_terminal(match.state):
        active = game.current_player(match.state)
        strategy = minimax if active is minimax_player else random
        match.play_strategy(strategy)

    return game.outcome(match.state, minimax_player), match.state.turn


def experiment() -> tuple[tuple[Outcome, int], ...]:
    return tuple(
        play(seed, player) for seed in range(50) for player in (Player.NORTH, Player.SOUTH)
    )


def test_benchmark_is_reproducible_and_reaches_terminal_states() -> None:
    first = experiment()
    second = experiment()

    assert first == second
    assert len(first) == 100
    assert all(6 <= turns <= 18 for _, turns in first)

    counts = Counter(outcome for outcome, _ in first)
    assert sum(counts.values()) == 100
    assert set(counts).issubset(set(Outcome))
