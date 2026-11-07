"""Regression tests for the deterministic seating CSP solver."""

import pytest

from boardlab.puzzles.model import Assignment, Constraint, ConstraintKind, Puzzle
from boardlab.puzzles.solver import count_solutions, satisfies, solve
from boardlab.puzzles.topology import SeatTopology


def fixture() -> tuple[Puzzle, SeatTopology]:
    puzzle = Puzzle(
        ("A", "B", "C"),
        ("anna", "bruno", "carlo"),
        (
            Constraint(ConstraintKind.FIXED_SEAT, "anna", seat="A"),
            Constraint(ConstraintKind.ADJACENT, "anna", other="bruno"),
        ),
    )
    topology = SeatTopology(("A", "B", "C"), (("A", "B"), ("B", "C")))
    return puzzle, topology


def test_unique_solution() -> None:
    puzzle, topology = fixture()
    expected = Assignment(("anna", "bruno", "carlo"))

    assert solve(puzzle, topology) == (expected,)
    assert count_solutions(puzzle, topology) == 1
    assert satisfies(puzzle, topology, expected)


def test_partial_assignment_is_not_a_solution() -> None:
    puzzle, topology = fixture()
    assert not satisfies(puzzle, topology, Assignment(("anna", None, None)))


def test_invalid_complete_assignment_returns_false() -> None:
    puzzle, topology = fixture()
    assert not satisfies(puzzle, topology, Assignment(("anna", "carlo", "bruno")))


def test_unsatisfiable_constraints() -> None:
    puzzle = Puzzle(
        ("A", "B"),
        ("anna", "bruno"),
        (
            Constraint(ConstraintKind.FIXED_SEAT, "anna", seat="A"),
            Constraint(ConstraintKind.FIXED_SEAT, "anna", seat="B"),
        ),
    )
    topology = SeatTopology(puzzle.seats, (("A", "B"),))

    assert solve(puzzle, topology) == ()
    assert count_solutions(puzzle, topology) == 0


def test_multiple_solutions_and_limit() -> None:
    puzzle = Puzzle(("A", "B", "C"), ("anna", "bruno", "carlo"))
    topology = SeatTopology(puzzle.seats)

    assert count_solutions(puzzle, topology, limit=None) == 6
    assert count_solutions(puzzle, topology, limit=2) == 2
    assert len(solve(puzzle, topology, limit=2)) == 2


def test_deterministic_solution_order() -> None:
    puzzle = Puzzle(("A", "B"), ("anna", "bruno"))
    topology = SeatTopology(puzzle.seats)

    assert solve(puzzle, topology) == (
        Assignment(("anna", "bruno")),
        Assignment(("bruno", "anna")),
    )
    assert solve(puzzle, topology) == solve(puzzle, topology)


def test_separation_constraint() -> None:
    puzzle = Puzzle(
        ("A", "B", "C"),
        ("anna", "bruno", "carlo"),
        (Constraint(ConstraintKind.SEPARATE, "anna", other="bruno"),),
    )
    topology = SeatTopology(puzzle.seats, (("A", "B"), ("B", "C")))

    assert count_solutions(puzzle, topology, limit=None) == 2
    assert all(satisfies(puzzle, topology, assignment) for assignment in solve(puzzle, topology))


def test_disconnected_topology() -> None:
    puzzle = Puzzle(
        ("A", "B", "C"),
        ("anna", "bruno", "carlo"),
        (Constraint(ConstraintKind.ADJACENT, "anna", other="bruno"),),
    )
    topology = SeatTopology(puzzle.seats, (("A", "B"),))

    assert count_solutions(puzzle, topology, limit=None) == 2


def test_incompatible_topology_rejected() -> None:
    puzzle, _ = fixture()
    topology = SeatTopology(("B", "A", "C"))

    with pytest.raises(ValueError, match="Topology and puzzle seats differ"):
        solve(puzzle, topology)

    with pytest.raises(ValueError, match="Topology and puzzle seats differ"):
        satisfies(puzzle, topology, Assignment(("anna", "bruno", "carlo")))


@pytest.mark.parametrize("limit", [0, -1, True, 1.5, "2"])
def test_invalid_limits_rejected(limit: object) -> None:
    puzzle, topology = fixture()

    with pytest.raises(ValueError, match="Limit must be a positive integer"):
        solve(puzzle, topology, limit=limit)  # type: ignore[arg-type]


def test_original_inputs_remain_unchanged() -> None:
    puzzle, topology = fixture()

    original_people = puzzle.people
    original_seats = puzzle.seats
    original_edges = topology.edges

    solve(puzzle, topology)

    assert puzzle.people == original_people
    assert puzzle.seats == original_seats
    assert topology.edges == original_edges
