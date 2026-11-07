import assert from "node:assert/strict";
import test from "node:test";

import { socialColors } from "./team-colors.mjs";

const SERIE_A_2024_TEAM_IDS = [
  489, 497, 499, 500, 490, 895, 511, 502, 495, 504,
  505, 496, 487, 867, 1579, 492, 523, 503, 494, 517,
];

test("all persisted Serie A teams have one or two social colors", () => {
  for (const teamId of SERIE_A_2024_TEAM_IDS) {
    const colors = socialColors(teamId);

    assert.ok(colors.length === 1 || colors.length === 2);

    for (const color of colors) {
      assert.match(color, /^#[0-9A-F]{6}$/);
    }
  }
});

test("unknown teams cannot enter a catalogue without configured colors", () => {
  assert.throws(
    () => socialColors("unknown"),
    /social colors missing/,
  );
});
