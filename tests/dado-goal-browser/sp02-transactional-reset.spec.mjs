import { expect, test } from "@playwright/test";

const URL = "http://127.0.0.1:4173/web/dado-goal/";

const SINGLE_PLAYER_KEYS = [
  "dado-goal.team-management.v1",
  "dado-goal.manager.v1",
  "dado-goal.tournament.v1",
  "dado-goal.tournament-recovery.v1",
  "dado-goal.cup.v1",
  "dado-goal.cup-archive.v1",
  "dado-goal.cup-recovery.v1",
];

const MULTIPLAYER_KEY = "dado-goal.multiplayer-seat.v1";
const FOREIGN_KEY = "sp02-reset-foreign-sentinel";

test("7H reset removes single-player data and preserves multiplayer", async ({ page }) => {
  await page.goto(URL);

  await expect.poll(() => page.locator("#team-0 option").count())
    .toBeGreaterThan(2);

  const managerTeam = await page.locator("#team-0 option")
    .nth(1).getAttribute("value");
  expect(managerTeam).toBeTruthy();

  await page.locator("#team-0").selectOption(managerTeam);
  await page.locator("#manager-new-0").click();

  await expect.poll(() =>
    page.evaluate(() => localStorage.getItem("dado-goal.manager.v1"))
  ).not.toBeNull();

  await page.locator("#tournament-disclosure").evaluate(details => {
    details.open = true;
  });

  const participants = page.locator("#tournament-participants");
  await expect.poll(() => participants.locator("option").count())
    .toBeGreaterThan(3);

  const teams = await participants.locator("option").evaluateAll(options =>
    options.filter(option => option.value && option.textContent?.trim())
      .slice(0, 4)
      .map(option => option.value)
  );

  expect(teams).toHaveLength(4);
  await participants.selectOption(teams);
  await page.locator("#tournament-create")
    .evaluate(button => button.click());

  await expect.poll(() =>
    page.evaluate(() => localStorage.getItem("dado-goal.tournament.v1"))
  ).not.toBeNull();

  const sentinels = {
    multiplayer: '{"seat":2,"preserve":"SP02-7H"}',
    foreign: "other application must survive",
  };

  await page.evaluate(({ multiplayer, foreign }) => {
    localStorage.setItem("dado-goal.multiplayer-seat.v1", multiplayer);
    localStorage.setItem("sp02-reset-foreign-sentinel", foreign);
  }, sentinels);

  page.once("dialog", dialog => dialog.accept());

  await Promise.all([
    page.waitForEvent("load"),
    page.locator("#reset-local-data")
      .evaluate(button => button.click()),
  ]);

  const after = await page.evaluate(({ keys, multiplayer, foreign }) => ({
    documents: Object.fromEntries(
      keys.map(key => [key, localStorage.getItem(key)])
    ),
    multiplayer: localStorage.getItem(multiplayer),
    foreign: localStorage.getItem(foreign),
    journal: localStorage.getItem("dado-goal.global-save-transaction.v1"),
    generation: localStorage.getItem("dado-goal.global-save-generation.v1"),
  }), {
    keys: SINGLE_PLAYER_KEYS,
    multiplayer: MULTIPLAYER_KEY,
    foreign: FOREIGN_KEY,
  });

  expect(Object.values(after.documents).every(value => value === null))
    .toBe(true);
  expect(after.multiplayer).toBe(sentinels.multiplayer);
  expect(after.foreign).toBe(sentinels.foreign);
  expect(after.journal).toBeNull();
  expect(Number(after.generation)).toBeGreaterThan(0);
});

test("7H reset recovers after committed journal cleanup failure", async ({ page }) => {
  await page.goto(URL);

  await expect.poll(() => page.locator("#team-0 option").count())
    .toBeGreaterThan(1);

  const managedId = await page.locator("#team-0 option")
    .nth(1).getAttribute("value");

  await page.locator("#team-0").selectOption(managedId);
  await page.locator("#manager-new-0").click();

  await expect.poll(() =>
    page.evaluate(() => localStorage.getItem("dado-goal.manager.v1"))
  ).not.toBeNull();

  const multiplayerValue = '{"seat":1,"recovery":"preserved"}';

  await page.evaluate(value => {
    localStorage.setItem("dado-goal.multiplayer-seat.v1", value);

    const originalRemove = Storage.prototype.removeItem;
    const journalKey = "dado-goal.global-save-transaction.v1";

    Storage.prototype.removeItem = function(key) {
      const journal = this === localStorage && key === journalKey
        ? this.getItem(journalKey)
        : null;

      if (
        journal?.includes('"phase":"committed"') &&
        sessionStorage.getItem("sp02-reset-cleanup-fault") !== "injected"
      ) {
        sessionStorage.setItem("sp02-reset-cleanup-fault", "injected");
        throw new DOMException(
          "Simulated committed journal cleanup failure",
          "QuotaExceededError",
        );
      }

      return originalRemove.call(this, key);
    };
  }, multiplayerValue);

  page.once("dialog", dialog => dialog.accept());

  await Promise.all([
    page.waitForEvent("load"),
    page.locator("#reset-local-data")
      .evaluate(button => button.click()),
  ]);

  const recovered = await page.evaluate(({ keys, multiplayer }) => ({
    images: keys.map(key => localStorage.getItem(key)),
    multiplayer: localStorage.getItem(multiplayer),
    journal: localStorage.getItem("dado-goal.global-save-transaction.v1"),
    generation: localStorage.getItem("dado-goal.global-save-generation.v1"),
    faultInjected: sessionStorage.getItem("sp02-reset-cleanup-fault"),
  }), {
    keys: SINGLE_PLAYER_KEYS,
    multiplayer: MULTIPLAYER_KEY,
  });

  expect(recovered.faultInjected).toBe("injected");
  expect(recovered.images).toEqual(SINGLE_PLAYER_KEYS.map(() => null));
  expect(recovered.multiplayer).toBe(multiplayerValue);
  expect(recovered.journal).toBeNull();
  expect(Number(recovered.generation)).toBeGreaterThan(0);
});
