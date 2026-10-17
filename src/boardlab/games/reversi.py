"""Immutable value model for Reversi on a 6x6 board."""

from dataclasses import dataclass
from enum import Enum


class Player(Enum):
    """Reversi participants."""

    BLACK = "black"
    WHITE = "white"


type Cell = Player | None
type Board = tuple[Cell, ...]


@dataclass(frozen=True)
class State:
    """Complete board position and participant to act."""

    cells: Board
    active_player: Player


@dataclass(frozen=True)
class Place:
    """Place a disc at a zero-based board index."""

    cell: int


@dataclass(frozen=True)
class Pass:
    """Explicit action when no placement is available."""


type Move = Place | Pass


class Reversi:
    """Reversi 6x6 model; gameplay rules follow in later milestones."""

    SIZE = 6
    CELL_COUNT = SIZE * SIZE

    @staticmethod
    def _validate(state: State) -> None:
        """Reject malformed state representations."""
        if not isinstance(state, State):
            raise ValueError("Invalid game state")

        if (
            not isinstance(state.cells, tuple)
            or len(state.cells) != Reversi.CELL_COUNT
            or any(cell is not None and not isinstance(cell, Player) for cell in state.cells)
            or not isinstance(state.active_player, Player)
        ):
            raise ValueError("Invalid game state")

    def initial_state(self) -> State:
        """Return the standard immutable opening position."""
        cells: list[Cell] = [None] * self.CELL_COUNT
        cells[14] = Player.WHITE
        cells[15] = Player.BLACK
        cells[20] = Player.BLACK
        cells[21] = Player.WHITE

        state = State(tuple(cells), Player.BLACK)
        self._validate(state)
        return state
