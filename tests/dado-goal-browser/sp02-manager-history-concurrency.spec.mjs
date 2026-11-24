import { expect, test } from "@playwright/test";

const URL = "http://127.0.0.1:4173/web/dado-goal/";
const SHARED = "dado-goal.team-management.v1";
const MANAGER = "dado-goal.manager.v1";

async function documents(page) {
  return page.evaluate(({ sharedKey, managerKey }) => ({
    shared: JSON.parse(localStorage.getItem(sharedKey)),
    manager: JSON.parse(localStorage.getItem(managerKey)),
  }), { sharedKey: SHARED, managerKey: MANAGER });
}

async function startOrdinaryMatch(page, home, away) {
  await page.locator("#team-0").selectOption(home);
  await page.locator("#team-1").selectOption(away);

  await page.locator(".configuration-panel > details")
    .evaluate(details => { details.open = true; });

  await page.locator("#hands").fill("1");
  await page.locator("#new").click();
  await expect(page.locator("#roll")).toBeEnabled();
}

async function finishMatch(page) {
  const finished = async () =>
    !(await page.locator("#roll").isEnabled()) &&
    await page.locator("#hands").isEnabled();

  for (let attempt = 0; attempt < 120; attempt += 1) {
    if (await finished()) return;

    await expect(page.locator("#roll")).toBeEnabled();
    await page.locator("#roll").click();

    // Wait for the animated roll and its awaited persistence to finish.
    await expect.poll(async () =>
      (await finished()) ||
      await page.locator("#roll").isEnabled()
    ).toBe(true);

    if (await finished()) return;
  }

  throw new Error("MATCH_NOT_FINISHED_AFTER_120_ROLLS");
}

test("7G stale Manager tab cannot overwrite canonical match history", async ({
  page, context,
}) => {
  await page.goto(URL);

  await expect.poll(() => page.locator("#team-0 option").count())
    .toBeGreaterThan(2);

  const home = await page.locator("#team-0 option")
    .nth(1).getAttribute("value");
  const away = await page.locator("#team-0 option")
    .nth(2).getAttribute("value");

  expect(home).toBeTruthy();
  expect(away).toBeTruthy();
  expect(home).not.toBe(away);

  await page.locator("#team-0").selectOption(home);
  await page.locator("#manager-new-0").click();
  await expect(page.locator("#manager-dashboard")).toBeVisible();

  const stale = await context.newPage();

  try {
    // Keep this tab's in-memory history obsolete on purpose.
    // The coordinator must still compare against durable current data.
    await stale.addInitScript(() => {
      window.addEventListener(
        "storage",
        event => event.stopImmediatePropagation(),
        { capture: true },
      );
    });

    await stale.goto(URL);
    await expect.poll(async () =>
      (await documents(stale)).manager?.club?.providerTeamId
    ).toBe(String(home));

    await startOrdinaryMatch(page, home, away);
    await startOrdinaryMatch(stale, home, away);

    await finishMatch(page);

    await expect.poll(async () =>
      (await documents(page)).shared?.matches?.length ?? 0
    ).toBe(1);

    const canonical = await documents(page);
    expect(canonical.manager.management.matches).toHaveLength(1);
    expect(canonical.manager.management.matches[0].matchId)
      .toBe(canonical.shared.matches[0].matchId);

    await finishMatch(stale);

    const afterStaleCompletion = await documents(stale);
    expect(afterStaleCompletion.shared).toEqual(canonical.shared);
    expect(afterStaleCompletion.manager).toEqual(canonical.manager);

    await page.reload();

    const afterReload = await documents(page);
    expect(afterReload.shared).toEqual(canonical.shared);
    expect(afterReload.manager).toEqual(canonical.manager);
  } finally {
    await stale.close();
  }
});
