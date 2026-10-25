"""Unified durable authority regression tests."""

import sqlite3
import tempfile
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from unittest.mock import patch

from boardlab.games.dado_goal_durable import (
    DurableMatchError,
    DurableMatchManager,
)


class DurableAuthorityTests(unittest.TestCase):

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "match.sqlite3"
        self.manager = DurableMatchManager(self.path)
        self.manager.initialize()
        self.created = self.manager.create(1)
        self.match_id = self.created["matchId"]
        self.host = self.created["hostToken"]
        self.invite = self.created["inviteToken"]

    def reopen(self):
        restored = DurableMatchManager(self.path)
        restored.initialize()
        return restored

    def join(self):
        return self.manager.join(self.match_id, self.invite)

    def test_initialization_and_recovery(self):
        self.assertEqual(
            self.reopen().snapshot(
                self.match_id, self.host
            )["revision"], 0
        )

    def test_invite_is_not_host_token(self):
        with self.assertRaises(DurableMatchError):
            self.manager.snapshot(self.match_id, self.invite)

    def test_redeem_once_and_recover_guest(self):
        guest = self.join()["guestToken"]
        self.assertEqual(
            self.reopen().snapshot(
                self.match_id, guest
            )["seat"], 1
        )
        with self.assertRaises(DurableMatchError):
            self.join()

    def test_authoritative_roll_and_durable_replay(self):
        self.join()

        with patch(
            "boardlab.games.dado_goal_durable.secrets.randbelow",
            return_value=2,
        ) as random:
            result = self.manager.roll(
                self.match_id, self.host, "request-1", 1
            )
            duplicate = self.reopen().roll(
                self.match_id, self.host, "request-1", 1
            )

        random.assert_called_once()
        self.assertFalse(result["duplicate"])
        self.assertTrue(duplicate["duplicate"])
        self.assertEqual(result["state"], duplicate["state"])
        self.assertEqual(self.manager.counts(self.match_id), (2, 1))

    def test_complete_match_survives_restart(self):
        guest = self.join()["guestToken"]

        with patch(
            "boardlab.games.dado_goal_durable.secrets.randbelow",
            return_value=2,
        ):
            first = self.manager.roll(
                self.match_id, self.host, "host-1", 1
            )
            second = self.reopen().roll(
                self.match_id, guest, "guest-1", 2
            )

        self.assertEqual(first["revision"], 2)
        self.assertEqual(second["status"], "completed")
        self.assertEqual(
            self.reopen().snapshot(
                self.match_id, self.host
            )["status"], "completed"
        )

    def test_wrong_seat_rejected(self):
        guest = self.join()["guestToken"]
        before = self.manager.counts(self.match_id)

        with self.assertRaises(DurableMatchError):
            self.manager.roll(
                self.match_id, guest, "out-of-turn", 1
            )

        self.assertEqual(self.manager.counts(self.match_id), before)

    def test_concurrent_join_one_winner(self):
        def attempt(_):
            try:
                return self.reopen().join(
                    self.match_id, self.invite
                )
            except DurableMatchError:
                return None

        with ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(attempt, range(8)))

        self.assertEqual(
            sum(result is not None for result in results), 1
        )

    def test_concurrent_roll_one_winner(self):
        self.join()

        def attempt(i):
            try:
                return self.reopen().roll(
                    self.match_id,
                    self.host,
                    f"request-{i}",
                    1,
                )
            except DurableMatchError:
                return None

        with ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(attempt, range(8)))

        self.assertEqual(
            sum(result is not None for result in results), 1
        )
        self.assertEqual(self.manager.counts(self.match_id), (2, 1))

    def test_receipt_failure_rolls_back_state(self):
        self.join()
        before = self.manager.snapshot(self.match_id, self.host)

        with sqlite3.connect(self.path) as db:
            db.execute("""
                CREATE TRIGGER reject_receipt
                BEFORE INSERT ON durable_receipts
                BEGIN
                    SELECT RAISE(ABORT, 'simulated failure');
                END
            """)

        with self.assertRaises(sqlite3.DatabaseError):
            self.manager.roll(
                self.match_id, self.host, "request-1", 1
            )

        self.assertEqual(
            self.reopen().snapshot(self.match_id, self.host),
            before,
        )
        self.assertEqual(self.manager.counts(self.match_id), (1, 0))

    def test_no_plaintext_credentials_in_database(self):
        guest = self.join()["guestToken"]

        with sqlite3.connect(self.path) as db:
            dump = "\n".join(db.iterdump())

        for token in (self.host, self.invite, guest):
            self.assertNotIn(token, dump)

    def test_reject_legacy_database(self):
        other = Path(self.temp.name) / "legacy.sqlite3"
        with sqlite3.connect(other) as db:
            db.execute("PRAGMA user_version=2")
            db.execute("CREATE TABLE legacy (id INTEGER)")

        with self.assertRaises(DurableMatchError):
            DurableMatchManager(other).initialize()

    def test_no_database_created_by_constructor(self):
        path = Path(self.temp.name) / "unused.sqlite3"
        DurableMatchManager(path)
        self.assertFalse(path.exists())


if __name__ == "__main__":
    unittest.main()
