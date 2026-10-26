"""Isolated SQLite v4 lifecycle schema qualification."""

import sqlite3
from pathlib import Path

import pytest

from boardlab.games.dado_goal_lifecycle_store import (
    SCHEMA_VERSION,
    LifecycleDatabaseV4,
    LifecycleSchemaError,
)


@pytest.fixture
def db_path(tmp_path: Path) -> Path:
    return tmp_path / "lifecycle-v4.sqlite"


def test_constructor_has_no_storage_side_effect(
    db_path: Path,
) -> None:
    LifecycleDatabaseV4(db_path)
    assert not db_path.exists()


def test_create_v4_schema(db_path: Path) -> None:
    storage = LifecycleDatabaseV4(db_path)
    storage.initialize()

    with sqlite3.connect(db_path) as db:
        version = db.execute("PRAGMA user_version").fetchone()[0]
        tables = {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}

    assert version == SCHEMA_VERSION
    assert tables >= LifecycleDatabaseV4.REQUIRED_TABLES


def test_reopening_existing_v4(db_path: Path) -> None:
    storage = LifecycleDatabaseV4(db_path)
    storage.initialize()
    storage.initialize()


@pytest.mark.parametrize("legacy_version", [1, 2, 3])
def test_legacy_schema_rejected_without_mutation(
    db_path: Path,
    legacy_version: int,
) -> None:
    with sqlite3.connect(db_path) as db:
        db.execute(f"PRAGMA user_version={legacy_version}")
        db.execute("CREATE TABLE legacy_marker (value TEXT)")
        db.execute("INSERT INTO legacy_marker VALUES ('preserve')")

    before = db_path.read_bytes()

    with pytest.raises(
        LifecycleSchemaError,
        match="Unsupported",
    ):
        LifecycleDatabaseV4(db_path).initialize()

    assert db_path.read_bytes() == before


def test_reject_incomplete_v4(db_path: Path) -> None:
    with sqlite3.connect(db_path) as db:
        db.execute("PRAGMA user_version=4")

    before = db_path.read_bytes()

    with pytest.raises(LifecycleSchemaError):
        LifecycleDatabaseV4(db_path).initialize()

    assert db_path.read_bytes() == before


def test_reject_invalid_timestamp_order(db_path: Path) -> None:
    LifecycleDatabaseV4(db_path).initialize()

    with sqlite3.connect(db_path) as db, pytest.raises(sqlite3.IntegrityError):
        db.execute(
            "INSERT INTO lifecycle_matches "
            "(match_id,revision,status,state_json,"
            "created_at,updated_at,expires_at) "
            "VALUES ('m1',0,'waiting','{}',100,110,110)"
        )


def test_fk_cascade_is_explicit(db_path: Path) -> None:
    LifecycleDatabaseV4(db_path).initialize()

    with sqlite3.connect(db_path) as db:
        rows = db.execute("PRAGMA foreign_key_list(lifecycle_seats)").fetchall()

    assert any(row[6] == "CASCADE" for row in rows)
