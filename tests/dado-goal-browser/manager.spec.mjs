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

  await page.locator("#manager-new-0").click();

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

  expect(runtimeErrors).toEqual([]);
});

test("Manager launches its next canonical tournament fixture", async ({ page }) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);

  await page.locator("#competition").selectOption("40:2024");

  const participants = page.locator("#tournament-participants");
  await expect(participants).toBeEnabled();

  const selected = await participants.locator("option").evaluateAll(options =>
    options.slice(0, 4).map(option => option.value)
  );

  expect(selected).toHaveLength(4);

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  const tournament = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.tournament.v1"))
  );

  expect(tournament).toBeTruthy();
  expect(tournament.participants).toHaveLength(4);

  const managedId = String(tournament.participants[0].providerTeamId);

  await page.locator("#team-0").selectOption(managedId);
  await page.locator("#manager-new-0").click();

  await expect(page.locator("#manager-dashboard")).toBeVisible();

  const expectedFixture = tournament.fixtures.find(fixture =>
    fixture.matchId === null &&
    (
      String(fixture.homeProviderTeamId) === managedId ||
      String(fixture.awayProviderTeamId) === managedId
    )
  );

  expect(expectedFixture).toBeTruthy();

  const expectedFixtureIndex = tournament.fixtures.findIndex(
    fixture => fixture.fixtureId === expectedFixture.fixtureId
  );
  const precedingFixtures = tournament.fixtures.slice(0, expectedFixtureIndex);

  expect(precedingFixtures.every(fixture =>
    String(fixture.homeProviderTeamId) !== managedId &&
    String(fixture.awayProviderTeamId) !== managedId
  )).toBe(true);

  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  await page.locator("#hands").fill("1");
  await page.locator("#tournament-play-next").click();

  await expect(page.locator("#team-0")).toHaveValue(
    String(expectedFixture.homeProviderTeamId)
  );
  await expect(page.locator("#team-1")).toHaveValue(
    String(expectedFixture.awayProviderTeamId)
  );

  await expect(page.locator("#roll")).toBeEnabled();
  await expect(page.locator("#tournament-play-next")).toBeDisabled();

  const routed = await page.evaluate(() => ({
    tournament: JSON.parse(
      localStorage.getItem("dado-goal.tournament.v1")
    ),
    management: JSON.parse(
      localStorage.getItem("dado-goal.team-management.v1")
    ),
  }));

  const simulatedFixtures = routed.tournament.fixtures.slice(
    0, expectedFixtureIndex
  );

  expect(simulatedFixtures.every(fixture => fixture.matchId !== null)).toBe(true);
  expect(
    routed.tournament.fixtures[expectedFixtureIndex].matchId
  ).toBeNull();

  for (const fixture of simulatedFixtures) {
    expect(
      routed.management.matches.some(
        match => match.matchId === fixture.matchId
      )
    ).toBe(true);
  }

  await page.reload();

  await expect(page.locator("#manager-body")).toBeHidden();
  await expect(page.locator("#manager-toggle")).toHaveAttribute(
    "aria-expanded",
    "false"
  );
  await page.locator("#manager-toggle").click();
  await expect(page.locator("#manager-dashboard")).toBeVisible();

  const restored = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.tournament.v1"))
  );

  expect(
    restored.fixtures.slice(0, expectedFixtureIndex)
      .every(fixture => fixture.matchId !== null)
  ).toBe(true);
  expect(restored.fixtures[expectedFixtureIndex].matchId).toBeNull();

  expect(runtimeErrors).toEqual([]);
});

test("Manager can start a career from Squadra 2", async ({ page }) => {
  await page.goto(GAME_URL);

  const team1 = page.locator("#team-1");
  const managerNew1 = page.locator("#manager-new-1");

  await expect.poll(async () =>
    team1.locator("option").count()
  ).toBeGreaterThan(1);

  const clubId = await team1.locator("option").nth(1).getAttribute("value");
  expect(clubId).toBeTruthy();

  await team1.selectOption(clubId);
  await expect(managerNew1).toBeEnabled();

  await managerNew1.click();

  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect(page.locator("#manager-new-0")).toBeHidden();
  await expect(page.locator("#manager-new-1")).toBeHidden();

  const storedClub = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.manager.v1"))
      .club.providerTeamId
  );

  expect(String(storedClub)).toBe(String(clubId));
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
  await page.locator("#manager-new-0").click();

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
  await expect(page.locator("#manager-body")).toBeHidden();
  await expect(page.locator("#manager-toggle")).toHaveAttribute(
    "aria-expanded",
    "false"
  );
  await page.locator("#manager-toggle").click();
  await expect(page.locator("#manager-dashboard")).toBeVisible();

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

test("ending Manager career preserves the current tournament", async ({ page }) => {
  await page.goto(GAME_URL);

  await page.locator("#competition").selectOption("40:2024");

  const participants = page.locator("#tournament-participants");
  const selected = await participants.locator("option").evaluateAll(options =>
    options.slice(0, 4).map(option => option.value)
  );

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  const managedId = selected[0];
  await page.locator("#team-0").selectOption(managedId);
  await page.locator("#manager-new-0").click();

  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect(page.locator("#tournament-dashboard")).toBeVisible();

  page.once("dialog", dialog => dialog.accept());
  await page.locator("#manager-close").click();

  await expect(page.locator("#manager-dashboard")).toBeHidden();
  await expect(page.locator("#tournament-dashboard")).toBeVisible();

  const persisted = await page.evaluate(() => ({
    manager: localStorage.getItem("dado-goal.manager.v1"),
    tournament: localStorage.getItem("dado-goal.tournament.v1"),
  }));

  expect(persisted.manager).toBeNull();
  expect(persisted.tournament).not.toBeNull();
});

test("abandoning tournament preserves the Manager career", async ({ page }) => {
  await page.goto(GAME_URL);

  await page.locator("#competition").selectOption("40:2024");

  const participants = page.locator("#tournament-participants");
  const selected = await participants.locator("option").evaluateAll(options =>
    options.slice(0, 4).map(option => option.value)
  );

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  await page.locator("#team-0").selectOption(selected[0]);
  await page.locator("#manager-new-0").click();

  await expect(page.locator("#manager-dashboard")).toBeVisible();
  await expect(page.locator("#tournament-dashboard")).toBeVisible();

  const dialogHandled = new Promise(resolve => {
    page.once("dialog", async dialog => {
      await dialog.accept();
      resolve();
    });
  });

  await page.locator("#tournament-close").click();
  await dialogHandled;

  await expect(page.locator("#tournament-dashboard")).toBeHidden();
  await expect(page.locator("#tournament-setup")).toBeVisible();
  await expect(page.locator("#manager-dashboard")).toBeVisible();

  const persisted = await page.evaluate(() => ({
    manager: localStorage.getItem("dado-goal.manager.v1"),
    tournament: localStorage.getItem("dado-goal.tournament.v1"),
    recovery: localStorage.getItem("dado-goal.tournament-recovery.v1"),
  }));

  expect(persisted.manager).not.toBeNull();
  expect(persisted.tournament).toBeNull();
  expect(persisted.recovery).toBeNull();
});


test("Global save restores tournament and Manager together", async ({ page }) => {
  await page.goto(GAME_URL);

  await page.locator("#competition").selectOption("40:2024");

  const participants = page.locator("#tournament-participants");
  const selected = await participants.locator("option").evaluateAll(
    options => options.slice(0, 4).map(option => option.value)
  );

  expect(selected).toHaveLength(4);

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  await page.locator("#team-0").selectOption(selected[0]);
  await page.locator("#manager-new-0").click();

  const before = await page.evaluate(() => ({
    tournament: JSON.parse(
      localStorage.getItem("dado-goal.tournament.v1")
    ),
    manager: JSON.parse(
      localStorage.getItem("dado-goal.manager.v1")
    ),
    management: JSON.parse(
      localStorage.getItem("dado-goal.team-management.v1")
    ),
  }));

  await page.evaluate(() => {
    window.__dadoGoalSaveBlob = null;

    URL.createObjectURL = blob => {
      window.__dadoGoalSaveBlob = blob;
      return "blob:dado-goal-test";
    };

    HTMLAnchorElement.prototype.click = function () {
      window.__dadoGoalSaveFilename = this.download;
    };
  });

  await page.locator("#game-export").click();

  const exported = await page.evaluate(async () => ({
    filename: window.__dadoGoalSaveFilename,
    source: await window.__dadoGoalSaveBlob.text(),
  }));

  expect(exported.filename).toBe("dado-goal-save-v1.json");

  const exportedDocument = JSON.parse(exported.source);
  expect(exportedDocument.schemaVersion).toBe(1);
  expect(exportedDocument.persistent.tournament).toBeTruthy();
  expect(exportedDocument.persistent.manager).toBeTruthy();

  await page.evaluate(() => {
    for (const key of [
      "dado-goal.tournament.v1",
      "dado-goal.tournament-recovery.v1",
      "dado-goal.manager.v1",
      "dado-goal.team-management.v1",
    ]) {
      localStorage.removeItem(key);
    }
  });

  await page.reload();

  await expect(page.locator("#tournament-dashboard")).toBeHidden();
  await expect(page.locator("#manager-toggle")).toBeDisabled();

  let importAlert = null;
  page.once("dialog", async dialog => {
    importAlert = dialog.message();
    await dialog.accept();
  });

  await page.locator("#game-import-file").setInputFiles({
    name: "dado-goal-save-v1.json",
    mimeType: "application/json",
    buffer: Buffer.from(exported.source),
  });

  await page.waitForTimeout(250);

  expect(importAlert).toBeNull();

  await expect(page.locator("#game-save-status"))
    .toHaveText("Salvataggio caricato.");
  await expect(page.locator("#tournament-dashboard")).toBeVisible();
  await expect(page.locator("#manager-toggle")).toBeEnabled();

  const after = await page.evaluate(() => ({
    tournament: JSON.parse(
      localStorage.getItem("dado-goal.tournament.v1")
    ),
    manager: JSON.parse(
      localStorage.getItem("dado-goal.manager.v1")
    ),
    management: JSON.parse(
      localStorage.getItem("dado-goal.team-management.v1")
    ),
  }));

  expect(after.tournament).toEqual(before.tournament);
  expect(after.manager).toEqual(before.manager);
  expect(after.management).toEqual(
    exportedDocument.persistent.management
  );
});
