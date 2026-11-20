/**
 * M034-RT03: isolated, test-only HTTP bridge. Requires Node 24 with SQLite
 * setAuthorizer support. Never import this module from production code.
 *
 * API: await startRoadTestServer({ port: 0, rulesVersion: 2 }) returns
 * { origin, url, address, tempDir, databasePath, stop }. Port 0 selects a free
 * loopback port. Await handle.stop() (idempotent), or stopRoadTestServer(handle),
 * to stop HTTP, close SQLite and remove exactly this instance's temporary dir.
 * Importing has no startup side effects; callers own shutdown in a finally.
 * rulesVersion: 1 selects the ordinary D1Authority for historical V1 tests.
 * Authority selection is a startup option, never an HTTP parameter.
 *
 * CLI: node v2-roadtest-server.mjs --roadtest-v2 [--port=8787]
 *                               [--rules-version=2|1]
 * The authorization flag is mandatory even in V1 mode. SIGINT/SIGTERM clean up.
 * Readiness is one JSON line containing origin, rulesVersion and address.
 *
 * Each start applies only migrations 0001–0005 to a newly created SQLite file,
 * then seeds the existing Serie A 2024 catalogue (all real teams and players).
 * Select Serie A and two hands per team for the RT02 V2 fixture. Other static
 * catalogues remain real assets but are deliberately not seeded into SQLite.
 * Guest login/logout and static routing use the actual Worker; multiplayer
 * routing and session verification use its existing modules. The API gateway
 * below mirrors the Worker's boundary and rate limits because the Worker does
 * not accept an injected authority. CF-Connecting-IP comes from the socket,
 * never a browser header. Secure __Host cookies and CSRF headers are unchanged.
 * No credentials, bindings, database paths or remote services are accepted.
 */
import { createServer } from "node:http";
import { mkdtemp, readFile, realpath, rm, stat } from "node:fs/promises";
import { tmpdir } from "node:os";
import { isAbsolute, join, relative, sep, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { Readable } from "node:stream";
import worker from "../../../cloudflare/dado-goal/worker.mjs";
import { D1Authority } from "../../../cloudflare/dado-goal/authority.mjs";
import { routeMultiplayer } from "../../../cloudflare/dado-goal/api.mjs";
import {
  authenticate, sessionDenied,
} from "../../../cloudflare/dado-goal/session.mjs";
import { putFootballCatalogue } from "../../../cloudflare/dado-goal/football-catalogue-d1.mjs";
import { createAuthoritativeMatchRosters } from "../../../cloudflare/dado-goal/match-team-rosters.mjs";
import { importCatalogueJson } from "../../../cloudflare/dado-goal/public/catalogue-json.mjs";
import { RoadTestAuthority } from "./v2-roadtest-authority.mjs";

const GAME_ROOT = fileURLToPath(new URL("../../../cloudflare/dado-goal/", import.meta.url));
const MIGRATIONS = [
  "0001_init.sql",
  "0002_football_catalogues.sql",
  "0003_match_teams.sql",
  "0004_rules_version.sql",
  "0005_rules_version_immutable.sql",
];
const MIME = new Map([
  ["html", "text/html; charset=utf-8"],
  ["mjs", "text/javascript; charset=utf-8"],
  ["js", "text/javascript; charset=utf-8"],
  ["css", "text/css; charset=utf-8"],
  ["json", "application/json; charset=utf-8"],
  ["png", "image/png"],
  ["svg", "image/svg+xml"],
  ["ico", "image/x-icon"],
  ["webp", "image/webp"],
  ["jpg", "image/jpeg"],
  ["jpeg", "image/jpeg"],
  ["woff2", "font/woff2"],
]);

function failure(status, code) {
  return Response.json({ error: { code } }, {
    status,
    headers: {
      "Cache-Control": "no-store",
      "X-Content-Type-Options": "nosniff",
    },
  });
}

// Check the raw request target before WHATWG URL dot-segment normalization.
function safeTarget(target) {
  if (typeof target !== "string" || !target.startsWith("/") ||
      target.startsWith("//") || target.includes("#")) return false;
  try {
    return target.split("?", 1)[0].split("/").every(segment => {
      const decoded = decodeURIComponent(segment);
      return decoded !== "." && decoded !== ".." &&
        !/[\\/\u0000-\u001f\u007f]/.test(decoded);
    });
  } catch {
    return false;
  }
}

function inside(root, path) {
  const suffix = relative(root, path);
  return suffix !== "" && suffix !== ".." &&
    !suffix.startsWith(`..${sep}`) && !isAbsolute(suffix);
}

async function assetResponse(request, publicRoot) {
  const path = new URL(request.url).pathname;
  if (!safeTarget(path)) return failure(404, "NOT_FOUND");
  const candidate = resolve(publicRoot, `.${decodeURIComponent(path)}`);
  if (!inside(publicRoot, candidate)) return failure(404, "NOT_FOUND");
  try {
    const canonical = await realpath(candidate);
    if (!inside(publicRoot, canonical) || !(await stat(canonical)).isFile()) {
      return failure(404, "NOT_FOUND");
    }
    const type = MIME.get(canonical.split(".").at(-1));
    if (!type) return failure(404, "NOT_FOUND");
    return new Response(await readFile(canonical), {
      headers: {
        "Content-Type": type,
        "Cache-Control": "no-store",
        "X-Content-Type-Options": "nosniff",
      },
    });
  } catch (error) {
    if (["ENOENT", "ENOTDIR"].includes(error.code)) return failure(404, "NOT_FOUND");
    throw error;
  }
}

// Same digest, scopes, windows and limits as worker.mjs; no test bypass.
async function allowApiRequest(db, request, path) {
  if (request.method !== "POST") return true;
  const [scope, limit] = path === "/api/v1/matches" ? ["create", 5]
    : path.endsWith("/join") ? ["join", 15]
      : path.endsWith("/roll") ? ["roll", 60] : ["other", 10];
  const bytes = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(
    `${scope}:${request.headers.get("CF-Connecting-IP")}`,
  ));
  const client = Array.from(new Uint8Array(bytes), byte =>
    byte.toString(16).padStart(2, "0")).join("");
  const row = await db.prepare(`
    INSERT INTO dado_login_limits (client_digest, window_start, attempts)
    VALUES (?, ?, 1)
    ON CONFLICT(client_digest) DO UPDATE SET
      attempts = CASE WHEN window_start <= excluded.window_start - 60
        THEN 1 ELSE attempts + 1 END,
      window_start = CASE WHEN window_start <= excluded.window_start - 60
        THEN excluded.window_start ELSE window_start END
    RETURNING attempts
  `).bind(client, Math.floor(Date.now() / 1000)).first();
  return row !== null && row.attempts <= limit;
}

async function send(response, outgoing, method) {
  const body = Buffer.from(await response.arrayBuffer());
  if (outgoing.destroyed) return;
  for (const [name, value] of response.headers) {
    if (name !== "set-cookie") outgoing.setHeader(name, value);
  }
  const cookies = response.headers.getSetCookie();
  if (cookies.length) outgoing.setHeader("Set-Cookie", cookies);
  outgoing.writeHead(response.status);
  outgoing.end(method === "HEAD" ? undefined : body);
}

export async function startRoadTestServer({ port = 0, rulesVersion = 2 } = {}) {
  if (Number(process.versions.node.split(".")[0]) !== 24 ||
      typeof globalThis.crypto?.getRandomValues !== "function" ||
      typeof globalThis.crypto?.subtle?.digest !== "function" || typeof btoa !== "function" ||
      typeof Headers.prototype.getSetCookie !== "function") {
    throw new Error("Road-test server requires Node 24, Web Crypto and Fetch APIs");
  }
  if (!Number.isInteger(port) || port < 0 || port > 65535 ||
      ![1, 2].includes(rulesVersion)) throw new TypeError("Invalid server options");
  const { DatabaseSync } = await import("node:sqlite");
  if (typeof DatabaseSync.prototype.setAuthorizer !== "function") {
    throw new Error("Road-test SQLite adapter requires Node 24 with setAuthorizer");
  }
  const { openSqliteD1 } = await import("./sqlite-d1.mjs");
  const publicRoot = await realpath(join(GAME_ROOT, "public"));
  for (const file of [
    "index.html", "multiplayer.html", "app.mjs", "multiplayer-play.mjs",
    "multiplayer-client.mjs", "engine-v2.mjs", "roll-event-v2.mjs",
    "statistics-v2.mjs", "error-i18n.mjs",
  ]) {
    const canonical = await realpath(join(publicRoot, file));
    if (!inside(publicRoot, canonical) || !(await stat(canonical)).isFile()) {
      throw new Error(`Required road-test asset unavailable: ${file}`);
    }
  }
  const migrations = await Promise.all(MIGRATIONS.map(async name => ({
    name, sql: await readFile(join(GAME_ROOT, "migrations", name), "utf8"),
  })));
  const catalogue = importCatalogueJson(await readFile(
    join(publicRoot, "catalogues/serie-a-2024.json"), "utf8",
  ));
  if (catalogue.provider !== "api-football" || catalogue.competitionId !== "135" ||
      catalogue.season !== "2024" || catalogue.teams.length < 2) {
    throw new Error("Required Serie A 2024 road-test fixture unavailable");
  }

  const tempDir = await mkdtemp(join(tmpdir(), "dado-goal-roadtest-v2-"));
  const databasePath = join(tempDir, "roadtest.sqlite");
  let db;
  let server;
  let origin;
  let stopping;
  const pending = new Set();
  function stop() {
    stopping ??= (async () => {
      try {
        if (server?.listening) {
          const closed = new Promise((resolveClose, reject) => server.close(error =>
            error ? reject(error) : resolveClose()));
          // Also terminate incomplete HTTP uploads, so shutdown cannot hang.
          server.closeAllConnections();
          await closed;
        }
        await Promise.allSettled([...pending]);
      } finally {
        try { db?.close(); }
        finally { await rm(tempDir, { recursive: true, force: true }); }
      }
    })();
    return stopping;
  }

  try {
    // Native exec handles whole SQL files, including the multi-statement trigger.
    // It is used only here, on the freshly created file, before opening RT01.
    const schema = new DatabaseSync(databasePath, { enableForeignKeyConstraints: true });
    try {
      for (const { name, sql } of migrations) {
        if (!sql.trim()) throw new Error(`Empty road-test migration: ${name}`);
        try { schema.exec(sql); }
        catch (cause) { throw new Error(`Road-test migration failed: ${name}`, { cause }); }
      }
      for (const table of [
        "lifecycle_matches", "lifecycle_seats", "lifecycle_events",
        "lifecycle_receipts", "dado_browser_sessions", "dado_login_limits",
        "football_catalogues", "lifecycle_match_teams",
      ]) schema.prepare(`SELECT * FROM ${table} LIMIT 0`).all();
      schema.prepare("SELECT rules_version FROM lifecycle_matches LIMIT 0").all();
      if (!schema.prepare("SELECT 1 FROM sqlite_schema WHERE type = 'trigger' AND name = ?")
        .get("lifecycle_rules_version_immutable")) throw new Error("Missing immutable rules schema");
    } finally { schema.close(); }
    db = openSqliteD1(databasePath);
    await putFootballCatalogue(db, catalogue);
    // Resolve every seeded team through the production authoritative roster path.
    const selections = catalogue.teams.map(team => ({
      provider: catalogue.provider, competitionId: catalogue.competitionId,
      season: catalogue.season, providerTeamId: String(team.providerTeamId),
    }));
    for (let index = 1; index < selections.length; index++) {
      await createAuthoritativeMatchRosters(db, [selections[0], selections[index]]);
    }
    const authority = rulesVersion === 2 ? new RoadTestAuthority(db) : new D1Authority(db);
    const env = { DB: db, ASSETS: { fetch: request => assetResponse(request, publicRoot) } };

    async function handle(incoming, outgoing) {
      if (stopping) return send(failure(503, "SERVICE_UNAVAILABLE"), outgoing, incoming.method);
      if (!safeTarget(incoming.url)) return send(failure(404, "NOT_FOUND"), outgoing, incoming.method);
      const headers = new Headers();
      for (const [name, value] of Object.entries(incoming.headers)) {
        if (value !== undefined) headers.set(name, Array.isArray(value) ? value.join(", ") : value);
      }
      // Pin the URL to the actual listener, rather than trusting Host/proxy headers.
      const duplicateBoundary = ["host", "origin"].some(name =>
        incoming.rawHeaders.filter((value, index) => index % 2 === 0 &&
          value.toLowerCase() === name).length > 1);
      const suppliedOrigin = headers.get("Origin");
      if (duplicateBoundary || headers.get("Host") !== new URL(origin).host ||
          (incoming.method === "GET" ? suppliedOrigin !== null && suppliedOrigin !== origin
            : suppliedOrigin !== origin)) {
        return send(failure(403, "BOUNDARY_DENIED"), outgoing, incoming.method);
      }
      headers.set("CF-Connecting-IP", incoming.socket.remoteAddress);
      const request = new Request(`${origin}${incoming.url}`, {
        method: incoming.method, headers,
        ...(!["GET", "HEAD"].includes(incoming.method)
          ? { body: Readable.toWeb(incoming), duplex: "half" } : {}),
      });
      const path = new URL(request.url).pathname;
      let response;
      if (path.startsWith("/api/v1/")) {
        if (!(await authenticate(request, db))) response = sessionDenied();
        else if (!(await allowApiRequest(db, request, path))) response = failure(429, "RATE_LIMITED");
        else response = await routeMultiplayer(request, authority);
      } else response = await worker.fetch(request, env);
      await send(response, outgoing, incoming.method);
    }

    server = createServer({ requestTimeout: 30_000, headersTimeout: 10_000 }, (incoming, outgoing) => {
      const task = handle(incoming, outgoing).catch(async () => {
        if (outgoing.headersSent) outgoing.destroy();
        else await send(failure(500, "INTERNAL_ERROR"), outgoing, incoming.method);
      });
      pending.add(task);
      void task.then(() => pending.delete(task), () => {
        pending.delete(task);
        outgoing.destroy();
      });
    });
    await new Promise((resolveListen, reject) => {
      server.once("error", reject);
      server.listen({ host: "127.0.0.1", port, exclusive: true }, () => {
        server.removeListener("error", reject);
        resolveListen();
      });
    });
    const address = server.address();
    if (address?.address !== "127.0.0.1" || address.family !== "IPv4") {
      throw new Error("Road-test listener must bind exclusively to IPv4 loopback");
    }
    origin = `http://127.0.0.1:${address.port}`;
    return Object.freeze({ origin, url: origin, address, tempDir, databasePath, stop });
  } catch (error) {
    try { await stop(); }
    catch (cleanupError) { throw new AggregateError([error, cleanupError], "Startup and cleanup failed"); }
    throw error;
  }
}

export async function stopRoadTestServer(handle) {
  await handle.stop();
}

async function cli(args) {
  if (!args.includes("--roadtest-v2")) throw new Error("CLI startup requires explicit --roadtest-v2 authorization");
  const seen = new Set();
  const options = { port: 8787, rulesVersion: 2 };
  for (const arg of args) {
    const [key, value] = arg.split("=");
    if (seen.has(key)) throw new Error(`Duplicate CLI option: ${key}`);
    seen.add(key);
    if (arg === "--roadtest-v2") continue;
    if (/^--port=\d+$/.test(arg)) options.port = Number(value);
    else if (/^--rules-version=[12]$/.test(arg)) options.rulesVersion = Number(value);
    else throw new Error(`Unknown CLI option: ${arg}`);
  }
  let handle;
  let shutdownRequested = false;
  const removeSignals = () => {
    process.removeListener("SIGINT", shutdown);
    process.removeListener("SIGTERM", shutdown);
  };
  const shutdown = () => {
    shutdownRequested = true;
    if (!handle) return;
    void handle.stop().catch(error => {
      console.error(error);
      process.exitCode = 1;
    }).finally(removeSignals);
  };
  process.on("SIGINT", shutdown);
  process.on("SIGTERM", shutdown);
  try {
    handle = await startRoadTestServer(options);
    if (shutdownRequested) {
      await handle.stop();
      removeSignals();
    } else {
      console.log(JSON.stringify({ origin: handle.origin, rulesVersion: options.rulesVersion, address: handle.address }));
    }
  } catch (error) {
    removeSignals();
    throw error;
  }
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  await cli(process.argv.slice(2)).catch(error => {
    console.error(error);
    process.exitCode = 1;
  });
}
