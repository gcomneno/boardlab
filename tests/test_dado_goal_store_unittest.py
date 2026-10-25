"""Durability, rollback, replay and concurrent SQLite tests."""

import sqlite3
import tempfile
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from boardlab.games.dado_goal import initial_state
from boardlab.games.dado_goal_store import (
    MatchStore,
    StorageConflict,
)


class StoreTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "matches.sqlite3"
        self.store = MatchStore(self.path)
        self.store.initialize()
        self.store.create("match-one", initial_state(1))
        self.store.activate("match-one")

    def test_explicit_creation_and_restart(self):
        self.assertTrue(self.path.is_file())

        reopened = MatchStore(self.path)
        self.assertEqual(
            reopened.snapshot("match-one"),
            self.store.snapshot("match-one"),
        )
        self.assertTrue(reopened.integrity())

    def test_durable_roll_and_receipt(self):
        first = self.store.roll(
            "match-one", 0, "req-1", 1,
            "Passaggio intercettato!",
        )

        reopened = MatchStore(self.path)
        snap = reopened.snapshot("match-one")

        self.assertEqual(snap["revision"], 2)
        self.assertEqual(snap["state"]["hands"], [1, 0])
        self.assertEqual(reopened.counts("match-one"), (2, 1))

        duplicate = reopened.roll(
            "match-one", 0, "req-1", 1,
            "Passaggio intercettato!",
        )
        self.assertTrue(duplicate["duplicate"])
        self.assertEqual(duplicate["state"], first["state"])
        self.assertEqual(reopened.counts("match-one"), (2, 1))

    def test_completed_match_survives_restart(self):
        self.store.roll(
            "match-one", 0, "a", 1,
            "Passaggio intercettato!",
        )
        self.store.roll(
            "match-one", 1, "b", 2,
            "Passaggio intercettato!",
        )

        reopened = MatchStore(self.path)
        snap = reopened.snapshot("match-one")
        self.assertEqual(snap["status"], "completed")
        self.assertEqual(snap["revision"], 3)
        self.assertEqual(snap["state"]["phase"], "finished")
        self.assertEqual(reopened.counts("match-one"), (3, 2))

    def test_invalid_face_rolls_back(self):
        before = self.store.snapshot("match-one")
        counts = self.store.counts("match-one")

        with self.assertRaises(ValueError):
            self.store.roll(
                "match-one", 0, "bad", 1, "GOAL!"
            )

        self.assertEqual(
            self.store.snapshot("match-one"), before
        )
        self.assertEqual(self.store.counts("match-one"), counts)

    def test_stale_revision_rolls_back(self):
        with self.assertRaises(StorageConflict):
            self.store.roll(
                "match-one", 0, "stale", 0,
                "Passaggio intercettato!",
            )
        self.assertEqual(self.store.counts("match-one"), (1, 0))

    def test_concurrent_writers_one_winner(self):
        def submit(i):
            candidate = MatchStore(self.path)
            try:
                return candidate.roll(
                    "match-one", 0, f"req-{i}", 1,
                    "Passaggio intercettato!",
                )
            except StorageConflict:
                return None

        with ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(submit, range(8)))

        self.assertEqual(
            sum(result is not None for result in results), 1
        )
        self.assertEqual(self.store.counts("match-one"), (2, 1))
        self.assertEqual(
            MatchStore(self.path).snapshot("match-one")["revision"],
            2,
        )

    def test_failed_receipt_insert_rolls_back_all(self):
        with sqlite3.connect(self.path) as db:
            db.execute("""
                CREATE TRIGGER deny_receipt
                BEFORE INSERT ON receipts
                BEGIN
                    SELECT RAISE(ABORT, 'injected failure');
                END
            """)

        before = self.store.snapshot("match-one")

        with self.assertRaises(sqlite3.DatabaseError):
            self.store.roll(
                "match-one", 0, "fail", 1,
                "Passaggio intercettato!",
            )

        reopened = MatchStore(self.path)
        self.assertEqual(
            reopened.snapshot("match-one"), before
        )
        self.assertEqual(reopened.counts("match-one"), (1, 0))
        self.assertTrue(reopened.integrity())

    def test_schema_version_rejected(self):
        with sqlite3.connect(self.path) as db:
            db.execute("UPDATE schema_meta SET version=999")

        with self.assertRaises(StorageConflict):
            MatchStore(self.path).initialize()


if __name__ == "__main__":
    unittest.main()
