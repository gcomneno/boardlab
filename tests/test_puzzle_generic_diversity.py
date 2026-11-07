"""PZ-001F: bounded semantic diversity experiments."""

import pytest

from boardlab.puzzles.generic.diversity import measure_diversity
from boardlab.puzzles.generic.generator import GenerationSpace
from boardlab.puzzles.generic.model import Constraint, ConstraintKind, Variable


def space() -> GenerationSpace:
    return GenerationSpace(
        (Variable("a", (1, 2)), Variable("b", (1, 2))),
        (
            Constraint(ConstraintKind.EQUAL, ("a", "b")),
            Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),
            Constraint(ConstraintKind.ALL_DIFFERENT, ("a", "b")),
        ),
    )


def test_semantic_diversity_collapses_redundancy() -> None:
    report = measure_diversity(space())

    assert report.candidates == 8
    assert report.semantic_classes == 4
    assert sorted(report.class_sizes) == [1, 1, 3, 3]
    assert report.diversity_ratio == 0.5


def test_candidate_budget_is_explicit() -> None:
    with pytest.raises(ValueError, match="Candidate budget"):
        measure_diversity(space(), max_candidates=7)


def test_restricted_population() -> None:
    report = measure_diversity(space(), min_constraints=1, max_constraints=1)
    assert report.candidates == 3
    assert report.semantic_classes == 2


def test_empty_population() -> None:
    report = measure_diversity(space(), min_constraints=4)
    assert report.candidates == 0
    assert report.semantic_classes == 0
    assert report.diversity_ratio == 0.0


def test_invalid_candidate_budget() -> None:
    with pytest.raises(ValueError, match="Invalid candidate budget"):
        measure_diversity(space(), max_candidates=0)
