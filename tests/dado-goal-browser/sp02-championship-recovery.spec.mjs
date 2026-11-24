import { expect, test } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";

const readState = page => page.evaluate(() => {
  const read = key => {
    const raw = localStorage.getItem(key);
    return raw === null ? null : JSON.parse(raw);
  };

  return {
    tournament: read("dado-goal.tournament.v1"),
    management: read("dado-goal.team-management.v1"),
    recovery: read("dado-goal.tournament-recovery.v1"),
  };
});

test("SP-02 manual championship fixture recovers after failed persistence", async ({
  page,
}) => {
  const runtimeErrors = [];
  page.on("pageerror", error => runtimeErrors.push(error.message));

  await page.goto(GAME_URL);
  await page.locator("#tournament-disclosure > summary").click();

  const participants = page.locator("#tournament-participants");
  await expect.poll(
    () => participants.locator("option").count(),
  ).toBeGreaterThan(3);

  const selected = await participants.locator("option").evaluateAll(
    options => options.slice(0, 4).map(option => option.value),
  );

  expect(selected).toHaveLength(4);

  await participants.selectOption(selected);
  await page.locator("#tournament-create").click();

  // Creation is coordinated asynchronously; wait for the durable document.
  await expect.poll(async () => {
    const current = await readState(page);
    return current.tournament?.fixtures.length ?? 0;
  }).toBe(12);

  const initial = await readState(page);
  expect(initial.tournament.fixtures).toHaveLength(12);
  expect(initial.management?.matches ?? []).toHaveLength(0);

  const fixture = initial.tournament.fixtures[0];

  await page.locator(".configuration-panel > details > summary").click();
  await page.locator("#hands").fill("1");
  await page.locator("#tournament-play-next").click();
  await expect(page.locator("#roll")).toBeEnabled();

  // The journal and canonical match are written first. Fail only the
  // first attempt to persist the completed tournament fixture.
  await page.evaluate(() => {
    const original = Storage.prototype.setItem;
    let injected = false;

    Storage.prototype.setItem = function(key, value) {
      if (key === "dado-goal.tournament.v1" && !injected) {
        injected = true;
        throw new DOMException(
          "injected manual championship persistence failure",
          "QuotaExceededError",
        );
      }

      return original.call(this, key, value);
    };
  });

  for (let attempt = 0; attempt < 100; attempt += 1) {
    const current = await readState(page);
    if (current.recovery !== null) break;

    await expect(page.locator("#roll")).toBeEnabled();
    await page.locator("#roll").click();

    await expect.poll(async () => {
      const state = await readState(page);
      return state.recovery !== null ||
        await page.locator("#roll").isEnabled();
    }).toBe(true);
  }

  const interrupted = await readState(page);

  expect(interrupted.recovery).toBeTruthy();
  expect(interrupted.recovery.fixtureId).toBe(fixture.fixtureId);

  const incompleteFixture = interrupted.tournament.fixtures.find(
    item => item.fixtureId === fixture.fixtureId,
  );

  expect(incompleteFixture.matchId).toBeNull();

  const matchId = interrupted.recovery.matchId;
  expect(interrupted.management.matches.filter(
    match => match.matchId === matchId,
  )).toHaveLength(1);

  const countBeforeReload = interrupted.management.matches.length;

  // Fail the first startup reconciliation write, preserving its journal.
  // This fault is installed for just one navigation in this browser context.
  await page.addInitScript(() => {
    const marker = "sp02-7e-startup-recovery-fault";
    if (sessionStorage.getItem(marker) === "injected") return;

    const original = Storage.prototype.setItem;
    Storage.prototype.setItem = function(key, value) {
      if (this === localStorage && key === "dado-goal.tournament.v1") {
        sessionStorage.setItem(marker, "injected");
        throw new DOMException(
          "injected startup recovery write failure",
          "QuotaExceededError",
        );
      }
      return original.call(this, key, value);
    };
  });

  await page.reload();

  const failedRecovery = await readState(page);
  expect(failedRecovery.recovery).toEqual(interrupted.recovery);
  expect(failedRecovery.tournament.fixtures.find(
    item => item.fixtureId === fixture.fixtureId,
  )?.matchId).toBeNull();
  expect(failedRecovery.management.matches).toHaveLength(countBeforeReload);
  expect(failedRecovery.management.matches.filter(
    match => match.matchId === matchId,
  )).toHaveLength(1);

  // The second navigation retries without the injected write fault.
  await page.reload();

  await expect.poll(async () => {
    const current = await readState(page);
    return current.tournament.fixtures.find(
      item => item.fixtureId === fixture.fixtureId,
    )?.matchId;
  }).toBe(matchId);

  const recovered = await readState(page);
  const recoveredFixture = recovered.tournament.fixtures.find(
    item => item.fixtureId === fixture.fixtureId,
  );

  expect(recoveredFixture.matchId).toBe(matchId);
  expect(recovered.recovery).toBeNull();
  expect(recovered.management.matches).toHaveLength(countBeforeReload);
  expect(recovered.management.matches.filter(
    match => match.matchId === matchId,
  )).toHaveLength(1);
  expect(runtimeErrors).toEqual([]);
});
