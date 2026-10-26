import assert from "node:assert/strict";
import {
  messages, supportedLocales, getLocale, setLocale,
  translate, formatMessage, explainAnalysis
} from "./i18n.mjs";

assert.deepEqual(supportedLocales, ["en", "it"]);
assert.deepEqual(Object.keys(messages.en).sort(), Object.keys(messages.it).sort());

for (const locale of supportedLocales) {
  setLocale(locale);
  for (const key of Object.keys(messages.en)) {
    assert.notEqual(translate(key), key, `Missing ${locale} translation: ${key}`);
  }
  assert.match(formatMessage("gameTurn", {
    turn: 3, player: translate("ivory")
  }), /3/);
}

setLocale("it");
assert.equal(getLocale(), "it");
assert.equal(translate("newGame"), "Nuova partita");

setLocale("en");
assert.equal(getLocale(), "en");
assert.equal(translate("newGame"), "New game");

setLocale("unsupported");
assert.equal(getLocale(), "en");

const analysis = {
  move: { type: "place", cell: 2 },
  scoreAfter: { north: 4, south: 3 },
  scoreDelta: { north: 1, south: -1 },
  changedCells: []
};

setLocale("en");
assert.match(explainAnalysis(analysis), /Place a seal/);
setLocale("it");
assert.match(explainAnalysis(analysis), /Colloca un sigillo/);

console.log("I18N_KEY_PARITY_GATE=PASS");
console.log("I18N_LOCALE_AND_FALLBACK_GATE=PASS");
console.log("I18N_DYNAMIC_MESSAGES_GATE=PASS");
console.log("I18N_TUTOR_TRANSLATION_GATE=PASS");
