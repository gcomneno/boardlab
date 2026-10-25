"""Concurrent access and replay tests for the in-memory match manager."""

from concurrent.futures import ThreadPoolExecutor
from threading import Barrier, Lock
from unittest import TestCase, main
from unittest.mock import patch

from boardlab.games.dado_goal_match import MatchError, MatchManager


def race(count, operation):
    """Start all participating workers at the same barrier."""
    barrier = Barrier(count)

    def worker(index):
        barrier.wait(timeout=10)
        try:
            return ("ok", operation(index))
        except MatchError as exc:
            return ("rejected", str(exc))

    with ThreadPoolExecutor(max_workers=count) as pool:
        return list(pool.map(worker, range(count)))


class ConcurrencyTests(TestCase):

    def setUp(self):
        self.manager = MatchManager()
        created = self.manager.create(2)
        self.match_id = created["matchId"]
        self.host = created["hostToken"]
        self.invite = created["inviteToken"]

    def activate(self):
        return self.manager.join(
            self.match_id, self.invite
        )["guestToken"]

    def test_simultaneous_join_allows_one_guest(self):
        results = race(
            8,
            lambda _: self.manager.join(
                self.match_id, self.invite
            ),
        )

        successful = [v for status, v in results if status == "ok"]
        rejected = [v for status, v in results if status == "rejected"]

        self.assertEqual(len(successful), 1)
        self.assertEqual(len(rejected), 7)

        token = successful[0]["guestToken"]
        self.assertEqual(
            self.manager.snapshot(self.match_id, token)["seat"], 1
        )
        self.assertEqual(
            self.manager.snapshot(self.match_id, self.host)["revision"], 1
        )

    def test_identical_concurrent_roll_is_idempotent(self):
        self.activate()

        count_lock = Lock()
        draws = [0]

        def deterministic_roll(_):
            with count_lock:
                draws[0] += 1
            return 2

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            side_effect=deterministic_roll,
        ):
            results = race(
                12,
                lambda _: self.manager.roll(
                    self.match_id, self.host, "same-roll", 1
                ),
            )

        accepted = [v for status, v in results if status == "ok"]

        self.assertEqual(len(accepted), 12)
        self.assertEqual(draws[0], 1)
        self.assertEqual(sum(not x["duplicate"] for x in accepted), 1)
        self.assertEqual(sum(x["duplicate"] for x in accepted), 11)
        self.assertEqual({x["revision"] for x in accepted}, {2})

        self.assertEqual(
            self.manager.snapshot(self.match_id, self.host)["revision"], 2
        )

    def test_distinct_requests_same_revision_only_one_wins(self):
        self.activate()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ) as rng:
            results = race(
                12,
                lambda index: self.manager.roll(
                    self.match_id,
                    self.host,
                    f"request-{index}",
                    1,
                ),
            )

        accepted = [v for status, v in results if status == "ok"]
        rejected = [v for status, v in results if status == "rejected"]

        self.assertEqual(len(accepted), 1)
        self.assertEqual(len(rejected), 11)
        self.assertTrue(all(x == "Stale revision" for x in rejected))
        self.assertEqual(rng.call_count, 1)
        self.assertEqual(accepted[0]["revision"], 2)

    def test_wrong_seat_cannot_race_active_player(self):
        guest = self.activate()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ) as rng:
            results = race(
                2,
                lambda index: self.manager.roll(
                    self.match_id,
                    self.host if index == 0 else guest,
                    f"seat-{index}",
                    1,
                ),
            )

        accepted = [v for status, v in results if status == "ok"]

        self.assertEqual(len(accepted), 1)
        self.assertEqual(accepted[0]["revision"], 2)
        self.assertEqual(rng.call_count, 1)

    def test_replayed_receipt_does_not_advance_state(self):
        self.activate()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ) as rng:
            original = self.manager.roll(
                self.match_id, self.host, "stable-id", 1
            )

            before = self.manager.snapshot(self.match_id, self.host)

            for _ in range(50):
                replay = self.manager.roll(
                    self.match_id, self.host, "stable-id", 1
                )
                self.assertTrue(replay["duplicate"])
                self.assertEqual(replay["state"], original["state"])

            after = self.manager.snapshot(self.match_id, self.host)

        self.assertEqual(before, after)
        self.assertEqual(rng.call_count, 1)

    def test_replayed_id_with_new_revision_rejected(self):
        self.activate()

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ):
            self.manager.roll(
                self.match_id, self.host, "bound-id", 1
            )

        with self.assertRaisesRegex(
            MatchError, "Request ID reused"
        ):
            self.manager.roll(
                self.match_id, self.host, "bound-id", 2
            )

    def test_concurrent_capacity_limit(self):
        manager = MatchManager(max_matches=3)

        results = race(12, lambda _: manager.create(1))

        accepted = [v for status, v in results if status == "ok"]
        rejected = [v for status, v in results if status == "rejected"]

        self.assertEqual(len(accepted), 3)
        self.assertEqual(len(rejected), 9)
        self.assertEqual(
            len({v["matchId"] for v in accepted}), 3
        )


if __name__ == "__main__":
    main()
