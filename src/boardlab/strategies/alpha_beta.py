"""Deterministic, depth-limited minimax with alpha-beta pruning."""

from math import isfinite

from boardlab.engine.game import Game, Outcome
from boardlab.strategies.minimax import Evaluator, SearchStats


class AlphaBetaStrategy[StateT, MoveT, PlayerT]:
    """Choose the first optimal move using alpha-beta pruning."""

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
        """Return statistics from the last completed search."""
        return self._last_stats

    def choose_move(
        self,
        game: Game[StateT, MoveT, PlayerT],
        state: StateT,
    ) -> MoveT:
        """Return the first optimal legal move without modifying state."""
        moves = game.legal_moves(state)

        if not moves:
            raise ValueError("No legal moves")

        root_player = game.current_player(state)
        nodes = 1
        leaves = 0

        def search(
            position: StateT,
            remaining: int,
            alpha: float,
            beta: float,
        ) -> float:
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
                value = search(
                    game.apply_move(position, move),
                    remaining - 1,
                    alpha,
                    beta,
                )

                if maximizing:
                    best = max(best, value)
                    alpha = max(alpha, best)
                else:
                    best = min(best, value)
                    beta = min(beta, best)

                if alpha >= beta:
                    break

            return best

        best_move = moves[0]
        best_value = float("-inf")
        alpha = float("-inf")

        for move in moves:
            value = search(
                game.apply_move(state, move),
                self._depth - 1,
                alpha,
                float("inf"),
            )

            if value > best_value:
                best_value = value
                best_move = move

            alpha = max(alpha, best_value)

        self._last_stats = SearchStats(nodes, leaves)
        return best_move
