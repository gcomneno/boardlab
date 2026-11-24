import { test, expect } from "@playwright/test";

const URL = "http://127.0.0.1:4173/web/dado-goal/";
const KEY = "dado-goal.manager.v1";

async function managerDocument(page) {
  return page.evaluate(key => {
    const raw = localStorage.getItem(key);
    return raw === null ? null : JSON.parse(raw);
  }, KEY);
}

async function createCareer(page) {
  await page.goto(URL);
  await expect.poll(() =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(1);

  const team = await page.locator("#team-0 option")
    .nth(1).getAttribute("value");

  await page.locator("#team-0").selectOption(team);
  await page.locator("#manager-new-0").click();

  await expect.poll(async () => {
    const document = await managerDocument(page);
    return document?.club?.providerTeamId ?? null;
  }).not.toBeNull();
}

async function openRosterEditor(page) {
  await page.locator("#manager-edit").click();
  const dialog = page.locator("dialog.manager-editor");
  await expect(dialog).toBeVisible();
  const input = dialog.locator("[data-player-name]").first();
  await expect(input).toBeVisible();
  return { dialog, input };
}

test("Manager roster edit persists after reload", async ({ page }) => {
  const errors = [];
  page.on("pageerror", error => errors.push(error.message));

  await createCareer(page);
  const before = await managerDocument(page);

  const { dialog, input } = await openRosterEditor(page);
  const playerId = await input.getAttribute("data-player-name");
  const originalName = await input.inputValue();
  const changedName = `${originalName} SP02`;

  await input.fill(changedName);
  await dialog.getByRole("button", {
    name: "Salva rosa e formazione",
  }).click();

  await expect(dialog).toHaveCount(0);

  await expect.poll(async () => {
    const document = await managerDocument(page);
    return document.club.players.find(
      player => String(player.providerPlayerId) === playerId,
    )?.name;
  }).toBe(changedName);

  await page.reload();

  const restored = await managerDocument(page);
  expect(restored.club.players.find(
    player => String(player.providerPlayerId) === playerId,
  )?.name).toBe(changedName);
  expect(restored.club.providerTeamId).toBe(
    before.club.providerTeamId,
  );
  expect(errors).toEqual([]);
});

test("stale Manager roster editor cannot overwrite another tab", async ({
  page,
  context,
}) => {
  const errors = [];
  page.on("pageerror", error => errors.push(error.message));

  await createCareer(page);

  const stalePage = await context.newPage();
  stalePage.on("pageerror", error => errors.push(error.message));
  await stalePage.goto(URL);

  await expect.poll(async () => {
    const document = await managerDocument(stalePage);
    return document?.club?.providerTeamId ?? null;
  }).not.toBeNull();

  const disclosure = stalePage.locator("#manager-disclosure");
  if (!(await disclosure.evaluate(element => element.open))) {
    await disclosure.locator(":scope > summary").click();
  }
  await expect(stalePage.locator("#manager-dashboard")).toBeVisible();

  const staleEditor = await openRosterEditor(stalePage);
  const freshEditor = await openRosterEditor(page);

  const playerId = await freshEditor.input.getAttribute("data-player-name");
  const originalName = await freshEditor.input.inputValue();
  const freshName = `${originalName} FRESH-SP02`;

  await freshEditor.input.fill(freshName);
  await freshEditor.dialog.getByRole("button", {
    name: "Salva rosa e formazione",
  }).click();

  await expect(freshEditor.dialog).toHaveCount(0);

  await expect.poll(async () => {
    const document = await managerDocument(page);
    return document.club.players.find(
      player => String(player.providerPlayerId) === playerId,
    )?.name;
  }).toBe(freshName);

  await staleEditor.input.fill(`${originalName} STALE-SP02`);
  await staleEditor.dialog.getByRole("button", {
    name: "Salva rosa e formazione",
  }).click();

  await expect(staleEditor.dialog).toBeVisible();
  await expect(
    staleEditor.dialog.locator('[role="alert"]'),
  ).not.toBeEmpty();

  const persisted = await managerDocument(stalePage);
  expect(persisted.club.players.find(
    player => String(player.providerPlayerId) === playerId,
  )?.name).toBe(freshName);

  await stalePage.reload();

  const restored = await managerDocument(stalePage);
  expect(restored.club.players.find(
    player => String(player.providerPlayerId) === playerId,
  )?.name).toBe(freshName);

  expect(errors).toEqual([]);
});
