/**
 * Dado Goal multiplayer API client.
 *
 * Holds one seat credential in memory.
 * Never generates authoritative dice outcomes.
 * Never stores credentials in a URL or browser storage.
 */

export const RECOVERY_KEY = "dado-goal.multiplayer-seat.v1";

export class MultiplayerAPIError extends Error {
  constructor(status, code) {
    super(code);
    this.name = "MultiplayerAPIError";
    this.status = status;
    this.code = code;
  }
}

export class MultiplayerClient {
  constructor({
    fetchImpl = (...args) => globalThis.fetch(...args),
    storage = null,
  } = {}) {
    if (typeof fetchImpl !== "function") {
      throw new TypeError("Fetch implementation required");
    }

    this.fetchImpl = fetchImpl;
    this.storage = storage;
    this.sessionGeneration = 0;
    this.matchId = null;
    this.seatToken = null;
    this.seat = null;
  }

  async request(method, path, body, token = null) {
    const headers = { Accept: "application/json" };

    if (body !== undefined) {
      headers["Content-Type"] = "application/json";
    }

    if (token !== null) {
      headers.Authorization = `Bearer ${token}`;
    }

    const response = await this.fetchImpl.call(globalThis, path, {
      method,
      headers,
      cache: "no-store",
      credentials: "omit",
      ...(body !== undefined
        ? { body: JSON.stringify(body) }
        : {}),
    });

    const data = await response.json();

    if (!response.ok) {
      throw new MultiplayerAPIError(
        response.status,
        data?.error?.code ?? "HTTP_ERROR"
      );
    }

    return data;
  }

  static matchPath(matchId) {
    if (
      typeof matchId !== "string" ||
      !/^[A-Za-z0-9_-]{20,128}$/.test(matchId)
    ) {
      throw new TypeError("Invalid match identifier");
    }

    return `/api/v1/matches/${matchId}`;
  }

  async create(handsPerTeam) {
    if (!Number.isSafeInteger(handsPerTeam) ||
        handsPerTeam < 1 || handsPerTeam > 1000) {
      throw new RangeError("Invalid match length");
    }

    const result = await this.request(
      "POST", "/api/v1/matches", { handsPerTeam }
    );

    this.sessionGeneration++;
    this.matchId = result.matchId;
    this.seatToken = result.hostToken;
    this.seat = 0;

    return {
      inviteToken: result.inviteToken,
      matchId: result.matchId,
      snapshot: result.snapshot,
    };
  }

  async join(matchId, inviteToken) {
    const path = MultiplayerClient.matchPath(matchId);

    if (typeof inviteToken !== "string" ||
        inviteToken.length === 0) {
      throw new TypeError("Invalid invitation");
    }

    const result = await this.request(
      "POST", `${path}/join`, { inviteToken }
    );

    this.sessionGeneration++;
    this.matchId = matchId;
    this.seatToken = result.guestToken;
    this.seat = 1;

    return result.snapshot;
  }

  requireSession() {
    if (!this.matchId || !this.seatToken) {
      throw new Error("No active multiplayer session");
    }

    return MultiplayerClient.matchPath(this.matchId);
  }

  async snapshot() {
    return this.request(
      "GET",
      this.requireSession(),
      undefined,
      this.seatToken
    );
  }

  async roll(expectedRevision, requestId) {
    if (!Number.isSafeInteger(expectedRevision) ||
        expectedRevision < 0) {
      throw new RangeError("Invalid match revision");
    }

    if (typeof requestId !== "string" ||
        requestId.length < 1 ||
        requestId.length > 128) {
      throw new TypeError("Invalid request identifier");
    }

    return this.request(
      "POST",
      `${this.requireSession()}/roll`,
      { requestId, expectedRevision },
      this.seatToken
    );
  }

  remember() {
    this.requireSession();

    if (!this.storage) {
      throw new Error("Memorizzazione browser non disponibile");
    }

    const record = {
      formatVersion: 1,
      matchId: this.matchId,
      seat: this.seat,
      seatToken: this.seatToken,
    };

    this.storage.setItem(RECOVERY_KEY, JSON.stringify(record));
  }

  forget() {
    this.storage?.removeItem(RECOVERY_KEY);
  }

  async restore() {
    if (!this.storage) return null;

    const generation = this.sessionGeneration;
    let saved;
    try {
      saved = this.storage.getItem(RECOVERY_KEY);
    } catch {
      return null;
    }
    if (saved === null) return null;

    let record;
    try {
      record = JSON.parse(saved);
    } catch {
      this.forget();
      return null;
    }

    const valid = record &&
      typeof record === "object" &&
      !Array.isArray(record) &&
      Object.keys(record).sort().join(",") ===
        "formatVersion,matchId,seat,seatToken" &&
      record.formatVersion === 1 &&
      (record.seat === 0 || record.seat === 1) &&
      typeof record.seatToken === "string" &&
      record.seatToken.length >= 1 &&
      record.seatToken.length <= 256 &&
      record.seatToken.trim() === record.seatToken;

    let path;
    try {
      if (!valid) throw new TypeError("Invalid recovery record");
      path = MultiplayerClient.matchPath(record.matchId);
    } catch {
      this.forget();
      return null;
    }

    let snapshot;
    try {
      snapshot = await this.request(
        "GET", path, undefined, record.seatToken
      );
    } catch (error) {
      if (
        error instanceof MultiplayerAPIError &&
        (error.status === 401 || error.status === 403 ||
         error.status === 404)
      ) {
        if (generation !== this.sessionGeneration) return null;
        this.forget();
        return null;
      }
      // Network failures and server outages must not destroy credentials.
      throw error;
    }

    if (generation !== this.sessionGeneration) return null;

    if (
      !snapshot ||
      snapshot.seat !== record.seat ||
      !Number.isSafeInteger(snapshot.revision) ||
      snapshot.revision < 0 ||
      !["waiting", "active", "completed"].includes(snapshot.status) ||
      !snapshot.state
    ) {
      this.forget();
      return null;
    }

    if (generation !== this.sessionGeneration) {
      return null;
    }

    this.sessionGeneration++;
    this.matchId = record.matchId;
    this.seatToken = record.seatToken;
    this.seat = record.seat;
    return snapshot;
  }

  leave() {
    this.sessionGeneration++;
    try {
      this.forget();
    } finally {
      this.matchId = null;
      this.seatToken = null;
      this.seat = null;
    }
  }
}
