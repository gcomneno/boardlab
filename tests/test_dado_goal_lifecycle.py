"""Deterministic tests for the isolated v4 lifecycle policy."""

import pytest

from boardlab.games.dado_goal_lifecycle import (
    LifecyclePolicy,
    LifecycleStatus,
    create_record,
    is_expired,
    transition,
)


@pytest.fixture
def policy() -> LifecyclePolicy:
    return LifecyclePolicy(
        waiting_ttl=60,
        active_idle_ttl=120,
        completed_retention_ttl=300,
    )


def test_waiting_expiry_boundary(policy: LifecyclePolicy) -> None:
    record = create_record(policy, 1000)
    assert record.expires_at == 1060
    assert not is_expired(record, 1059)
    assert is_expired(record, 1060)


def test_join_starts_active_idle_window(policy: LifecyclePolicy) -> None:
    waiting = create_record(policy, 1000)
    active = transition(waiting, policy, LifecycleStatus.ACTIVE, 1030)
    assert active.expires_at == 1150
    assert active.created_at == 1000


def test_authoritative_activity_extends_idle(
    policy: LifecyclePolicy,
) -> None:
    waiting = create_record(policy, 1000)
    active = transition(waiting, policy, LifecycleStatus.ACTIVE, 1010)
    updated = transition(active, policy, LifecycleStatus.ACTIVE, 1050)
    assert updated.expires_at == 1170


def test_completed_retention(policy: LifecyclePolicy) -> None:
    waiting = create_record(policy, 1000)
    active = transition(waiting, policy, LifecycleStatus.ACTIVE, 1010)
    completed = transition(active, policy, LifecycleStatus.COMPLETED, 1020)
    assert completed.completed_at == 1020
    assert completed.expires_at == 1320


def test_expired_transition_rejected(
    policy: LifecyclePolicy,
) -> None:
    waiting = create_record(policy, 1000)
    with pytest.raises(ValueError, match="Match expired"):
        transition(waiting, policy, LifecycleStatus.ACTIVE, 1060)


def test_completed_match_cannot_resume(
    policy: LifecyclePolicy,
) -> None:
    waiting = create_record(policy, 1000)
    active = transition(waiting, policy, LifecycleStatus.ACTIVE, 1010)
    completed = transition(active, policy, LifecycleStatus.COMPLETED, 1020)
    with pytest.raises(ValueError, match="Invalid lifecycle transition"):
        transition(completed, policy, LifecycleStatus.ACTIVE, 1030)


@pytest.mark.parametrize("duration", [0, -1, True, 1.5, None])
def test_invalid_duration(duration: object) -> None:
    with pytest.raises(ValueError, match="Invalid lifecycle duration"):
        LifecyclePolicy(duration, 120, 300)  # type: ignore[arg-type]
