"""Verify Dado Goal's deterministic rules and state invariants."""

from dataclasses import replace

import pytest

from boardlab.games.dado_goal import (
    PINK_FACES,
    RESULT_FACES,
    TEAM_FACES,
    Phase,
    PinkFace,
    ResultFace,
    State,
    TeamFace,
    apply_roll,
    initial_state,
)


def test_dice_distributions() -> None:
    assert len(TEAM_FACES) == 6
    assert TEAM_FACES.count(TeamFace.SHOT) == 2
    assert TEAM_FACES.count(TeamFace.INTERCEPTED) == 2
    assert TEAM_FACES.count(TeamFace.CROSS) == 1
    assert TEAM_FACES.count(TeamFace.PASS) == 1
    assert len(PINK_FACES) == 6
    assert len(set(PINK_FACES)) == 6
    assert len(RESULT_FACES) == 6
    assert len(set(RESULT_FACES)) == 6


@pytest.mark.parametrize("face", [TeamFace.SHOT, TeamFace.CROSS])
def test_attack_always_reaches_pink(face: TeamFace) -> None:
    original = initial_state(2)
    successor = apply_roll(original, face)

    assert successor.phase is Phase.PINK
    assert original.phase is Phase.TEAM
    assert successor.completed_hands == (0, 0)


def test_passage_and_repeated_penalties_preserve_hand() -> None:
    state = initial_state(2)

    for _ in range(10):
        state = apply_roll(state, TeamFace.PASS)

    assert state == initial_state(2)

    state = apply_roll(state, TeamFace.SHOT)
    state = apply_roll(state, PinkFace.HEADER)

    for _ in range(10):
        state = apply_roll(state, ResultFace.PENALTY)

    assert state.phase is Phase.RESULT
    assert state.completed_hands == (0, 0)
    assert state.goals == (0, 0)

    state = apply_roll(state, ResultFace.GOAL)

    assert state.phase is Phase.TEAM
    assert state.active_player == 1
    assert state.completed_hands == (1, 0)
    assert state.goals == (1, 0)


@pytest.mark.parametrize(
    "face",
    [
        PinkFace.HALF_BICYCLE,
        PinkFace.BICYCLE,
        PinkFace.HEADER,
    ],
)
def test_pink_success_reaches_result(face: PinkFace) -> None:
    state = apply_roll(initial_state(1), TeamFace.CROSS)
    assert apply_roll(state, face).phase is Phase.RESULT


@pytest.mark.parametrize(
    "face",
    [
        PinkFace.BOTTOM,
        PinkFace.ABOVE_BAR,
        PinkFace.INTERCEPTED,
    ],
)
def test_pink_failure_ends_hand(face: PinkFace) -> None:
    state = apply_roll(initial_state(1), TeamFace.SHOT)
    successor = apply_roll(state, face)

    assert successor.phase is Phase.TEAM
    assert successor.active_player == 1
    assert successor.completed_hands == (1, 0)


@pytest.mark.parametrize(
    "face",
    [
        ResultFace.REBOUND,
        ResultFace.WIDE,
        ResultFace.WOODWORK,
        ResultFace.SAVED,
    ],
)
def test_result_failure_does_not_score(face: ResultFace) -> None:
    state = apply_roll(initial_state(1), TeamFace.SHOT)
    state = apply_roll(state, PinkFace.BICYCLE)
    state = apply_roll(state, face)

    assert state.goals == (0, 0)
    assert state.completed_hands == (1, 0)


def test_completed_match_and_draw() -> None:
    original = initial_state(1)
    state = apply_roll(original, TeamFace.INTERCEPTED)
    state = apply_roll(state, TeamFace.INTERCEPTED)

    assert state.phase is Phase.FINISHED
    assert state.completed_hands == (1, 1)
    assert state.goals == (0, 0)
    assert original == initial_state(1)

    with pytest.raises(ValueError, match="finished"):
        apply_roll(state, TeamFace.SHOT)


@pytest.mark.parametrize("n", [0, -1, True, 1.5])
def test_invalid_match_length(n: object) -> None:
    with pytest.raises(ValueError):
        initial_state(n)  # type: ignore[arg-type]


def test_wrong_die_is_rejected() -> None:
    state = initial_state(1)

    with pytest.raises(ValueError, match="team-die"):
        apply_roll(state, PinkFace.BOTTOM)

    state = apply_roll(state, TeamFace.SHOT)

    with pytest.raises(ValueError, match="pink-die"):
        apply_roll(state, ResultFace.GOAL)


def test_goal_cannot_precede_completed_hand() -> None:
    state = replace(initial_state(2), goals=(1, 0))

    with pytest.raises(ValueError, match="Invalid"):
        apply_roll(state, TeamFace.PASS)


def test_invalid_hand_counter_is_rejected() -> None:
    state = State(
        hands_per_team=2,
        completed_hands=(0, 1),
        active_player=0,
    )

    with pytest.raises(ValueError, match="Invalid"):
        apply_roll(state, TeamFace.SHOT)
