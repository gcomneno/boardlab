"""Qualification of isolated SQLite v4 create/join/recovery."""

from pathlib import Path

import pytest

from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import (
    LifecycleAuthorityError,
    LifecycleAuthorityV4,
)


class Clock:
    def __init__(self, now: int = 1000) -> None:
        self.now = now

    def __call__(self) -> int:
        return self.now


@pytest.fixture
def session(tmp_path: Path) -> tuple[LifecycleAuthorityV4, Clock]:
    clock = Clock()
    manager = LifecycleAuthorityV4(
        tmp_path / "matches.sqlite",
        LifecyclePolicy(60, 120, 300),
        clock=clock,
    )
    manager.initialize()
    return manager, clock


def test_create_waiting_session(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    created = manager.create(2)
    assert created["snapshot"]["status"] == "waiting"
    assert created["snapshot"]["revision"] == 0
    assert created["snapshot"]["seat"] == 0
    assert manager.snapshot(created["matchId"], created["hostToken"]) == created["snapshot"]


def test_join_and_authenticated_recovery(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    clock.now = 1030
    joined = manager.join(created["matchId"], created["inviteToken"])
    assert joined["snapshot"]["status"] == "active"
    assert joined["snapshot"]["revision"] == 1
    assert joined["snapshot"]["seat"] == 1

    recovered = LifecycleAuthorityV4(manager.path, manager.policy, clock=clock)
    recovered.initialize()
    assert recovered.snapshot(created["matchId"], joined["guestToken"]) == joined["snapshot"]


def test_waiting_invitation_boundary(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    clock.now = 1060
    with pytest.raises(LifecycleAuthorityError, match="Match expired"):
        manager.join(created["matchId"], created["inviteToken"])


def test_expired_session_rejects_snapshot(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    clock.now = 1060
    with pytest.raises(LifecycleAuthorityError, match="Match expired"):
        manager.snapshot(created["matchId"], created["hostToken"])


def test_invite_cannot_authenticate(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    created = manager.create(2)
    with pytest.raises(LifecycleAuthorityError, match="Unauthorized participant"):
        manager.snapshot(created["matchId"], created["inviteToken"])


def test_join_once_only(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])
    with pytest.raises(LifecycleAuthorityError, match="Match is not joinable"):
        manager.join(created["matchId"], created["inviteToken"])


def test_poll_does_not_extend_expiration(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    clock.now = 1050
    manager.snapshot(created["matchId"], created["hostToken"])
    clock.now = 1060
    with pytest.raises(LifecycleAuthorityError, match="Match expired"):
        manager.snapshot(created["matchId"], created["hostToken"])


def test_invalid_match_length(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    with pytest.raises(LifecycleAuthorityError, match="Invalid match"):
        manager.create(0)


def test_v4_roll_and_idempotent_receipt(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    joined = manager.join(created["matchId"], created["inviteToken"])

    clock.now = 1020
    result = manager.roll(created["matchId"], created["hostToken"], "request-1", 1)
    assert result["revision"] == 2
    assert result["duplicate"] is False

    replay = manager.roll(created["matchId"], created["hostToken"], "request-1", 1)
    assert replay["duplicate"] is True
    assert replay["face"] == result["face"]

    recovered = LifecycleAuthorityV4(manager.path, manager.policy, clock=clock)
    recovered.initialize()
    assert recovered.snapshot(created["matchId"], joined["guestToken"])["revision"] == 2


def test_v4_expired_roll_rejected_without_advancement(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])

    clock.now = 1120
    with pytest.raises(LifecycleAuthorityError, match="Match expired"):
        manager.roll(created["matchId"], created["hostToken"], "request-1", 1)

    clock.now = 1119
    assert manager.snapshot(created["matchId"], created["hostToken"])["revision"] == 1


def test_v4_roll_wrong_seat_rejected(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    created = manager.create(2)
    joined = manager.join(created["matchId"], created["inviteToken"])

    with pytest.raises(LifecycleAuthorityError, match="Not your turn"):
        manager.roll(created["matchId"], joined["guestToken"], "request-1", 1)


def test_v4_request_id_conflict(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, _ = session
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])

    manager.roll(created["matchId"], created["hostToken"], "request-1", 1)
    with pytest.raises(LifecycleAuthorityError, match="other revision"):
        manager.roll(created["matchId"], created["hostToken"], "request-1", 2)


def test_v4_cleanup_preserves_unexpired_matches(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    expired = manager.create(2)
    clock.now = 1030
    retained = manager.create(2)

    clock.now = 1060
    assert manager.cleanup_expired(limit=10) == 1
    assert manager.snapshot(retained["matchId"], retained["hostToken"])["status"] == "waiting"

    # Missing seats must not disclose whether a match existed.
    with pytest.raises(LifecycleAuthorityError, match="Unauthorized participant"):
        manager.snapshot(expired["matchId"], expired["hostToken"])

    import sqlite3

    with sqlite3.connect(manager.path) as db:
        for table in (
            "lifecycle_matches",
            "lifecycle_seats",
            "lifecycle_events",
            "lifecycle_receipts",
        ):
            count = db.execute(
                f"SELECT COUNT(*) FROM {table} WHERE match_id=?",
                (expired["matchId"],),
            ).fetchone()[0]
            assert count == 0, table


def test_v4_cleanup_cascades_related_records(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    import sqlite3

    manager, clock = session
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])
    manager.roll(created["matchId"], created["hostToken"], "r1", 1)

    clock.now = 1120
    assert manager.cleanup_expired(limit=10) == 1

    with sqlite3.connect(manager.path) as db:
        for table in (
            "lifecycle_matches",
            "lifecycle_seats",
            "lifecycle_events",
            "lifecycle_receipts",
        ):
            assert db.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0] == 0


def test_v4_cleanup_respects_batch_limit(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    for _ in range(3):
        manager.create(2)

    clock.now = 1060
    assert manager.cleanup_expired(limit=2) == 2
    assert manager.cleanup_expired(limit=2) == 1
    assert manager.cleanup_expired(limit=2) == 0


@pytest.mark.parametrize("limit", [0, -1, True, 1001])
def test_v4_invalid_cleanup_limit(
    session: tuple[LifecycleAuthorityV4, Clock],
    limit: object,
) -> None:
    manager, _ = session
    with pytest.raises(LifecycleAuthorityError, match="Invalid cleanup"):
        manager.cleanup_expired(limit=limit)  # type: ignore[arg-type]


def test_v4_committed_roll_survives_restart(
    session: tuple[LifecycleAuthorityV4, Clock],
) -> None:
    manager, clock = session
    created = manager.create(2)
    manager.join(created["matchId"], created["inviteToken"])
    result = manager.roll(created["matchId"], created["hostToken"], "request-1", 1)

    restarted = LifecycleAuthorityV4(manager.path, manager.policy, clock=clock)
    restarted.initialize()
    duplicate = restarted.roll(created["matchId"], created["hostToken"], "request-1", 1)

    assert duplicate["duplicate"] is True
    assert duplicate["face"] == result["face"]
    assert restarted.snapshot(created["matchId"], created["hostToken"])["revision"] == 2
