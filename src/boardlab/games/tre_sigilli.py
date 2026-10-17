"""Immutable state and action model for Tre Sigilli v0.2."""

from dataclasses import dataclass
from enum import Enum

from boardlab.engine.game import Outcome


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

    @staticmethod
    def _active_player(state: State) -> Player:
        """Derive the participant from the completed-action count."""
        return Player.NORTH if state.turn % 2 == 0 else Player.SOUTH

    @staticmethod
    def _adjacent(origin: int, destination: int) -> bool:
        """Check edge-sharing adjacency without wrapping across rows."""
        return abs(origin // 4 - destination // 4) + abs(origin % 4 - destination % 4) == 1

    def _maneuver_moves(self, state: State) -> tuple[Slide, ...]:
        """Enumerate maneuver actions without terminal-state recursion."""
        player = self._active_player(state)
        moves: list[Slide] = []

        for origin, owner in enumerate(state.cells):
            if owner is not player:
                continue

            for destination, occupant in enumerate(state.cells):
                if occupant is None and self._adjacent(origin, destination):
                    moves.append(Slide(origin, destination))

        return tuple(moves)

    def is_terminal(self, state: State) -> bool:
        """Detect the action limit or immobilization during maneuver."""
        self._validate(state)

        if state.turn == 18:
            return True

        return state.turn >= 6 and not self._maneuver_moves(state)

    def current_player(self, state: State) -> Player:
        """Return the active participant only while play can continue."""
        self._validate(state)

        if self.is_terminal(state):
            raise ValueError("Terminal state has no active player")

        return self._active_player(state)

    def legal_moves(self, state: State) -> tuple[Move, ...]:
        """Return deterministic legal actions for the current phase."""
        self._validate(state)

        if self.is_terminal(state):
            return ()

        if state.turn < 6:
            return tuple(Place(index) for index, owner in enumerate(state.cells) if owner is None)

        return self._maneuver_moves(state)

    def apply_move(self, state: State, move: Move) -> State:
        """Apply exactly one legal action, preserving the original state."""
        self._validate(state)

        if self.is_terminal(state):
            raise ValueError("Illegal move")

        if isinstance(move, Place):
            valid_shape = type(move.cell) is int
        elif isinstance(move, Slide):
            valid_shape = type(move.origin) is int and type(move.destination) is int
        else:
            valid_shape = False

        if not valid_shape or move not in self.legal_moves(state):
            raise ValueError("Illegal move")

        cells = list(state.cells)
        player = self._active_player(state)

        if isinstance(move, Place):
            cells[move.cell] = player
        else:
            cells[move.origin] = None
            cells[move.destination] = player

        successor = State(tuple(cells), state.turn + 1)
        self._validate(successor)
        return successor

    @staticmethod
    def _territory_scores(state: State) -> tuple[int, int]:
        """Score cells with strictly greater orthogonal influence."""
        north_score = 0
        south_score = 0

        for cell in range(16):
            row, col = divmod(cell, 4)
            influenced_by = [cell]

            if row > 0:
                influenced_by.append(cell - 4)
            if row < 3:
                influenced_by.append(cell + 4)
            if col > 0:
                influenced_by.append(cell - 1)
            if col < 3:
                influenced_by.append(cell + 1)

            north = sum(state.cells[index] is Player.NORTH for index in influenced_by)
            south = sum(state.cells[index] is Player.SOUTH for index in influenced_by)

            if north > south:
                north_score += 1
            elif south > north:
                south_score += 1

        return north_score, south_score

    def outcome(self, state: State, player: Player) -> Outcome:
        """Return the terminal territorial result for a participant."""
        self._validate(state)

        if not isinstance(player, Player):
            raise ValueError("Invalid player")

        if not self.is_terminal(state):
            raise ValueError("Outcome requires a terminal state")

        north, south = self._territory_scores(state)

        if north == south:
            return Outcome.DRAW

        winner = Player.NORTH if north > south else Player.SOUTH
        return Outcome.WIN if player is winner else Outcome.LOSS
