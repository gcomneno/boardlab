"""Generic coordination of a manually played game."""

from boardlab.engine.game import Game


class Match[StateT, MoveT, PlayerT]:
    """Maintain the current state while delegating rules to a game.

    A failed transition must leave the current state unchanged.
    """

    def __init__(self, game: Game[StateT, MoveT, PlayerT]) -> None:
        self._game = game
        self._state = game.initial_state()

    @property
    def state(self) -> StateT:
        """Return the current game state."""
        return self._state

    def play(self, move: MoveT) -> StateT:
        """Apply a move and commit the successor only on success."""
        successor = self._game.apply_move(self._state, move)
        self._state = successor
        return successor
