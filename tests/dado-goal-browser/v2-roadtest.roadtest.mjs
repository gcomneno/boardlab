import { test as base, expect } from "@playwright/test";
import { fork } from "node:child_process";
import { once } from "node:events";
import { existsSync } from "node:fs";
import { randomBytes, createHash } from "node:crypto";
import { DatabaseSync } from "node:sqlite";
import { fileURLToPath } from "node:url";
import { newV2Match } from "../../web/dado-goal/engine-v2.mjs";
import { replayServerV2 } from "../../cloudflare/dado-goal/rules-v2.mjs";
import { replayMatchEvents } from "../../cloudflare/dado-goal/match-events-replay.mjs";
import { initialState, applyRoll } from "../../cloudflare/dado-goal/game.mjs";

const configPath = fileURLToPath(new URL("./playwright.v2-roadtest.config.mjs", import.meta.url));

async function deadline(promise, label, milliseconds = 15_000) {
  let timer;
  try {
    return await Promise.race([promise, new Promise((_, reject) => {
      timer = setTimeout(() => reject(new Error(label)), milliseconds);
    })]);
  } finally { clearTimeout(timer); }
}

const test = base.extend({
  rulesVersion: [2, { option: true }],
  roadServer: async ({ rulesVersion }, use, testInfo) => {
    const child = fork(configPath, ["--roadtest-v2", `--rules-version=${rulesVersion}`], {
      execPath: process.execPath, execArgv: [], silent: true,
    });
    let output = "";
    child.stdout.on("data", data => { output += data; });
    child.stderr.on("data", data => { output += data; });
    const exited = once(child, "exit");
    let server;
    try {
      server = await deadline(Promise.race([
        once(child, "message").then(([message]) => message),
        exited.then(([code, signal]) => {
          throw new Error(`RT03 exited before readiness (${code}, ${signal}): ${output}`);
        }),
      ]), "RT03 startup timed out");
      expect(server.address).toMatchObject({ address: "127.0.0.1", family: "IPv4" });
      expect(new URL(server.origin).hostname).toBe("127.0.0.1");
      expect(existsSync(server.databasePath)).toBe(true);
      await use(server);
    } finally {
      try {
        if (child.connected) child.send("stop");
        const [code, signal] = await deadline(exited, "RT03 cleanup timed out");
        expect({ code, signal }, output).toEqual({ code: 0, signal: null });
        if (server) expect(existsSync(server.tempDir), "RT03 must remove its SQLite directory").toBe(false);
      } finally {
        // SIGTERM invokes RT03 handle.stop(), including SQLite cleanup.
        if (child.exitCode === null && child.signalCode === null) {
          child.kill("SIGTERM");
          await deadline(exited, "RT03 did not exit after SIGTERM");
        }
        await testInfo.attach("isolated-server.log", { body: output, contentType: "text/plain" });
      }
    }
  },
  players: async ({ browser, roadServer }, use, testInfo) => {
    const records = [];
    const contexts = [];
    const players = [];
    let closing = false;
    try {
      for (const seat of [0, 1]) {
        const context = await browser.newContext({ locale: "it-IT" });
        context.setDefaultTimeout(8_000);
        contexts.push(context);
        const player = { context, offline: false, csrf: null, seat };
        // Real catalogue logos have external URLs. Deny them before any
        // network access. Local assets and every API response remain real.
        await context.route(url => !["data:", "blob:"].includes(url.protocol) &&
          url.origin !== roadServer.origin, async route => {
          records.push({ seat, type: "external-resource-blocked", url: route.request().url(), expected: true });
          await route.abort("blockedbyclient");
        });
        const page = await context.newPage();
        player.page = page;
        page.on("pageerror", error => records.push({ seat, type: "pageerror", message: error.message }));
        page.on("console", message => {
          if (message.type() !== "error") return;
          const text = message.text();
          const url = message.location().url;
          const external = url && new URL(url, roadServer.origin).origin !== roadServer.origin;
          const expected = (external && text.includes("ERR_BLOCKED_BY_CLIENT")) ||
            (player.offline && /ERR_INTERNET_DISCONNECTED|Failed to fetch/.test(text));
          records.push({ seat, type: "console-error", message: text, url, expected });
        });
        page.on("requestfailed", request => records.push({ seat, type: "requestfailed",
          url: request.url(), message: request.failure()?.errorText,
          expected: new URL(request.url()).origin !== roadServer.origin ||
            ((player.offline || (closing && request.failure()?.errorText === "net::ERR_ABORTED")) &&
              request.method() === "GET" && request.url().includes("/api/v1/matches/")),
        }));
        page.on("response", response => {
          if (response.status() >= 400) records.push({ seat, type: "http-error",
            url: response.url(), status: response.status() });
        });
        players.push(player);
      }
      await use(players);
    } finally {
      closing = true;
      for (const context of contexts) await context.close();
      await testInfo.attach("browser-diagnostics.json", {
        body: JSON.stringify(records, null, 2), contentType: "application/json",
      });
      expect(records.filter(record => !record.expected), "Unexpected browser or HTTP errors; see browser-diagnostics.json").toEqual([]);
    }
  },
});

async function login(player, origin) {
  const { page } = player;
  await page.goto(`${origin}/multiplayer.html`);
  await page.locator("#language").selectOption("it");
  const responsePromise = page.waitForResponse(response => response.url() === `${origin}/auth/guest`);
  await page.locator("#login").click();
  const response = await responsePromise;
  expect(response.status(), "real guest authentication").toBe(200);
  player.csrf = (await response.json()).csrfToken;
  expect(player.csrf).toEqual(expect.any(String));
  await expect(page.locator("#logout")).toBeVisible();
}

async function api(player, path, { method = "GET", body, token } = {}) {
  const response = await player.page.evaluate(async ({ path, method, body, token, csrf }) => {
    const headers = { Accept: "application/json" };
    if (token) headers.Authorization = `Bearer ${token}`;
    if (body !== undefined) headers["Content-Type"] = "application/json";
    if (method !== "GET") headers["X-Dado-CSRF"] = csrf;
    const result = await fetch(path, { method, headers, credentials: "same-origin", cache: "no-store",
      ...(body !== undefined ? { body: JSON.stringify(body) } : {}) });
    return { status: result.status, data: await result.json() };
  }, { path, method, body, token, csrf: player.csrf });
  expect(response.status, `${method} ${path}: ${JSON.stringify(response.data)}`).toBe(200);
  return response.data;
}

async function createAndJoin(players, origin, hands = 2) {
  const [host, guest] = players;
  await login(host, origin);
  await login(guest, origin);
  const hostCookie = (await host.context.cookies()).find(cookie => cookie.name === "__Host-dado-session");
  const guestCookie = (await guest.context.cookies()).find(cookie => cookie.name === "__Host-dado-session");
  expect(hostCookie).toBeTruthy();
  expect(guestCookie).toBeTruthy();
  expect(hostCookie.value).not.toBe(guestCookie.value);
  await expect.poll(() => host.page.locator("#team-0 option").count()).toBeGreaterThan(2);
  await expect(host.page.locator("#competition")).toHaveValue("135:2024");
  const team0 = await host.page.locator("#team-0 option").nth(1).getAttribute("value");
  const team1 = await host.page.locator("#team-1 option").nth(2).getAttribute("value");
  await host.page.locator("#team-0").selectOption(team0);
  await host.page.locator("#team-1").selectOption(team1);
  await host.page.locator("#hands").fill(String(hands));
  const createdPromise = host.page.waitForResponse(response =>
    new URL(response.url()).pathname === "/api/v1/matches" && response.request().method() === "POST");
  await host.page.locator("#create").click();
  const createdResponse = await createdPromise;
  expect(createdResponse.status()).toBe(201);
  const created = await createdResponse.json();
  host.token = created.hostToken;
  await expect(host.page.locator("#invite-code")).toHaveValue(`DG1.${created.matchId}.${created.inviteToken}`);
  const invitation = await host.page.locator("#invite-code").inputValue();
  await guest.page.locator("#invite-code").fill(invitation);
  const joinedPromise = guest.page.waitForResponse(response => response.url().endsWith(`/${created.matchId}/join`));
  await guest.page.locator("#join").click();
  const joinedResponse = await joinedPromise;
  expect(joinedResponse.status()).toBe(200);
  const joined = await joinedResponse.json();
  guest.token = joined.guestToken;
  expect(guest.token).not.toBe(host.token);
  await focus(host.page);
  await expect(host.page.locator("#roll")).toBeEnabled();
  await expect(guest.page.locator("#roll")).toBeDisabled();
  return { created, joined, path: `/api/v1/matches/${created.matchId}` };
}

async function focus(page) {
  await page.bringToFront();
  // Exercise the existing production focus-refresh hook in headless Chromium.
  await page.evaluate(() => window.dispatchEvent(new Event("focus")));
}

const phaseLabels = { team: "Dado squadra", pink: "Dado rosa", result: "Dado finale",
  penalty: "Dado rigore verde bottiglia", finished: "Partita conclusa" };

async function uiState(player, state) {
  const { page } = player;
  await expect(page.locator("#phase")).toHaveText(phaseLabels[state.phase]);
  for (const seat of [0, 1]) {
    await expect(page.locator(`#goals-${seat}`)).toHaveText(String(state.goals[seat]));
    await expect(page.locator(`#hands-${seat}`)).toHaveText(`${state.hands[seat]} / ${state.handsPerTeam} lanci`);
  }
  if (state.phase === "finished" || state.player !== player.seat) await expect(page.locator("#roll")).toBeDisabled();
  else await expect(page.locator("#roll")).toBeEnabled();
}

async function clickRoll(player, path, revision) {
  await focus(player.page);
  await expect(player.page.locator("#roll")).toBeEnabled();
  const responsePromise = player.page.waitForResponse(response => response.url().endsWith(`${path}/roll`));
  await player.page.locator("#roll").click();
  const response = await responsePromise;
  expect(response.status(), `roll at revision ${revision}`).toBe(200);
  const result = await response.json();
  expect(response.request().postDataJSON()).toMatchObject({ expectedRevision: revision, requestId: expect.any(String) });
  expect(result).toMatchObject({ revision: revision + 1, duplicate: false });
  await uiState(player, result.state);
  return { result, body: response.request().postDataJSON() };
}

function persisted(databasePath, matchId) {
  const db = new DatabaseSync(databasePath, { readOnly: true });
  try {
    return {
      match: { ...db.prepare("SELECT revision, status, state_json, rules_version FROM lifecycle_matches WHERE match_id = ?").get(matchId) },
      events: db.prepare("SELECT revision, event_json FROM lifecycle_events WHERE match_id = ? ORDER BY revision").all(matchId).map(row => ({ ...row })),
      receipts: db.prepare("SELECT seat, request_id, expected_revision, response_json FROM lifecycle_receipts WHERE match_id = ? ORDER BY expected_revision").all(matchId).map(row => ({ ...row })),
    };
  } finally { db.close(); }
}

function replayWithoutRandomness(events) {
  // Guard only synchronous replay in this Node test process. The HTTP server
  // and both browser clients retain their real, untouched random sources.
  const mathRandom = Math.random;
  const cryptoRandom = crypto.getRandomValues;
  const unexpectedDraw = () => { throw new Error("Stored V2 replay attempted a new random draw"); };
  try {
    Math.random = unexpectedDraw;
    crypto.getRandomValues = unexpectedDraw;
    const states = [];
    let state = newV2Match(2);
    for (const event of events) {
      state = replayServerV2(state, event);
      states.push(state);
    }
    return states;
  } finally {
    Math.random = mathRandom;
    crypto.getRandomValues = cryptoRandom;
  }
}

test("M034-RT04: two real sessions, pending penalty reconnect, receipts and stored V2 replay", async ({ players, roadServer }, testInfo) => {
  const { created, joined, path } = await createAndJoin(players, roadServer.origin);
  expect(created.snapshot).toMatchObject({ rulesVersion: 2, revision: 0, status: "waiting", state: newV2Match(2) });
  expect(joined.snapshot).toMatchObject({ rulesVersion: 2, revision: 1, status: "active" });
  const kinds = ["attack", "powerful-shot", "penalty-awarded", "penalty-goal",
    "attack", "powerful-shot", "penalty-awarded", "penalty-missed",
    "attack", "powerful-shot", "goal", "attack", "powerful-shot", "save"];
  const seats = [0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1];
  const actions = [];
  for (let index = 0; index < kinds.length; index++) {
    await test.step(`revision ${index + 2}: ${kinds[index]}`, async () => {
      const actor = players[seats[index]];
      const action = await clickRoll(actor, path, index + 1);
      actions.push(action);
      expect(action.result.event).toMatchObject({ kind: kinds[index], seat: seats[index], rulesVersion: 2 });
      for (const player of players) {
        await focus(player.page);
        await uiState(player, action.result.state);
      }
      if (kinds[index] === "penalty-awarded") {
        const before = actions[index - 1].result.state;
        expect(action.result.state).toEqual({ ...before, phase: "penalty" });
        expect(action.result.event).toEqual({ type: "penalty-awarded", rulesVersion: 2, seat: seats[index], kind: "penalty-awarded" });
        const stored = persisted(roadServer.databasePath, created.matchId);
        expect(stored.events.at(-1).revision).toBe(index + 2);
        expect(JSON.parse(stored.events.at(-1).event_json)).toEqual(action.result.event);
        await expect(actor.page.locator("#result")).toHaveText("Rigore da battere");
      }
      if (index === 2) {
        // Interrupt snapshot traffic while the penalty is pending. No reload:
        // multiplayer-play does not currently call MultiplayerClient.restore().
        const beforeOffline = persisted(roadServer.databasePath, created.matchId);
        actor.offline = true;
        await actor.context.setOffline(true);
        try {
          const failed = actor.page.waitForEvent("requestfailed", request =>
            request.method() === "GET" && request.url().endsWith(path));
          await focus(actor.page);
          await failed;
          await expect(actor.page.locator("#result")).toHaveText("Rigore da battere");
          await uiState(actor, action.result.state);
          const observed = await api(players[1], path, { token: players[1].token });
          expect(observed).toMatchObject({ revision: 4, status: "active", state: action.result.state });
          expect(persisted(roadServer.databasePath, created.matchId)).toEqual(beforeOffline);
        } finally { await actor.context.setOffline(false); }
        const recoveredPromise = actor.page.waitForResponse(response => response.url().endsWith(path) && response.status() === 200);
        await focus(actor.page);
        const recovered = await (await recoveredPromise).json();
        actor.offline = false;
        expect(recovered).toMatchObject({ revision: 4, state: action.result.state });
        await expect(actor.page.locator("#result")).toHaveText("Rigore da battere");
      }
    });
  }
  // Receipt lookup must win even after completion, at a now-stale revision.
  // A new random draw here would also violate the RT02 fixture coordinates.
  const beforeRetry = persisted(roadServer.databasePath, created.matchId);
  const duplicate = await api(players[0], `${path}/roll`, {
    method: "POST", token: players[0].token, body: actions[2].body,
  });
  expect(duplicate).toEqual({ ...actions[2].result, duplicate: true });
  expect(persisted(roadServer.databasePath, created.matchId)).toEqual(beforeRetry);
  expect(beforeRetry.match).toMatchObject({ revision: 15, status: "completed", rules_version: 2 });
  expect(beforeRetry.events).toHaveLength(15); // join plus fourteen actions
  expect(beforeRetry.receipts).toHaveLength(14);
  expect(beforeRetry.receipts.map(row => row.expected_revision)).toEqual(Array.from({ length: 14 }, (_, i) => i + 1));
  for (const [index, receipt] of beforeRetry.receipts.entries()) {
    expect(receipt).toMatchObject({ seat: seats[index], request_id: actions[index].body.requestId });
    expect(JSON.parse(receipt.response_json)).toEqual(actions[index].result);
  }
  expect(beforeRetry.events.map(row => row.revision)).toEqual(Array.from({ length: 15 }, (_, i) => i + 1));
  const events = beforeRetry.events.slice(1).map(row => ({ ...JSON.parse(row.event_json), revision: row.revision }));
  expect(JSON.parse(beforeRetry.events[0].event_json)).toMatchObject({ type: "joined", rosters: expect.any(Array) });
  expect(events.map(event => event.kind)).toEqual(kinds);
  expect(events.map(event => event.seat)).toEqual(seats);
  const replayedStates = replayWithoutRandomness(events);
  for (const [index, state] of replayedStates.entries()) {
    expect(state, `stored replay at revision ${index + 2}`).toEqual(actions[index].result.state);
  }
  const replayed = replayedStates.at(-1);
  expect(replayed).toEqual(JSON.parse(beforeRetry.match.state_json));
  expect(replayed).toMatchObject({ goals: [2, 0], hands: [2, 2], phase: "finished" });
  expect(replayMatchEvents(beforeRetry.events).rosters).toHaveLength(2);
  for (const player of players) {
    const snapshot = await api(player, path, { token: player.token });
    expect(snapshot).toMatchObject({ rulesVersion: 2, revision: 15, status: "completed", state: replayed });
    expect(snapshot.rollEvents).toEqual(events);
    expect(snapshot.lastRoll).toEqual(events.at(-1));
    for (const [seat, statistics] of snapshot.statistics.entries()) {
      expect(statistics).toMatchObject({ attacks: 2, shots: 2, powerfulShots: 2,
        penalties: 1, possession: 50, possessionRolls: 6,
        goals: seat === 0 ? 2 : 0, penaltyGoals: seat === 0 ? 1 : 0,
        penaltyMissed: seat === 0 ? 0 : 1, saves: seat === 0 ? 1 : 0 });
      const nonzero = new Set(["attacks", "shots", "powerfulShots", "penalties", "possession", "possessionRolls",
        ...(seat === 0 ? ["goals", "penaltyGoals", "saves"] : ["penaltyMissed"])]);
      for (const [key, value] of Object.entries(statistics)) if (!nonzero.has(key)) expect(value, key).toBe(0);
    }
    await uiState(player, replayed);
    await expect(player.page.locator("#match-stats .stat-values strong")).toHaveText([
      "2", "0", "50%", "50%", "2", "2", "0", "0", "0", "0", "1", "0", "0", "0", "1", "1",
    ]);
  }
  expect(persisted(roadServer.databasePath, created.matchId)).toEqual(beforeRetry);
  await testInfo.attach("canonical-v2-history.json", {
    body: JSON.stringify({ events, replayed }, null, 2), contentType: "application/json",
  });
  expect(events[7]).toMatchObject({ kind: "penalty-missed", die: "penalty", faceIndex: 5, face: "SBAGLIATO!" });
  expect(events[10]).toMatchObject({ kind: "goal", die: "result", faceIndex: 0 });
  expect(events[13]).toMatchObject({ kind: "save", die: "result", faceIndex: 1 });
  // Keep the RT04 acceptance criterion strict. RT02 currently scripts slot 0;
  // it must supply slot 4 before this acceptance assertion can pass.
  expect(events[3], "RT04 requires penalty goal faceIndex=4; RT02 currently scripts D6 slot 0").toMatchObject({
    kind: "penalty-goal", die: "penalty", faceIndex: 4, face: "GOAL!",
  });
});

test.describe("V1 compatibility in an ordinary isolated D1Authority server", () => {
  test.use({ rulesVersion: 1 });
  test("historical unversioned snapshot and ordinary UI create/join/legal continuation", async ({ players, roadServer }) => {
    const { created, joined, path } = await createAndJoin(players, roadServer.origin);
    expect(created.snapshot).toMatchObject({ rulesVersion: 1, revision: 0, status: "waiting", state: initialState(2) });
    expect(created.snapshot.state).not.toHaveProperty("rulesVersion");
    expect(joined.snapshot).toMatchObject({ rulesVersion: 1, revision: 1, status: "active" });
    const ordinary = await clickRoll(players[0], path, 1);
    expect(ordinary.result).not.toHaveProperty("event");
    expect(ordinary.result.die).toBe("team");
    expect(ordinary.result.state).toEqual(applyRoll(initialState(2), ordinary.result.face));
    for (const player of players) {
      await focus(player.page);
      await uiState(player, ordinary.result.state);
      const snapshot = await api(player, path, { token: player.token });
      expect(snapshot).toMatchObject({ rulesVersion: 1, revision: 2, state: ordinary.result.state });
    }

    // Historical row: no rulesVersion in state, no initial-roster event and
    // omitted rules_version column (migration default must remain V1).
    const historical = { ...initialState(2), hands: [1, 1], goals: [1, 0], phase: "result" };
    const matchId = randomBytes(32).toString("base64url");
    const token = randomBytes(32).toString("base64url");
    const now = Math.floor(Date.now() / 1000);
    const db = new DatabaseSync(roadServer.databasePath);
    try {
      db.prepare(`INSERT INTO lifecycle_matches
        (match_id, revision, status, state_json, invite_used, created_at, updated_at, expires_at)
        VALUES (?, 7, 'active', ?, 1, ?, ?, ?)`)
        .run(matchId, JSON.stringify(historical), now, now, now + 3600);
      db.prepare("INSERT INTO lifecycle_seats (match_id, seat, token_digest) VALUES (?, 0, ?)")
        .run(matchId, createHash("sha256").update(token).digest());
    } finally { db.close(); }
    const historicalPath = `/api/v1/matches/${matchId}`;
    const snapshot = await api(players[0], historicalPath, { token });
    expect(snapshot).toMatchObject({ rulesVersion: 1, revision: 7, status: "active", state: historical,
      lastRoll: null, rollEvents: [], matchEventContext: null });
    const continued = await api(players[0], `${historicalPath}/roll`, { method: "POST", token,
      body: { expectedRevision: 7, requestId: "rt04-historical-v1" } });
    expect(continued).toMatchObject({ revision: 8, duplicate: false, die: "result" });
    expect(continued.state).toEqual(applyRoll(historical, continued.face));
    expect(continued.state).not.toHaveProperty("rulesVersion");
    const stored = persisted(roadServer.databasePath, matchId);
    expect(stored.match.rules_version).toBe(1);
    expect(stored.events).toHaveLength(1);
    expect(stored.receipts).toHaveLength(1);
  });
});
