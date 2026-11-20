import test from "node:test";
import assert from "node:assert/strict";
import { setLocale } from "./i18n.mjs";
import { localizedError } from "./error-i18n.mjs";

test("known TypeError follows the active locale", () => {
  const err = new TypeError("manager lineup must contain exactly 11 players");
  setLocale("it");
  assert.equal(localizedError(err, "manager"),
    "La formazione deve contenere esattamente 11 titolari.");
  setLocale("en");
  assert.equal(localizedError(err, "manager"),
    "The lineup must contain exactly 11 starters.");
});

test("unknown TypeError never leaks the raw diagnostic", () => {
  const err = new TypeError("sensitive internal diagnostic");
  for (const lang of ["it", "en"]) {
    setLocale(lang);
    const text = localizedError(err, "save");
    assert.ok(text.length > 0);
    assert.ok(!text.includes("sensitive"));
  }
});

test("server error codes are localized without changing codes", () => {
  const err = Object.freeze({ code: "MATCH_EXPIRED", message: "MATCH_EXPIRED" });
  setLocale("it");
  assert.equal(localizedError(err, "multiplayer"),
    "La partita multiplayer è scaduta.");
  setLocale("en");
  assert.equal(localizedError(err, "multiplayer"),
    "The multiplayer match has expired.");
  assert.equal(err.code, "MATCH_EXPIRED");
});

test("unknown multiplayer error codes use localized safe fallback", () => {
  for (const lang of ["it", "en"]) {
    setLocale(lang);
    assert.ok(!localizedError({ code: "PRIVATE_BACKEND_ERROR" }, "multiplayer")
      .includes("PRIVATE_BACKEND_ERROR"));
  }
});
