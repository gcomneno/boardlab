import { FACES } from "./engine.mjs";
import { rollEventKind } from "./roll-event.mjs";
import {
  advanceCupIfReady,
  resolveCupTie,
  validateCupProgression,
} from "./cup.mjs";

export class CupShootoutError extends TypeError {
  constructor(code, cause) {
    super(code, cause === undefined ? undefined : { cause });
    this.name = "CupShootoutError";
    this.code = code;
  }
}

function requireState(condition) {
  if (!condition) throw new CupShootoutError("CUP_SHOOTOUT_INVALID_STATE");
}

function plain(value) {
  return value !== null && typeof value === "object" &&
    [Object.prototype, null].includes(Object.getPrototypeOf(value));
}

function shape(value, keys) {
  return plain(value) && Object.keys(value).length === keys.length &&
    keys.every(key => Object.hasOwn(value, key));
}

function text(value) {
  return typeof value === "string" && value.trim().length > 0;
}

function positive(value) {
  return Number.isSafeInteger(value) && value > 0;
}

// Every prefix is authoritative: initial clinches may have unequal counts;
// sudden death can clinch only at the end of an equal-attempt pair.
function projection(attemptsByTeam, goals) {
  const { home, away } = attemptsByTeam;
  let winner = null;
  if (home <= 5 && away <= 5) {
    if (goals.home > goals.away + (5 - away)) winner = "home";
    if (goals.away > goals.home + (5 - home)) winner = "away";
  } else if (home === away && goals.home !== goals.away) {
    winner = goals.home > goals.away ? "home" : "away";
  }
  return {
    status: winner ? "complete" :
      home >= 5 && away >= 5 ? "sudden-death" : "initial",
    attemptsByTeam,
    goals,
    nextTeam: winner ? null : home === away ? "home" : "away",
    winner,
  };
}

function increment(value) {
  requireState(Number.isSafeInteger(value) && value < Number.MAX_SAFE_INTEGER);
  return value + 1;
}

export function replayCupShootout(session) {
  requireState(plain(session));
  if (session.schemaVersion !== 1 || session.rulesVersion !== "extended-v1") {
    throw new CupShootoutError("CUP_SHOOTOUT_UNSUPPORTED_VERSION");
  }
  requireState(shape(session, [
    "schemaVersion", "rulesVersion", "shootoutId", "round", "pair",
    "homeProviderTeamId", "awayProviderTeamId", "fixtureRefs", "firstTeam", "status", "attempts",
  ]) && positive(session.round) && positive(session.pair) &&
    text(session.homeProviderTeamId) && text(session.awayProviderTeamId) &&
    session.homeProviderTeamId !== session.awayProviderTeamId &&
    session.firstTeam === "home" && Array.isArray(session.attempts) &&
    ["initial", "sudden-death", "complete"].includes(session.status));

  let identity;
  try { identity = JSON.parse(session.shootoutId); } catch { requireState(false); }
  const key = `r${session.round}-p${session.pair}`;
  requireState(Array.isArray(identity) && identity.length === 2 &&
    text(identity[0]) && identity[1] === key &&
    session.shootoutId === JSON.stringify(identity));
  requireState(Array.isArray(session.fixtureRefs) &&
    [1, 2].includes(session.fixtureRefs.length));
  const matchIds = new Set();
  for (let index = 0; index < session.fixtureRefs.length; index += 1) {
    const ref = session.fixtureRefs[index];
    requireState(shape(ref, ["fixtureId", "matchId"]) && ref.fixtureId === `${key}-l${index + 1}` &&
      text(ref.matchId) && !matchIds.has(ref.matchId));
    matchIds.add(ref.matchId);
  }

  const counts = { home: 0, away: 0 };
  const goals = { home: 0, away: 0 };
  let state = projection(counts, goals);
  for (let offset = 0; offset < session.attempts.length; offset += 1) {
    const attempt = session.attempts[offset];
    const index = increment(offset);
    requireState(state.status !== "complete" &&
      shape(attempt, ["attemptId", "index", "team", "face", "kind"]) &&
      attempt.index === index &&
      attempt.attemptId === JSON.stringify([...identity, index]) &&
      attempt.team === state.nextTeam && FACES.penalty.includes(attempt.face) &&
      attempt.kind === rollEventKind("penalty", attempt.face));
    counts[attempt.team] = increment(counts[attempt.team]);
    if (attempt.kind === "penalty-goal") goals[attempt.team] = increment(goals[attempt.team]);
    state = projection(counts, goals);
  }
  requireState(session.status === state.status);
  return state;
}

export function applyCupShootoutFace(session, { attemptId, expectedRevision, face } = {}) {
  const state = replayCupShootout(session);
  if (state.status === "complete") throw new CupShootoutError("CUP_SHOOTOUT_DECIDED");
  const index = increment(session.attempts.length);
  if (expectedRevision !== session.attempts.length ||
      attemptId !== JSON.stringify([...JSON.parse(session.shootoutId), index])) {
    throw new CupShootoutError("CUP_SHOOTOUT_STALE");
  }
  requireState(FACES.penalty.includes(face));
  const next = structuredClone(session);
  next.attempts.push({
    attemptId, index, team: state.nextTeam, face,
    kind: rollEventKind("penalty", face),
  });
  const counts = { ...state.attemptsByTeam };
  counts[state.nextTeam] = increment(counts[state.nextTeam]);
  const goals = { ...state.goals };
  if (face === "GOAL!") goals[state.nextTeam] = increment(goals[state.nextTeam]);
  next.status = projection(counts, goals).status;
  return next;
}

function pairing(document, key) {
  requireState(/^r[1-9]\d*-p[1-9]\d*$/.test(key));
  const fixtures = document.fixtures.filter(f => `r${f.round}-p${f.pair}` === key)
    .sort((a, b) => a.leg - b.leg);
  requireState(fixtures.length === document.legs && fixtures.every((f, i) =>
    f.fixtureId === `${key}-l${i + 1}` && f.leg === i + 1 && text(f.matchId)));
  requireState(fixtures.length === 1 ||
    (fixtures[1].homeProviderTeamId === fixtures[0].awayProviderTeamId &&
     fixtures[1].awayProviderTeamId === fixtures[0].homeProviderTeamId));
  return fixtures;
}

// Shape/identity checks are usable without match records. Supplying matches
// additionally proves aggregate eligibility and the current readiness gate.
export function validateCupShootouts(document, matches) {
  const results = document.shootouts ?? {};
  const sessions = document.shootoutSessions ?? {};
  requireState((document.shootouts === undefined || plain(document.shootouts)) &&
    (document.shootoutSessions === undefined || plain(document.shootoutSessions)));
  for (const [key, result] of Object.entries(results)) {
    const fixtures = pairing(document, key);
    requireState(plain(result) && Object.keys(result).length === 2 &&
      Object.hasOwn(result, "home") && Object.hasOwn(result, "away") &&
      Number.isSafeInteger(result.home) && result.home >= 0 &&
      Number.isSafeInteger(result.away) && result.away >= 0 && result.home !== result.away);
    if (matches !== undefined) resolveCupTie({ fixtures, matches, penalties: result });
  }
  let unfinished = null;
  for (const [key, session] of Object.entries(sessions)) {
    const state = replayCupShootout(session);
    const fixtures = pairing(document, key);
    requireState(key === `r${session.round}-p${session.pair}` &&
      session.shootoutId === JSON.stringify([document.cupId, key]) &&
      session.homeProviderTeamId === fixtures[0].homeProviderTeamId &&
      session.awayProviderTeamId === fixtures[0].awayProviderTeamId &&
      session.fixtureRefs.length === fixtures.length &&
      session.fixtureRefs.every((ref, i) => ref.fixtureId === fixtures[i].fixtureId &&
        ref.matchId === fixtures[i].matchId));
    if (matches !== undefined) {
      requireState(resolveCupTie({ fixtures, matches }).decidedBy === "pending-penalties");
    }
    if (state.status === "complete") {
      requireState(Object.hasOwn(results, key) &&
        results[key].home === state.goals.home && results[key].away === state.goals.away);
    } else {
      requireState(unfinished === null && !Object.hasOwn(results, key) &&
        document.championProviderTeamId === undefined &&
        session.round === Math.max(...document.fixtures.map(f => f.round)) &&
        document.fixtures.filter(f => f.round === session.round).every(f => text(f.matchId)));
      unfinished = session;
    }
  }
  if (unfinished && matches !== undefined) {
    const ready = advanceCupIfReady(document, matches, results);
    requireState(ready.status === "pending-penalties" &&
      ready.round === unfinished.round && ready.pair === unfinished.pair);
  }
}

export function createCupShootout(document, matches, round, pair) {
  requireState(document?.schemaVersion === 1 && positive(round) && positive(pair));
  validateCupProgression(document, matches, document.shootouts ?? {});
  validateCupShootouts(document, matches);
  const key = `r${round}-p${pair}`;
  if (Object.hasOwn(document.shootouts ?? {}, key)) {
    throw new CupShootoutError("CUP_SHOOTOUT_DECIDED");
  }
  const ready = advanceCupIfReady(document, matches, document.shootouts ?? {});
  if (ready.status !== "pending-penalties" || ready.round !== round || ready.pair !== pair ||
      Object.values(document.shootoutSessions ?? {}).some(s => s.status !== "complete")) {
    throw new CupShootoutError("CUP_SHOOTOUT_NOT_ELIGIBLE");
  }
  const fixtures = pairing(document, key);
  return {
    schemaVersion: 1,
    rulesVersion: "extended-v1",
    shootoutId: JSON.stringify([document.cupId, key]),
    round, pair,
    homeProviderTeamId: fixtures[0].homeProviderTeamId,
    awayProviderTeamId: fixtures[0].awayProviderTeamId,
    fixtureRefs: fixtures.map(({ fixtureId, matchId }) => ({ fixtureId, matchId })),
    firstTeam: "home",
    status: "initial",
    attempts: [],
  };
}
