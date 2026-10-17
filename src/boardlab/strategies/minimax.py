"""Deterministic, depth-limited minimax without pruning."""

from collections.abc import Callable
from dataclasses import dataclass
from math import isfinite

from boardlab.engine.game import Game, Outcome

type Evaluator[StateT, MoveT, PlayerT] = Callable[
    [Game[StateT, MoveT, PlayerT], StateT, PlayerT], float
]


@dataclass(frozen=True)
class SearchStats:
    """Counters for one completed search, including its root."""

    nodes_visited: int
    evaluated_leaves: int


class MinimaxStrategy[StateT, MoveT, PlayerT]:
    """Choose a move by adversarial search from the root player's perspective."""

    def __init__(
        self,
        evaluator: Evaluator[StateT, MoveT, PlayerT],
        depth: int,
    ) -> None:
        if type(depth) is not int or depth < 1:
            raise ValueError("Depth must be a positive integer")

        self._evaluator = evaluator
        self._depth = depth
        self._last_stats = SearchStats(0, 0)

    @property
    def last_stats(self) -> SearchStats:
        """Return immutable statistics from the last completed search."""
        return self._last_stats

    def choose_move(
        self,
        game: Game[StateT, MoveT, PlayerT],
        state: StateT,
    ) -> MoveT:
        """Return the first optimal legal move without changing the game state."""
        moves = game.legal_moves(state)

        if not moves:
            raise ValueError("No legal moves")

        root_player = game.current_player(state)
        nodes = 1
        leaves = 0

        def search(position: StateT, remaining: int) -> float:
            nonlocal nodes, leaves

            nodes += 1

            if game.is_terminal(position):
                leaves += 1
                outcome = game.outcome(position, root_player)

                if outcome is Outcome.WIN:
                    return float("inf")
                if outcome is Outcome.LOSS:
                    return float("-inf")
                return 0.0

            if remaining == 0:
                leaves += 1
                value = self._evaluator(game, position, root_player)

                if not isfinite(value):
                    raise ValueError("Evaluator must return a finite score")

                return value

            maximizing = game.current_player(position) == root_player
            candidates = game.legal_moves(position)

            if not candidates:
                raise ValueError("Nonterminal state has no legal moves")

            best = float("-inf") if maximizing else float("inf")

            for move in candidates:
                value = search(game.apply_move(position, move), remaining - 1)

                best = max(best, value) if maximizing else min(best, value)

            return best

        best_move = moves[0]
        best_value = float("-inf")

        for move in moves:
            value = search(game.apply_move(state, move), self._depth - 1)

            if value > best_value:
                best_value = value
                best_move = move

        self._last_stats = SearchStats(nodes, leaves)
        return best_move
