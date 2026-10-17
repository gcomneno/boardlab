"""Immutable state and action model for Tre Sigilli v0.2."""

from dataclasses import dataclass
from enum import Enum


class Player(Enum):
    """Participants in Tre Sigilli."""

    NORTH = "north"
    SOUTH = "south"


type Cell = Player | None
type Board = tuple[Cell, ...]


@dataclass(frozen=True)
class State:
    """Board occupancy and number of completed actions."""

    cells: Board
    turn: int


@dataclass(frozen=True)
class Place:
    """Place a seal on an empty cell."""

    cell: int


@dataclass(frozen=True)
class Slide:
    """Move a seal between orthogonally adjacent cells."""

    origin: int
    destination: int


type Move = Place | Slide


class TreSigilli:
    """Tre Sigilli model; gameplay rules follow in later milestones."""

    @staticmethod
    def _validate(state: State) -> None:
        """Reject structurally invalid states and phase counts."""
        if not isinstance(state, State):
            raise ValueError("Invalid game state")

        if (
            not isinstance(state.cells, tuple)
            or len(state.cells) != 16
            or any(cell is not None and not isinstance(cell, Player) for cell in state.cells)
            or type(state.turn) is not int
            or not 0 <= state.turn <= 18
        ):
            raise ValueError("Invalid game state")

        north = state.cells.count(Player.NORTH)
        south = state.cells.count(Player.SOUTH)

        expected_north = min((state.turn + 1) // 2, 3)
        expected_south = min(state.turn // 2, 3)

        if north != expected_north or south != expected_south:
            raise ValueError("Invalid game state")

    def initial_state(self) -> State:
        """Return a validated, empty, immutable board."""
        state = State((None,) * 16, 0)
        self._validate(state)
        return state
