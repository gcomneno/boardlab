"""Generated Ark CSP puzzle tests."""

import pytest

from boardlab.games.arca_di_darwin_levels import (
    ANIMALS,
    build_level,
    check_level,
)
from boardlab.puzzles.generic.solver import count_solutions


@pytest.mark.parametrize("difficulty", ("facile", "medio", "difficile"))
def test_generated_level_is_unique(difficulty: str) -> None:
    level = build_level(difficulty)
    assert count_solutions(level.problem, limit=2) == 1
    arrangement = tuple(sorted(ANIMALS, key=lambda animal: level.solution[ANIMALS.index(animal)]))
    assert check_level(level, arrangement)


def test_wrong_arrangement_is_rejected() -> None:
    level = build_level()
    assert not check_level(level, tuple(reversed(ANIMALS)))


def test_generation_is_deterministic() -> None:
    assert build_level() == build_level()


def test_invalid_difficulty() -> None:
    with pytest.raises(ValueError):
        build_level("impossibile")
