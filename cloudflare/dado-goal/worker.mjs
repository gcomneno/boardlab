/**
 * Dado Goal multiplayer Cloudflare entrypoint.
 *
 * Requires D1 binding DB, static asset binding ASSETS,
 * and secret ACCESS_KEY. No public deployment until all
 * migrations, browser tests and resource limits are qualified.
 */

import { D1Authority } from "./authority.mjs";
import { routeMultiplayer } from "./api.mjs";
import {
  login, logout, authenticate, sessionDenied,
} from "./session.mjs";

const LOGIN_RATE_SCHEMA = `
CREATE TABLE IF NOT EXISTS dado_login_limits (
  client_digest TEXT PRIMARY KEY,
  window_start INTEGER NOT NULL,
  attempts INTEGER NOT NULL CHECK (attempts >= 1)
)`;

const STATIC = new Set([
  "/multiplayer.html",
  "/multiplayer-play.mjs",
  "/multiplayer-client.mjs",
  "/multiplayer-theme.css",
  "/dado-goal-handmade-dice.png",
]);

function error(status, code) {
  return Response.json(
    { error: { code } },
    {
      status,
      headers: {
        "Cache-Control": "no-store",
        "X-Content-Type-Options": "nosniff",
      },
    }
  );
}

async function digest(value) {
  const bytes = await crypto.subtle.digest(
    "SHA-256", new TextEncoder().encode(value)
  );
  return Array.from(new Uint8Array(bytes), value =>
    value.toString(16).padStart(2, "0")
  ).join("");
}

async function allowRequest(db, request, scope, limit) {
  // Cloudflare supplies this header; direct public access without
  // an identifiable client is rejected rather than unmetered.
  const ip = request.headers.get("CF-Connecting-IP");
  if (!ip || ip.length > 64) return false;

  const client = await digest(`${scope}:${ip}`);
  const now = Math.floor(Date.now() / 1000);

  const row = await db.prepare(`
    INSERT INTO dado_login_limits
      (client_digest, window_start, attempts)
    VALUES (?, ?, 1)
    ON CONFLICT(client_digest) DO UPDATE SET
      attempts = CASE
        WHEN window_start <= excluded.window_start - 60
        THEN 1 ELSE attempts + 1 END,
      window_start = CASE
        WHEN window_start <= excluded.window_start - 60
        THEN excluded.window_start ELSE window_start END
    RETURNING attempts
  `).bind(client, now).first();

  return row !== null && row.attempts <= limit;
}

function boundaryValid(request) {
  const url = new URL(request.url);
  const origin = request.headers.get("Origin");
  const host = request.headers.get("Host");

  if (
    url.protocol !== "https:" &&
    !(url.protocol === "http:" &&
      ["127.0.0.1", "localhost"].includes(url.hostname))
  ) {
    return false;
  }

  if (host && host !== url.host) return false;

  if (request.method !== "GET") {
    return origin === url.origin;
  }

  return origin === null || origin === url.origin;
}

export default {
  async fetch(request, env) {
    try {
      if (!boundaryValid(request)) {
        return error(403, "BOUNDARY_DENIED");
      }

      const url = new URL(request.url);
      const path = url.pathname;

      if (request.method === "GET" && STATIC.has(path)) {
        if (!env.ASSETS || typeof env.ASSETS.fetch !== "function") {
          return error(503, "ASSETS_UNAVAILABLE");
        }
        return env.ASSETS.fetch(request);
      }

      if (!env.DB || !env.ACCESS_KEY) {
        return error(503, "SERVICE_UNAVAILABLE");
      }

      if (path === "/auth/login" && request.method === "POST") {
        if (!(await allowRequest(env.DB, request, "login", 10))) {
          return error(429, "RATE_LIMITED");
        }
        return login(request, env.DB, env.ACCESS_KEY);
      }

      if (path === "/auth/logout" && request.method === "POST") {
        return logout(request, env.DB);
      }

      if (path.startsWith("/api/v1/")) {
        if (!(await authenticate(request, env.DB))) {
          return sessionDenied();
        }

        if (request.method === "POST") {
          const operation = path === "/api/v1/matches"
            ? ["create", 5]
            : path.endsWith("/join")
              ? ["join", 15]
              : path.endsWith("/roll")
                ? ["roll", 60]
                : ["other", 10];

          if (!(await allowRequest(
            env.DB, request, operation[0], operation[1]
          ))) {
            return error(429, "RATE_LIMITED");
          }
        }

        const authority = new D1Authority(env.DB);
        return routeMultiplayer(request, authority);
      }

      return error(404, "NOT_FOUND");
    } catch {
      return error(500, "INTERNAL_ERROR");
    }
  },
};
