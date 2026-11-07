import test from "node:test";
import assert from "node:assert/strict";
import {
  supportedLocales, messages, getLocale,
  setLocale, translate, formatMessage
} from "./i18n.mjs";

test("EN/IT keys and placeholders have parity", () => {
  assert.deepEqual(supportedLocales, ["en", "it"]);

  const en = Object.keys(messages.en).sort();
  const it = Object.keys(messages.it).sort();
  assert.deepEqual(en, it);

  const placeholders = text =>
    [...text.matchAll(/\{([a-z][a-zA-Z]*)\}/g)]
      .map(match => match[1]).sort();

  for (const key of en) {
    assert.ok(messages.en[key]);
    assert.ok(messages.it[key]);
    assert.deepEqual(
      placeholders(messages.en[key]),
      placeholders(messages.it[key]),
      `Placeholder mismatch: ${key}`
    );
  }
});

test("locale selection and deterministic fallback", () => {
  setLocale("it");
  assert.equal(getLocale(), "it");
  assert.equal(translate("newGame"), "Nuova partita");

  setLocale("en");
  assert.equal(translate("newGame"), "New match");

  setLocale("unsupported");
  assert.equal(getLocale(), "en");
  assert.equal(translate("newGame"), "New match");
  assert.equal(translate("missingKey"), "missingKey");
});

test("dynamic messages preserve interpolation", () => {
  setLocale("it");
  assert.equal(
    formatMessage("matchTurn", { name: "Gialli" }),
    "Tocca a Gialli"
  );

  setLocale("en");
  assert.equal(
    formatMessage("matchTurn", { name: "Gialli" }),
    "Gialli's turn"
  );
});

test("localization does not change game state", async () => {
  const { newMatch } = await import("./engine.mjs");
  const state = newMatch(2);
  const snapshot = structuredClone(state);

  setLocale("it");
  formatMessage("handsProgress", { done: 1, total: 2 });
  setLocale("en");
  formatMessage("handsProgress", { done: 1, total: 2 });

  assert.deepEqual(state, snapshot);
});
