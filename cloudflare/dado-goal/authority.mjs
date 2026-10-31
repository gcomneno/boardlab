/**
 * Server-side multiplayer authority backed by Cloudflare D1.
 *
 * This stage wires create, join and snapshot.
 * Roll is deliberately not exposed until its complete authorization,
 * game transition and durable receipt handling are integrated.
 */

import { initialState, validateState } from "./game.mjs";
import { createMatch, joinMatch } from "./lifecycle-d1.mjs";
import { rollAuthorized } from "./roll-d1.mjs";
import { matchStatistics } from "../../web/dado-goal/statistics.mjs";

export class AuthorityError extends Error {
  constructor(message) {
    super(message);
    this.name = "AuthorityError";
  }
}

function token() {
  const bytes = crypto.getRandomValues(new Uint8Array(32));
  let binary = "";
  for (const byte of bytes) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary)
    .replace(/\+/g, "-")
    .replace(/\//g, "_")
    .replace(/=/g, "");
}

async function digest(value) {
  const input = new TextEncoder().encode(value);
  return new Uint8Array(
    await crypto.subtle.digest("SHA-256", input)
  );
}

function equalBytes(left, right) {
  if (!left || !right) return false;

  const a = new Uint8Array(left);
  const b = new Uint8Array(right);

  if (a.length !== 32 || b.length !== 32) return false;

  let difference = 0;
  for (let i = 0; i < 32; i++) {
    difference |= a[i] ^ b[i];
  }
  return difference === 0;
}

function snapshot(row, seat) {
  return {
    revision: row.revision,
    status: row.status,
    seat,
    state: validateState(JSON.parse(row.state_json)),
  };
}

export class D1Authority {
  constructor(db, {
    waitingTTL = 3600,
    activeTTL = 86400,
  } = {}) {
    if (!db || typeof db.prepare !== "function" ||
        typeof db.batch !== "function") {
      throw new TypeError("D1 binding required");
    }

    for (const ttl of [waitingTTL, activeTTL]) {
      if (!Number.isSafeInteger(ttl) || ttl < 1 ||
          ttl > 31536000) {
        throw new TypeError("Invalid lifecycle policy");
      }
    }

    this.db = db;
    this.waitingTTL = waitingTTL;
    this.activeTTL = activeTTL;
  }

  now() {
    return Math.floor(Date.now() / 1000);
  }

  async match(matchId) {
    const row = await this.db.prepare(`
      SELECT revision, status, state_json,
             created_at, updated_at, expires_at, completed_at
      FROM lifecycle_matches
      WHERE match_id = ?
    `).bind(matchId).first();

    if (!row) throw new AuthorityError("Unknown match");

    return row;
  }

  async seat(matchId, seatToken) {
    if (typeof seatToken !== "string" ||
        !/^[A-Za-z0-9_-]{20,256}$/.test(seatToken)) {
      throw new AuthorityError("Unauthorized participant");
    }

    const candidate = await digest(seatToken);

    const result = await this.db.prepare(`
      SELECT seat, token_digest
      FROM lifecycle_seats
      WHERE match_id = ?
      ORDER BY seat
    `).bind(matchId).all();

    for (const row of result.results ?? []) {
      if (equalBytes(row.token_digest, candidate)) {
        return row.seat;
      }
    }

    throw new AuthorityError("Unauthorized participant");
  }

  async create(handsPerTeam) {
    const state = initialState(handsPerTeam);
    const now = this.now();

    const matchId = token();
    const hostToken = token();
    const inviteToken = token();

    await createMatch(this.db, {
      matchId,
      stateJson: JSON.stringify(state),
      inviteDigest: await digest(inviteToken),
      hostDigest: await digest(hostToken),
      createdAt: now,
      expiresAt: now + this.waitingTTL,
    });

    return {
      matchId,
      hostToken,
      inviteToken,
      snapshot: {
        revision: 0,
        status: "waiting",
        seat: 0,
        state,
      },
    };
  }

  async join(matchId, inviteToken) {
    if (typeof inviteToken !== "string" ||
        !/^[A-Za-z0-9_-]{20,256}$/.test(inviteToken)) {
      throw new AuthorityError("Invalid invitation");
    }

    const now = this.now();
    const guestToken = token();

    const result = await joinMatch(this.db, {
      matchId,
      inviteDigest: await digest(inviteToken),
      guestDigest: await digest(guestToken),
      now,
      expiresAt: now + this.activeTTL,
    });

    if (!result.joined) {
      throw new AuthorityError("Match is not joinable");
    }

    const row = await this.match(matchId);

    return {
      guestToken,
      snapshot: snapshot(row, 1),
    };
  }

  async roll(matchId, seatToken, requestId, expectedRevision) {
    return rollAuthorized(
      this, matchId, seatToken, requestId, expectedRevision
    );
  }

  async snapshot(matchId, seatToken) {
    const seat = await this.seat(matchId, seatToken);
    const row = await this.match(matchId);

    if (this.now() >= row.expires_at) {
      throw new AuthorityError("Match expired");
    }

    const history = await this.db.prepare(`
      SELECT revision, event_json
      FROM lifecycle_events
      WHERE match_id = ?
      ORDER BY revision
    `).bind(matchId).all();

    const rollEvents = [];
    let lastRoll = null;

    for (const entry of history.results ?? []) {
      const event = JSON.parse(entry.event_json);
      if (event.type !== "roll") continue;

      rollEvents.push({
        seat: event.seat,
        die: event.die,
        face: event.face,
      });

      lastRoll = {
        revision: entry.revision,
        seat: event.seat,
        die: event.die,
        face: event.face,
      };
    }

    return {
      ...snapshot(row, seat),
      lastRoll,
      statistics: matchStatistics(rollEvents),
    };
  }
}
