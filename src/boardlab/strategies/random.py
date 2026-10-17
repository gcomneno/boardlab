"""Uniform random selection from legal moves."""

from random import Random

from boardlab.engine.game import Game


class RandomStrategy[StateT, MoveT, PlayerT]:
    """Select legal moves using an independently supplied random generator."""

    def __init__(self, rng: Random | None = None) -> None:
        self._rng = rng if rng is not None else Random()

    def choose_move(
        self,
        game: Game[StateT, MoveT, PlayerT],
        state: StateT,
    ) -> MoveT:
        """Choose a legal move; reject states without legal moves."""
        moves = game.legal_moves(state)

        if not moves:
            raise ValueError("No legal moves")

        return self._rng.choice(moves)
