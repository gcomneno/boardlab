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

test("all 2024 second-division teams have configured social colors", () => {
  const ids = [
    158, 159, 166, 171, 174, 175, 177, 178, 179,
    181, 185, 192, 744, 745, 785, 1313, 1652, 1660,
    87, 88, 90, 97, 98, 99, 101, 104, 110,
    112, 114, 433, 1297, 1301, 1304, 1305, 3012, 3200,
    535, 539, 544, 545, 713, 715, 718, 719, 722, 723, 724,
    726, 731, 732, 797, 799, 4665, 5254, 5262, 9409, 9580, 9692,
  ];

  assert.equal(ids.length, 58);
  assert.equal(new Set(ids).size, 58);

  for (const id of ids) {
    const colors = socialColors(id);
    assert.ok(colors.length === 1 || colors.length === 2);
    for (const color of colors) {
      assert.match(color, /^#[0-9A-F]{6}$/);
    }
  }
});
