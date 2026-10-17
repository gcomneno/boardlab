"""Generic coordination of a manually played game."""

from boardlab.engine.game import Game
from boardlab.engine.strategy import Strategy


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

    def play_strategy(
        self,
        strategy: Strategy[StateT, MoveT, PlayerT],
    ) -> StateT:
        """Select and apply a move, committing only a valid successor."""
        if self._game.is_terminal(self._state):
            raise ValueError("Cannot play from a terminal state")

        move = strategy.choose_move(self._game, self._state)
        return self.play(move)
