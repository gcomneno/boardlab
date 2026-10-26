"""Isolated SQLite v4 schema for Dado Goal lifecycle research.

Schema qualification only: no legacy migration, public server,
implicit cleanup or gameplay integration.
"""

from __future__ import annotations

import sqlite3
from pathlib import Path

SCHEMA_VERSION = 4

SCHEMA = """
CREATE TABLE lifecycle_matches (
    match_id TEXT PRIMARY KEY,
    revision INTEGER NOT NULL CHECK(revision >= 0),
    status TEXT NOT NULL
        CHECK(status IN ('waiting', 'active', 'completed')),
    state_json TEXT NOT NULL,
    invite_digest BLOB,
    invite_used INTEGER NOT NULL DEFAULT 0
        CHECK(invite_used IN (0, 1)),
    created_at INTEGER NOT NULL CHECK(created_at >= 0),
    updated_at INTEGER NOT NULL CHECK(updated_at >= created_at),
    expires_at INTEGER NOT NULL CHECK(expires_at > updated_at),
    completed_at INTEGER,
    CHECK(
        (status = 'completed'
         AND completed_at IS NOT NULL
         AND completed_at >= created_at
         AND completed_at <= updated_at)
        OR
        (status != 'completed' AND completed_at IS NULL)
    )
);

CREATE TABLE lifecycle_seats (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    seat INTEGER NOT NULL CHECK(seat IN (0, 1)),
    token_digest BLOB NOT NULL,
    PRIMARY KEY(match_id, seat),
    UNIQUE(token_digest)
);

CREATE TABLE lifecycle_events (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    revision INTEGER NOT NULL,
    event_json TEXT NOT NULL,
    PRIMARY KEY(match_id, revision)
);

CREATE TABLE lifecycle_receipts (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    seat INTEGER NOT NULL CHECK(seat IN (0, 1)),
    request_id TEXT NOT NULL,
    expected_revision INTEGER NOT NULL,
    response_json TEXT NOT NULL,
    PRIMARY KEY(match_id, seat, request_id)
);
"""


class LifecycleSchemaError(ValueError):
    """Unsupported or incomplete lifecycle schema."""


class LifecycleDatabaseV4:
    """Explicit v4 initializer; no implicit migrations."""

    REQUIRED_TABLES = frozenset(
        {
            "lifecycle_matches",
            "lifecycle_seats",
            "lifecycle_events",
            "lifecycle_receipts",
        }
    )

    def __init__(self, path: str | Path) -> None:
        self.path = Path(path)
        if str(path) == ":memory:" or not self.path.parent.is_dir():
            raise ValueError("Explicit filesystem database path required")

    def _connect(self) -> sqlite3.Connection:
        db = sqlite3.connect(
            self.path,
            timeout=5,
            isolation_level=None,
        )
        db.execute("PRAGMA foreign_keys=ON")
        db.execute("PRAGMA busy_timeout=5000")
        return db

    def initialize(self) -> None:
        """Create only a fresh database, or verify existing v4."""

        if self.path.exists():
            db = self._connect()
            try:
                version = db.execute("PRAGMA user_version").fetchone()[0]
                tables = {
                    row[0]
                    for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")
                }
                if version != SCHEMA_VERSION or not self.REQUIRED_TABLES.issubset(tables):
                    raise LifecycleSchemaError("Unsupported or incomplete lifecycle schema")

                match_columns = {
                    row[1] for row in db.execute("PRAGMA table_info(lifecycle_matches)")
                }
                required_columns = {
                    "match_id",
                    "revision",
                    "status",
                    "state_json",
                    "invite_digest",
                    "invite_used",
                    "created_at",
                    "updated_at",
                    "expires_at",
                    "completed_at",
                }
                if not required_columns.issubset(match_columns):
                    raise LifecycleSchemaError("Incomplete lifecycle match columns")
            finally:
                db.close()
            return

        # No database files are created in the constructor.
        # SQLite executescript commits on completion of this schema
        # initialization. This is not a legacy migration.
        db = self._connect()
        try:
            script = (
                "BEGIN IMMEDIATE;\n"
                + SCHEMA
                + f"\nPRAGMA user_version={SCHEMA_VERSION};\n"
                + "COMMIT;\n"
            )
            try:
                db.executescript(script)
            except BaseException:
                if db.in_transaction:
                    db.rollback()
                raise
        finally:
            db.close()
