"""Immutable model for finite seating-assignment puzzles."""

from dataclasses import dataclass
from enum import Enum


class ConstraintKind(Enum):
    """Supported structural relationships."""

    ADJACENT = "adjacent"
    SEPARATE = "separate"
    FIXED_SEAT = "fixed_seat"


@dataclass(frozen=True)
class Constraint:
    """A relationship between people or a person and a seat."""

    kind: ConstraintKind
    person: str
    other: str | None = None
    seat: str | None = None

    def __post_init__(self) -> None:
        if not isinstance(self.kind, ConstraintKind):
            raise ValueError("Invalid constraint kind")
        if not isinstance(self.person, str) or not self.person:
            raise ValueError("Invalid person identifier")

        if self.kind is ConstraintKind.FIXED_SEAT:
            if not isinstance(self.seat, str) or not self.seat or self.other is not None:
                raise ValueError("Invalid fixed-seat constraint")
        elif (
            not isinstance(self.other, str)
            or not self.other
            or self.other == self.person
            or self.seat is not None
        ):
            raise ValueError("Invalid person relationship")


@dataclass(frozen=True)
class Puzzle:
    """Finite assignment problem and its immutable requirements."""

    seats: tuple[str, ...]
    people: tuple[str, ...]
    constraints: tuple[Constraint, ...] = ()

    def __post_init__(self) -> None:
        if not isinstance(self.seats, tuple) or not isinstance(self.people, tuple):
            raise ValueError("Seats and people must be tuples")
        if not self.seats or not self.people:
            raise ValueError("Seats and people must be nonempty")
        if len(self.seats) != len(self.people):
            raise ValueError("Every person must have exactly one seat")

        for collection in (self.seats, self.people):
            if any(not isinstance(item, str) or not item for item in collection):
                raise ValueError("Identifiers must be nonempty strings")
            if len(collection) != len(set(collection)):
                raise ValueError("Duplicate identifiers")

        if not isinstance(self.constraints, tuple):
            raise ValueError("Constraints must be a tuple")

        for constraint in self.constraints:
            if not isinstance(constraint, Constraint):
                raise ValueError("Invalid constraint")
            if not isinstance(constraint.kind, ConstraintKind):
                raise ValueError("Invalid constraint kind")
            if constraint.person not in self.people:
                raise ValueError("Unknown person")

            if constraint.kind is ConstraintKind.FIXED_SEAT:
                if constraint.seat not in self.seats or constraint.other is not None:
                    raise ValueError("Invalid fixed-seat constraint")
            else:
                if constraint.other not in self.people:
                    raise ValueError("Unknown person")
                if constraint.other == constraint.person or constraint.seat is not None:
                    raise ValueError("Invalid person relationship")


@dataclass(frozen=True)
class Assignment:
    """Partial or complete seating, indexed by puzzle seat order."""

    occupants: tuple[str | None, ...]

    def validate(self, puzzle: Puzzle) -> None:
        """Validate occupancy without evaluating puzzle constraints."""
        if not isinstance(self.occupants, tuple):
            raise ValueError("Occupants must be a tuple")
        if len(self.occupants) != len(puzzle.seats):
            raise ValueError("Incorrect seat count")

        assigned = [person for person in self.occupants if person is not None]

        if any(person not in puzzle.people for person in assigned):
            raise ValueError("Unknown occupant")
        if len(assigned) != len(set(assigned)):
            raise ValueError("Person assigned more than once")

    def is_complete(self, puzzle: Puzzle) -> bool:
        """Return whether every seat has an occupant."""
        self.validate(puzzle)
        return all(person is not None for person in self.occupants)
