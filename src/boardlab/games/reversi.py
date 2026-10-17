"""Immutable value model for Reversi on a 6x6 board."""

from dataclasses import dataclass
from enum import Enum

from boardlab.engine.game import Outcome


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

    @staticmethod
    def _opponent(player: Player) -> Player:
        return Player.WHITE if player is Player.BLACK else Player.BLACK

    @classmethod
    def _captures(cls, state: State, cell: int, player: Player) -> tuple[int, ...]:
        if state.cells[cell] is not None:
            return ()

        row, col = divmod(cell, cls.SIZE)
        opponent = cls._opponent(player)
        captured: list[int] = []

        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                if dr == dc == 0:
                    continue

                r, c = row + dr, col + dc
                ray: list[int] = []

                while 0 <= r < cls.SIZE and 0 <= c < cls.SIZE:
                    index = r * cls.SIZE + c
                    if state.cells[index] is opponent:
                        ray.append(index)
                    elif state.cells[index] is player:
                        if ray:
                            captured.extend(ray)
                        break
                    else:
                        break

                    r += dr
                    c += dc

        return tuple(captured)

    @classmethod
    def _placements(cls, state: State, player: Player) -> tuple[Place, ...]:
        return tuple(
            Place(cell) for cell in range(cls.CELL_COUNT) if cls._captures(state, cell, player)
        )

    def outcome(self, state: State, player: Player) -> Outcome:
        """Return the terminal disc-count result for a participant."""
        self._validate(state)

        if not isinstance(player, Player):
            raise ValueError("Invalid player")

        if not self.is_terminal(state):
            raise ValueError("Outcome requires a terminal state")

        owned = state.cells.count(player)
        other = state.cells.count(self._opponent(player))

        if owned == other:
            return Outcome.DRAW

        return Outcome.WIN if owned > other else Outcome.LOSS

    def is_terminal(self, state: State) -> bool:
        self._validate(state)
        return not self._placements(state, Player.BLACK) and not self._placements(
            state, Player.WHITE
        )

    def current_player(self, state: State) -> Player:
        self._validate(state)
        if self.is_terminal(state):
            raise ValueError("Terminal state has no active player")
        return state.active_player

    def legal_moves(self, state: State) -> tuple[Move, ...]:
        self._validate(state)

        placements = self._placements(state, state.active_player)
        if placements:
            return placements

        if self._placements(state, self._opponent(state.active_player)):
            return (Pass(),)

        return ()

    def apply_move(self, state: State, move: Move) -> State:
        self._validate(state)

        valid_shape = type(move.cell) is int if isinstance(move, Place) else isinstance(move, Pass)

        if not valid_shape or move not in self.legal_moves(state):
            raise ValueError("Illegal move")

        opponent = self._opponent(state.active_player)

        if isinstance(move, Pass):
            successor = State(state.cells, opponent)
        else:
            cells = list(state.cells)
            cells[move.cell] = state.active_player

            for index in self._captures(state, move.cell, state.active_player):
                cells[index] = state.active_player

            successor = State(tuple(cells), opponent)

        self._validate(successor)
        return successor
