"""Acceptance tests for L'Arca di Darwin M1."""

from boardlab.games.arca_di_darwin import (
    SPECIES,
    build_problem,
    solution,
    validate_selection,
)


def test_ten_species() -> None:
    assert len(SPECIES) == 10


def test_four_survivors() -> None:
    assert len(solution()) == 4


def test_solution_is_valid() -> None:
    assert validate_selection(solution())


def test_invalid_selection() -> None:
    assert not validate_selection(("leone", "lupo", "zebra", "elefante"))


def test_unknown_species() -> None:
    assert not validate_selection(("drago",))


def test_duplicate_species() -> None:
    assert not validate_selection(("zebra", "zebra"))


def test_problem_has_constraints() -> None:
    assert build_problem().constraints
