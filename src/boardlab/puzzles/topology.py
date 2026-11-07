"""Immutable, undirected spatial topology for finite seating puzzles."""

from dataclasses import dataclass


@dataclass(frozen=True)
class SeatTopology:
    """Seats connected by explicitly declared, undirected adjacency edges.

    Edge endpoints follow the order in ``seats`` for canonical storage.
    Seat order itself does not imply adjacency.
    """

    seats: tuple[str, ...]
    edges: tuple[tuple[str, str], ...] = ()

    def __post_init__(self) -> None:
        if not isinstance(self.seats, tuple) or not self.seats:
            raise ValueError("Seats must be a nonempty tuple")

        if any(type(seat) is not str or not seat for seat in self.seats):
            raise ValueError("Invalid seat identifier")

        if len(set(self.seats)) != len(self.seats):
            raise ValueError("Duplicate seats")

        if not isinstance(self.edges, tuple):
            raise ValueError("Edges must be a tuple")

        indices = {seat: index for index, seat in enumerate(self.seats)}
        canonical: list[tuple[str, str]] = []
        seen: set[tuple[str, str]] = set()

        for edge in self.edges:
            if not isinstance(edge, tuple) or len(edge) != 2:
                raise ValueError("Invalid edge structure")

            left, right = edge

            if type(left) is not str or type(right) is not str:
                raise ValueError("Invalid edge endpoint")

            if left not in indices or right not in indices:
                raise ValueError("Unknown seat")

            if left == right:
                raise ValueError("Self adjacency is forbidden")

            if indices[left] > indices[right]:
                left, right = right, left

            normalized = (left, right)

            if normalized in seen:
                raise ValueError("Duplicate adjacency edge")

            seen.add(normalized)
            canonical.append(normalized)

        canonical.sort(key=lambda edge: (indices[edge[0]], indices[edge[1]]))
        object.__setattr__(self, "edges", tuple(canonical))

    def adjacent(self, left: str, right: str) -> bool:
        """Check explicit undirected adjacency between existing seats."""
        if left not in self.seats or right not in self.seats:
            raise ValueError("Unknown seat")

        return (left, right) in self.edges or (right, left) in self.edges

    def neighbors(self, seat: str) -> tuple[str, ...]:
        """Return adjacent seats in canonical seat order."""
        if seat not in self.seats:
            raise ValueError("Unknown seat")

        linked = {
            right if left == seat else left
            for left, right in self.edges
            if left == seat or right == seat
        }

        return tuple(candidate for candidate in self.seats if candidate in linked)

    def validate_seats(self, seats: tuple[str, ...]) -> None:
        """Require exact correspondence with a puzzle's ordered seats."""
        if type(seats) is not tuple or seats != self.seats:
            raise ValueError("Topology and puzzle seats differ")
