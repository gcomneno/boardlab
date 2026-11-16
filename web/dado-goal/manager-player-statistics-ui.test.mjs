import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";

const html = fs.readFileSync(
  new URL("./index.html", import.meta.url),
  "utf8",
);

const app = fs.readFileSync(
  new URL("./app.mjs", import.meta.url),
  "utf8",
);

test("Manager dashboard exposes cumulative player statistics", () => {
  assert.match(html, /id="manager-player-statistics"/);
  assert.match(html, /id="manager-player-stat-injury"/);
  assert.match(
    app,
    /managerPlayerStatistics\(\s*matches,\s*club\.providerTeamId,\s*club\.players/,
  );
});

test("Manager cumulative table is rebuilt from persisted Manager matches", () => {
  assert.match(app, /const matches = managerClubMatches\(\);/);
  assert.match(
    app,
    /managerPlayerStatsBody\.replaceChildren\(\);/,
  );
});
