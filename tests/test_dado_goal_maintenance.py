"""Qualification of explicitly invoked SQLite maintenance."""

from __future__ import annotations

import sqlite3
from pathlib import Path

import pytest

from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import LifecycleAuthorityV4
from boardlab.games.dado_goal_maintenance import LifecycleMaintenance


class Clock:
    def __init__(self, now: int = 1000) -> None:
        self.now = now

    def __call__(self) -> int:
        return self.now


@pytest.fixture
def setup(
    tmp_path: Path,
) -> tuple[LifecycleAuthorityV4, Clock]:
    clock = Clock()
    manager = LifecycleAuthorityV4(
        tmp_path / "maintenance.sqlite",
        LifecyclePolicy(60, 120, 300),
        clock=clock,
        max_matches=3,
    )
    manager.initialize()
    return manager, clock


def test_no_implicit_cleanup(
    setup: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = setup
    manager.create(2)
    clock.now = 1060

    LifecycleMaintenance(
        manager,
        interval_seconds=30,
        batch_limit=1,
        clock=clock,
    )

    with sqlite3.connect(manager.path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 1


def test_bounded_batch_and_interval(
    setup: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = setup
    for _ in range(3):
        manager.create(2)

    clock.now = 1060
    maintenance = LifecycleMaintenance(
        manager,
        interval_seconds=30,
        batch_limit=2,
        clock=clock,
    )

    assert maintenance.run_if_due() == 2
    assert maintenance.run_if_due() is None

    clock.now = 1090
    assert maintenance.run_if_due() == 1
    assert maintenance.run_if_due() is None

    with sqlite3.connect(manager.path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 0


def test_cleanup_restores_capacity(
    setup: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = setup
    for _ in range(3):
        manager.create(2)

    clock.now = 1060
    maintenance = LifecycleMaintenance(
        manager,
        interval_seconds=30,
        batch_limit=1,
        clock=clock,
    )

    assert maintenance.run_if_due() == 1
    manager.create(2)

    with sqlite3.connect(manager.path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 3


@pytest.mark.parametrize("interval", [0, -1, True, 86401])
def test_reject_invalid_interval(
    setup: tuple[LifecycleAuthorityV4, Clock],
    interval: object,
) -> None:
    manager, _ = setup
    with pytest.raises(ValueError, match="Invalid maintenance interval"):
        LifecycleMaintenance(
            manager,
            interval_seconds=interval,  # type: ignore[arg-type]
            batch_limit=1,
        )


@pytest.mark.parametrize("limit", [0, -1, True, 1001])
def test_reject_invalid_batch(
    setup: tuple[LifecycleAuthorityV4, Clock],
    limit: object,
) -> None:
    manager, _ = setup
    with pytest.raises(ValueError, match="Invalid maintenance batch"):
        LifecycleMaintenance(
            manager,
            interval_seconds=30,
            batch_limit=limit,  # type: ignore[arg-type]
        )
