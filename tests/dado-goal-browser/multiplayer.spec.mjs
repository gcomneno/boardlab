import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:8787/multiplayer.html";

async function loginGuest(page) {
  await page.goto(GAME_URL);
  await expect(page.locator("#login")).toBeVisible();
  await page.locator("#login").click();
  await expect(page.locator("#logout")).toBeVisible();
}

test("multiplayer authoritative host/join browser smoke", async ({ browser }) => {
  const hostContext = await browser.newContext({
    extraHTTPHeaders: {
      "CF-Connecting-IP": "127.0.0.11",
    },
  });
  const guestContext = await browser.newContext({
    extraHTTPHeaders: {
      "CF-Connecting-IP": "127.0.0.12",
    },
  });

  const host = await hostContext.newPage();
  const guest = await guestContext.newPage();

  const hostErrors = [];
  const guestErrors = [];

  host.on("pageerror", error => hostErrors.push(error.message));
  guest.on("pageerror", error => guestErrors.push(error.message));

  try {
    await loginGuest(host);

    await expect.poll(async () =>
      host.locator("#team-0 option").count()
    ).toBeGreaterThan(1);

    await expect.poll(async () =>
      host.locator("#team-1 option").count()
    ).toBeGreaterThan(1);

    const team0 = await host.locator("#team-0 option").nth(1).getAttribute("value");
    const team1 = await host.locator("#team-1 option").nth(2).getAttribute("value");

    expect(team0).toBeTruthy();
    expect(team1).toBeTruthy();
    expect(team0).not.toBe(team1);

    await host.locator("#team-0").selectOption(team0);
    await host.locator("#team-1").selectOption(team1);
    await host.locator("#hands").fill("1");

    await host.locator("#create").click();

    await expect(host.locator("#invite-code")).not.toHaveValue("");
    const invite = await host.locator("#invite-code").inputValue();

    await expect(host.locator("#pitch-logo-0")).toHaveAttribute("src", /.+/);
    await expect(host.locator("#pitch-logo-1")).toHaveAttribute("src", /.+/);
    await expect(host.locator("#stats-team-0")).toBeAttached();
    await expect(host.locator("#stats-team-1")).toBeAttached();

    await loginGuest(guest);

    await guest.locator("#invite-code").fill(invite);
    await guest.locator("#join").click();

    await expect(guest.locator("#pitch-logo-0")).toHaveAttribute("src", /.+/);
    await expect(guest.locator("#pitch-logo-1")).toHaveAttribute("src", /.+/);
    await expect(guest.locator("#stats-team-0")).toBeAttached();
    await expect(guest.locator("#stats-team-1")).toBeAttached();

    await expect(host.locator("#roll")).toBeEnabled();
    await expect(guest.locator("#roll")).toBeDisabled();

    const snapshot = async page => ({
      turn: await page.locator("#turn").textContent(),
      phase: await page.locator("#phase").textContent(),
      hands0: await page.locator("#hands-0").textContent(),
      hands1: await page.locator("#hands-1").textContent(),
      clock: await page.locator("#match-clock").textContent(),
    });

    await expect.poll(async () => snapshot(guest)).toEqual(
      await snapshot(host)
    );

    const initialDieLabel = await host.locator("#die").getAttribute(
      "aria-label"
    );

    await host.locator("#roll").click();

    await expect.poll(async () =>
      host.locator("#die").getAttribute("aria-label")
    ).not.toBe(initialDieLabel);

    await expect.poll(async () => snapshot(guest)).toEqual(
      await snapshot(host)
    );

    await expect(host.locator("#stats-team-0")).not.toHaveText("");
    await expect(host.locator("#stats-team-1")).not.toHaveText("");
    await expect(guest.locator("#stats-team-0")).not.toHaveText("");
    await expect(guest.locator("#stats-team-1")).not.toHaveText("");

    expect(hostErrors).toEqual([]);
    expect(guestErrors).toEqual([]);
  } finally {
    await hostContext.close();
    await guestContext.close();
  }
});
