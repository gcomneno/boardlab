"""SQLite v2 credential tests, without external dependencies."""

import sqlite3
import tempfile
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from boardlab.games.dado_goal_credentials import (
    CredentialError,
    DurableCredentials,
)


class DurableCredentialTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "credentials.sqlite3"
        self.store = DurableCredentials(self.path)
        self.store.initialize()

    def test_creation_and_restart(self):
        created = self.store.create(2)
        restored = DurableCredentials(self.path)

        restored.initialize()

        self.assertTrue(
            restored.verify_seat(
                created["matchId"], 0, created["hostToken"]
            )
        )
        self.assertEqual(
            restored.snapshot(created["matchId"])["revision"], 0
        )

    def test_invitation_is_not_seat_credential(self):
        created = self.store.create(1)

        self.assertFalse(
            self.store.verify_seat(
                created["matchId"], 0, created["inviteToken"]
            )
        )

    def test_guest_join_survives_restart(self):
        created = self.store.create(1)

        guest = self.store.join(
            created["matchId"], created["inviteToken"]
        )

        reopened = DurableCredentials(self.path)

        self.assertTrue(
            reopened.verify_seat(created["matchId"], 1, guest)
        )
        self.assertEqual(
            reopened.snapshot(created["matchId"])["revision"], 1
        )

        with self.assertRaises(CredentialError):
            reopened.join(
                created["matchId"], created["inviteToken"]
            )

    def test_concurrent_join_one_winner(self):
        created = self.store.create(1)

        def attempt(_):
            try:
                return self.store.join(
                    created["matchId"], created["inviteToken"]
                )
            except CredentialError:
                return None

        with ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(attempt, range(8)))

        accepted = [value for value in results if value is not None]

        self.assertEqual(len(accepted), 1)
        self.assertEqual(
            self.store.snapshot(created["matchId"])["revision"], 1
        )

    def test_database_contains_no_plaintext_tokens(self):
        created = self.store.create(1)
        guest = self.store.join(
            created["matchId"], created["inviteToken"]
        )

        with sqlite3.connect(self.path) as db:
            rows = list(db.iterdump())

        dump = "\n".join(rows)

        for token in (
            created["hostToken"],
            created["inviteToken"],
            guest,
        ):
            self.assertNotIn(token, dump)

    def test_existing_v1_database_refused(self):
        path = Path(self.temp.name) / "old.sqlite3"

        with sqlite3.connect(path) as db:
            db.execute(
                "CREATE TABLE schema_meta(version INTEGER NOT NULL)"
            )
            db.execute("INSERT INTO schema_meta VALUES (1)")

        with self.assertRaisesRegex(
            CredentialError, "migration required"
        ):
            DurableCredentials(path).initialize()

    def test_no_database_on_construction(self):
        path = Path(self.temp.name) / "unused.sqlite3"
        DurableCredentials(path)
        self.assertFalse(path.exists())


if __name__ == "__main__":
    unittest.main()
