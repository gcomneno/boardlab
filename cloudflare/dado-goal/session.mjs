/**
 * Browser session authentication for the Cloudflare deployment.
 *
 * Session credentials and CSRF tokens are independent, random values.
 * Only SHA-256 digests are persisted in D1.
 * Logout deletes the server-side session.
 */

const COOKIE = "__Host-dado-session";
const TTL = 3600;

export const SESSION_SCHEMA = `
CREATE TABLE IF NOT EXISTS dado_browser_sessions (
  session_digest TEXT PRIMARY KEY,
  csrf_digest TEXT NOT NULL,
  expires_at INTEGER NOT NULL
)`;

function randomToken() {
  const bytes = crypto.getRandomValues(new Uint8Array(32));
  return Array.from(bytes, b => b.toString(16).padStart(2, "0")).join("");
}

async function hash(value) {
  const bytes = await crypto.subtle.digest(
    "SHA-256",
    new TextEncoder().encode(value)
  );
  return Array.from(new Uint8Array(bytes), b =>
    b.toString(16).padStart(2, "0")
  ).join("");
}

function constantTimeEqual(a, b) {
  if (typeof a !== "string" || typeof b !== "string") return false;
  const left = new TextEncoder().encode(a);
  const right = new TextEncoder().encode(b);
  if (left.length !== right.length) return false;

  let diff = 0;
  for (let i = 0; i < left.length; i++) {
    diff |= left[i] ^ right[i];
  }
  return diff === 0;
}

function cookieValue(request) {
  const header = request.headers.get("Cookie") || "";
  const values = header.split(";").map(part => part.trim())
    .filter(part => part.startsWith(`${COOKIE}=`));

  if (values.length !== 1) return null;

  const value = values[0].slice(COOKIE.length + 1);
  return /^[0-9a-f]{64}$/.test(value) ? value : null;
}

function reply(status, payload, cookie = null) {
  const headers = new Headers({
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store",
    "X-Content-Type-Options": "nosniff",
  });
  if (cookie !== null) headers.set("Set-Cookie", cookie);

  return new Response(JSON.stringify(payload), { status, headers });
}

const denied = () => reply(403, { error: "SESSION_DENIED" });

export async function login(request, db, accessKey) {
  if (typeof accessKey !== "string" || accessKey.length < 32) {
    return reply(503, { error: "AUTH_NOT_CONFIGURED" });
  }

  if (request.headers.get("Content-Type") !== "application/json") {
    return reply(415, { error: "CONTENT_TYPE" });
  }

  const raw = await request.text();
  if (new TextEncoder().encode(raw).length > 4096) {
    return reply(413, { error: "BODY_LIMIT" });
  }

  let body;
  try {
    body = JSON.parse(raw);
  } catch {
    return reply(400, { error: "BAD_JSON" });
  }

  if (
    !body ||
    typeof body !== "object" ||
    Array.isArray(body) ||
    Object.keys(body).length !== 1 ||
    typeof body.accessKey !== "string"
  ) {
    return reply(400, { error: "BAD_REQUEST" });
  }

  // Compare digests of equal length, never the secrets directly.
  const supplied = await hash(body.accessKey);
  const expected = await hash(accessKey);
  if (!constantTimeEqual(supplied, expected)) {
    return reply(403, { error: "ACCESS_DENIED" });
  }

  return issueSession(db);
}

// Anonymous access uses the same secure cookie and CSRF contract.
export async function guestLogin(db) {
  return issueSession(db);
}

async function issueSession(db) {
  const sessionToken = randomToken();
  const csrfToken = randomToken();
  const now = Math.floor(Date.now() / 1000);

  await db.prepare(`
    INSERT INTO dado_browser_sessions
      (session_digest, csrf_digest, expires_at)
    VALUES (?, ?, ?)
  `).bind(
    await hash(sessionToken),
    await hash(csrfToken),
    now + TTL
  ).run();

  return reply(
    200,
    { csrfToken },
    `${COOKIE}=${sessionToken}; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=${TTL}`
  );
}

export async function authenticate(request, db) {
  const value = cookieValue(request);
  if (!value) return false;

  const row = await db.prepare(`
    SELECT csrf_digest, expires_at
    FROM dado_browser_sessions
    WHERE session_digest = ?
  `).bind(await hash(value)).first();

  if (!row || row.expires_at <= Math.floor(Date.now() / 1000)) {
    return false;
  }

  if (request.method === "GET") return true;

  const csrf = request.headers.get("X-Dado-CSRF");
  if (!csrf || !/^[0-9a-f]{64}$/.test(csrf)) return false;

  return constantTimeEqual(await hash(csrf), row.csrf_digest);
}

export async function logout(request, db) {
  if (!(await authenticate(request, db))) return denied();

  const value = cookieValue(request);

  await db.prepare(`
    DELETE FROM dado_browser_sessions
    WHERE session_digest = ?
  `).bind(await hash(value)).run();

  return reply(
    200,
    { loggedOut: true },
    `${COOKIE}=; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=0`
  );
}

export function sessionDenied() {
  return denied();
}
