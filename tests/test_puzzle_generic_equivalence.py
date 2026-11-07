"""PZ-001F: exact CSP semantic equivalence experiments."""

from boardlab.puzzles.generic.equivalence import semantically_equivalent
from boardlab.puzzles.generic.model import (
    Constraint,
    ConstraintKind,
    Problem,
    Variable,
)


def test_reordered_variables_and_domains_are_equivalent() -> None:
    first = Problem(
        (Variable("a", (1, 2)), Variable("b", (1, 2))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    second = Problem(
        (Variable("b", (2, 1)), Variable("a", (2, 1))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("b", "a")),),
    )
    assert semantically_equivalent(first, second)


def test_redundant_constraint_preserves_semantics() -> None:
    variables = (Variable("a", (1, 2)), Variable("b", (1, 2)))
    first = Problem(
        variables,
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    second = Problem(
        variables,
        (
            Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),
            Constraint(ConstraintKind.ALL_DIFFERENT, ("a", "b")),
        ),
    )
    assert semantically_equivalent(first, second)


def test_different_solution_sets_are_not_equivalent() -> None:
    variables = (Variable("a", (1, 2)), Variable("b", (1, 2)))
    first = Problem(
        variables,
        (Constraint(ConstraintKind.EQUAL, ("a", "b")),),
    )
    second = Problem(
        variables,
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    assert not semantically_equivalent(first, second)


def test_different_domains_are_not_equivalent() -> None:
    first = Problem((Variable("a", (1, 2)),))
    second = Problem((Variable("a", (1, 3)),))
    assert not semantically_equivalent(first, second)


def test_sum_equals_semantics_are_preserved() -> None:
    variables = (Variable("a", (1, 2)), Variable("b", (1, 2)))
    first = Problem(
        variables,
        (Constraint(ConstraintKind.SUM_EQUALS, ("a", "b"), 3),),
    )
    second = Problem(
        variables,
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    assert semantically_equivalent(first, second)


def test_variable_renaming_preserves_equivalence() -> None:
    from boardlab.puzzles.generic.equivalence import (
        equivalent_under_renaming,
    )

    first = Problem(
        (Variable("a", (1, 2)), Variable("b", (1, 2))),
        (Constraint(ConstraintKind.EQUAL, ("a", "b")),),
    )
    renamed = Problem(
        (Variable("x", (2, 1)), Variable("y", (2, 1))),
        (Constraint(ConstraintKind.EQUAL, ("y", "x")),),
    )

    assert equivalent_under_renaming(first, renamed)


def test_renaming_does_not_conflate_different_relations() -> None:
    from boardlab.puzzles.generic.equivalence import (
        equivalent_under_renaming,
    )

    variables = (Variable("a", (1, 2)), Variable("b", (1, 2)))
    equal = Problem(
        variables,
        (Constraint(ConstraintKind.EQUAL, ("a", "b")),),
    )
    different = Problem(
        variables,
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    assert not equivalent_under_renaming(equal, different)


def test_semantic_classes_merge_redundant_and_renamed_problems() -> None:
    from boardlab.puzzles.generic.equivalence import equivalence_classes

    original = Problem(
        (Variable("a", (1, 2)), Variable("b", (1, 2))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),),
    )
    renamed = Problem(
        (Variable("x", (1, 2)), Variable("y", (1, 2))),
        (Constraint(ConstraintKind.NOT_EQUAL, ("x", "y")),),
    )
    redundant = Problem(
        original.variables,
        (
            Constraint(ConstraintKind.NOT_EQUAL, ("a", "b")),
            Constraint(ConstraintKind.ALL_DIFFERENT, ("a", "b")),
        ),
    )
    different = Problem(
        original.variables,
        (Constraint(ConstraintKind.EQUAL, ("a", "b")),),
    )

    assert equivalence_classes((original, renamed, redundant, different)) == ((0, 1, 2), (3,))


def test_budget_exceeded_is_not_reported_as_inequivalence() -> None:
    import pytest

    from boardlab.puzzles.generic.equivalence import (
        equivalent_under_renaming,
    )

    large = Problem(tuple(Variable(f"v{i}", (0, 1)) for i in range(7)))

    with pytest.raises(ValueError, match="Variable budget"):
        equivalent_under_renaming(large, large)


def test_sum_target_values_are_not_renamed() -> None:
    from boardlab.puzzles.generic.equivalence import (
        equivalent_under_renaming,
    )

    variables = (Variable("a", (1, 2)), Variable("b", (1, 2)))
    first = Problem(
        variables,
        (Constraint(ConstraintKind.SUM_EQUALS, ("a", "b"), 2),),
    )
    second = Problem(
        variables,
        (Constraint(ConstraintKind.SUM_EQUALS, ("a", "b"), 4),),
    )

    assert not equivalent_under_renaming(first, second)
