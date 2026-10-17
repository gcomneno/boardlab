"""Verify Tre Sigilli territorial influence and terminal outcomes."""

import pytest

from boardlab.engine.game import Outcome
from boardlab.games.tre_sigilli import Place, Player, State, TreSigilli


def board(
    turn: int,
    north: tuple[int, ...],
    south: tuple[int, ...],
) -> State:
    """Construct a structurally valid state for score-focused tests."""
    cells: list[Player | None] = [None] * 16

    for index in north:
        cells[index] = Player.NORTH

    for index in south:
        if cells[index] is not None:
            raise ValueError("Overlapping seals")
        cells[index] = Player.SOUTH

    state = State(tuple(cells), turn)
    TreSigilli._validate(state)
    return state


def placements(*indices: int) -> State:
    """Build a reachable state through legal placements."""
    game = TreSigilli()
    state = game.initial_state()

    for index in indices:
        state = game.apply_move(state, Place(index))

    return state


def reference_scores(state: State) -> tuple[int, int]:
    """Independent cell-centric scoring oracle."""
    scores = [0, 0]

    for target in range(16):
        target_row, target_col = divmod(target, 4)
        influence = {Player.NORTH: 0, Player.SOUTH: 0}

        for source, owner in enumerate(state.cells):
            if owner is None:
                continue

            source_row, source_col = divmod(source, 4)
            distance = abs(target_row - source_row) + abs(target_col - source_col)

            if distance <= 1:
                influence[owner] += 1

        north = influence[Player.NORTH]
        south = influence[Player.SOUTH]

        if north > south:
            scores[0] += 1
        elif south > north:
            scores[1] += 1

    return scores[0], scores[1]


def test_empty_board_has_no_territory() -> None:
    game = TreSigilli()
    assert game._territory_scores(game.initial_state()) == (0, 0)


def test_single_seal_controls_its_cell_and_neighbors() -> None:
    game = TreSigilli()
    state = placements(0)

    assert game._territory_scores(state) == (3, 0)


def test_contested_and_neutral_cells_score_zero() -> None:
    game = TreSigilli()
    state = placements(0, 2)

    # Cells 0 and 2 are controlled; cell 1 is contested.
    # Uninfluenced cells remain neutral.
    assert game._territory_scores(state) == (2, 3)


@pytest.mark.parametrize(
    ("north", "south"),
    [
        ((0, 1, 4), (2, 5, 8)),
        ((0, 2, 4), (5, 6, 8)),
        ((0, 3, 12), (1, 4, 15)),
        ((5, 6, 11), (2, 3, 7)),
        ((0, 1, 4), (5, 2, 8)),
    ],
)
def test_scores_match_independent_oracle(
    north: tuple[int, ...],
    south: tuple[int, ...],
) -> None:
    game = TreSigilli()
    state = board(6, north, south)

    assert game._territory_scores(state) == reference_scores(state)


def test_reachable_block_is_scored_not_forfeited() -> None:
    game = TreSigilli()
    state = placements(0, 2, 1, 5, 4, 8)

    assert game.is_terminal(state)
    north, south = reference_scores(state)

    expected_north = (
        Outcome.DRAW if north == south else Outcome.WIN if north > south else Outcome.LOSS
    )
    expected_south = (
        Outcome.DRAW if north == south else Outcome.WIN if south > north else Outcome.LOSS
    )

    assert game.outcome(state, Player.NORTH) is expected_north
    assert game.outcome(state, Player.SOUTH) is expected_south


def test_turn_eighteen_outcome_matches_territory() -> None:
    game = TreSigilli()
    state = board(18, (0, 2, 4), (5, 6, 8))

    assert game.is_terminal(state)

    north, south = reference_scores(state)
    assert game.outcome(state, Player.NORTH) is (
        Outcome.DRAW if north == south else Outcome.WIN if north > south else Outcome.LOSS
    )


def test_outcome_rejects_ongoing_state() -> None:
    game = TreSigilli()

    with pytest.raises(ValueError, match="terminal"):
        game.outcome(game.initial_state(), Player.NORTH)


@pytest.mark.parametrize("player", [None, "north", 0, True])
def test_outcome_rejects_invalid_player(player: object) -> None:
    game = TreSigilli()
    state = placements(0, 2, 1, 5, 4, 8)

    with pytest.raises(ValueError, match="Invalid player"):
        game.outcome(state, player)  # type: ignore[arg-type]


def test_outcome_rejects_invalid_state() -> None:
    game = TreSigilli()
    invalid = State((None,) * 16, 18)

    with pytest.raises(ValueError, match="Invalid game state"):
        game.outcome(invalid, Player.NORTH)


def test_scoring_does_not_mutate_state() -> None:
    game = TreSigilli()
    state = board(18, (0, 2, 4), (5, 6, 8))
    before = state

    assert game._territory_scores(state) == reference_scores(state)
    assert state == before
