"""SQLite v4 concurrency and crash-recovery qualification."""

from __future__ import annotations

import multiprocessing as mp
import os
import sqlite3
from pathlib import Path

import pytest

from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import (
    LifecycleAuthorityError,
    LifecycleAuthorityV4,
)

POLICY = LifecyclePolicy(60, 120, 300)


def authority(path: str, now: int = 1000) -> LifecycleAuthorityV4:
    return LifecycleAuthorityV4(path, POLICY, clock=lambda: now)


def worker_roll(
    path: str,
    match_id: str,
    token: str,
    request_id: str,
    queue: mp.Queue,
) -> None:
    manager = authority(path)
    try:
        result = manager.roll(match_id, token, request_id, 1)
        queue.put(("PASS", result["revision"]))
    except LifecycleAuthorityError as exc:
        queue.put(("REJECTED", str(exc)))
    except BaseException as exc:
        queue.put(("ERROR", repr(exc)))


def worker_cleanup(path: str, queue: mp.Queue) -> None:
    manager = authority(path, 1120)
    try:
        queue.put(("PASS", manager.cleanup_expired(limit=10)))
    except BaseException as exc:
        queue.put(("ERROR", repr(exc)))


@pytest.fixture
def match(tmp_path: Path) -> tuple[str, dict[str, object]]:
    path = str(tmp_path / "v4.sqlite")
    manager = authority(path)
    manager.initialize()
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])
    return path, created


def test_two_processes_one_authoritative_roll(
    match: tuple[str, dict[str, object]],
) -> None:
    path, created = match
    context = mp.get_context("spawn")
    queue = context.Queue()

    processes = [
        context.Process(
            target=worker_roll,
            args=(
                path,
                created["matchId"],
                created["hostToken"],
                f"request-{index}",
                queue,
            ),
        )
        for index in range(2)
    ]

    for process in processes:
        process.start()
    for process in processes:
        process.join(timeout=15)
        assert process.exitcode == 0

    results = [queue.get(timeout=5) for _ in processes]
    assert sum(result[0] == "PASS" for result in results) == 1
    assert sum(result[0] == "REJECTED" for result in results) == 1

    with sqlite3.connect(path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_receipts").fetchone()[0] == 1
        assert db.execute("SELECT COUNT(*) FROM lifecycle_events").fetchone()[0] == 2


def test_cleanup_after_expiration(
    match: tuple[str, dict[str, object]],
) -> None:
    path, created = match
    context = mp.get_context("spawn")
    queue = context.Queue()
    process = context.Process(target=worker_cleanup, args=(path, queue))
    process.start()
    process.join(timeout=15)
    assert process.exitcode == 0

    assert queue.get(timeout=5) == ("PASS", 1)

    with sqlite3.connect(path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0] == 0


def test_crash_before_commit_preserves_revision(
    match: tuple[str, dict[str, object]],
) -> None:
    path, created = match

    # A separate process holds an uncommitted write transaction.
    context = mp.get_context("spawn")
    ready = context.Event()

    process = context.Process(
        target=uncommitted_writer,
        args=(path, ready),
    )
    process.start()
    assert ready.wait(timeout=10)

    process.kill()
    process.join(timeout=10)
    assert process.exitcode is not None

    manager = authority(path)
    snapshot = manager.snapshot(created["matchId"], created["hostToken"])
    assert snapshot["revision"] == 1

    with sqlite3.connect(path) as db:
        assert db.execute("PRAGMA integrity_check").fetchone()[0] == "ok"


def uncommitted_writer(path: str, ready: mp.Event) -> None:
    db = sqlite3.connect(path, isolation_level=None)
    db.execute("BEGIN IMMEDIATE")
    db.execute("UPDATE lifecycle_matches SET revision=99 WHERE status='active'")
    ready.set()
    ready.wait(timeout=30)
    os._exit(0)


def worker_roll_commit_then_exit(
    path: str,
    match_id: str,
    token: str,
    ready: mp.Event,
) -> None:
    """Simulate losing the response after durable commit."""
    manager = authority(path)
    manager.roll(match_id, token, "lost-response-1", 1)
    ready.set()
    os._exit(0)


def test_post_commit_crash_preserves_receipt(
    match: tuple[str, dict[str, object]],
) -> None:
    path, created = match
    context = mp.get_context("spawn")
    ready = context.Event()

    process = context.Process(
        target=worker_roll_commit_then_exit,
        args=(
            path,
            created["matchId"],
            created["hostToken"],
            ready,
        ),
    )
    process.start()

    try:
        assert ready.wait(timeout=10)
    finally:
        if process.is_alive():
            process.kill()
        process.join(timeout=10)

    assert process.exitcode == 0

    recovered = authority(path)
    recovered.initialize()

    replay = recovered.roll(
        created["matchId"],
        created["hostToken"],
        "lost-response-1",
        1,
    )

    assert replay["duplicate"] is True
    assert replay["revision"] == 2
    assert recovered.snapshot(created["matchId"], created["hostToken"])["revision"] == 2

    with sqlite3.connect(path) as db:
        assert db.execute("SELECT COUNT(*) FROM lifecycle_receipts").fetchone()[0] == 1
        assert db.execute("SELECT COUNT(*) FROM lifecycle_events").fetchone()[0] == 2
        assert db.execute("PRAGMA integrity_check").fetchone()[0] == "ok"
