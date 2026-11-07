"""Deterministic constraint solver for finite seating puzzles."""

from boardlab.puzzles.model import Assignment, ConstraintKind, Puzzle
from boardlab.puzzles.topology import SeatTopology


def _check_compatibility(puzzle: Puzzle, topology: SeatTopology) -> None:
    """Reject incompatible puzzle/topology combinations."""
    topology.validate_seats(puzzle.seats)


def _consistent(
    puzzle: Puzzle,
    topology: SeatTopology,
    occupants: tuple[str | None, ...],
) -> bool:
    """Reject partial assignments that already violate a constraint."""
    positions = {person: index for index, person in enumerate(occupants) if person is not None}

    for constraint in puzzle.constraints:
        person_position = positions.get(constraint.person)

        if constraint.kind is ConstraintKind.FIXED_SEAT:
            required_position = puzzle.seats.index(constraint.seat)

            if person_position is not None and person_position != required_position:
                return False

            occupant = occupants[required_position]
            if occupant is not None and occupant != constraint.person:
                return False

        else:
            if constraint.other is None:
                raise ValueError("Invalid person relationship")

            other_position = positions.get(constraint.other)

            if person_position is None or other_position is None:
                continue

            linked = topology.adjacent(
                puzzle.seats[person_position],
                puzzle.seats[other_position],
            )

            if constraint.kind is ConstraintKind.ADJACENT and not linked:
                return False

            if constraint.kind is ConstraintKind.SEPARATE and linked:
                return False

    return True


def satisfies(
    puzzle: Puzzle,
    topology: SeatTopology,
    assignment: Assignment,
) -> bool:
    """Check whether a complete assignment satisfies every constraint."""
    _check_compatibility(puzzle, topology)
    assignment.validate(puzzle)

    if not assignment.is_complete(puzzle):
        return False

    return _consistent(puzzle, topology, assignment.occupants)


def solve(
    puzzle: Puzzle,
    topology: SeatTopology,
    limit: int | None = None,
) -> tuple[Assignment, ...]:
    """Enumerate solutions deterministically, stopping at an optional limit."""
    _check_compatibility(puzzle, topology)

    if limit is not None and (type(limit) is not int or limit < 1):
        raise ValueError("Limit must be a positive integer")

    solutions: list[Assignment] = []
    occupants: list[str | None] = [None] * len(puzzle.seats)
    assigned: set[str] = set()

    def search(index: int) -> None:
        if limit is not None and len(solutions) >= limit:
            return

        if index == len(puzzle.seats):
            assignment = Assignment(tuple(occupants))
            if _consistent(puzzle, topology, assignment.occupants):
                solutions.append(assignment)
            return

        for person in puzzle.people:
            if person in assigned:
                continue

            occupants[index] = person
            assigned.add(person)

            if _consistent(puzzle, topology, tuple(occupants)):
                search(index + 1)

            assigned.remove(person)
            occupants[index] = None

            if limit is not None and len(solutions) >= limit:
                return

    search(0)
    return tuple(solutions)


def count_solutions(
    puzzle: Puzzle,
    topology: SeatTopology,
    limit: int | None = 2,
) -> int:
    """Count solutions up to a limit; None requests exhaustive counting."""
    return len(solve(puzzle, topology, limit=limit))
