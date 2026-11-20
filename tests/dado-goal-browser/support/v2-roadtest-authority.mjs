/**
 * M034-RT02: local, test-only V2 authority. Never wire this into the Worker.
 *
 * Usage: new RoadTestAuthority(db, lifecycleOptions).create(2, selections).
 * The inherited API, seat checks, invitation consumption, history and snapshots
 * remain authoritative. No HTTP field, environment switch or constructor
 * option selects random bytes or enables V2 in production.
 *
 * Assumptions: the database implements atomic D1 batch() and has the schema
 * described by migrations 0001–0005; this module never applies migrations.
 * D1Authority's private token helpers currently use 32 Web Crypto bytes,
 * unpadded base64url tokens and SHA-256 UTF-8 digests. Those helpers must be
 * mirrored here because they are not exported. Selected teams must resolve
 * through the existing authoritative catalogue/roster helpers (including the
 * distinct-team and >=11-player checks); no fallback catalogue is provided.
 * As in D1Authority, null selections retain legacy synthetic join rosters.
 */
import { createHash } from "node:crypto";
import { D1Authority } from "../../../cloudflare/dado-goal/authority.mjs";
import {
  createAuthoritativeMatchRosters,
  resolveMatchTeam,
} from "../../../cloudflare/dado-goal/match-team-rosters.mjs";
import { rollAuthorized } from "../../../cloudflare/dado-goal/roll-d1.mjs";
import { newV2Match } from "../../../web/dado-goal/engine-v2.mjs";

function step(expectedRevision, seat, phase, kind, draws) {
  return Object.freeze({
    expectedRevision,
    seat,
    phase,
    kind,
    draws: Object.freeze(draws.map(([sides, slot]) =>
      Object.freeze({ sides, slot }))),
  });
}

/**
 * Immutable semantic draw fixture. Join commits revision 1; each entry names
 * the revision BEFORE its roll. Draw ordinals are zero-based within a roll.
 * A result action first draws one of 40 penalty-award slots (0 awards), then
 * a D6 slot only when no penalty was awarded. Other actions draw one D6 slot.
 *
 * Hands: host penalty goal, guest penalty miss, host normal goal, guest normal
 * miss (save). Final state: hands [2, 2], goals [2, 0], revision 15, completed.
 * Rejection-free bytes vary by match identity but preserve these exact slots.
 */
export const ROADTEST_V2_FIXTURE = Object.freeze([
  step(1, 0, "team", "attack", [[6, 0]]),
  step(2, 0, "pink", "powerful-shot", [[6, 0]]),
  step(3, 0, "result", "penalty-awarded", [[40, 0]]),
  step(4, 0, "penalty", "penalty-goal", [[6, 4]]),
  step(5, 1, "team", "attack", [[6, 0]]),
  step(6, 1, "pink", "powerful-shot", [[6, 0]]),
  step(7, 1, "result", "penalty-awarded", [[40, 0]]),
  step(8, 1, "penalty", "penalty-missed", [[6, 5]]),
  step(9, 0, "team", "attack", [[6, 0]]),
  step(10, 0, "pink", "powerful-shot", [[6, 0]]),
  step(11, 0, "result", "goal", [[40, 1], [6, 0]]),
  step(12, 1, "team", "attack", [[6, 0]]),
  step(13, 1, "pink", "powerful-shot", [[6, 0]]),
  step(14, 1, "result", "save", [[40, 1], [6, 1]]),
]);

function fixtureByte(matchId, expectedRevision, ordinal) {
  const draw = ROADTEST_V2_FIXTURE[expectedRevision - 1]?.draws[ordinal];
  if (!draw || typeof matchId !== "string" ||
      !/^[A-Za-z0-9_-]{20,128}$/.test(matchId)) {
    throw new Error("Unsupported V2 road-test draw coordinate");
  }

  // Pick an accepted byte congruent to the fixture slot. Hashing the full
  // coordinate makes this independent of other matches, requests, retries,
  // authority instances and invocation order. This is a scripted fixture,
  // not a probability simulation or a source of credential randomness.
  const hash = createHash("sha256")
    .update(JSON.stringify([matchId, expectedRevision, ordinal]))
    .digest();
  const equivalents = Math.floor(256 / draw.sides);
  return draw.slot + draw.sides * (hash[0] % equivalents);
}

function token() {
  if (typeof globalThis.crypto?.getRandomValues !== "function" ||
      typeof globalThis.crypto?.subtle?.digest !== "function" ||
      typeof globalThis.btoa !== "function") {
    throw new Error("V2 road-test requires the D1Authority Web Crypto token contract");
  }
  const bytes = crypto.getRandomValues(new Uint8Array(32));
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return btoa(binary)
    .replace(/\+/g, "-")
    .replace(/\//g, "_")
    .replace(/=/g, "");
}

async function digest(value) {
  return new Uint8Array(await crypto.subtle.digest(
    "SHA-256", new TextEncoder().encode(value),
  ));
}

export class RoadTestAuthority extends D1Authority {
  async create(handsPerTeam = 2, teamSelections = null) {
    if (handsPerTeam !== 2) throw new RangeError("Invalid match length");
    const state = newV2Match(2);
    const now = this.now();

    if (teamSelections !== null) {
      await createAuthoritativeMatchRosters(this.db, teamSelections);
    }
    const teams = teamSelections === null ? null : await Promise.all(
      teamSelections.map(selection => resolveMatchTeam(this.db, selection)),
    );

    const matchId = token();
    const hostToken = token();
    const inviteToken = token();
    const hostDigest = await digest(hostToken);
    const inviteDigest = await digest(inviteToken);

    // createMatch() intentionally accepts V1 only. Mirror its creation batch
    // locally, inserting rules_version=2 from the outset. The match, host seat
    // and optional team selections must all commit or roll back together.
    const statements = [
      this.db.prepare(`
        INSERT INTO lifecycle_matches (
          match_id, revision, status, state_json,
          invite_digest, invite_used,
          created_at, updated_at, expires_at, completed_at, rules_version
        ) VALUES (?, 0, 'waiting', ?, ?, 0, ?, ?, ?, NULL, 2)
      `).bind(
        matchId, JSON.stringify(state), inviteDigest,
        now, now, now + this.waitingTTL,
      ),
      this.db.prepare(`
        INSERT INTO lifecycle_seats (match_id, seat, token_digest)
        VALUES (?, 0, ?)
      `).bind(matchId, hostDigest),
    ];

    if (teamSelections !== null) {
      for (const [seat, team] of teamSelections.entries()) {
        statements.push(this.db.prepare(`
          INSERT INTO lifecycle_match_teams (
            match_id, seat, provider, competition_id, season, provider_team_id
          ) VALUES (?, ?, ?, ?, ?, ?)
        `).bind(
          matchId, seat, team.provider, team.competitionId,
          team.season, team.providerTeamId,
        ));
      }
    }

    const results = await this.db.batch(statements);
    if (!Array.isArray(results) || results.length !== statements.length ||
        results.some(result => result?.success !== true ||
          result.meta?.changes !== 1)) {
      throw new Error("V2 road-test creation requires successful atomic D1 inserts");
    }

    // Verify the duplicated private token contract against inherited seat()
    // rather than returning a host credential that cannot authorize a seat.
    if (await this.seat(matchId, hostToken) !== 0) {
      throw new Error("V2 road-test host token contract mismatch");
    }

    return {
      matchId,
      hostToken,
      inviteToken,
      snapshot: {
        rulesVersion: 2,
        revision: 0,
        status: "waiting",
        seat: 0,
        state,
        teamNames: teams === null ? null : teams.map(team => team.name),
        teams: teams === null ? null : teams.map(team => ({
          name: team.name,
          logo: team.logo ?? null,
          colors: Array.isArray(team.colors) ? [...team.colors] : [],
        })),
      },
    };
  }

  async roll(matchId, seatToken, requestId, expectedRevision) {
    let ordinal = 0;
    return rollAuthorized(
      this, matchId, seatToken, requestId, expectedRevision,
      {
        allowV2: true,
        // Lazy: rollAuthorized authenticates and checks receipts before this
        // is ever called. Only this invocation owns the ordinal counter.
        byteSource: () => fixtureByte(matchId, expectedRevision, ordinal++),
      },
    );
  }
}
