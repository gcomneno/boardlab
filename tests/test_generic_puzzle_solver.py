"""Regression tests for the generic declarative CSP solver."""

from itertools import product

import pytest

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    ConstraintKind,
    Problem,
    Variable,
)
from boardlab.puzzles.generic.solver import count_solutions, satisfies, solve


def test_spatial_all_different() -> None:
    problem = Problem(
        (Variable("anna", ("A", "B")), Variable("bruno", ("A", "B"))),
        (Constraint(ConstraintKind.ALL_DIFFERENT, ("anna", "bruno")),),
    )
    assert solve(problem) == (
        Assignment(("A", "B")),
        Assignment(("B", "A")),
    )


def test_numeric_sum() -> None:
    problem = Problem(
        (Variable("x", (1, 2, 3)), Variable("y", (1, 2, 3))),
        (Constraint(ConstraintKind.SUM_EQUALS, ("x", "y"), target=4),),
    )
    assert solve(problem) == (
        Assignment((1, 3)),
        Assignment((2, 2)),
        Assignment((3, 1)),
    )


def test_negative_numbers_and_pruning() -> None:
    problem = Problem(
        (Variable("x", (5,)), Variable("y", (-4, -1, 2))),
        (Constraint(ConstraintKind.SUM_EQUALS, ("x", "y"), target=1),),
    )
    assert solve(problem) == (Assignment((5, -4)),)


def test_equality_is_type_sensitive() -> None:
    problem = Problem(
        (Variable("x", (1, "1")), Variable("y", ("1", 1))),
        (Constraint(ConstraintKind.EQUAL, ("x", "y")),),
    )
    assert solve(problem) == (
        Assignment((1, 1)),
        Assignment(("1", "1")),
    )


def test_not_equal_is_type_sensitive() -> None:
    problem = Problem(
        (Variable("x", (1,)), Variable("y", ("1", 1))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("x", "y")),),
    )
    assert solve(problem) == (Assignment((1, "1")),)


def test_unsatisfiable_problem() -> None:
    problem = Problem(
        (Variable("x", (1,)), Variable("y", (1,))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("x", "y")),),
    )
    assert solve(problem) == ()
    assert count_solutions(problem) == 0


def test_limits_and_determinism() -> None:
    problem = Problem(
        (Variable("x", (1, 2)), Variable("y", (3, 4))),
    )
    assert count_solutions(problem, limit=None) == 4
    assert count_solutions(problem, limit=2) == 2
    assert solve(problem, limit=2) == (
        Assignment((1, 3)),
        Assignment((1, 4)),
    )
    assert solve(problem) == solve(problem)


@pytest.mark.parametrize("limit", (0, -1, True, 1.5, "2"))
def test_invalid_limit(limit: object) -> None:
    problem = Problem((Variable("x", (1,)),))
    with pytest.raises(ValueError, match="positive integer"):
        solve(problem, limit=limit)  # type: ignore[arg-type]


def test_partial_assignment_is_not_complete_solution() -> None:
    problem = Problem((Variable("x", (1, 2)),))
    assert not satisfies(problem, Assignment((None,)))


def test_assignment_does_not_mutate_inputs() -> None:
    problem = Problem((Variable("x", (1, 2)),))
    original = problem.variables
    assert solve(problem) == (Assignment((1,)), Assignment((2,)))
    assert problem.variables == original


def test_exhaustive_oracle_for_numeric_constraints() -> None:
    domains = ((-2, 0, 2), (-1, 1, 3), (-3, 0, 3))
    variables = tuple(Variable(f"x{i}", domain) for i, domain in enumerate(domains))
    for target in range(-6, 7):
        problem = Problem(
            variables,
            (
                Constraint(
                    ConstraintKind.SUM_EQUALS,
                    ("x0", "x1", "x2"),
                    target=target,
                ),
            ),
        )
        expected = tuple(
            Assignment(values) for values in product(*domains) if sum(values) == target
        )
        assert solve(problem) == expected
        assert count_solutions(problem, limit=2) == min(len(expected), 2)
