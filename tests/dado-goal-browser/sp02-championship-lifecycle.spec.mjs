import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";
const TOURNAMENT_KEY = "dado-goal.tournament.v1";

async function prepareSelection(page) {
  await page.locator("#tournament-disclosure > summary")
    .evaluate(element => {
      element.parentElement.open = true;
    });

  const participants = page.locator("#tournament-participants");

  await expect.poll(() => participants.locator("option").count())
    .toBeGreaterThan(3);

  const values = await participants.locator("option").evaluateAll(
    options => options
      .filter(option => option.value && option.textContent?.trim())
      .slice(0, 4)
      .map(option => option.value),
  );

  expect(values).toHaveLength(4);
  await participants.selectOption(values);
}

async function createChampionship(page) {
  await prepareSelection(page);
  await page.locator("#tournament-create")
    .evaluate(button => button.click());
}

async function persistedChampionship(page) {
  return page.evaluate(key => {
    const raw = localStorage.getItem(key);
    return raw === null ? null : JSON.parse(raw);
  }, TOURNAMENT_KEY);
}

async function abandonChampionship(page) {
  page.once("dialog", dialog => dialog.accept());
  await page.locator("#tournament-close")
    .evaluate(button => button.click());
}

test("7F stale tab cannot replace an existing championship", async ({
  page, context,
}) => {
  await page.goto(GAME_URL);

  const stale = await context.newPage();
  try {
    await stale.goto(GAME_URL);
    await prepareSelection(stale);

    await createChampionship(page);

    await expect.poll(() => persistedChampionship(page))
      .not.toBeNull();

    const canonical = await persistedChampionship(page);

    // This page still believes no competition exists.
    await stale.locator("#tournament-create")
      .evaluate(button => button.click());

    await expect.poll(() => persistedChampionship(stale))
      .toEqual(canonical);

    expect(await persistedChampionship(page)).toEqual(canonical);
    await expect(stale.locator("#tournament-error"))
      .not.toHaveText("");
  } finally {
    await stale.close();
  }
});

test("7F stale abandonment cannot erase a replacement championship", async ({
  page, context,
}) => {
  await page.goto(GAME_URL);
  await createChampionship(page);

  const original = await persistedChampionship(page);
  expect(original).not.toBeNull();

  const stale = await context.newPage();
  try {
    await stale.goto(GAME_URL);
    await expect(stale.locator("#tournament-dashboard"))
      .toHaveJSProperty("hidden", false);

    await abandonChampionship(page);
    await expect.poll(() => persistedChampionship(page))
      .toBeNull();

    await createChampionship(page);

    await expect.poll(() => persistedChampionship(page))
      .not.toBeNull();

    const replacement = await persistedChampionship(page);
    expect(replacement.championshipId)
      .not.toBe(original.championshipId);

    // The stale tab still holds the original championship in memory.
    await abandonChampionship(stale);

    await expect.poll(() => persistedChampionship(page))
      .toEqual(replacement);

    await expect(stale.locator("#tournament-error"))
      .not.toHaveText("");
  } finally {
    await stale.close();
  }
});
