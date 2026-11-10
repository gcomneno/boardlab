import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";
const MANAGER_KEY = "dado-goal.manager.v1";
const SHARED_KEY = "dado-goal.team-management.v1";

test("Manager persists an isolated catalogue-backed squad and launches normal play", async ({ page }) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);

  await expect.poll(async () =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(1);

  await expect.poll(async () =>
    page.locator("#team-1 option").count()
  ).toBeGreaterThan(2);

  const team0 = await page.locator("#team-0 option").nth(1).getAttribute("value");
  const team1 = await page.locator("#team-1 option").nth(2).getAttribute("value");

  expect(team0).toBeTruthy();
  expect(team1).toBeTruthy();
  expect(team0).not.toBe(team1);

  await page.locator("#team-0").selectOption(team0);
  await page.locator("#team-1").selectOption(team1);

  await expect(page.locator("#manager-enabled")).not.toBeChecked();
  await page.locator("#manager-enabled").check();

  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect(page.locator("#manager-club-name")).not.toHaveText("");

  await expect(page.locator("#manager-lineup li")).toHaveCount(11);

  const benchCount = await page.locator("#manager-bench li").count();
  expect(benchCount).toBeGreaterThanOrEqual(0);
  expect(benchCount).toBeLessThanOrEqual(9);

  const persisted = await page.evaluate(({ managerKey, sharedKey }) => ({
    manager: localStorage.getItem(managerKey),
    shared: localStorage.getItem(sharedKey),
  }), {
    managerKey: MANAGER_KEY,
    sharedKey: SHARED_KEY,
  });

  expect(persisted.manager).toBeTruthy();

  const managerDocument = JSON.parse(persisted.manager);
  expect(managerDocument.schemaVersion).toBe(1);
  expect(managerDocument.club).toBeTruthy();
  expect(managerDocument.club.lineup).toHaveLength(11);
  expect(managerDocument.club.bench.length).toBeLessThanOrEqual(9);
  expect(managerDocument.management.schemaVersion).toBe(1);

  if (persisted.shared) {
    const sharedDocument = JSON.parse(persisted.shared);
    expect(sharedDocument.club).toBeUndefined();
  }

  await page.locator("#hands").fill("1");
  await page.locator("#new").click();

  await expect(page.locator("#roll")).toBeEnabled();
  await page.locator("#roll").click();

  await expect.poll(async () =>
    page.locator("#turn").textContent()
  ).not.toBeNull();

  await page.reload();

  const restored = await page.evaluate(key =>
    JSON.parse(localStorage.getItem(key)),
  MANAGER_KEY);

  expect(restored.schemaVersion).toBe(1);
  expect(restored.club.providerTeamId).toBe(
    managerDocument.club.providerTeamId
  );
  expect(restored.club.lineup).toEqual(managerDocument.club.lineup);
  expect(restored.club.bench).toEqual(managerDocument.club.bench);

  await expect(page.locator("#manager-enabled")).not.toBeChecked();

  expect(runtimeErrors).toEqual([]);
});
