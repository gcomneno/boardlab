"""Exercise the generic contract using the Session 01 Three Stones example."""

from dataclasses import dataclass
from enum import Enum

import pytest

from boardlab.engine.game import Game, Outcome


class Player(Enum):
    NORTH = "north"
    SOUTH = "south"


@dataclass(frozen=True)
class State:
    cells: tuple[Player | None, Player | None, Player | None]
    next_player: Player


class ThreeStones:
    """Test fixture: fill three cells; the majority owner wins."""

    def initial_state(self) -> State:
        return State((None, None, None), Player.NORTH)

    def current_player(self, state: State) -> Player:
        if self.is_terminal(state):
            raise ValueError("Terminal state has no active player")
        return state.next_player

    def legal_moves(self, state: State) -> tuple[int, ...]:
        return tuple(index for index, owner in enumerate(state.cells) if owner is None)

    def apply_move(self, state: State, move: int) -> State:
        if move not in self.legal_moves(state):
            raise ValueError("Illegal move")
        cells = tuple(
            state.next_player if index == move else owner for index, owner in enumerate(state.cells)
        )
        next_player = Player.SOUTH if state.next_player is Player.NORTH else Player.NORTH
        return State((cells[0], cells[1], cells[2]), next_player)

    def is_terminal(self, state: State) -> bool:
        return all(owner is not None for owner in state.cells)

    def outcome(self, state: State, player: Player) -> Outcome:
        if not self.is_terminal(state):
            raise ValueError("Outcome requires a terminal state")
        owned = state.cells.count(player)
        other = len(state.cells) - owned
        if owned == other:
            return Outcome.DRAW
        return Outcome.WIN if owned > other else Outcome.LOSS


def game_fixture() -> Game[State, int, Player]:
    """Static typing checks structural compatibility with the public contract."""
    return ThreeStones()


def test_transition_preserves_original_and_independent_branches() -> None:
    game = game_fixture()
    original = game.initial_state()

    assert game.current_player(original) is Player.NORTH
    assert game.legal_moves(original) == (0, 1, 2)
    assert not game.is_terminal(original)

    left = game.apply_move(original, 0)
    right = game.apply_move(original, 1)

    assert original == State((None, None, None), Player.NORTH)
    assert left == State((Player.NORTH, None, None), Player.SOUTH)
    assert right == State((None, Player.NORTH, None), Player.SOUTH)
    assert game.current_player(left) is Player.SOUTH
    assert game.legal_moves(left) == (1, 2)

    continued = game.apply_move(left, 1)
    assert continued == State((Player.NORTH, Player.SOUTH, None), Player.NORTH)
    assert left == State((Player.NORTH, None, None), Player.SOUTH)
    assert right == State((None, Player.NORTH, None), Player.SOUTH)


@pytest.mark.parametrize("move", [-1, 0, 3])
def test_illegal_move_is_rejected_without_changing_state(move: int) -> None:
    game = game_fixture()
    state = game.apply_move(game.initial_state(), 0)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, move)

    assert state == State((Player.NORTH, None, None), Player.SOUTH)


def test_terminal_result_uses_player_perspective() -> None:
    game = game_fixture()
    state = game.initial_state()

    with pytest.raises(ValueError, match="terminal"):
        game.outcome(state, Player.NORTH)

    for move in (0, 1, 2):
        state = game.apply_move(state, move)

    assert state == State((Player.NORTH, Player.SOUTH, Player.NORTH), Player.SOUTH)
    assert game.is_terminal(state)
    assert game.legal_moves(state) == ()
    assert game.outcome(state, Player.NORTH) is Outcome.WIN
    assert game.outcome(state, Player.SOUTH) is Outcome.LOSS

    with pytest.raises(ValueError, match="active player"):
        game.current_player(state)

    with pytest.raises(ValueError, match="Illegal move"):
        game.apply_move(state, 0)
