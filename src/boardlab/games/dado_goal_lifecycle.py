"""Deterministic lifecycle policy for Dado Goal multiplayer v4.

Pure policy only. No database access, migration or server exposure.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum


class LifecycleStatus(StrEnum):
    WAITING = "waiting"
    ACTIVE = "active"
    COMPLETED = "completed"


@dataclass(frozen=True)
class LifecyclePolicy:
    waiting_ttl: int
    active_idle_ttl: int
    completed_retention_ttl: int

    def __post_init__(self) -> None:
        for value in (
            self.waiting_ttl,
            self.active_idle_ttl,
            self.completed_retention_ttl,
        ):
            if type(value) is not int or not 1 <= value <= 31_536_000:
                raise ValueError("Invalid lifecycle duration")

    def duration(self, status: LifecycleStatus) -> int:
        if status is LifecycleStatus.WAITING:
            return self.waiting_ttl
        if status is LifecycleStatus.ACTIVE:
            return self.active_idle_ttl
        if status is LifecycleStatus.COMPLETED:
            return self.completed_retention_ttl
        raise ValueError("Invalid lifecycle status")


@dataclass(frozen=True)
class LifecycleRecord:
    status: LifecycleStatus
    created_at: int
    updated_at: int
    expires_at: int
    completed_at: int | None = None

    def __post_init__(self) -> None:
        if not isinstance(self.status, LifecycleStatus):
            raise ValueError("Invalid lifecycle status")

        for timestamp in (
            self.created_at,
            self.updated_at,
            self.expires_at,
        ):
            if type(timestamp) is not int or timestamp < 0:
                raise ValueError("Invalid lifecycle timestamp")

        if not self.created_at <= self.updated_at < self.expires_at:
            raise ValueError("Invalid lifecycle ordering")

        if self.status is LifecycleStatus.COMPLETED:
            if (
                type(self.completed_at) is not int
                or not self.created_at <= self.completed_at <= self.updated_at
            ):
                raise ValueError("Invalid completion timestamp")
        elif self.completed_at is not None:
            raise ValueError("Unexpected completion timestamp")


def create_record(
    policy: LifecyclePolicy,
    now: int,
) -> LifecycleRecord:
    if type(now) is not int or now < 0:
        raise ValueError("Invalid server timestamp")

    return LifecycleRecord(
        status=LifecycleStatus.WAITING,
        created_at=now,
        updated_at=now,
        expires_at=now + policy.waiting_ttl,
    )


def is_expired(record: LifecycleRecord, now: int) -> bool:
    if type(now) is not int or now < 0:
        raise ValueError("Invalid server timestamp")
    return now >= record.expires_at


def transition(
    record: LifecycleRecord,
    policy: LifecyclePolicy,
    status: LifecycleStatus,
    now: int,
) -> LifecycleRecord:
    if not isinstance(status, LifecycleStatus):
        raise ValueError("Invalid lifecycle status")
    if type(now) is not int or now < record.updated_at:
        raise ValueError("Invalid server timestamp")
    if is_expired(record, now):
        raise ValueError("Match expired")

    allowed = {
        LifecycleStatus.WAITING: {LifecycleStatus.ACTIVE},
        LifecycleStatus.ACTIVE: {
            LifecycleStatus.ACTIVE,
            LifecycleStatus.COMPLETED,
        },
        LifecycleStatus.COMPLETED: set(),
    }

    if status not in allowed[record.status]:
        raise ValueError("Invalid lifecycle transition")

    return LifecycleRecord(
        status=status,
        created_at=record.created_at,
        updated_at=now,
        expires_at=now + policy.duration(status),
        completed_at=(now if status is LifecycleStatus.COMPLETED else None),
    )
