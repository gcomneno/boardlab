"""Reproducible Reversi 6x6 benchmark harness.

This module defines experiments; it does not execute them on import.
"""

from dataclasses import dataclass
from random import Random
from time import perf_counter
from typing import Literal

from boardlab.engine.game import Game, Outcome
from boardlab.engine.match import Match
from boardlab.engine.strategy import Strategy
from boardlab.games.reversi import Move, Player, Reversi, State
from boardlab.strategies.alpha_beta import AlphaBetaStrategy
from boardlab.strategies.mcts import MCTSStrategy
from boardlab.strategies.minimax import MinimaxStrategy
from boardlab.strategies.random import RandomStrategy

type StrategyName = Literal["random", "minimax", "alpha_beta", "mcts"]
type ReversiStrategy = Strategy[State, Move, Player]


@dataclass(frozen=True)
class Configuration:
    """Explicit experimental budgets."""

    depth: int = 2
    simulations: int = 32

    def __post_init__(self) -> None:
        if type(self.depth) is not int or self.depth < 1:
            raise ValueError("Depth must be a positive integer")
        if type(self.simulations) is not int or self.simulations < 1:
            raise ValueError("Simulations must be a positive integer")


@dataclass(frozen=True)
class Result:
    """Observation from one complete match, from the candidate perspective."""

    candidate: StrategyName
    seed: int
    candidate_player: Player
    outcome: Outcome
    actions: int
    elapsed_seconds: float
    nodes_visited: int | None
    evaluated_leaves: int | None


def disc_difference(
    game: Game[State, Move, Player],
    state: State,
    player: Player,
) -> float:
    """Evaluate disc advantage from the requested perspective."""
    opponent = Player.WHITE if player is Player.BLACK else Player.BLACK
    return float(state.cells.count(player) - state.cells.count(opponent))


def make_strategy(
    name: StrategyName,
    seed: int,
    config: Configuration,
) -> ReversiStrategy:
    """Construct an independent strategy for one participant and match."""
    if name == "random":
        return RandomStrategy[State, Move, Player](Random(seed))
    if name == "minimax":
        return MinimaxStrategy[State, Move, Player](disc_difference, config.depth)
    if name == "alpha_beta":
        return AlphaBetaStrategy[State, Move, Player](disc_difference, config.depth)
    if name == "mcts":
        return MCTSStrategy[State, Move, Player](
            config.simulations,
            rng=Random(seed),
        )
    raise ValueError(f"Unknown strategy: {name}")


def play_match(
    candidate: StrategyName,
    seed: int,
    candidate_player: Player,
    config: Configuration | None = None,
) -> Result:
    """Play one candidate-versus-Random match with isolated RNG streams."""
    if config is None:
        config = Configuration()

    if candidate not in ("minimax", "alpha_beta", "mcts"):
        raise ValueError("Candidate must be minimax, alpha_beta or mcts")
    if type(seed) is not int or seed < 0:
        raise ValueError("Seed must be a nonnegative integer")
    if not isinstance(candidate_player, Player):
        raise ValueError("Invalid candidate player")

    game = Reversi()
    match: Match[State, Move, Player] = Match(game)

    candidate_strategy = make_strategy(candidate, seed, config)
    baseline_strategy = make_strategy("random", seed + 10_000, config)

    actions = 0
    nodes = 0
    leaves = 0
    has_search_stats = isinstance(candidate_strategy, (MinimaxStrategy, AlphaBetaStrategy))

    started = perf_counter()

    while not game.is_terminal(match.state):
        active = game.current_player(match.state)

        if active is candidate_player:
            match.play_strategy(candidate_strategy)

            if isinstance(candidate_strategy, (MinimaxStrategy, AlphaBetaStrategy)):
                stats = candidate_strategy.last_stats
                nodes += stats.nodes_visited
                leaves += stats.evaluated_leaves
        else:
            match.play_strategy(baseline_strategy)

        actions += 1

    elapsed = perf_counter() - started

    return Result(
        candidate=candidate,
        seed=seed,
        candidate_player=candidate_player,
        outcome=game.outcome(match.state, candidate_player),
        actions=actions,
        elapsed_seconds=elapsed,
        nodes_visited=nodes if has_search_stats else None,
        evaluated_leaves=leaves if has_search_stats else None,
    )


def pilot(
    config: Configuration | None = None,
    seeds: tuple[int, ...] = (0, 1, 2, 3, 4),
) -> tuple[Result, ...]:
    """Run the declared 30-match pilot in deterministic experiment order."""
    if config is None:
        config = Configuration()

    return tuple(
        play_match(candidate, seed, player, config)
        for candidate in ("minimax", "alpha_beta", "mcts")
        for seed in seeds
        for player in (Player.BLACK, Player.WHITE)
    )
