"""Regression tests for generic declarative CSP definitions."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    ConstraintKind,
    Problem,
    Variable,
)


def test_spatial_definition() -> None:
    problem = Problem(
        (
            Variable("anna", ("A", "B")),
            Variable("bruno", ("A", "B")),
        ),
        (Constraint(ConstraintKind.ALL_DIFFERENT, ("anna", "bruno")),),
    )
    assignment = Assignment(("A", "B"))
    assignment.validate(problem)
    assert assignment.is_complete(problem)


def test_numeric_definition() -> None:
    problem = Problem(
        (
            Variable("x", (1, 2, 3)),
            Variable("y", (1, 2, 3)),
        ),
        (Constraint(ConstraintKind.SUM_EQUALS, ("x", "y"), target=4),),
    )
    partial = Assignment((1, None))
    partial.validate(problem)
    assert not partial.is_complete(problem)


def test_structurally_valid_but_unsatisfiable() -> None:
    problem = Problem(
        (
            Variable("x", (1,)),
            Variable("y", (1,)),
        ),
        (Constraint(ConstraintKind.NOT_EQUAL, ("x", "y")),),
    )
    assert len(problem.constraints) == 1


@pytest.mark.parametrize(
    "factory",
    [
        lambda: Variable("", (1,)),
        lambda: Variable("x", ()),
        lambda: Variable("x", [1, 2]),  # type: ignore[arg-type]
        lambda: Variable("x", (True,)),
        lambda: Variable("x", (1, 1)),
        lambda: Constraint(ConstraintKind.EQUAL, ("x",)),
        lambda: Constraint(ConstraintKind.NOT_EQUAL, ("x", "x")),
        lambda: Constraint(ConstraintKind.ALL_DIFFERENT, ("x",)),
        lambda: Constraint(ConstraintKind.SUM_EQUALS, ("x",), target=None),
        lambda: Constraint(ConstraintKind.SUM_EQUALS, ("x",), target=True),
        lambda: Problem((Variable("x", (1,)), Variable("x", (2,)))),
        lambda: Problem(
            (Variable("x", (1,)),),
            (Constraint(ConstraintKind.EQUAL, ("x", "missing")),),
        ),
        lambda: Problem(
            (Variable("x", ("A",)),),
            (Constraint(ConstraintKind.SUM_EQUALS, ("x",), target=1),),
        ),
    ],
)
def test_invalid_definitions_rejected(factory: object) -> None:
    assert callable(factory)
    with pytest.raises(ValueError):
        factory()


def test_invalid_assignment_rejected() -> None:
    problem = Problem((Variable("x", (1, 2)),))

    with pytest.raises(ValueError):
        Assignment((3,)).validate(problem)

    with pytest.raises(ValueError):
        Assignment((True,)).validate(problem)

    with pytest.raises(ValueError):
        Assignment((1, 2)).validate(problem)


def test_frozen_models() -> None:
    variable = Variable("x", (1, 2))
    constraint = Constraint(ConstraintKind.EQUAL, ("x", "y"))
    problem = Problem((variable,))
    assignment = Assignment((1,))

    with pytest.raises(FrozenInstanceError):
        variable.name = "changed"  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        constraint.target = 1  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        problem.variables = ()  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        assignment.values = ()  # type: ignore[misc]


def test_assignment_does_not_evaluate_constraints() -> None:
    problem = Problem(
        (Variable("x", (1,)), Variable("y", (1,))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("x", "y")),),
    )
    assignment = Assignment((1, 1))
    assignment.validate(problem)
    assert assignment.is_complete(problem)
