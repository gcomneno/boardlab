"""Wire compatibility tests without external test dependencies."""

import copy
import json
import unittest

from boardlab.games.dado_goal import (
    Phase,
    PinkFace,
    ResultFace,
    TeamFace,
    apply_roll,
    initial_state,
)
from boardlab.games.dado_goal_wire import (
    decode_state,
    encode_state,
)


class WireSchemaTests(unittest.TestCase):
    def test_initial_roundtrip(self):
        for n in (1, 2, 3, 10, 1000):
            state = initial_state(n)
            wire = json.loads(json.dumps(encode_state(state)))
            self.assertEqual(decode_state(wire), state)

    def test_goal_roundtrip(self):
        state = initial_state(2)
        for face in (
            TeamFace.SHOT,
            PinkFace.HEADER,
            ResultFace.GOAL,
        ):
            state = apply_roll(state, face)

        self.assertEqual(state.goals, (1, 0))
        self.assertEqual(
            decode_state(encode_state(state)),
            state,
        )

    def test_finished_roundtrip(self):
        state = initial_state(1)
        for _ in range(2):
            state = apply_roll(
                state, TeamFace.INTERCEPTED
            )

        self.assertIs(state.phase, Phase.FINISHED)
        self.assertEqual(
            decode_state(encode_state(state)),
            state,
        )

    def test_duplicate_keys_not_accepted_as_fields(self):
        payload = encode_state(initial_state(1))
        payload["unexpected"] = 42

        with self.assertRaises(ValueError):
            decode_state(payload)

    def test_reject_malformed_snapshots(self):
        valid = encode_state(initial_state(2))

        modifications = [
            ("schemaVersion", 2),
            ("schemaVersion", True),
            ("handsPerTeam", 0),
            ("handsPerTeam", 1001),
            ("handsPerTeam", True),
            ("player", 2),
            ("player", False),
            ("hands", [0]),
            ("hands", [0, True]),
            ("hands", [0, 1]),
            ("goals", [1, 0]),
            ("goals", [0, 0, 0]),
            ("phase", "unknown"),
            ("phase", "finished"),
            ("phase", 1),
        ]

        for key, value in modifications:
            with self.subTest(key=key, value=value):
                payload = copy.deepcopy(valid)
                payload[key] = value

                with self.assertRaises(ValueError):
                    decode_state(payload)

    def test_reject_non_objects(self):
        for value in (None, [], "state", 123):
            with self.subTest(value=value), self.assertRaises(ValueError):
                decode_state(value)

    def test_no_presentation_fields(self):
        payload = encode_state(initial_state(1))

        self.assertNotIn("lastRoll", payload)
        self.assertNotIn("names", payload)
        self.assertNotIn("colors", payload)

    def test_json_roundtrip_is_stable(self):
        state = initial_state(2)

        state = apply_roll(state, TeamFace.SHOT)
        state = apply_roll(state, PinkFace.HEADER)
        state = apply_roll(state, ResultFace.PENALTY)

        first = encode_state(state)
        second = encode_state(
            decode_state(json.loads(json.dumps(first)))
        )

        self.assertEqual(first, second)


if __name__ == "__main__":
    unittest.main()
