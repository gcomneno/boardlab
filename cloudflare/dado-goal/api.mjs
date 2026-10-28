/**
 * HTTP contract adapter for the existing Dado Goal multiplayer client.
 *
 * Business logic and persistence belong to the server-side authority.
 * This module neither trusts browser state nor generates game outcomes.
 *
 * Not a deployable Worker entrypoint: authentication and the D1-backed
 * authority must be wired before exposing these routes publicly.
 */

const MATCH = /^\/api\/v1\/matches\/([A-Za-z0-9_-]{20,128})(?:\/(join|roll))?$/;
const JSON_HEADERS = {
  "Content-Type": "application/json; charset=utf-8",
  "Cache-Control": "no-store",
  "X-Content-Type-Options": "nosniff",
};

function respond(status, data) {
  return new Response(JSON.stringify(data), {
    status,
    headers: JSON_HEADERS,
  });
}

function fail(status, code) {
  return respond(status, { error: { code } });
}

function bearer(request) {
  const value = request.headers.get("Authorization") || "";
  const match = /^Bearer ([A-Za-z0-9_-]{20,256})$/.exec(value);
  if (!match) {
    throw new HttpError(401, "UNAUTHORIZED");
  }
  return match[1];
}

class HttpError extends Error {
  constructor(status, code) {
    super(code);
    this.status = status;
    this.code = code;
  }
}

async function jsonBody(request, expectedFields) {
  const contentType = request.headers.get("Content-Type") || "";
  if (!/^application\/json(?:\s*;|$)/i.test(contentType)) {
    throw new HttpError(400, "BAD_REQUEST");
  }

  let text;
  try {
    text = await request.text();
  } catch {
    throw new HttpError(400, "BAD_REQUEST");
  }

  if (new TextEncoder().encode(text).length > 8192) {
    throw new HttpError(413, "PAYLOAD_TOO_LARGE");
  }

  let body;
  try {
    body = JSON.parse(text);
  } catch {
    throw new HttpError(400, "BAD_REQUEST");
  }

  if (
    body === null ||
    typeof body !== "object" ||
    Array.isArray(body) ||
    Object.keys(body).length !== expectedFields.length ||
    expectedFields.some(key => !Object.hasOwn(body, key))
  ) {
    throw new HttpError(400, "BAD_REQUEST");
  }

  return body;
}

function requireAuthority(authority) {
  if (
    !authority ||
    ["create", "join", "snapshot", "roll"].some(
      name => typeof authority[name] !== "function"
    )
  ) {
    throw new TypeError("D1 authority is not configured");
  }
}

/**
 * Route exactly the existing multiplayer API.
 *
 * The caller must enforce gateway authentication, origin and request
 * limits before invoking this adapter. The authority independently
 * validates match ownership, seat tokens, revisions and game turns.
 */
export async function routeMultiplayer(request, authority) {
  requireAuthority(authority);

  try {
    const url = new URL(request.url);
    if (url.search || url.hash) {
      return fail(400, "BAD_REQUEST");
    }

    const path = url.pathname;

    if (request.method === "POST" && path === "/api/v1/matches") {
      const body = await jsonBody(request, ["handsPerTeam"]);
      if (
        !Number.isInteger(body.handsPerTeam) ||
        body.handsPerTeam < 1 ||
        body.handsPerTeam > 1000
      ) {
        return fail(400, "BAD_REQUEST");
      }
      return respond(201, await authority.create(body.handsPerTeam));
    }

    const match = MATCH.exec(path);
    if (!match) {
      return fail(404, "NOT_FOUND");
    }

    const [, matchId, action] = match;

    if (request.method === "GET" && action === undefined) {
      return respond(200, await authority.snapshot(
        matchId, bearer(request)
      ));
    }

    if (request.method === "POST" && action === "join") {
      const body = await jsonBody(request, ["inviteToken"]);
      if (typeof body.inviteToken !== "string" ||
          body.inviteToken.length === 0) {
        return fail(400, "BAD_REQUEST");
      }
      return respond(200, await authority.join(
        matchId, body.inviteToken
      ));
    }

    if (request.method === "POST" && action === "roll") {
      const token = bearer(request);
      const body = await jsonBody(
        request, ["requestId", "expectedRevision"]
      );
      if (
        typeof body.requestId !== "string" ||
        body.requestId.length < 1 ||
        body.requestId.length > 128 ||
        !Number.isSafeInteger(body.expectedRevision) ||
        body.expectedRevision < 0
      ) {
        return fail(400, "BAD_REQUEST");
      }
      return respond(200, await authority.roll(
        matchId, token, body.requestId, body.expectedRevision
      ));
    }

    return fail(404, "NOT_FOUND");
  } catch (error) {
    if (error instanceof HttpError) {
      return fail(error.status, error.code);
    }

    // Preserve the existing Python API status/code contract.
    // Never expose internal exception messages to clients.
    const domainErrors = {
      "Unknown match": [404, "MATCH_NOT_FOUND"],
      "Match expired": [410, "MATCH_EXPIRED"],
      "Unauthorized participant": [401, "UNAUTHORIZED"],
      "Invalid invitation": [401, "UNAUTHORIZED"],
      "Not your turn": [403, "NOT_YOUR_TURN"],
      "Stale revision": [409, "STALE_REVISION"],
      "Match is not joinable": [409, "MATCH_NOT_JOINABLE"],
      "Match is not active": [409, "MATCH_NOT_ACTIVE"],
      "Request ID reused with other revision": [409, "REQUEST_ID_CONFLICT"],
      "Invalid match length": [400, "BAD_REQUEST"],
      "Invalid request ID": [400, "BAD_REQUEST"],
      "Invalid revision": [400, "BAD_REQUEST"],
    };

    const mapped = domainErrors[error?.message];
    return mapped
      ? fail(mapped[0], mapped[1])
      : fail(500, "INTERNAL_ERROR");
  }
}
