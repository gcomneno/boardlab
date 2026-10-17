"""Three Stones: a minimal concrete game implementing the Game contract."""

from dataclasses import dataclass
from enum import Enum

from boardlab.engine.game import Outcome


class Player(Enum):
    """Participants in Three Stones."""

    NORTH = "north"
    SOUTH = "south"


@dataclass(frozen=True)
class State:
    """Complete game position with an immutable board."""

    cells: tuple[Player | None, Player | None, Player | None]
    next_player: Player


class ThreeStones:
    """Fill three cells in alternating turns; the majority owner wins."""

    @staticmethod
    def _validate(state: State) -> None:
        if not isinstance(state, State):
            raise ValueError("Invalid game state")

        if (
            not isinstance(state.cells, tuple)
            or len(state.cells) != 3
            or any(cell is not None and not isinstance(cell, Player) for cell in state.cells)
            or not isinstance(state.next_player, Player)
        ):
            raise ValueError("Invalid game state")

        north = state.cells.count(Player.NORTH)
        south = state.cells.count(Player.SOUTH)

        if north < south or north > south + 1:
            raise ValueError("Invalid game state")

        expected = Player.NORTH if north == south else Player.SOUTH

        if state.next_player is not expected:
            raise ValueError("Invalid game state")

    def initial_state(self) -> State:
        """Return the empty initial position."""
        return State((None, None, None), Player.NORTH)

    def current_player(self, state: State) -> Player:
        """Return the participant to move in a nonterminal state."""
        self._validate(state)

        if self.is_terminal(state):
            raise ValueError("Terminal state has no active player")

        return state.next_player

    def legal_moves(self, state: State) -> tuple[int, ...]:
        """Return indices of unoccupied cells."""
        self._validate(state)
        return tuple(index for index, owner in enumerate(state.cells) if owner is None)

    def apply_move(self, state: State, move: int) -> State:
        """Apply a legal move without modifying the input state."""
        self._validate(state)

        if type(move) is not int or move not in self.legal_moves(state):
            raise ValueError("Illegal move")

        cells = tuple(
            state.next_player if index == move else owner for index, owner in enumerate(state.cells)
        )
        next_player = Player.SOUTH if state.next_player is Player.NORTH else Player.NORTH

        successor = State((cells[0], cells[1], cells[2]), next_player)
        self._validate(successor)
        return successor

    def is_terminal(self, state: State) -> bool:
        """Return whether every cell has been claimed."""
        self._validate(state)
        return all(owner is not None for owner in state.cells)

    def outcome(self, state: State, player: Player) -> Outcome:
        """Return the terminal outcome from a participant's perspective."""
        self._validate(state)

        if not isinstance(player, Player):
            raise ValueError("Invalid player")

        if not self.is_terminal(state):
            raise ValueError("Outcome requires a terminal state")

        owned = state.cells.count(player)
        other = len(state.cells) - owned

        if owned == other:
            return Outcome.DRAW

        return Outcome.WIN if owned > other else Outcome.LOSS
