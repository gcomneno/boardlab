import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";

const html = fs.readFileSync(new URL("./index.html", import.meta.url), "utf8");
const app = fs.readFileSync(new URL("./app.mjs", import.meta.url), "utf8");
const css = fs.readFileSync(new URL("./style.css", import.meta.url), "utf8");
const i18n = fs.readFileSync(new URL("./i18n.mjs", import.meta.url), "utf8");

test("obsolete LIVE statistics panel is absent", () => {
  assert.doesNotMatch(html, /id="player-stats(?:-disclosure|-body|-heading)?"/);
  assert.doesNotMatch(app, /playerStatsRoot|player-stats-heading/);
});

test("match statistics and career statistics remain available", () => {
  assert.match(html, /id="match-stats"/);
  assert.match(html, /id="manager-player-statistics-title"/);
  assert.match(app, /byId\("stats-team-0"\)/);
});

test("radio commentary uses readable system typography", () => {
  assert.match(css, /radio typography refresh v1/);
  assert.match(css, /font-family: system-ui/);
});

test("auxiliary disclosure titles retain bilingual translations", () => {
  for (const key of [
    "disclosureSaves", "disclosureFriendly", "disclosureTournament",
    "disclosureManager", "disclosureEvents", "disclosureMatchStats"
  ]) {
    assert.equal((i18n.match(new RegExp(`${key}:`, "g")) ?? []).length, 2);
  }
});
