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

const i18n = fs.readFileSync(
  new URL("./i18n.mjs", import.meta.url),
  "utf8",
);

test("LIVE player statistics live in an independent collapsible panel", () => {
  assert.match(
    html,
    /id="player-stats-toggle"[\s\S]*aria-expanded="false"/,
  );
  assert.match(
    html,
    /id="player-stats-body" hidden/,
  );
  assert.match(
    html,
    /id="player-stats"/,
  );
});

test("LIVE statistics toggle only controls presentation", () => {
  assert.match(
    app,
    /byId\("player-stats-toggle"\)\.addEventListener/,
  );
  assert.match(
    app,
    /body\.hidden = !body\.hidden;/,
  );
  assert.match(
    app,
    /const playerTeams = playerStatistics\(matchEvents\);/,
  );
});

test("LIVE statistics panel has bilingual labels", () => {
  assert.match(i18n, /playerStatistics: "LIVE statistics"/);
  assert.match(i18n, /playerStatisticsShow: "Show"/);
  assert.match(i18n, /playerStatisticsHide: "Hide"/);
  assert.match(i18n, /playerStatistics: "Statistiche LIVE"/);
  assert.match(i18n, /playerStatisticsShow: "Mostra"/);
  assert.match(i18n, /playerStatisticsHide: "Nascondi"/);
});
