import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";
const STABLE_MULTIPLAYER =
  "https://dado-goal-multiplayer.giancarlo-cicellyn.workers.dev/multiplayer.html";

test("single-player release-critical browser smoke", async ({ page }) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);

  await expect(page.locator("#die")).toBeVisible();
  await expect(page.locator("#pitch-logo-0")).toBeAttached();
  await expect(page.locator("#pitch-logo-1")).toBeAttached();
  await expect(page.locator("#stats-team-0")).toBeAttached();
  await expect(page.locator("#stats-team-1")).toBeAttached();
  await expect(page.locator("#match-events-list")).toBeAttached();

  await expect(page.locator("a.multiplayer-button")).toHaveAttribute(
    "href",
    STABLE_MULTIPLAYER
  );

  await expect.poll(async () =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(1);

  await expect.poll(async () =>
    page.locator("#team-1 option").count()
  ).toBeGreaterThan(1);

  const team0 = await page.locator("#team-0 option").nth(1).getAttribute("value");
  const team1 = await page.locator("#team-1 option").nth(2).getAttribute("value");

  expect(team0).toBeTruthy();
  expect(team1).toBeTruthy();
  expect(team0).not.toBe(team1);

  await page.locator("#team-0").selectOption(team0);
  await page.locator("#team-1").selectOption(team1);
  await page.locator("#hands").fill("1");

  await page.locator("#new").click();

  await expect(page.locator("#roll")).toBeEnabled();
  await expect(page.locator("#die")).toHaveAttribute(
    "aria-label",
    "In attesa del primo lancio"
  );

  await expect(page.locator("#pitch-logo-0")).toHaveAttribute("src", /.+/);
  await expect(page.locator("#pitch-logo-1")).toHaveAttribute("src", /.+/);

  const initialDieLabel = await page.locator("#die").getAttribute(
    "aria-label"
  );

  await page.locator("#roll").click();

  await expect.poll(async () =>
    page.locator("#die").getAttribute("aria-label")
  ).not.toBe(initialDieLabel);

  await expect(page.locator("#stats-team-0")).not.toHaveText("");
  await expect(page.locator("#stats-team-1")).not.toHaveText("");

  expect(runtimeErrors).toEqual([]);
});

test("single-player rejects catalogue team versus local placeholder", async ({ page }) => {
  await page.goto(GAME_URL);

  await expect.poll(async () =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(1);

  const realTeam = await page.locator("#team-0 option")
    .nth(1)
    .getAttribute("value");

  expect(realTeam).toBeTruthy();

  await page.locator("#team-0").selectOption(realTeam);
  await page.locator("#team-1").selectOption("");
  await page.locator("#new").click();

  await expect(page.locator("#roll")).toBeDisabled();

  const message = await page.locator("#message").textContent();
  expect(message).toContain(
    "scegli due squadre del catalogo oppure due squadre locali"
  );
});
