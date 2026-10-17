"""Minimal game contract, independent of concrete game representations."""

from enum import Enum
from typing import Protocol


class Outcome(Enum):
    """Terminal result from a participant's perspective."""

    WIN = "win"
    LOSS = "loss"
    DRAW = "draw"


class Game[StateT, MoveT, PlayerT](Protocol):
    """Rules own legality, transitions, termination and terminal results.

    States must contain all gameplay information. Applying a move must
    preserve the input state and produce an independent successor.
    """

    def initial_state(self) -> StateT:
        """Return a complete, valid initial state."""
        ...

    def current_player(self, state: StateT) -> PlayerT:
        """Return the active participant in a nonterminal state."""
        ...

    def legal_moves(self, state: StateT) -> tuple[MoveT, ...]:
        """Return the legal actions according to this game's rules."""
        ...

    def apply_move(self, state: StateT, move: MoveT) -> StateT:
        """Return an independent successor; reject illegal moves with ValueError."""
        ...

    def is_terminal(self, state: StateT) -> bool:
        """Determine termination from game rules, not generic assumptions."""
        ...

    def outcome(self, state: StateT, player: PlayerT) -> Outcome:
        """Return a terminal result; raise ValueError if play is ongoing."""
        ...
