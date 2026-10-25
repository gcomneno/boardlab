"""Standard-library tests for the local authoritative manager."""

import unittest
from unittest.mock import patch

from boardlab.games.dado_goal_match import MatchError, MatchManager


class MatchManagerTests(unittest.TestCase):
    def setUp(self):
        self.manager = MatchManager()
        created = self.manager.create(1)
        self.match_id = created["matchId"]
        self.host = created["hostToken"]
        self.invite = created["inviteToken"]

    def join(self):
        return self.manager.join(
            self.match_id, self.invite
        )["guestToken"]

    def test_create_waits_for_friend(self):
        state = self.manager.snapshot(self.match_id, self.host)
        self.assertEqual(state["status"], "waiting")
        self.assertEqual(state["revision"], 0)

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, self.host, "a", 0
            )

    def test_invite_is_not_a_seat_token(self):
        with self.assertRaises(MatchError):
            self.manager.snapshot(self.match_id, self.invite)

        with self.assertRaises(MatchError):
            self.manager.join(self.match_id, "wrong")

    def test_join_and_permissions(self):
        guest = self.join()

        self.assertEqual(
            self.manager.snapshot(self.match_id, guest)["seat"],
            1,
        )

        with self.assertRaises(MatchError):
            self.manager.join(self.match_id, self.invite)

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, guest, "early", 1
            )

    def test_duplicate_roll_is_idempotent(self):
        self.join()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ) as rng:
            first = self.manager.roll(
                self.match_id, self.host, "req1", 1
            )
            second = self.manager.roll(
                self.match_id, self.host, "req1", 1
            )

        self.assertEqual(rng.call_count, 1)
        self.assertFalse(first["duplicate"])
        self.assertTrue(second["duplicate"])
        self.assertEqual(first["state"], second["state"])
        self.assertEqual(first["revision"], second["revision"])

    def test_stale_and_wrong_revision(self):
        self.join()

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, self.host, "a", 0
            )

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, self.host, "a", True
            )

    def test_complete_two_player_match(self):
        guest = self.join()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ):
            first = self.manager.roll(
                self.match_id, self.host, "one", 1
            )
            second = self.manager.roll(
                self.match_id, guest, "two", 2
            )

        self.assertEqual(first["face"], "Passaggio intercettato!")
        self.assertEqual(second["status"], "completed")
        self.assertEqual(second["state"]["hands"], [1, 1])
        self.assertEqual(second["state"]["phase"], "finished")

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, self.host, "three", 3
            )

    def test_request_id_binding(self):
        self.join()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ):
            self.manager.roll(
                self.match_id, self.host, "same", 1
            )

        with self.assertRaises(MatchError):
            self.manager.roll(
                self.match_id, self.host, "same", 2
            )

    def test_capacity(self):
        limited = MatchManager(max_matches=1)
        limited.create(1)

        with self.assertRaises(MatchError):
            limited.create(1)

    def test_invalid_match_length(self):
        for value in (0, -1, 1001, True, 1.5):
            with self.subTest(value=value), self.assertRaises(MatchError):
                self.manager.create(value)


if __name__ == "__main__":
    unittest.main()
