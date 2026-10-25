"""Independent exact-probability checks for Dado Goal."""

from fractions import Fraction
from math import comb

import pytest

from boardlab.games.dado_goal_probability import (
    expected_goals,
    goal_distribution,
    goal_given_result_probability,
    goal_per_hand_probability,
    reach_pink_probability,
    reach_result_given_pink_probability,
)


def test_exact_stage_probabilities() -> None:
    assert reach_pink_probability() == Fraction(3, 5)
    assert reach_result_given_pink_probability() == Fraction(1, 2)
    assert goal_given_result_probability() == Fraction(1, 5)


def test_exact_goal_probability() -> None:
    assert goal_per_hand_probability() == Fraction(3, 50)


@pytest.mark.parametrize("n", [1, 2, 5, 10, 20])
def test_binomial_distribution(n: int) -> None:
    distribution = goal_distribution(n)
    p = Fraction(3, 50)
    q = 1 - p

    assert len(distribution) == n + 1
    assert sum(distribution) == 1
    assert all(0 <= value <= 1 for value in distribution)

    for k, probability in enumerate(distribution):
        assert probability == comb(n, k) * p**k * q ** (n - k)


@pytest.mark.parametrize("n", [1, 2, 5, 10, 20])
def test_expected_goals_match_distribution(n: int) -> None:
    distribution = goal_distribution(n)
    expectation = sum(
        (k * probability for k, probability in enumerate(distribution)),
        Fraction(0),
    )

    assert expectation == expected_goals(n)
    assert expectation == n * Fraction(3, 50)


@pytest.mark.parametrize("invalid", [0, -1, True, 1.5, "10", None])
def test_invalid_match_length_is_rejected(invalid: object) -> None:
    for function in (goal_distribution, expected_goals):
        with pytest.raises(ValueError, match="positive integer"):
            function(invalid)  # type: ignore[arg-type]


def test_single_hand_distribution() -> None:
    assert goal_distribution(1) == (
        Fraction(47, 50),
        Fraction(3, 50),
    )


def test_ten_hand_expectation() -> None:
    assert expected_goals(10) == Fraction(3, 5)
