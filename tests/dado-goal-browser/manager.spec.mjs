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

test("Manager prepares and launches the next match through the normal engine", async ({ page }) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);

  await expect.poll(async () =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(2);

  const managedId = await page.locator("#team-0 option")
    .nth(1)
    .getAttribute("value");

  expect(managedId).toBeTruthy();

  await page.locator("#team-0").selectOption(managedId);
  await page.locator("#manager-enabled").check();

  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect.poll(async () =>
    page.locator("#manager-opponent option").count()
  ).toBeGreaterThan(1);

  const opponentOptions = page.locator(
    "#manager-opponent option:not([value=''])"
  );
  const opponentCount = await opponentOptions.count();

  let opponentId = null;
  for (let index = 0; index < opponentCount; index += 1) {
    const value = await opponentOptions.nth(index).getAttribute("value");
    if (value && value !== managedId) {
      opponentId = value;
      break;
    }
  }

  expect(opponentId).toBeTruthy();

  await page.locator("#manager-opponent").selectOption(opponentId);

  await expect(page.locator("#manager-play-next")).toBeEnabled();
  await expect(page.locator("#manager-next-action"))
    .toHaveText("Gioca la prossima partita");

  const prepared = await page.evaluate(key =>
    JSON.parse(localStorage.getItem(key)),
  MANAGER_KEY);

  expect(prepared.nextMatch).toEqual({
    opponentProviderTeamId: opponentId,
  });
  expect(prepared.club.providerTeamId).toBe(managedId);
  expect(prepared.club.lineup).toHaveLength(11);

  await page.locator("#hands").fill("1");
  await page.locator("#manager-play-next").click();

  await expect(page.locator("#team-0")).toHaveValue(managedId);
  await expect(page.locator("#team-1")).toHaveValue(opponentId);
  await expect(page.locator("#roll")).toBeEnabled();

  const persistedAfterLaunch = await page.evaluate(key =>
    JSON.parse(localStorage.getItem(key)),
  MANAGER_KEY);

  expect(persistedAfterLaunch.nextMatch).toEqual({
    opponentProviderTeamId: opponentId,
  });

  await page.reload();

  const restored = await page.evaluate(key =>
    JSON.parse(localStorage.getItem(key)),
  MANAGER_KEY);

  expect(restored.nextMatch).toEqual({
    opponentProviderTeamId: opponentId,
  });

  await page.locator("#manager-enabled").check();

  await expect(page.locator("#manager-opponent")).toHaveValue(opponentId);
  await expect(page.locator("#manager-play-next")).toBeEnabled();

  expect(runtimeErrors).toEqual([]);
});

test("Manager can start a new career with another club", async ({ page }) => {
  await page.goto(GAME_URL);

  const team0 = page.locator("#team-0");
  const team1 = page.locator("#team-1");
  const managerNew = page.locator("#manager-new-0");

  await expect(managerNew).toBeDisabled();

  const available = await team0.locator("option").evaluateAll(options =>
    options.map(option => option.value).filter(Boolean)
  );

  expect(available.length).toBeGreaterThanOrEqual(2);

  const firstClub = available[0];
  const secondClub = available[1];

  await team0.selectOption(firstClub);
  await expect(managerNew).toBeEnabled();

  const quickMatchTeam1Before = await team1.inputValue();
  const competitionBefore = await page.locator("#competition").inputValue();

  await managerNew.click();

  await expect(page.locator("#manager-enabled")).toBeChecked();
  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect(team0).toHaveValue(firstClub);
  await expect(team1).toHaveValue(quickMatchTeam1Before);
  await expect(page.locator("#competition")).toHaveValue(competitionBefore);

  const firstStoredClub = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.manager.v1"))
      .club.providerTeamId
  );
  expect(String(firstStoredClub)).toBe(String(firstClub));

  await expect(managerNew).toBeHidden();

  await page.locator("#manager-enabled").uncheck();
  await expect(managerNew).toBeVisible();

  await team0.selectOption(secondClub);
  await expect(managerNew).toBeEnabled();
  await managerNew.click();

  await expect(page.locator("#manager-enabled")).toBeChecked();
  await expect(page.locator("#manager-dashboard")).toBeVisible();

  const secondStoredClub = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.manager.v1"))
      .club.providerTeamId
  );

  expect(String(secondStoredClub)).toBe(String(secondClub));
  expect(String(secondStoredClub)).not.toBe(String(firstStoredClub));
  await expect(team1).toHaveValue(quickMatchTeam1Before);
  await expect(page.locator("#competition")).toHaveValue(competitionBefore);
});

test("Manager derives career summary and recent results from persisted history", async ({ page }) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);

  await expect.poll(async () =>
    page.locator("#team-0 option").count()
  ).toBeGreaterThan(2);

  const managedId = await page.locator("#team-0 option")
    .nth(1)
    .getAttribute("value");

  const opponentId = await page.locator("#team-0 option")
    .nth(2)
    .getAttribute("value");

  expect(managedId).toBeTruthy();
  expect(opponentId).toBeTruthy();
  expect(managedId).not.toBe(opponentId);

  await page.locator("#team-0").selectOption(managedId);
  await page.locator("#manager-enabled").check();

  await expect(page.locator("#manager-dashboard")).toBeVisible();

  await expect(page.locator("#manager-career-played")).toHaveText("0");
  await expect(page.locator("#manager-career-won")).toHaveText("0");
  await expect(page.locator("#manager-career-drawn")).toHaveText("0");
  await expect(page.locator("#manager-career-lost")).toHaveText("0");

  await page.evaluate(({ key, managedId, opponentId }) => {
    const document = JSON.parse(localStorage.getItem(key));
    const managedName = document.club.name;

    const team = (id, name) => ({
      provider: "browser-test",
      providerTeamId: id,
      name,
      players: [],
    });

    const managed = team(managedId, managedName);
    const opponent = team(opponentId, "Test Opponent");

    const record = (matchId, home, away, homeGoals, awayGoals) => ({
      schemaVersion: 1,
      matchId,
      playedAt: `2026-11-11T${matchId.slice(-2)}:00:00.000Z`,
      home,
      away,
      score: { home: homeGoals, away: awayGoals },
      events: [],
      statistics: {},
      ratings: [],
    });

    document.management.matches = [
      record("m-01", managed, opponent, 2, 0),
      record("m-02", opponent, managed, 1, 1),
      record("m-03", managed, opponent, 0, 3),
      record("m-04", opponent, managed, 1, 4),
    ];

    localStorage.setItem(key, JSON.stringify(document));
  }, {
    key: MANAGER_KEY,
    managedId,
    opponentId,
  });

  await page.reload();
  await page.locator("#manager-enabled").check();

  await expect(page.locator("#manager-career-played")).toHaveText("4");
  await expect(page.locator("#manager-career-won")).toHaveText("2");
  await expect(page.locator("#manager-career-drawn")).toHaveText("1");
  await expect(page.locator("#manager-career-lost")).toHaveText("1");
  await expect(page.locator("#manager-career-goalsFor")).toHaveText("7");
  await expect(page.locator("#manager-career-goalsAgainst")).toHaveText("5");

  await expect(page.locator("#manager-history li")).toHaveCount(4);
  await expect(page.locator("#manager-history li").first())
    .toContainText("V · Test Opponent 1–4");

  const persisted = await page.evaluate(key =>
    JSON.parse(localStorage.getItem(key)),
  MANAGER_KEY);

  expect(persisted.management.matches).toHaveLength(4);
  expect(runtimeErrors).toEqual([]);
});
