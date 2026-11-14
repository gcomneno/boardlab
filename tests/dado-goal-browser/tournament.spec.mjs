import { expect, test } from "@playwright/test";

test("Championship creates, plays, records and reloads its next fixture", async ({ page }) => {
  await page.goto("http://127.0.0.1:4173/web/dado-goal/");

  const participants = page.locator("#tournament-participants");

  await expect(participants).toBeVisible();
  await expect(participants.locator("option")).toHaveCount(84);

  const selected = await participants.locator("option").evaluateAll(options => {
    const competitionIds = new Set();
    const values = [];

    for (const option of options) {
      const catalogueTeam = option.value;
      const label = option.textContent?.trim() ?? "";

      if (!catalogueTeam || !label) continue;

      values.push(catalogueTeam);

      if (values.length === 4) break;
    }

    return values;
  });

  expect(selected).toHaveLength(4);

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  await expect(page.locator("#tournament-setup")).toBeHidden();
  await expect(page.locator("#tournament-dashboard")).toBeVisible();
  await expect(page.locator("#tournament-progress")).toHaveText("0 / 12");
  await expect(page.locator("#tournament-status")).toHaveText(
    "In corso · giornata 1 di 6"
  );
  await expect(page.locator("#tournament-results li")).toHaveCount(0);
  await expect(page.locator("#tournament-pending li")).toHaveCount(2);
  await expect(page.locator("#tournament-standings tr")).toHaveCount(4);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  const persisted = await page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.tournament.v1"))
  );

  expect(persisted.participants).toHaveLength(4);
  expect(persisted.fixtures).toHaveLength(12);
  expect(persisted.fixtures.every(fixture => fixture.matchId === null)).toBe(true);

  const firstFixture = persisted.fixtures[0];

  await page.locator("#hands").fill("1");
  await page.locator("#tournament-play-next").click();

  await expect(page.locator("#team-0")).toHaveValue(
    String(firstFixture.homeProviderTeamId)
  );
  await expect(page.locator("#team-1")).toHaveValue(
    String(firstFixture.awayProviderTeamId)
  );
  await expect(page.locator("#roll")).toBeEnabled();
  await expect(page.locator("#tournament-play-next")).toBeDisabled();

  for (let roll = 0; roll < 100; roll += 1) {
    if (await page.locator("#tournament-progress").textContent() === "1 / 12") {
      break;
    }

    await expect(page.locator("#roll")).toBeEnabled();
    await page.locator("#roll").click();

    await expect.poll(async () => {
      const progress = await page.locator("#tournament-progress").textContent();
      const rollEnabled = await page.locator("#roll").isEnabled();

      return progress === "1 / 12" || rollEnabled;
    }).toBe(true);
  }

  await expect(page.locator("#tournament-progress")).toHaveText("1 / 12");
  await expect(page.locator("#tournament-status")).toHaveText(
    "In corso · giornata 1 di 6"
  );
  await expect(page.locator("#tournament-results li")).toHaveCount(1);
  await expect(page.locator("#tournament-pending li")).toHaveCount(1);
  await expect(page.locator("#roll")).toBeDisabled();
  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  const completed = await page.evaluate(() => ({
    tournament: JSON.parse(localStorage.getItem("dado-goal.tournament.v1")),
    management: JSON.parse(localStorage.getItem("dado-goal.team-management.v1")),
  }));

  const completedFixture = completed.tournament.fixtures[0];

  expect(completedFixture.matchId).toBeTruthy();
  expect(
    completed.tournament.fixtures.filter(fixture => fixture.matchId !== null)
  ).toHaveLength(1);

  const canonicalMatch = completed.management.matches.find(
    match => match.matchId === completedFixture.matchId
  );

  expect(canonicalMatch).toBeTruthy();
  expect(canonicalMatch.home.providerTeamId).toBe(
    String(firstFixture.homeProviderTeamId)
  );
  expect(canonicalMatch.away.providerTeamId).toBe(
    String(firstFixture.awayProviderTeamId)
  );
  expect(Array.isArray(canonicalMatch.events)).toBe(true);

  const playedRows = await page.locator("#tournament-standings tr").evaluateAll(
    rows => rows.filter(row => {
      const cells = row.querySelectorAll("td");
      return cells.length > 2 && cells[2].textContent?.trim() === "1";
    }).length
  );

  expect(playedRows).toBe(2);

  // Simulate a crash after canonical match persistence but before the
  // tournament fixture association is durably saved.
  await page.evaluate(({ originalTournament, fixtureId, matchId }) => {
    localStorage.setItem(
      "dado-goal.tournament.v1",
      JSON.stringify(originalTournament),
    );
    localStorage.setItem(
      "dado-goal.tournament-recovery.v1",
      JSON.stringify({
        championshipId: originalTournament.championshipId,
        fixtureId,
        matchId,
      }),
    );
  }, {
    originalTournament: persisted,
    fixtureId: firstFixture.fixtureId,
    matchId: completedFixture.matchId,
  });

  await page.reload();

  await expect(page.locator("#tournament-dashboard")).toBeVisible();
  await expect(page.locator("#tournament-progress")).toHaveText("1 / 12");
  await expect(page.locator("#tournament-standings tr")).toHaveCount(4);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  const reloaded = await page.evaluate(() => ({
    tournament: JSON.parse(localStorage.getItem("dado-goal.tournament.v1")),
    management: JSON.parse(localStorage.getItem("dado-goal.team-management.v1")),
  }));

  expect(reloaded.tournament.championshipId).toBe(persisted.championshipId);
  expect(reloaded.tournament.fixtures[0].matchId).toBe(completedFixture.matchId);
  expect(
    reloaded.tournament.fixtures.filter(fixture => fixture.matchId !== null)
  ).toHaveLength(1);
  expect(
    reloaded.management.matches.filter(
      match => match.matchId === completedFixture.matchId
    )
  ).toHaveLength(1);

  expect(
    await page.evaluate(() =>
      localStorage.getItem("dado-goal.tournament-recovery.v1")
    )
  ).toBeNull();
});
