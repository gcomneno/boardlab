"""Regression tests for immutable seating puzzle foundations."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.puzzles.model import Assignment, Constraint, ConstraintKind, Puzzle


def sample_puzzle() -> Puzzle:
    return Puzzle(
        seats=("A", "B", "C"),
        people=("anna", "bruno", "carlo"),
        constraints=(
            Constraint(ConstraintKind.ADJACENT, "anna", other="bruno"),
            Constraint(ConstraintKind.FIXED_SEAT, "carlo", seat="C"),
        ),
    )


def test_valid_model_and_assignments() -> None:
    puzzle = sample_puzzle()
    partial = Assignment(("anna", None, "carlo"))
    complete = Assignment(("anna", "bruno", "carlo"))

    partial.validate(puzzle)
    complete.validate(puzzle)

    assert not partial.is_complete(puzzle)
    assert complete.is_complete(puzzle)
    assert partial.occupants == ("anna", None, "carlo")
    assert complete.occupants == ("anna", "bruno", "carlo")


def test_models_are_frozen() -> None:
    puzzle = sample_puzzle()
    constraint = puzzle.constraints[0]
    assignment = Assignment(("anna", None, "carlo"))

    with pytest.raises(FrozenInstanceError):
        puzzle.seats = ("X",)  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        constraint.person = "changed"  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        assignment.occupants = ()  # type: ignore[misc]


@pytest.mark.parametrize(
    "factory",
    [
        lambda: Puzzle(["A", "B"], ("anna", "bruno")),  # type: ignore[arg-type]
        lambda: Puzzle(("A", "B"), ["anna", "bruno"]),  # type: ignore[arg-type]
        lambda: Constraint(ConstraintKind.ADJACENT, "anna"),
        lambda: Constraint("unknown", "anna"),  # type: ignore[arg-type]
        lambda: Assignment((123, None)).validate(sample_puzzle()),  # type: ignore[arg-type]
    ],
)
def test_invalid_inputs_are_rejected(factory: object) -> None:
    assert callable(factory)
    with pytest.raises(ValueError):
        factory()


def test_invalid_references_are_rejected() -> None:
    with pytest.raises(ValueError, match="Unknown person"):
        Puzzle(
            ("A", "B"),
            ("anna", "bruno"),
            (Constraint(ConstraintKind.ADJACENT, "anna", other="unknown"),),
        )

    with pytest.raises(ValueError, match="Invalid fixed-seat constraint"):
        Puzzle(
            ("A", "B"),
            ("anna", "bruno"),
            (Constraint(ConstraintKind.FIXED_SEAT, "anna", seat="Z"),),
        )


def test_duplicate_occupants_are_rejected() -> None:
    with pytest.raises(ValueError, match="more than once"):
        Assignment(("anna", "anna", None)).validate(sample_puzzle())
