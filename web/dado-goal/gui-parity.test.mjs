import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const root = new URL("../../", import.meta.url);

const singleUrl = new URL("web/dado-goal/index.html", root);
const multiplayerUrl = new URL(
  "cloudflare/dado-goal/public/multiplayer.html",
  root,
);
const singleI18nUrl = new URL("web/dado-goal/i18n.mjs", root);
const multiplayerI18nUrl = new URL(
  "cloudflare/dado-goal/public/i18n.mjs",
  root,
);

const commonIds = [
  "panel-0",
  "panel-1",
  "match-events-list",
  "match-events-empty",
  "match-clock",
  "turn",
  "pitch-logo-0",
  "die",
  "pitch-logo-1",
  "phase",
  "result",
  "roll",
  "auto",
  "commentary-toggle",
  "commentary-text",
  "match-stats",
  "message",
];

const commonClasses = [
  "scoreboard",
  "match-events",
  "pitch",
  "pitch-branding",
  "dice-guide",
  "roll-actions",
  "match-commentary",
  "match-stats",
];

async function html(url) {
  return readFile(url, "utf8");
}

function hasId(source, id) {
  return source.includes(`id="${id}"`) || source.includes(`id='${id}'`);
}

function hasClass(source, className) {
  const attributes = [...source.matchAll(/\bclass=["']([^"']*)["']/g)];

  return attributes.some((match) =>
    match[1].split(/\s+/).includes(className),
  );
}

test("single and multiplayer expose the canonical common match surface", async () => {
  const [single, multiplayer] = await Promise.all([
    html(singleUrl),
    html(multiplayerUrl),
  ]);

  for (const id of commonIds) {
    assert.ok(hasId(single, id), `single-player must expose #${id}`);
    assert.ok(hasId(multiplayer, id), `multiplayer must expose #${id}`);
  }

  for (const className of commonClasses) {
    assert.ok(
      hasClass(single, className),
      `single-player must expose .${className}`,
    );
    assert.ok(
      hasClass(multiplayer, className),
      `multiplayer must expose .${className}`,
    );
  }
});

test("both pitches keep team crests around the central die", async () => {
  for (const [surface, source] of [
    ["single-player", await html(singleUrl)],
    ["multiplayer", await html(multiplayerUrl)],
  ]) {
    const branding = source.indexOf('class="pitch-branding"');
    const left = source.indexOf('id="pitch-logo-0"', branding);
    const die = source.indexOf('id="die"', branding);
    const right = source.indexOf('id="pitch-logo-1"', branding);
    const guide = source.indexOf('class="dice-guide"', branding);

    assert.ok(branding >= 0, `${surface} must expose pitch branding`);
    assert.ok(
      branding < left &&
        left < die &&
        die < right &&
        right < guide,
      `${surface} must order crest 0, die, crest 1 before dice guide`,
    );
  }
});

test("both surfaces depend on the canonical shared stylesheet", async () => {
  const [single, multiplayer] = await Promise.all([
    html(singleUrl),
    html(multiplayerUrl),
  ]);

  assert.match(single, /href=["']\.\/style\.css["']/);
  assert.match(multiplayer, /href=["']\.\/style\.css["']/);
});

test("deployed multiplayer localization matches canonical localization", async () => {
  const [canonical, deployed] = await Promise.all([
    readFile(singleI18nUrl),
    readFile(multiplayerI18nUrl),
  ]);

  assert.deepEqual(deployed, canonical);
});

test("single AUTO configuration remains mode-specific", async () => {
  const [single, multiplayer] = await Promise.all([
    html(singleUrl),
    html(multiplayerUrl),
  ]);

  assert.ok(
    hasId(single, "auto-config"),
    "single-player must expose #auto-config",
  );
  assert.ok(
    !hasId(multiplayer, "auto-config"),
    "multiplayer must not inherit configurable single-player AUTO",
  );
});
