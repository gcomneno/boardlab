"""Exact probabilities for Dado Goal under fair independent dice.

These probabilities describe a digital mathematical model, not
measured properties of the original handmade dice.
"""

from fractions import Fraction
from math import comb

from boardlab.games.dado_goal import (
    PINK_FACES,
    RESULT_FACES,
    TEAM_FACES,
    PinkFace,
    ResultFace,
    TeamFace,
)


def _face_probability(faces: tuple[object, ...], face: object) -> Fraction:
    """Probability of a face, accounting for its multiplicity."""
    return Fraction(faces.count(face), len(faces))


def reach_pink_probability() -> Fraction:
    """Eventually advance past team-die passes."""
    advance = _face_probability(TEAM_FACES, TeamFace.SHOT) + _face_probability(
        TEAM_FACES, TeamFace.CROSS
    )
    repeat = _face_probability(TEAM_FACES, TeamFace.PASS)

    return advance / (1 - repeat)


def reach_result_given_pink_probability() -> Fraction:
    """Advance from the pink die to the result die."""
    success = (
        PinkFace.HALF_BICYCLE,
        PinkFace.BICYCLE,
        PinkFace.HEADER,
    )
    return sum(
        (_face_probability(PINK_FACES, face) for face in success),
        Fraction(0),
    )


def goal_given_result_probability() -> Fraction:
    """Eventually score after any number of repeated penalties."""
    goal = _face_probability(RESULT_FACES, ResultFace.GOAL)
    repeat = _face_probability(RESULT_FACES, ResultFace.PENALTY)

    return goal / (1 - repeat)


def goal_per_hand_probability() -> Fraction:
    """Exact probability of one goal in a complete hand."""
    return (
        reach_pink_probability()
        * reach_result_given_pink_probability()
        * goal_given_result_probability()
    )


def goal_distribution(hands_per_team: int) -> tuple[Fraction, ...]:
    """Probability of scoring k goals for k=0,...,N complete hands."""
    if type(hands_per_team) is not int or hands_per_team < 1:
        raise ValueError("hands_per_team must be a positive integer")

    p = goal_per_hand_probability()
    q = 1 - p

    return tuple(
        Fraction(comb(hands_per_team, k)) * p**k * q ** (hands_per_team - k)
        for k in range(hands_per_team + 1)
    )


def expected_goals(hands_per_team: int) -> Fraction:
    """Expected goals for one team over N complete hands."""
    if type(hands_per_team) is not int or hands_per_team < 1:
        raise ValueError("hands_per_team must be a positive integer")

    return hands_per_team * goal_per_hand_probability()
