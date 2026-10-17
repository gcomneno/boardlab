"""Verify Tre Sigilli's immutable model and structural invariants."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.games.tre_sigilli import Place, Player, Slide, State, TreSigilli


def board(
    north: tuple[int, ...] = (),
    south: tuple[int, ...] = (),
) -> tuple[Player | None, ...]:
    """Construct test occupancy without using gameplay transitions."""
    cells: list[Player | None] = [None] * 16

    for index in north:
        cells[index] = Player.NORTH

    for index in south:
        cells[index] = Player.SOUTH

    return tuple(cells)


def test_initial_state_is_empty_and_valid() -> None:
    game = TreSigilli()
    state = game.initial_state()

    assert state == State((None,) * 16, 0)
    assert len(state.cells) == 16
    game._validate(state)


@pytest.mark.parametrize(
    ("turn", "north", "south"),
    [
        (0, (), ()),
        (1, (0,), ()),
        (2, (0,), (1,)),
        (3, (0, 2), (1,)),
        (4, (0, 2), (1, 3)),
        (5, (0, 2, 4), (1, 3)),
        (6, (0, 2, 4), (1, 3, 5)),
        (7, (0, 2, 4), (1, 3, 5)),
        (18, (0, 2, 4), (1, 3, 5)),
    ],
)
def test_valid_phase_counts(
    turn: int,
    north: tuple[int, ...],
    south: tuple[int, ...],
) -> None:
    TreSigilli._validate(State(board(north, south), turn))


@pytest.mark.parametrize(
    "state",
    [
        State((None,) * 15, 0),
        State((None,) * 17, 0),
        State([None] * 16, 0),  # type: ignore[arg-type]
        State((Player.NORTH, "north") + (None,) * 14, 1),  # type: ignore[arg-type]
        State((None,) * 16, True),
        State((None,) * 16, -1),
        State((None,) * 16, 19),
        State(board(north=(0,)), 0),
        State(board(south=(0,)), 1),
        State(board(north=(0, 1)), 2),
        State(board(north=(0, 1, 2), south=(3, 4)), 6),
        State(board(north=(0, 1, 2), south=(3, 4, 5)), 5),
    ],
)
def test_invalid_state_is_rejected(state: State) -> None:
    with pytest.raises(ValueError, match="Invalid game state"):
        TreSigilli._validate(state)


def test_invalid_state_type_is_rejected() -> None:
    with pytest.raises(ValueError, match="Invalid game state"):
        TreSigilli._validate(None)  # type: ignore[arg-type]


def test_state_is_immutable() -> None:
    state = TreSigilli().initial_state()

    with pytest.raises(FrozenInstanceError):
        state.turn = 1  # type: ignore[misc]

    with pytest.raises(TypeError):
        state.cells[0] = Player.NORTH  # type: ignore[index]

    assert state == State((None,) * 16, 0)


def test_actions_are_distinct_immutable_value_types() -> None:
    place = Place(3)
    slide = Slide(3, 7)

    assert place == Place(3)
    assert slide == Slide(3, 7)
    assert isinstance(place, Place)
    assert not isinstance(place, Slide)
    assert isinstance(slide, Slide)
    assert not isinstance(slide, Place)

    with pytest.raises(FrozenInstanceError):
        place.cell = 4  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        slide.destination = 8  # type: ignore[misc]
