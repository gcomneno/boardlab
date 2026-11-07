"""Regression tests for generic CSP instance generation."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.puzzles.generic.generator import (
    GenerationOutcome,
    GenerationPolicy,
    GenerationSpace,
    generate,
)
from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    Variable,
)
from boardlab.puzzles.generic.model import (
    ConstraintKind as K,
)
from boardlab.puzzles.generic.solver import solve


def symbolic_space() -> GenerationSpace:
    return GenerationSpace(
        (
            Variable("a", ("A", "B")),
            Variable("b", ("B", "C")),
        ),
        (
            Constraint(K.NOT_EQUAL, ("a", "b")),
            Constraint(K.EQUAL, ("a", "b")),
        ),
    )


def numeric_space() -> GenerationSpace:
    return GenerationSpace(
        (
            Variable("x", (1, 2, 3)),
            Variable("y", (1, 2, 3)),
            Variable("z", (1, 2, 3)),
        ),
        (
            Constraint(K.SUM_EQUALS, ("x", "y"), target=3),
            Constraint(K.SUM_EQUALS, ("y", "z"), target=4),
            Constraint(K.SUM_EQUALS, ("x", "z"), target=5),
        ),
    )


def test_symbolic_unique_instance() -> None:
    result = generate(symbolic_space())
    assert result.outcome is GenerationOutcome.FOUND
    assert result.found
    assert result.candidates_checked == 2
    assert result.problem is not None
    assert len(result.problem.constraints) == 1
    assert result.solutions == (Assignment(("B", "B")),)
    assert solve(result.problem) == result.solutions


def test_numeric_unique_instance() -> None:
    result = generate(numeric_space())
    assert result.outcome is GenerationOutcome.FOUND
    assert result.candidates_checked == 5
    assert result.problem is not None
    assert len(result.problem.constraints) == 2
    assert result.solutions == (Assignment((2, 1, 3)),)
    assert solve(result.problem) == result.solutions


def test_deterministic_generation() -> None:
    space = numeric_space()
    assert generate(space) == generate(space)


def test_budget_exhaustion_is_not_search_exhaustion() -> None:
    result = generate(numeric_space(), GenerationPolicy(max_candidates=4))
    assert result.outcome is GenerationOutcome.BUDGET_EXHAUSTED
    assert not result.found
    assert result.problem is None
    assert result.solutions == ()
    assert result.candidates_checked == 4


def test_exhaustive_no_match() -> None:
    space = GenerationSpace(
        (Variable("x", (1, 2)), Variable("y", (1, 2))),
        (Constraint(K.NOT_EQUAL, ("x", "y")),),
    )
    result = generate(space)
    assert result.outcome is GenerationOutcome.EXHAUSTED
    assert result.candidates_checked == 1
    assert result.problem is None


def test_zero_solutions_are_distinct_from_no_candidate() -> None:
    space = GenerationSpace(
        (Variable("x", (1,)), Variable("y", (1,))),
        (Constraint(K.NOT_EQUAL, ("x", "y")),),
    )
    result = generate(space, GenerationPolicy(required_solutions=0))
    assert result.outcome is GenerationOutcome.FOUND
    assert result.problem is not None
    assert result.solutions == ()
    assert result.candidates_checked == 1


def test_specific_solution_cardinality() -> None:
    space = GenerationSpace(
        (Variable("x", (1, 2)), Variable("y", (1, 2))),
        (Constraint(K.NOT_EQUAL, ("x", "y")),),
    )
    result = generate(space, GenerationPolicy(required_solutions=2))
    assert result.outcome is GenerationOutcome.FOUND
    assert result.solutions == (
        Assignment((1, 2)),
        Assignment((2, 1)),
    )


def test_maximum_constraints_can_prevent_a_result() -> None:
    result = generate(numeric_space(), GenerationPolicy(max_constraints=1))
    assert result.outcome is GenerationOutcome.EXHAUSTED
    assert result.candidates_checked == 3


def test_empty_constraint_subset_is_supported() -> None:
    space = GenerationSpace((Variable("x", (1,)),), ())
    result = generate(
        space,
        GenerationPolicy(min_constraints=0, max_constraints=0),
    )
    assert result.outcome is GenerationOutcome.FOUND
    assert result.problem is not None
    assert result.problem.constraints == ()
    assert result.solutions == (Assignment((1,)),)


def test_empty_search_range_is_exhausted() -> None:
    space = GenerationSpace((Variable("x", (1,)),), ())
    result = generate(space)
    assert result.outcome is GenerationOutcome.EXHAUSTED
    assert result.candidates_checked == 0


def test_duplicate_catalog_constraints_rejected() -> None:
    constraint = Constraint(K.NOT_EQUAL, ("x", "y"))
    with pytest.raises(ValueError, match="Duplicate"):
        GenerationSpace(
            (Variable("x", (1, 2)), Variable("y", (1, 2))),
            (constraint, constraint),
        )


def test_invalid_catalog_reference_rejected() -> None:
    with pytest.raises(ValueError):
        GenerationSpace(
            (Variable("x", (1,)),),
            (Constraint(K.NOT_EQUAL, ("x", "missing")),),
        )


@pytest.mark.parametrize(
    "kwargs",
    (
        {"required_solutions": -1},
        {"required_solutions": True},
        {"min_constraints": -1},
        {"min_constraints": True},
        {"max_constraints": -1},
        {"max_constraints": 1, "min_constraints": 2},
        {"max_constraints": True},
        {"max_candidates": 0},
        {"max_candidates": True},
    ),
)
def test_invalid_policy(kwargs: dict[str, int | bool]) -> None:
    with pytest.raises(ValueError):
        GenerationPolicy(**kwargs)


def test_result_is_immutable() -> None:
    result = generate(symbolic_space())
    with pytest.raises(FrozenInstanceError):
        result.candidates_checked = 99  # type: ignore[misc]  # Verify frozen dataclass at runtime
