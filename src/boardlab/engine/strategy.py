"""Generic contract for choosing moves without coordinating a match."""

from typing import Protocol

from boardlab.engine.game import Game


class Strategy[StateT, MoveT, PlayerT](Protocol):
    """Choose a legal move without modifying the supplied game state.

    Implementations must raise ValueError when no legal move exists.
    The game remains responsible for validating and applying moves.
    """

    def choose_move(
        self,
        game: Game[StateT, MoveT, PlayerT],
        state: StateT,
    ) -> MoveT:
        """Return a legal move for the supplied nonterminal state."""
        ...
