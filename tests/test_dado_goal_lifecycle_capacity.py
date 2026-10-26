"""Transactional capacity qualification for Dado Goal SQLite v4."""

from __future__ import annotations

import multiprocessing as mp
import sqlite3
from pathlib import Path
from typing import Any

import pytest

from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import (
    LifecycleAuthorityError,
    LifecycleAuthorityV4,
)

POLICY = LifecyclePolicy(60, 120, 300)


def manager(
    path: Path,
    *,
    now: int = 1000,
    capacity: int | None = 2,
) -> LifecycleAuthorityV4:
    instance = LifecycleAuthorityV4(
        path,
        POLICY,
        clock=lambda: now,
        max_matches=capacity,
    )
    instance.initialize()
    return instance


def test_capacity_is_enforced_atomically(tmp_path: Path) -> None:
    instance = manager(tmp_path / "capacity.sqlite")
    instance.create(2)
    instance.create(2)

    with pytest.raises(LifecycleAuthorityError, match="Match capacity reached"):
        instance.create(2)

    with sqlite3.connect(instance.path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 2


def test_cleanup_restores_capacity(tmp_path: Path) -> None:
    path = tmp_path / "cleanup.sqlite"
    instance = manager(path)
    instance.create(2)
    instance.create(2)

    with pytest.raises(LifecycleAuthorityError, match="capacity"):
        instance.create(2)

    later = manager(path, now=1060)
    assert later.cleanup_expired(limit=1) == 1
    later.create(2)

    with sqlite3.connect(path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 2


@pytest.mark.parametrize("value", [0, -1, True, 1_000_001, 1.5, "10"])
def test_invalid_capacity(tmp_path: Path, value: object) -> None:
    with pytest.raises(ValueError, match="Invalid match capacity"):
        LifecycleAuthorityV4(
            tmp_path / "invalid.sqlite",
            POLICY,
            max_matches=value,  # type: ignore[arg-type]
        )


def test_existing_default_remains_unlimited(tmp_path: Path) -> None:
    instance = manager(
        tmp_path / "unlimited.sqlite",
        capacity=None,
    )
    for _ in range(4):
        instance.create(2)


def concurrent_create(path: str, queue: mp.Queue[Any]) -> None:
    instance = LifecycleAuthorityV4(
        path,
        POLICY,
        clock=lambda: 1000,
        max_matches=1,
    )
    try:
        instance.create(2)
        queue.put("CREATED")
    except LifecycleAuthorityError as exc:
        queue.put(str(exc))
    except BaseException as exc:
        queue.put(f"UNEXPECTED:{exc!r}")


def test_multiprocess_capacity_cannot_be_oversubscribed(
    tmp_path: Path,
) -> None:
    path = tmp_path / "concurrent.sqlite"
    manager(path, capacity=1)

    context = mp.get_context("spawn")
    queue = context.Queue()
    processes = [
        context.Process(
            target=concurrent_create,
            args=(str(path), queue),
        )
        for _ in range(3)
    ]

    for process in processes:
        process.start()

    for process in processes:
        process.join(timeout=15)
        assert process.exitcode == 0

    outcomes = [queue.get(timeout=5) for _ in processes]

    assert outcomes.count("CREATED") == 1
    assert outcomes.count("Match capacity reached") == 2

    with sqlite3.connect(path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 1
