import test from "node:test";
import assert from "node:assert/strict";

import { describeExtraordinaryEvent } from "./extraordinary-commentary.mjs";

const TYPES = [
  "heavy-rain",
  "strong-wind",
  "snow-poor-visibility",
  "pitch-invasion",
  "object-on-pitch",
  "animal-on-pitch",
  "floodlight-failure",
  "damaged-goal-net",
  "referee-equipment-problem",
  "var-review",
];

test("all narrative v1 events have IT and EN commentary", () => {
  for (const locale of ["it", "en"]) {
    for (const type of TYPES) {
      const text = describeExtraordinaryEvent({ type, minute: 42 }, locale);
      assert.equal(typeof text, "string");
      assert.ok(text.startsWith("42′"));
      assert.ok(text.length > 20);
    }
  }
});

test("light knock commentary identifies the affected player", () => {
  const it = describeExtraordinaryEvent(
    { type: "light-knock", minute: 17, number: 8 },
    "it",
  );
  const en = describeExtraordinaryEvent(
    { type: "light-knock", minute: 17, number: 8 },
    "en",
  );

  assert.match(it, /numero 8/);
  assert.match(en, /number 8/);
});

test("unsupported locale falls back to Italian", () => {
  const text = describeExtraordinaryEvent(
    { type: "heavy-rain", minute: 30 },
    "xx",
  );

  assert.match(text, /Pioggia/);
});

test("unknown and malformed events are rejected", () => {
  assert.throws(
    () => describeExtraordinaryEvent({ type: "unknown", minute: 20 }),
    RangeError,
  );
  assert.throws(
    () => describeExtraordinaryEvent(
      { type: "light-knock", minute: 20 },
      "it",
    ),
    RangeError,
  );
});
