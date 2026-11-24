import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";
const CUP_KEY = "dado-goal.cup.v1";
const MANAGEMENT_KEY = "dado-goal.team-management.v1";
const GOAL = 1 / 12;
const WOODWORK = 7 / 12;
const SAVED = 9 / 12;
const WIDE = 11 / 12;
const INTERCEPTED = 5 / 12;
const HOME_WIN = [GOAL, WIDE, GOAL, WOODWORK, GOAL, SAVED];
const AWAY_WIN = [WIDE, GOAL, WOODWORK, GOAL, SAVED, GOAL];

test.use({ reducedMotion: "reduce" });
test.setTimeout(60_000);

// Install before application modules capture/use Math.random. A constant per
// regular-match click also makes auxiliary scorer/event draws deterministic;
// no production imports, synthetic matches, storage seeding or mocked locks.
test.beforeEach(async ({ context }) => {
  await context.addInitScript(({ fallback }) => {
    window.__cupRandom = { value: fallback, calls: 0 };
    Math.random = () => {
      window.__cupRandom.calls += 1;
      return window.__cupRandom.value;
    };
  }, { fallback: INTERCEPTED });
});

test.afterEach(async ({ context }) => {
  for (const page of context.pages()) {
    expect(await page.evaluate(() => window.__cupPageErrors ?? []))
      .toEqual([]);
  }
});

test.beforeEach(async ({ context }) => {
  await context.addInitScript(() => {
    window.__cupPageErrors = [];
    window.addEventListener("error", event => {
      window.__cupPageErrors.push(event.message);
    });
    window.addEventListener("unhandledrejection", event => {
      window.__cupPageErrors.push(String(event.reason));
    });
  });
});

async function openDisclosure(page, selector) {
  const disclosure = page.locator(selector);
  if (!(await disclosure.evaluate(element => element.open))) {
    await disclosure.locator(":scope > summary").click();
  }
  await expect(disclosure).toHaveAttribute("open", "");
}

async function openCup(page) {
  await openDisclosure(page, "#tournament-disclosure");
}

async function setLocale(page, locale) {
  await openDisclosure(page, ".configuration-panel > details");
  await page.locator("#language").selectOption(locale);
  await expect(page.locator("html")).toHaveAttribute("lang", locale);
}

async function setDraw(page, value) {
  await page.evaluate(value => { window.__cupRandom.value = value; }, value);
}

async function drawCount(page) {
  return page.evaluate(() => window.__cupRandom.calls);
}

async function readCup(page) {
  return page.evaluate(key => JSON.parse(localStorage.getItem(key)), CUP_KEY);
}

async function cupSource(page) {
  return page.evaluate(key => localStorage.getItem(key), CUP_KEY);
}

async function ordinaryState(page) {
  return page.evaluate(() => ({
    // Compare whole records, including scores, scorers, rollEvents, events,
    // statistics, ratings and Manager/player totals, not just displayed goals.
    storage: Object.fromEntries([
      "dado-goal.team-management.v1",
      "dado-goal.manager.v1",
      "dado-goal.tournament.v1",
      "dado-goal.tournament-recovery.v1",
      "dado-goal.cup-recovery.v1",
      "dado-goal.cup-archive.v1",
    ].map(key => [key, localStorage.getItem(key)])),
    display: Object.fromEntries([
      "goals-0", "goals-1", "hands-0", "hands-1", "match-stats",
      "match-events-list", "result", "commentary-text",
      "manager-career-played", "manager-career-won", "manager-career-drawn",
      "manager-career-lost", "manager-career-goalsFor",
      "manager-career-goalsAgainst", "manager-player-statistics",
    ].map(id => [id, document.getElementById(id).textContent])),
    matchDie: document.getElementById("die").innerHTML,
  }));
}

function teamName(cup, id) {
  const team = cup.participants.find(team =>
    String(team.providerTeamId) === String(id)
  );
  expect(team).toBeTruthy();
  return team.name;
}

function sessionTeams(cup, key) {
  const firstLeg = cup.fixtures.find(fixture =>
    `r${fixture.round}-p${fixture.pair}` === key && fixture.leg === 1
  );
  expect(firstLeg).toBeTruthy();
  return {
    home: teamName(cup, firstLeg.homeProviderTeamId),
    away: teamName(cup, firstLeg.awayProviderTeamId),
  };
}

async function createCup(page, { legs = 1, manager = false } = {}) {
  await page.goto(GAME_URL);
  await setLocale(page, "it");
  await page.locator("#hands").fill("1");
  await page.locator("#random-hands").uncheck();
  await openCup(page);
  await page.locator("#tournament-mode").selectOption("cup");
  await page.locator("#cup-legs").selectOption(String(legs));
  await page.locator("#tournament-country").selectOption("Italy");
  const participants = page.locator("#tournament-participants");
  await expect.poll(async () => participants.locator("option").count())
    .toBeGreaterThan(3);
  const selected = await participants.locator("option").evaluateAll(options =>
    options.filter(option => option.value).slice(0, 4).map(option => option.value)
  );
  expect(selected).toHaveLength(4);
  await participants.selectOption(selected);
  await page.locator("#tournament-name").fill("SP-01F Cup");
  await page.locator("#tournament-create").click();
  await expect(page.locator("#tournament-dashboard")).toBeVisible();
  await expect(page.locator("#tournament-play-next")).toBeEnabled();
  const cup = await readCup(page);
  expect(cup.legs).toBe(legs);
  expect(cup.participants).toHaveLength(4);
  expect(cup.fixtures).toHaveLength(2 * legs);
  expect(cup.fixtures.every(fixture => fixture.matchId === null)).toBe(true);
  if (manager) {
    await page.locator("#team-0").selectOption(cup.fixtures[0].homeProviderTeamId);
    await page.locator("#manager-new-0").click();
    await expect(page.locator("#manager-dashboard")).toBeVisible();
  }
  return cup;
}

async function regularRoll(page, value) {
  await expect(page.locator("#roll")).toBeEnabled();
  await setDraw(page, value);
  await page.locator("#roll").click();
}

async function playNextMatch(page, score = { home: 0, away: 0 }) {
  const before = await readCup(page);
  const fixture = before.fixtures.find(fixture => fixture.matchId === null);
  expect(fixture).toBeTruthy();
  // Match configuration and the live catalogue are initialized afresh after a
  // reload; Cup persistence alone does not restore the ordinary-match settings.
  await expect.poll(async () => page.locator("#team-0 option").count())
    .toBeGreaterThan(1);
  await openDisclosure(page, ".configuration-panel > details");
  await page.locator("#hands").fill("1");
  await page.locator("#random-hands").uncheck();
  await openCup(page);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();
  await page.locator("#tournament-play-next").click();
  await expect(page.locator("#team-0")).toHaveValue(fixture.homeProviderTeamId);
  await expect(page.locator("#team-1")).toHaveValue(fixture.awayProviderTeamId);
  await openDisclosure(page, "#match-disclosure");
  // With one fixed hand per side: interception ends it at 0; a team shot,
  // bicycle kick and GOAL face end it at 1. Auxiliary event draws see the same
  // constant, so their number never shifts the next sporting outcome.
  for (const goals of [score.home, score.away]) {
    expect([0, 1]).toContain(goals);
    if (goals === 0) {
      await regularRoll(page, INTERCEPTED);
    } else {
      await regularRoll(page, GOAL);
      await regularRoll(page, 3 / 12);
      await regularRoll(page, 3 / 12);
    }
  }
  await expect.poll(async () => {
    const cup = await readCup(page);
    return cup.fixtures.find(item => item.fixtureId === fixture.fixtureId).matchId;
  }).toBeTruthy();
  await expect(page.locator("#roll")).toBeDisabled();
  const cup = await readCup(page);
  const completed = cup.fixtures.find(item => item.fixtureId === fixture.fixtureId);
  const match = await page.evaluate(({ key, matchId }) =>
    JSON.parse(localStorage.getItem(key)).matches.find(match => match.matchId === matchId),
  { key: MANAGEMENT_KEY, matchId: completed.matchId });
  expect(match.score).toEqual(score);
  expect(String(match.home.providerTeamId)).toBe(fixture.homeProviderTeamId);
  expect(String(match.away.providerTeamId)).toBe(fixture.awayProviderTeamId);
  await openCup(page);
  return completed;
}

async function reachTiedRound(page, options) {
  const cup = await createCup(page, options);
  for (let index = 0; index < cup.fixtures.length; index += 1) {
    await playNextMatch(page);
    if (index < cup.fixtures.length - 1) {
      await expect(page.locator("#cup-shootout")).toBeHidden();
    }
  }
  await expect(page.locator("#cup-shootout-start")).toBeEnabled();
  await expect(page.locator("#cup-shootout-description")).toContainText("0–0");
  await expect(page.locator("#tournament-play-next")).toBeDisabled();
  return readCup(page);
}

async function startShootout(page, key, activation = "click") {
  const calls = await drawCount(page);
  const start = page.locator("#cup-shootout-start");
  await expect(start).toBeEnabled();
  if (activation === "click") await start.click();
  else { await start.focus(); await page.keyboard.press(activation); }
  await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
  await expect(page.locator("#cup-shootout-kick")).toBeEnabled();
  const cup = await readCup(page);
  expect(cup.shootoutSessions[key].attempts).toEqual([]);
  expect(cup.shootoutSessions[key].status).toBe("initial");
  expect(cup.shootouts?.[key]).toBeUndefined();
  expect(await drawCount(page)).toBe(calls);
  await expect(page.locator("#cup-shootout-manual")).toBeHidden();
  await expect(page.locator("#cup-shootout-confirm")).toBeDisabled();
  return cup;
}

async function kick(page, key, value, activation = "click") {
  const before = await readCup(page);
  const session = before.shootoutSessions[key];
  const index = session.attempts.length + 1;
  const names = sessionTeams(before, key);
  const side = index % 2 === 1 ? "home" : "away";
  const button = page.locator("#cup-shootout-kick");
  await expect(button).toBeEnabled();
  await expect(button).toContainText(names[side]);
  await setDraw(page, value);
  const calls = await drawCount(page);
  if (activation === "click") await button.click();
  else { await button.focus(); await page.keyboard.press(activation); }
  await expect.poll(async () =>
    (await readCup(page)).shootoutSessions[key].attempts.length
  ).toBe(index);
  await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
  const cup = await readCup(page);
  const attempt = cup.shootoutSessions[key].attempts.at(-1);
  expect(attempt).toEqual({
    attemptId: JSON.stringify([cup.cupId, key, index]),
    index,
    team: side,
    face: value < 0.5 ? "GOAL!" : value < 4 / 6 ? "PALO/TRAVERSA"
      : value < 5 / 6 ? "PARATO/RESPINTO" : "FUORI",
    kind: value < 0.5 ? "penalty-goal" : value < 4 / 6 ? "penalty-woodwork"
      : value < 5 / 6 ? "penalty-saved" : "penalty-wide",
  });
  expect(await drawCount(page)).toBe(calls + 1);
  // Finishing one tie can immediately display the next pending tie, whose
  // separate history is empty; inspect the visible sequence only if still here.
  if (cup.shootoutSessions[key].status !== "complete") {
    await expect(page.locator("#cup-shootout-sequence li")).toHaveCount(index);
    await expect(page.locator("#cup-shootout-sequence li").last())
      .toContainText(names[side]);
  }
  return cup;
}

async function finishShootout(page, key, values = HOME_WIN) {
  let cup;
  for (const value of values) cup = await kick(page, key, value);
  expect(cup.shootoutSessions[key].status).toBe("complete");
  return cup;
}

async function reloadShootout(page, key) {
  const source = await cupSource(page);
  const cup = JSON.parse(source);
  await page.reload();
  await openCup(page);
  await expect(page.locator("#cup-shootout-kick")).toBeEnabled();
  await expect(page.locator("#cup-storage-status")).toHaveText("");
  expect(await cupSource(page)).toBe(source);
  await expect(page.locator("#cup-shootout-sequence li"))
    .toHaveCount(cup.shootoutSessions[key].attempts.length);
  await expect(page.locator("#cup-shootout-turn"))
    .toContainText("Rigori ripresi dall'ultimo tentativo salvato.");
  return cup;
}

for (const scenario of [
  { name: "home clinches at six attempts", values: [1 / 12, WIDE, 3 / 12, WOODWORK, 5 / 12, SAVED], result: { home: 3, away: 0 } },
  { name: "away clinches at six attempts", values: AWAY_WIN, result: { home: 0, away: 3 } },
  { name: "home clinches at seven attempts", values: [GOAL, WIDE, GOAL, SAVED, WOODWORK, WIDE, GOAL], result: { home: 3, away: 0 } },
]) {
  test(`Cup alternates penalties and finishes early: ${scenario.name}`, async ({ page }) => {
    const cup = await reachTiedRound(page, { manager: true });
    const ordinary = await ordinaryState(page);
    expect(JSON.parse(ordinary.storage[MANAGEMENT_KEY]).matches).toHaveLength(2);
    expect(ordinary.storage["dado-goal.manager.v1"]).toBeTruthy();
    await startShootout(page, "r1-p1");
    for (let index = 0; index < scenario.values.length; index += 1) {
      const current = await kick(page, "r1-p1", scenario.values[index]);
      if (index < scenario.values.length - 1) {
        expect(current.shootoutSessions["r1-p1"].status).toBe("initial");
        expect(current.shootouts?.["r1-p1"]).toBeUndefined();
      }
    }
    const finished = await readCup(page);
    expect(finished.shootouts["r1-p1"]).toEqual(scenario.result);
    expect(finished.shootoutSessions["r1-p1"].status).toBe("complete");
    expect(finished.fixtures).toEqual(cup.fixtures);
    expect(finished.championProviderTeamId).toBeUndefined();
    expect(await ordinaryState(page)).toEqual(ordinary);
    await expect(page.locator("#cup-shootout-turn")).toContainText("vince ai rigori");
    await expect(page.locator("#cup-shootout-start")).toBeEnabled();
    await expect(page.locator("#cup-shootout-kick")).toBeHidden();
  });
}

test("Cup sudden death waits for equal kicks, resumes a half-pair and allows repeated tied pairs", async ({ page }) => {
  await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  for (let index = 0; index < 10; index += 1) await kick(page, "r1-p1", GOAL);
  let cup = await readCup(page);
  expect(cup.shootoutSessions["r1-p1"].status).toBe("sudden-death");
  expect(cup.shootouts?.["r1-p1"]).toBeUndefined();
  await expect(page.locator("#cup-shootout-score")).toContainText("5 – 5");
  await expect(page.locator("#cup-shootout-turn")).toContainText("Oltranza · coppia 1");
  const initialSource = await cupSource(page);
  const calls = await drawCount(page);
  await setLocale(page, "en");
  await expect(page.locator("#cup-shootout-turn")).toContainText("Sudden death · pair 1");
  expect(await cupSource(page)).toBe(initialSource);
  expect(await drawCount(page)).toBe(calls);
  await kick(page, "r1-p1", GOAL);
  await expect(page.locator("#cup-shootout-turn")).toContainText("No winner before");
  const ordinary = await ordinaryState(page);
  cup = await reloadShootout(page, "r1-p1");
  const names = sessionTeams(cup, "r1-p1");
  expect(cup.shootouts?.["r1-p1"]).toBeUndefined();
  await expect(page.locator("#cup-shootout-score")).toContainText("6 – 5");
  await expect(page.locator("#cup-shootout-turn"))
    .toContainText(`Nessun vincitore prima del rigore di ${names.away}.`);
  await setLocale(page, "en");
  await expect(page.locator("#cup-shootout-turn"))
    .toContainText("Shootout resumed from the last saved attempt.");
  await kick(page, "r1-p1", GOAL);
  await kick(page, "r1-p1", WOODWORK);
  cup = await kick(page, "r1-p1", SAVED);
  expect(cup.shootoutSessions["r1-p1"].status).toBe("sudden-death");
  await expect(page.locator("#cup-shootout-turn")).toContainText("Sudden death · pair 3");
  await kick(page, "r1-p1", WIDE);
  const beforeLast = await ordinaryState(page);
  cup = await kick(page, "r1-p1", GOAL);
  expect(cup.shootouts["r1-p1"]).toEqual({ home: 6, away: 7 });
  expect(cup.shootoutSessions["r1-p1"].attempts).toHaveLength(16);
  expect(cup.shootoutSessions["r1-p1"].status).toBe("complete");
  await expect(page.locator("#cup-shootout-turn"))
    .toContainText(`${names.away} wins the shootout.`);
  expect(await ordinaryState(page)).toEqual(beforeLast);
  // Ordinary runtime resets on reload; durable regular records do not.
  expect((await ordinaryState(page)).storage).toEqual(ordinary.storage);
});

test("Cup reconciles a canonical fixture after a failed Cup write", async ({ page }) => {
  const initial = await createCup(page);
  const fixture = initial.fixtures[0];

  // Preserve the canonical match and recovery intent, but reject the first
  // attempt to associate the match with its Cup fixture.
  await page.evaluate(key => {
    const original = Storage.prototype.setItem;
    let injected = false;
    Storage.prototype.setItem = function(name, value) {
      if (name === key && !injected) {
        injected = true;
        throw new DOMException(
          "injected Cup fixture persistence failure",
          "QuotaExceededError",
        );
      }
      return original.call(this, name, value);
    };
  }, CUP_KEY);

  await page.locator("#tournament-play-next").click();
  await expect(page.locator("#team-0")).toHaveValue(fixture.homeProviderTeamId);
  await expect(page.locator("#team-1")).toHaveValue(fixture.awayProviderTeamId);
  await openDisclosure(page, "#match-disclosure");

  await regularRoll(page, INTERCEPTED);
  await regularRoll(page, INTERCEPTED);

  const pending = await expect.poll(async () => page.evaluate(
    ({ cupKey, managementKey, fixtureId }) => {
      const cup = JSON.parse(localStorage.getItem(cupKey));
      const managementRaw = localStorage.getItem(managementKey);
      const matches = managementRaw === null
        ? []
        : JSON.parse(managementRaw).matches;
      const recoveryRaw = localStorage.getItem("dado-goal.cup-recovery.v1");
      const recovery = recoveryRaw === null ? null : JSON.parse(recoveryRaw);
      const linked = cup.fixtures.find(item => item.fixtureId === fixtureId);
      return {
        matchCount: matches.length,
        pendingMatchId: recovery?.matchId ?? null,
        fixtureMatchId: linked?.matchId ?? null,
      };
    },
    { cupKey: CUP_KEY, managementKey: MANAGEMENT_KEY, fixtureId: fixture.fixtureId },
  )).toMatchObject({
    matchCount: 1,
    fixtureMatchId: null,
  });

  const before = await page.evaluate(() => ({
    recovery: JSON.parse(localStorage.getItem("dado-goal.cup-recovery.v1")),
    matches: JSON.parse(localStorage.getItem("dado-goal.team-management.v1")).matches,
  }));
  expect(before.recovery?.fixtureId).toBe(fixture.fixtureId);
  expect(before.matches).toHaveLength(1);
  expect(before.matches[0].matchId).toBe(before.recovery.matchId);

  await page.reload();
  await openCup(page);

  await expect.poll(async () => {
    const cup = await readCup(page);
    return cup.fixtures.find(item => item.fixtureId === fixture.fixtureId)?.matchId;
  }).toBe(before.recovery.matchId);

  await expect.poll(async () => page.evaluate(() =>
    localStorage.getItem("dado-goal.cup-recovery.v1"),
  )).toBeNull();

  const recovered = await readCup(page);
  expect(recovered.fixtures.find(item =>
    item.fixtureId === fixture.fixtureId).matchId).toBe(before.recovery.matchId);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  await page.reload();
  await openCup(page);
  expect(await readCup(page)).toEqual(recovered);
  const after = await page.evaluate(() => ({
    recovery: localStorage.getItem("dado-goal.cup-recovery.v1"),
    matches: JSON.parse(localStorage.getItem("dado-goal.team-management.v1")).matches,
  }));
  expect(after.recovery).toBeNull();
  expect(after.matches).toEqual(before.matches);
});

test("Cup reload preserves the empty session and exact initial-series history", async ({ page }) => {
  await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  await reloadShootout(page, "r1-p1");
  for (const value of [GOAL, WOODWORK, SAVED]) await kick(page, "r1-p1", value);
  const cup = await reloadShootout(page, "r1-p1");
  await expect(page.locator("#cup-shootout-kick"))
    .toContainText(sessionTeams(cup, "r1-p1").away);
  await expect(page.locator("#cup-shootout-score")).toContainText("Tentativi: 2–1");
  const updated = await kick(page, "r1-p1", WIDE);
  expect(updated.shootoutSessions["r1-p1"].attempts.slice(0, 3))
    .toEqual(cup.shootoutSessions["r1-p1"].attempts);
});

test("Cup admits each winner once, creates one final and crowns one champion across reloads", async ({ page }) => {
  const original = await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  const first = await finishShootout(page, "r1-p1");
  expect(first.fixtures).toHaveLength(2);
  await startShootout(page, "r1-p2");
  const advanced = await finishShootout(page, "r1-p2", AWAY_WIN);
  const firstLegs = original.fixtures.filter(fixture => fixture.leg === 1);
  const finalists = [firstLegs[0].homeProviderTeamId, firstLegs[1].awayProviderTeamId];
  expect(advanced.fixtures.filter(fixture => fixture.round === 2)).toEqual([{
    fixtureId: "r2-p1-l1", round: 2, pair: 1, leg: 1,
    homeProviderTeamId: finalists[0], awayProviderTeamId: finalists[1], matchId: null,
  }]);
  const advancedSource = await cupSource(page);
  await page.reload();
  await openCup(page);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();
  expect(await cupSource(page)).toBe(advancedSource);
  await playNextMatch(page);
  await startShootout(page, "r2-p1");
  const ordinary = await ordinaryState(page);
  const completed = await finishShootout(page, "r2-p1");
  expect(completed.championProviderTeamId).toBe(finalists[0]);
  expect(completed.fixtures).toHaveLength(3);
  expect(new Set(completed.fixtures.map(fixture => fixture.fixtureId)).size).toBe(3);
  expect(Object.keys(completed.shootouts).sort()).toEqual(["r1-p1", "r1-p2", "r2-p1"]);
  expect(await ordinaryState(page)).toEqual(ordinary);
  await expect(page.locator("#tournament-status"))
    .toHaveText(`Campione: ${teamName(completed, finalists[0])}`);
  await expect(page.locator("#cup-shootout-kick")).toBeHidden();
  await expect(page.locator("#cup-shootout-start")).toBeHidden();
  await expect(page.locator("#tournament-play-next")).toBeDisabled();
  const completedSource = await cupSource(page);
  for (let reload = 0; reload < 2; reload += 1) {
    await page.reload();
    await openCup(page);
    await expect(page.locator("#tournament-close")).toBeEnabled();
    expect(await cupSource(page)).toBe(completedSource);
    await expect(page.locator("#cup-shootout-sequence li")).toHaveCount(6);
  }

  // Fail the first archive-image write, after the transaction has started.
  // Reload must reconcile the transaction without losing or duplicating Cup.
  await page.evaluate(() => {
    const original = Storage.prototype.setItem;
    let injected = false;
    Storage.prototype.setItem = function(key, value) {
      if (key === "dado-goal.cup-archive.v1" && !injected) {
        injected = true;
        throw new DOMException(
          "injected cup archive persistence failure",
          "QuotaExceededError",
        );
      }
      return original.call(this, key, value);
    };
  });

  page.once("dialog", dialog => dialog.accept());
  await page.locator("#tournament-close").click();

  await expect.poll(async () => page.evaluate(() =>
    localStorage.getItem("dado-goal.cup.v1") !== null,
  )).toBe(true);

  // Reload restores the pre-commit images and clears transaction evidence.
  await page.reload();
  await openCup(page);
  await expect(page.locator("#tournament-close")).toBeEnabled();
  expect(await readCup(page)).toEqual(completed);

  const afterRecovery = await page.evaluate(() => ({
    active: localStorage.getItem("dado-goal.cup.v1"),
    recovery: localStorage.getItem("dado-goal.cup-recovery.v1"),
    archive: localStorage.getItem("dado-goal.cup-archive.v1"),
  }));
  expect(afterRecovery.active).not.toBeNull();
  expect(afterRecovery.recovery).toBeNull();
  expect(afterRecovery.archive).toBeNull();

  // The storage fault was one-shot and the reload restored native methods.
  page.once("dialog", dialog => dialog.accept());
  await page.locator("#tournament-close").click();

  const archived = async () => page.evaluate(() => ({
    active: localStorage.getItem("dado-goal.cup.v1"),
    recovery: localStorage.getItem("dado-goal.cup-recovery.v1"),
    archive: localStorage.getItem("dado-goal.cup-archive.v1"),
  }));

  await expect.poll(async () => {
    const state = await archived();
    if (state.active !== null || state.recovery !== null ||
        state.archive === null) return false;
    const cups = JSON.parse(state.archive);
    return cups.length === 1 &&
      cups[0].cupId === completed.cupId;
  }).toBe(true);

  const committed = await archived();
  const archive = JSON.parse(committed.archive);
  expect(archive).toHaveLength(1);
  expect(archive[0]).toEqual(completed);

  await page.reload();
  await openCup(page);

  const restored = await archived();
  expect(restored).toEqual(committed);
  expect(JSON.parse(restored.archive)).toHaveLength(1);
  await expect(page.locator("#cup-archive")).toBeVisible();
});

test("Cup uses a real exclusive Web Lock across two tabs and resumes after owner handover", async ({ page, context }) => {
  const cup = await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  await kick(page, "r1-p1", GOAL);
  const lockName = `dado-goal.cup:${cup.cupId}`;
  const heldLocks = target => target.evaluate(async name =>
    (await navigator.locks.query()).held.filter(lock => lock.name === name), lockName);
  const owned = await heldLocks(page);
  expect(owned).toHaveLength(1);
  expect(owned[0].mode).toBe("exclusive");
  const reader = await context.newPage();
  try {
    await reader.goto(GAME_URL);
    await openCup(reader);
    await expect(reader.locator("#cup-storage-status"))
      .toHaveText("Coppa aperta in un'altra scheda. Questa vista è in sola lettura.");
    await expect(reader.locator("#cup-shootout-kick")).toBeDisabled();
    await expect(reader.locator("#cup-shootout-confirm")).toBeDisabled();
    await expect(reader.locator("#tournament-play-next")).toBeDisabled();
    expect(await cupSource(reader)).toBe(await cupSource(page));
    await kick(page, "r1-p1", WOODWORK);
    await expect(reader.locator("#cup-shootout-sequence li")).toHaveCount(2);
    await expect(reader.locator("#cup-shootout-kick")).toBeDisabled();
    await setLocale(reader, "en");
    await expect(reader.locator("#cup-storage-status"))
      .toHaveText("The Cup is open in another tab. This view is read-only.");
    const committed = await cupSource(page);
    expect(await cupSource(reader)).toBe(committed);
    expect(await heldLocks(reader)).toEqual(owned);
    const errors = await page.evaluate(() => window.__cupPageErrors);
    expect(errors).toEqual([]);
    await page.close();
    await expect.poll(async () => (await heldLocks(reader)).length).toBe(0);
    await expect(reader.locator("#cup-storage-retry")).toBeEnabled();
    await reader.locator("#cup-storage-retry").click();
    await expect(reader.locator("#cup-shootout-kick")).toBeEnabled();
    expect(await cupSource(reader)).toBe(committed);
    const transferred = await heldLocks(reader);
    expect(transferred).toHaveLength(1);
    expect(transferred[0].mode).toBe("exclusive");
    expect(transferred[0].clientId).not.toBe(owned[0].clientId);
    const updated = await kick(reader, "r1-p1", SAVED);
    expect(updated.shootoutSessions["r1-p1"].attempts).toHaveLength(3);
    expect(updated.shootoutSessions["r1-p1"].attempts[2].team).toBe("home");
    expect(await reader.evaluate(() => window.__cupPageErrors)).toEqual([]);
  } finally {
    await reader.close();
  }
});

test("Cup suppresses double clicks and held Enter/Space without duplicate kicks or draws", async ({ page }) => {
  await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  await setDraw(page, GOAL);
  const calls = await drawCount(page);
  const button = page.locator("#cup-shootout-kick");
  await button.dblclick();
  await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
  expect((await readCup(page)).shootoutSessions["r1-p1"].attempts).toHaveLength(1);
  expect(await drawCount(page)).toBe(calls + 1);
  await expect(button).toBeFocused();
  await page.keyboard.down("Enter");
  await expect(page.locator("#cup-shootout-sequence li")).toHaveCount(2);
  await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
  await page.keyboard.down("Enter");
  await page.keyboard.down("Enter");
  await page.keyboard.up("Enter");
  expect((await readCup(page)).shootoutSessions["r1-p1"].attempts).toHaveLength(2);
  expect(await drawCount(page)).toBe(calls + 2);
  await page.keyboard.down("Space");
  await page.keyboard.down("Space");
  await page.keyboard.up("Space");
  await expect(page.locator("#cup-shootout-sequence li")).toHaveCount(3);
  await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
  expect(await drawCount(page)).toBe(calls + 3);
  expect(new Set((await readCup(page)).shootoutSessions["r1-p1"].attempts
    .map(attempt => attempt.attemptId)).size).toBe(3);
});

for (const locale of ["it", "en"]) {
  test(`Cup localizes messages, outcomes and accessible keyboard controls in ${locale}`, async ({ page }) => {
    await reachTiedRound(page);
    await setLocale(page, locale);
    const english = locale === "en";
    await expect(page.locator("#cup-shootout-title"))
      .toHaveText(english ? "Penalty shootout" : "Rigori di spareggio");
    await expect(page.locator("#cup-shootout-turn")).toHaveText(english
      ? "Scores level: choose a playable shootout or a manual result."
      : "Parità: scegli i rigori giocabili o il risultato manuale.");
    await expect(page.locator("#cup-shootout-start"))
      .toHaveText(english ? "Play shootout" : "Gioca i rigori");
    await expect(page.locator("#cup-shootout-manual-title"))
      .toHaveText(english ? "Enter manual result" : "Inserisci risultato manuale");
    await expect(page.locator("#cup-shootout-turn")).toHaveAttribute("role", "status");
    await expect(page.locator("#cup-shootout-turn")).toHaveAttribute("aria-live", "polite");
    await expect(page.locator("#cup-shootout-turn")).toHaveAttribute("aria-atomic", "true");
    await expect(page.locator("#cup-shootout-error")).toHaveAttribute("role", "alert");
    await expect(page.locator("#cup-global-omission")).toHaveText(english
      ? "The global save includes the Cup and its archive."
      : "Il salvataggio globale include la Coppa e il suo archivio.");
    await startShootout(page, "r1-p1", "Enter");
    await expect(page.locator("#cup-shootout-kick")).toBeFocused();
    await expect(page.locator("#cup-shootout-turn"))
      .toContainText(english ? "Initial series" : "Serie iniziale");
    const outcomes = english
      ? ["Goal", "Post/crossbar · missed", "Saved/blocked · missed", "Wide · missed"]
      : ["Goal", "Palo/traversa · sbagliato", "Parato/respinto · sbagliato", "Fuori · sbagliato"];
    for (const [index, value] of [GOAL, WOODWORK, SAVED, WIDE].entries()) {
      await kick(page, "r1-p1", value, index % 2 === 0 ? "Space" : "Enter");
      await expect(page.locator("#cup-shootout-sequence li").last()).toContainText(outcomes[index]);
      await expect(page.locator("#cup-shootout-turn"))
        .toContainText(english ? "Penalty saved." : "Rigore salvato.");
      await expect(page.locator("#cup-shootout-die"))
        .toHaveAttribute("alt", `${english ? "Green die" : "Dado verde"}: ${outcomes[index]}`);
      await expect(page.locator("#cup-shootout-kick")).toBeFocused();
    }
    await expect(page.locator("#cup-shootout-sequence"))
      .toHaveAttribute("aria-label", english ? "Penalty sequence" : "Sequenza rigori");
    const source = await cupSource(page);
    const calls = await drawCount(page);
    await setLocale(page, english ? "it" : "en");
    expect(await cupSource(page)).toBe(source);
    expect(await drawCount(page)).toBe(calls);
    await expect(page.locator("#cup-shootout-title"))
      .toHaveText(english ? "Rigori di spareggio" : "Penalty shootout");
    await expect(page.locator("#cup-shootout-sequence li").last())
      .toContainText(english ? "Fuori · sbagliato" : "Wide · missed");
  });
}

for (const result of [{ home: 4, away: 3 }, { home: 0, away: 1 }, { home: 99, away: 1 }]) {
  test(`Cup legacy manual input accepts ${result.home}–${result.away} without fabricating attempts`, async ({ page }) => {
    const cup = await reachTiedRound(page);
    const ordinary = await ordinaryState(page);
    const calls = await drawCount(page);
    await page.locator("#cup-penalties-home").fill(String(result.home));
    await page.locator("#cup-penalties-away").fill(String(result.away));
    await page.locator("#cup-shootout-confirm").focus();
    await page.keyboard.press("Enter");
    await expect.poll(async () => (await readCup(page)).shootouts?.["r1-p1"])
      .toEqual(result);
    const stored = await readCup(page);
    expect(stored.shootoutSessions?.["r1-p1"]).toBeUndefined();
    expect(stored.fixtures).toEqual(cup.fixtures);
    expect(await ordinaryState(page)).toEqual(ordinary);
    expect(await drawCount(page)).toBe(calls);
    await expect(page.locator("#cup-storage-status"))
      .toContainText("Risultato rigori registrato manualmente:");
    await expect(page.locator("#tournament-results li").first())
      .toContainText("(risultato manuale)");
    const source = await cupSource(page);
    await page.reload();
    await openCup(page);
    await expect(page.locator("#cup-shootout-start")).toBeEnabled();
    expect(await cupSource(page)).toBe(source);
    await setLocale(page, "en");
    await expect(page.locator("#tournament-results li").first())
      .toContainText("(manual result)");
  });
}

for (const locale of ["it", "en"]) {
  test(`Cup legacy manual input rejects invalid values with ${locale} validation`, async ({ page }) => {
    await reachTiedRound(page);
    await setLocale(page, locale);
    const source = await cupSource(page);
    const ordinary = await ordinaryState(page);
    const calls = await drawCount(page);
    for (const [home, away] of [
      ["", "3"], ["4", ""], ["-1", "3"], ["4", "-1"],
      ["1.5", "3"], ["4", "2.5"], ["1e2", "3"],
      ["9007199254740992", "3"], ["4", "4"], ["0", "0"],
    ]) {
      await page.locator("#cup-penalties-home").fill(home);
      await page.locator("#cup-penalties-away").fill(away);
      await page.locator("#cup-shootout-confirm").click();
      await expect(page.locator("#cup-shootout-error")).toHaveText(locale === "en"
        ? "Enter two different nonnegative whole-number scores."
        : "Inserisci due punteggi interi, non negativi e diversi.");
      expect(await cupSource(page)).toBe(source);
    }
    // Number inputs sanitize letters in the real browser. Exercise that UI
    // behavior via typing instead of changing its type or injecting a value.
    await page.locator("#cup-penalties-home").fill("");
    await page.locator("#cup-penalties-home").pressSequentially("abc");
    await expect(page.locator("#cup-penalties-home")).toHaveValue("");
    await page.locator("#cup-penalties-away").fill("3");
    await page.locator("#cup-shootout-confirm").click();
    await expect(page.locator("#cup-shootout-error")).toHaveText(locale === "en"
      ? "Enter two different nonnegative whole-number scores."
      : "Inserisci due punteggi interi, non negativi e diversi.");
    expect(await cupSource(page)).toBe(source);
    expect(await ordinaryState(page)).toEqual(ordinary);
    expect(await drawCount(page)).toBe(calls);
    // A rejected command must leave the legacy form usable.
    await page.locator("#cup-penalties-home").fill("4");
    await page.locator("#cup-penalties-away").fill("3");
    await page.locator("#cup-shootout-confirm").click();
    await expect.poll(async () => (await readCup(page)).shootouts?.["r1-p1"])
      .toEqual({ home: 4, away: 3 });
    await expect(page.locator("#cup-storage-status")).toContainText(locale === "en"
      ? "Manually recorded shootout:" : "Risultato rigori registrato manualmente:");
  });
}

test("Cup two-legged tied aggregate uses both decisive legs and first-leg team orientation", async ({ page }) => {
  const original = await createCup(page, { legs: 2 });
  for (const [index, fixture] of original.fixtures.entries()) {
    await playNextMatch(page, fixture.pair === 1 || fixture.leg === 1
      ? { home: 1, away: 0 } : { home: 0, away: 0 });
    if (index < original.fixtures.length - 1) {
      await expect(page.locator("#cup-shootout")).toBeHidden();
    }
  }
  await expect(page.locator("#cup-shootout-description")).toContainText("Risultato partita 1–0");
  await expect(page.locator("#cup-shootout-description")).toContainText("Aggregato 1–1");
  const cup = await startShootout(page, "r1-p1");
  const legs = cup.fixtures.filter(fixture => fixture.pair === 1);
  expect(legs).toHaveLength(2);
  expect(legs[1].homeProviderTeamId).toBe(legs[0].awayProviderTeamId);
  expect(cup.shootoutSessions["r1-p1"].fixtureRefs).toEqual(
    legs.map(({ fixtureId, matchId }) => ({ fixtureId, matchId }))
  );
  expect(cup.shootoutSessions["r1-p1"].homeProviderTeamId).toBe(legs[0].homeProviderTeamId);
  expect(cup.shootoutSessions["r1-p1"].awayProviderTeamId).toBe(legs[0].awayProviderTeamId);
  const ordinary = await ordinaryState(page);
  const finished = await finishShootout(page, "r1-p1");
  expect(finished.shootouts["r1-p1"]).toEqual({ home: 3, away: 0 });
  expect(finished.shootouts["r1-p2"]).toBeUndefined();
  const final = finished.fixtures.filter(fixture => fixture.round === 2);
  expect(final).toHaveLength(2);
  expect(final[0].homeProviderTeamId).toBe(legs[0].homeProviderTeamId);
  expect(final[1].awayProviderTeamId).toBe(legs[0].homeProviderTeamId);
  expect(await ordinaryState(page)).toEqual(ordinary);
  await expect(page.locator("#cup-shootout-description")).toContainText("Aggregato 1–1");
  const source = await cupSource(page);
  await page.reload();
  await openCup(page);
  await expect(page.locator("#tournament-play-next")).toBeEnabled();
  expect(await cupSource(page)).toBe(source);
});

test("Cup two-legged decisive aggregates advance after drawn return legs without penalties", async ({ page }) => {
  const original = await createCup(page, { legs: 2 });
  for (const fixture of original.fixtures) {
    await playNextMatch(page, fixture.leg === 2 ? { home: 0, away: 0 }
      : fixture.pair === 1 ? { home: 1, away: 0 } : { home: 0, away: 1 });
    await expect(page.locator("#cup-shootout")).toBeHidden();
  }
  const cup = await readCup(page);
  expect(Object.keys(cup.shootouts ?? {})).toEqual([]);
  expect(Object.keys(cup.shootoutSessions ?? {})).toEqual([]);
  const firstLegs = original.fixtures.filter(fixture => fixture.leg === 1);
  const finals = cup.fixtures.filter(fixture => fixture.round === 2);
  expect(finals).toHaveLength(2);
  expect(finals[0].homeProviderTeamId).toBe(firstLegs[0].homeProviderTeamId);
  expect(finals[0].awayProviderTeamId).toBe(firstLegs[1].awayProviderTeamId);
  expect(finals[1].homeProviderTeamId).toBe(finals[0].awayProviderTeamId);
  expect(finals[1].awayProviderTeamId).toBe(finals[0].homeProviderTeamId);
});

for (const terminal of [false, true]) {
  test(`Cup retries the identical ${terminal ? "terminal" : "nonterminal"} saved candidate without another draw`, async ({ page }) => {
    await reachTiedRound(page);
    const key = terminal ? "r1-p2" : "r1-p1";
    if (terminal) {
      await startShootout(page, "r1-p1");
      await finishShootout(page, "r1-p1");
    }
    await startShootout(page, key);
    if (terminal) {
      for (const value of HOME_WIN.slice(0, -1)) await kick(page, key, value);
    }
    const source = await cupSource(page);
    const ordinary = await ordinaryState(page);
    const count = JSON.parse(source).shootoutSessions[key].attempts.length;
    await page.evaluate(key => {
      const original = Storage.prototype.setItem;
      let fail = true;
      Storage.prototype.setItem = function (storageKey, value) {
        if (this === window.localStorage && storageKey === key && fail) {
          fail = false;
          throw new DOMException("SP-01F injected Cup save failure", "QuotaExceededError");
        }
        return original.call(this, storageKey, value);
      };
    }, CUP_KEY);
    await setDraw(page, terminal ? SAVED : GOAL);
    const calls = await drawCount(page);
    await page.locator("#cup-shootout-kick").click();
    await expect(page.locator("#cup-shootout-error"))
      .toContainText("Salvataggio non completato.");
    await expect(page.locator("#cup-shootout-retry")).toBeEnabled();
    await expect(page.locator("#cup-shootout-kick")).toBeDisabled();
    await expect(page.locator("#tournament-play-next")).toBeDisabled();
    expect(await cupSource(page)).toBe(source);
    expect(await drawCount(page)).toBe(calls + 1);
    await setLocale(page, "en");
    await expect(page.locator("#cup-shootout-error")).toContainText("Save did not complete.");
    await expect(page.locator("#cup-shootout-retry")).toHaveText("Retry save");
    await setDraw(page, terminal ? GOAL : WIDE);
    await page.locator("#cup-shootout-retry").focus();
    await page.keyboard.press("Enter");
    await expect(page.locator("#cup-shootout")).toHaveAttribute("aria-busy", "false");
    const cup = await readCup(page);
    const session = cup.shootoutSessions[key];
    expect(session.attempts).toHaveLength(count + 1);
    expect(session.attempts.at(-1).face).toBe(terminal ? "PARATO/RESPINTO" : "GOAL!");
    expect(session.status).toBe(terminal ? "complete" : "initial");
    if (terminal) {
      expect(cup.shootouts[key]).toEqual({ home: 3, away: 0 });
      expect(JSON.parse(source).fixtures.filter(fixture => fixture.round === 2)).toEqual([]);
      expect(cup.fixtures.filter(fixture => fixture.round === 2)).toHaveLength(1);
    } else expect(cup.shootouts?.[key]).toBeUndefined();
    expect(await drawCount(page)).toBe(calls + 1);
    // Locale changes translate the ordinary display; compare durable records.
    expect((await ordinaryState(page)).storage).toEqual(ordinary.storage);
    await expect(page.locator("#cup-shootout-retry")).toBeHidden();
  });
}

test("Cup rejects an invalid draw without saving an attempt and localizes the error", async ({ page }) => {
  await reachTiedRound(page);
  await startShootout(page, "r1-p1");
  const source = await cupSource(page);
  await setDraw(page, 1);
  const calls = await drawCount(page);
  await page.locator("#cup-shootout-kick").click();
  await expect(page.locator("#cup-shootout-error"))
    .toHaveText("Impossibile risolvere il rigore. Nessun tentativo salvato.");
  expect(await cupSource(page)).toBe(source);
  expect(await drawCount(page)).toBe(calls + 1);
  await setLocale(page, "en");
  await expect(page.locator("#cup-shootout-error"))
    .toHaveText("Unable to resolve the penalty. No attempt was saved.");
  await kick(page, "r1-p1", GOAL);
  await expect(page.locator("#cup-shootout-error")).toHaveText("");
});

test("Cup preserves saved data and disables editing when Web Locks are unavailable", async ({ page }) => {
  await reachTiedRound(page);
  const source = await cupSource(page);
  await page.addInitScript(() => {
    Object.defineProperty(navigator, "locks", { configurable: true, value: undefined });
  });
  await page.reload();
  await openCup(page);
  await expect(page.locator("#cup-storage-status"))
    .toHaveText("Questo browser non supporta l'accesso esclusivo alla Coppa. Modifica non disponibile.");
  await expect(page.locator("#cup-shootout-start")).toBeDisabled();
  await expect(page.locator("#cup-shootout-confirm")).toBeDisabled();
  await expect(page.locator("#tournament-play-next")).toBeDisabled();
  expect(await cupSource(page)).toBe(source);
  await setLocale(page, "en");
  await expect(page.locator("#cup-storage-status"))
    .toHaveText("This browser does not support exclusive Cup access. Editing is unavailable.");
  expect(await cupSource(page)).toBe(source);
});


test("SP-02 Global Save blocks startup with a pending journal and unavailable Web Locks", async ({ context }) => {
  const probe = await context.newPage();
  const journalKey = "dado-goal.global-save-transaction.v1";
  const marker = "SP02_PENDING_RECOVERY_PROBE";
  const errors = [];

  try {
    await probe.addInitScript(({ key, value }) => {
      Object.defineProperty(navigator, "locks", {
        configurable: true,
        value: undefined,
      });
      localStorage.setItem(key, value);
    }, { key: journalKey, value: marker });

    probe.on("pageerror", error => errors.push(error.message));

    await probe.goto(GAME_URL);

    await expect.poll(
      () => errors.some(message =>
        message.includes("GLOBAL_RECOVERY_REQUIRES_WEB_LOCKS")),
      { timeout: 10000 },
    ).toBe(true);

    expect(await probe.evaluate(key => localStorage.getItem(key), journalKey))
      .toBe(marker);

    expect(await probe.evaluate(() =>
      localStorage.getItem("dado-goal.global-save-generation.v1")))
      .toBeNull();
  } finally {
    await probe.close();
  }
});

test("SP-02 Global Save exports current Cup and archive", async ({ page }) => {
  const cup = await createCup(page);

  const exportButton = page.locator("#game-export");
  await expect(exportButton).toBeEnabled();

  const downloadPromise = page.waitForEvent("download");
  await exportButton.evaluate(button => button.click());
  const download = await downloadPromise;

  expect(download.suggestedFilename()).toMatch(/^dado-goal-.*\.json$/);

  const { readFile } = await import("node:fs/promises");
  const exported = JSON.parse(
    await readFile(await download.path(), "utf8"),
  );

  const persisted = await page.evaluate(() => ({
    cup: JSON.parse(localStorage.getItem("dado-goal.cup.v1")),
    management: JSON.parse(
      localStorage.getItem("dado-goal.team-management.v1"),
    ),
  }));

  expect(exported.schemaVersion).toBe(1);
  expect(exported.persistent.cup).toEqual(cup);
  expect(exported.persistent.cup).toEqual(persisted.cup);
  expect(exported.persistent.cupArchive).toEqual([]);
  expect(exported.persistent.management).toEqual(
    persisted.management ?? {
      schemaVersion: 1,
      matches: [],
      ratings: [],
    },
  );
  expect(exported.persistent.tournament).toBeNull();
  expect(exported.runtime).toHaveProperty("competitionRef");
  await expect(page.locator("#game-save-status"))
    .toHaveText("Salvataggio creato.");
});


test("SP-02 Global Save round-trips an active Cup through the UI", async ({ page }) => {
  const originalCup = await createCup(page);

  // Cup creation persists before its asynchronous exclusive Web Lock
  // has necessarily been acquired. Wait for the real editable state.
  await expect(page.locator("#tournament-play-next")).toBeEnabled();

  const exportButton = page.locator("#game-export");
  await expect(exportButton).toBeEnabled();

  const downloadPromise = page.waitForEvent("download");
  await exportButton.evaluate(button => button.click());
  const download = await downloadPromise;

  const { readFile } = await import("node:fs/promises");
  const bytes = await readFile(await download.path());
  const portable = JSON.parse(bytes.toString("utf8"));

  expect(portable.persistent.cup).toEqual(originalCup);
  expect(portable.persistent.cupArchive).toEqual([]);

  const importDiagnostics = [];
  page.on("dialog", async dialog => {
    importDiagnostics.push(`DIALOG ${dialog.type()}: ${dialog.message()}`);
    await dialog.dismiss();
  });
  page.on("pageerror", error => {
    importDiagnostics.push(`PAGEERROR: ${error.message}`);
  });
  page.on("console", message => {
    if (message.type() === "error") {
      importDiagnostics.push(`CONSOLE ERROR: ${message.text()}`);
    }
  });

  await page.locator("#game-import-file").setInputFiles({
    name: "dado-goal-round-trip.json",
    mimeType: "application/json",
    buffer: bytes,
  });

  try {
    await expect(page.locator("#game-save-status"))
      .toHaveText("Salvataggio caricato.", { timeout: 5000 });
  } catch (error) {
    console.log("===== IMPORT DIAGNOSTICS =====");
    console.log(importDiagnostics.length
      ? importDiagnostics.join("\\n")
      : "NO_DIALOG_OR_BROWSER_ERROR_CAPTURED");
    console.log("===== END IMPORT DIAGNOSTICS =====");
    throw error;
  }

  const persisted = await page.evaluate(() => ({
    cup: JSON.parse(localStorage.getItem("dado-goal.cup.v1")),
    archive: JSON.parse(localStorage.getItem("dado-goal.cup-archive.v1")),
    journal: localStorage.getItem("dado-goal.global-save-transaction.v1"),
    generation: localStorage.getItem("dado-goal.global-save-generation.v1"),
  }));

  expect(persisted.cup).toEqual(originalCup);
  expect(persisted.archive).toEqual([]);
  expect(persisted.journal).toBeNull();
  expect(Number(persisted.generation)).toBeGreaterThan(0);

  await page.reload();
  await expect.poll(async () => page.evaluate(() =>
    JSON.parse(localStorage.getItem("dado-goal.cup.v1")),
  )).toEqual(originalCup);
  expect(await readCup(page)).toEqual(originalCup);
});


test("SP-02 Global Save replaces a different active Cup", async ({ browser }) => {
  const sourceContext = await browser.newContext({ acceptDownloads: true });
  const destinationContext = await browser.newContext({ acceptDownloads: true });

  try {
    const sourcePage = await sourceContext.newPage();
    const cupA = await createCup(sourcePage);
    await expect(sourcePage.locator("#tournament-play-next")).toBeEnabled();

    const downloadPromise = sourcePage.waitForEvent("download");
    await sourcePage.locator("#game-export").evaluate(button => button.click());
    const download = await downloadPromise;
    const { readFile } = await import("node:fs/promises");
    const bytes = await readFile(await download.path());
    const portable = JSON.parse(bytes.toString("utf8"));
    expect(portable.persistent.cup).toEqual(cupA);

    const destinationPage = await destinationContext.newPage();
    const cupB = await createCup(destinationPage);
    await expect(destinationPage.locator("#tournament-play-next")).toBeEnabled();
    expect(cupB.cupId).not.toBe(cupA.cupId);

    const errors = [];
    destinationPage.on("dialog", async dialog => {
      errors.push(dialog.message());
      await dialog.dismiss();
    });
    destinationPage.on("pageerror", error => errors.push(error.message));

    await destinationPage.locator("#game-import-file").setInputFiles({
      name: "cup-a-global-save.json",
      mimeType: "application/json",
      buffer: bytes,
    });

    await expect(destinationPage.locator("#game-save-status"))
      .toHaveText("Salvataggio caricato.", { timeout: 10000 });
    expect(errors).toEqual([]);

    const stored = await destinationPage.evaluate(() => ({
      cup: JSON.parse(localStorage.getItem("dado-goal.cup.v1")),
      archive: JSON.parse(localStorage.getItem("dado-goal.cup-archive.v1")),
      journal: localStorage.getItem("dado-goal.global-save-transaction.v1"),
    }));

    expect(stored.cup).toEqual(cupA);
    expect(stored.cup.cupId).not.toBe(cupB.cupId);
    expect(stored.archive).toEqual([]);
    expect(stored.journal).toBeNull();

    await destinationPage.reload();
    expect(await readCup(destinationPage)).toEqual(cupA);
  } finally {
    await destinationContext.close();
    await sourceContext.close();
  }
});


test("SP-02 Global Save resumes an in-progress Cup match", async ({ page }) => {
  const cup = await createCup(page);
  const fixture = cup.fixtures.find(item => item.matchId === null);
  expect(fixture).toBeTruthy();

  await page.locator("#tournament-play-next").click();
  await expect(page.locator("#team-0"))
    .toHaveValue(fixture.homeProviderTeamId);
  await expect(page.locator("#team-1"))
    .toHaveValue(fixture.awayProviderTeamId);
  await openDisclosure(page, "#match-disclosure");

  await regularRoll(page, GOAL);
  await expect(page.locator("#roll")).toBeEnabled();

  const { readFile } = await import("node:fs/promises");
  const exportDocument = async () => {
    const pending = page.waitForEvent("download");
    await page.locator("#game-export").evaluate(button => button.click());
    const download = await pending;
    const bytes = await readFile(await download.path());
    return { bytes, document: JSON.parse(bytes.toString("utf8")) };
  };
  const displayedMatch = async () => page.evaluate(() =>
    Object.fromEntries([
      "goals-0", "goals-1", "hands-0", "hands-1",
      "roll", "result", "commentary-text", "match-events-list",
    ].map(id => [id, document.getElementById(id).textContent])),
  );

  const before = await exportDocument();
  expect(before.document.runtime.matchStarted).toBe(true);
  expect(before.document.runtime.activeTournamentFixtureId)
    .toBe(fixture.fixtureId);
  expect(before.document.runtime.rollEvents.length).toBeGreaterThan(0);
  expect(before.document.runtime.state.phase).not.toBe("finished");
  const beforeDisplay = await displayedMatch();

  const errors = [];
  page.on("dialog", async dialog => {
    errors.push(`DIALOG ${dialog.type()}: ${dialog.message()}`);
    await dialog.dismiss();
  });
  page.on("pageerror", error => errors.push(`PAGEERROR: ${error.message}`));
  page.on("console", message => {
    if (message.type() === "error") {
      errors.push(`CONSOLE: ${message.text()}`);
    }
  });

  await page.locator("#game-import-file").setInputFiles({
    name: "cup-mid-match.json",
    mimeType: "application/json",
    buffer: before.bytes,
  });

  try {
    await expect(page.locator("#game-save-status"))
      .toHaveText("Salvataggio caricato.", { timeout: 10000 });
  } catch (error) {
    console.log("MID_MATCH_IMPORT_DIAGNOSTICS=",
      errors.length ? errors.join("\n") : "NONE");
    throw error;
  }

  expect(errors).toEqual([]);
  expect(await displayedMatch()).toEqual(beforeDisplay);
  await expect(page.locator("#roll")).toBeEnabled();

  // A successfully imported save is clean: exporting it again without
  // changes is deliberately disabled by the application.
  await expect(page.locator("#game-export")).toBeDisabled();
  expect(await readCup(page)).toEqual(cup);

  // Continue the imported match, without page reload or a new kickoff.
  await regularRoll(page, 3 / 12);
  await expect(page.locator("#roll")).toBeEnabled();
  await expect(page.locator("#game-export")).toBeEnabled();

  const continued = await exportDocument();
  expect(continued.document.runtime.rollEvents.length)
    .toBeGreaterThan(before.document.runtime.rollEvents.length);
  expect(continued.document.runtime.rollEvents.slice(
    0, before.document.runtime.rollEvents.length,
  )).toEqual(before.document.runtime.rollEvents);
  expect(continued.document.runtime.activeTournamentFixtureId)
    .toBe(fixture.fixtureId);
  expect(continued.document.persistent).toEqual(before.document.persistent);
});


test("SP-02 Global Save rejects import while another tab owns the Cup", async ({ page, context }) => {
  const cup = await createCup(page);
  const lockName = `dado-goal.cup:${cup.cupId}`;

  const heldLocks = () => page.evaluate(async name =>
    (await navigator.locks.query()).held
      .filter(lock => lock.name === name),
  lockName);

  await expect.poll(async () => (await heldLocks()).length).toBe(1);
  const initialLocks = await heldLocks();
  expect(initialLocks[0].mode).toBe("exclusive");

  const downloadPromise = page.waitForEvent("download");
  await page.locator("#game-export").evaluate(button => button.click());
  const download = await downloadPromise;
  const { readFile } = await import("node:fs/promises");
  const bytes = await readFile(await download.path());
  expect(JSON.parse(bytes.toString("utf8")).persistent.cup).toEqual(cup);

  const reader = await context.newPage();
  try {
    await reader.goto(GAME_URL);
    await openCup(reader);
    await expect(reader.locator("#cup-storage-status"))
      .toHaveText("Coppa aperta in un'altra scheda. Questa vista è in sola lettura.");

    const storageImage = () => reader.evaluate(() =>
      Object.fromEntries(
        Array.from({ length: localStorage.length }, (_, index) => {
          const key = localStorage.key(index);
          return [key, localStorage.getItem(key)];
        }).sort(([left], [right]) => left.localeCompare(right)),
      ),
    );

    const before = await storageImage();
    const dialogs = [];
    reader.on("dialog", async dialog => {
      dialogs.push({ type: dialog.type(), message: dialog.message() });
      await dialog.dismiss();
    });

    await reader.locator("#game-import-file").setInputFiles({
      name: "cup-owned-in-other-tab.json",
      mimeType: "application/json",
      buffer: bytes,
    });

    await expect.poll(() => dialogs.length, { timeout: 10000 }).toBe(1);
    expect(dialogs[0].type).toBe("alert");
    expect(dialogs[0].message).toBeTruthy();

    await expect(reader.locator("#game-save-status"))
      .not.toHaveText("Salvataggio caricato.");

    expect(await storageImage()).toEqual(before);
    expect(await readCup(page)).toEqual(cup);
    expect(await readCup(reader)).toEqual(cup);
    expect(await heldLocks()).toEqual(initialLocks);

    await expect(reader.locator("#tournament-play-next")).toBeDisabled();
    await expect(page.locator("#tournament-play-next")).toBeEnabled();
  } finally {
    await reader.close();
  }
});
