"""Regression tests for spatial puzzle topologies."""

from dataclasses import FrozenInstanceError

import pytest

from boardlab.puzzles.topology import SeatTopology


def test_empty_edges_are_valid() -> None:
    topology = SeatTopology(("A", "B", "C"))
    assert topology.edges == ()
    assert topology.neighbors("A") == ()
    assert not topology.adjacent("A", "B")


def test_explicit_adjacency_is_symmetric() -> None:
    topology = SeatTopology(
        ("A", "B", "C", "D"),
        (("B", "A"), ("D", "C")),
    )

    assert topology.edges == (("A", "B"), ("C", "D"))
    assert topology.adjacent("A", "B")
    assert topology.adjacent("B", "A")
    assert not topology.adjacent("B", "C")


def test_neighbors_follow_seat_order() -> None:
    topology = SeatTopology(
        ("C", "A", "B"),
        (("B", "A"), ("C", "A")),
    )

    assert topology.neighbors("A") == ("C", "B")
    assert topology.edges == (("C", "A"), ("A", "B"))


def test_topology_is_immutable() -> None:
    topology = SeatTopology(("A", "B"), (("A", "B"),))

    with pytest.raises(FrozenInstanceError):
        topology.seats = ("X",)  # type: ignore[misc]

    with pytest.raises(FrozenInstanceError):
        topology.edges = ()  # type: ignore[misc]


@pytest.mark.parametrize(
    "seats,edges",
    [
        ((), ()),
        (("A", "A"), ()),
        (("A", 3), ()),
        (["A", "B"], ()),
        (("A", "B"), [("A", "B")]),
        (("A", "B"), (["A", "B"],)),
        (("A", "B"), (("A",),)),
        (("A", "B"), (("A", "C"),)),
        (("A", "B"), (("A", "A"),)),
        (("A", "B"), (("A", "B"), ("B", "A"))),
        (("A", "B"), ((1, "B"),)),
    ],
)
def test_invalid_topologies_are_rejected(seats: object, edges: object) -> None:
    with pytest.raises(ValueError):
        SeatTopology(seats, edges)  # type: ignore[arg-type]


def test_unknown_seats_are_rejected() -> None:
    topology = SeatTopology(("A", "B"), (("A", "B"),))

    with pytest.raises(ValueError, match="Unknown seat"):
        topology.adjacent("A", "missing")

    with pytest.raises(ValueError, match="Unknown seat"):
        topology.neighbors("missing")


def test_puzzle_seat_compatibility() -> None:
    topology = SeatTopology(("A", "B"))

    topology.validate_seats(("A", "B"))

    with pytest.raises(ValueError, match="Topology and puzzle seats differ"):
        topology.validate_seats(("B", "A"))


def test_disconnected_components_are_allowed() -> None:
    topology = SeatTopology(
        ("A1", "A2", "B1", "B2"),
        (("A1", "A2"), ("B1", "B2")),
    )

    assert topology.neighbors("A2") == ("A1",)
    assert not topology.adjacent("A2", "B1")
