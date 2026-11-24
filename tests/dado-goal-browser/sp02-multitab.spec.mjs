import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";
const MANAGEMENT_KEY = "dado-goal.team-management.v1";

async function prepareMatch(page) {
  await page.goto(GAME_URL);

  await page.locator("#match-disclosure > summary").click();

  await expect.poll(
    () => page.locator("#team-0 option").count(),
  ).toBeGreaterThan(2);

  await expect.poll(
    () => page.locator("#team-1 option").count(),
  ).toBeGreaterThan(2);

  const home = await page.locator("#team-0 option")
    .nth(1).getAttribute("value");
  const away = await page.locator("#team-1 option")
    .nth(2).getAttribute("value");

  expect(home).toBeTruthy();
  expect(away).toBeTruthy();
  expect(home).not.toBe(away);

  await page.locator("#team-0").selectOption(home);
  await page.locator("#team-1").selectOption(away);

  await page.locator(".configuration-panel > details > summary").click();
  await page.locator("#hands").fill("1");
  await page.locator("#new").click();

  await expect(page.locator("#roll")).toBeEnabled();
}

async function finishMatch(page) {
  const roll = page.locator("#roll");
  const phase = page.locator("#phase");

  for (let attempt = 0; attempt < 400; attempt += 1) {
    await expect.poll(async () => {
      const phaseText = (await phase.textContent())?.trim();
      const rollText = (await roll.textContent())?.trim();
      const enabled = await roll.isEnabled();

      if (!enabled && phaseText === rollText) {
        return "finished";
      }

      return enabled ? "ready" : "rolling";
    }, { timeout: 10000 }).toMatch(/^(ready|finished)$/);

    const phaseText = (await phase.textContent())?.trim();
    const rollText = (await roll.textContent())?.trim();

    if (!await roll.isEnabled() && phaseText === rollText) {
      return;
    }

    await roll.click();
  }

  throw new Error("Match did not finish within 400 dice actions");
}

async function canonicalManagement(page) {
  return page.evaluate(key => {
    const raw = localStorage.getItem(key);
    return {
      raw,
      document: raw === null ? null : JSON.parse(raw),
    };
  }, MANAGEMENT_KEY);
}

test("SP-02 standalone stale tab cannot overwrite canonical match history", async ({
  context,
  page,
}) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  const stalePage = await context.newPage();
  stalePage.on("pageerror", error => runtimeErrors.push(error.message));

  // Simulate a missed cross-tab storage notification. The coordinator
  // must reject obsolete memory even without the UI refresh event.
  await stalePage.addInitScript(() => {
    window.addEventListener("storage", event => {
      event.stopImmediatePropagation();
    }, true);
  });

  // Both pages initialize from the same empty persistent history.
  await prepareMatch(page);
  await prepareMatch(stalePage);

  const initial = await canonicalManagement(page);
  const initialCount = initial.document?.matches?.length ?? 0;
  expect(initialCount).toBe(0);

  // First tab completes a match and persists the canonical record.
  await finishMatch(page);

  await expect.poll(async () => {
    const current = await canonicalManagement(page);
    return current.document?.matches?.length ?? 0;
  }).toBe(1);

  const committed = await canonicalManagement(page);
  expect(committed.document.matches[0].matchId).toBeTruthy();

  // Second tab still has its original in-memory management document.
  // Finishing its match must not replace the first tab's history.
  await finishMatch(stalePage);

  const after = await canonicalManagement(page);

  expect(after.raw).toBe(committed.raw);
  expect(after.document.matches).toHaveLength(1);
  expect(after.document.matches[0].matchId)
    .toBe(committed.document.matches[0].matchId);

  expect(runtimeErrors).toEqual([]);

  await stalePage.close();
});

test("SP-02 synchronized tabs preserve both canonical matches", async ({
  context,
  page,
}) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  const secondPage = await context.newPage();
  secondPage.on("pageerror", error => runtimeErrors.push(error.message));

  await prepareMatch(page);
  await prepareMatch(secondPage);

  await finishMatch(page);

  await expect.poll(async () => {
    const state = await canonicalManagement(page);
    return state.document?.matches?.length ?? 0;
  }).toBe(1);

  const first = await canonicalManagement(page);
  const firstRecord = first.document.matches[0];

  // The ordinary storage event must refresh the second tab.
  await finishMatch(secondPage);

  await expect.poll(async () => {
    const state = await canonicalManagement(page);
    return state.document?.matches?.length ?? 0;
  }).toBe(2);

  const final = await canonicalManagement(page);
  const records = final.document.matches;

  expect(records).toHaveLength(2);
  expect(records[0]).toEqual(firstRecord);
  expect(new Set(records.map(record => record.matchId)).size).toBe(2);
  expect(runtimeErrors).toEqual([]);

  await secondPage.close();
});
