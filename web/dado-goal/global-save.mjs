import { newMatch, applyRoll, FACES } from "./engine.mjs";
import { normalizeRollEvents, rollEventKind } from "./roll-event.mjs";
import { importManagementJson } from "./team-management-store.mjs";
import { importManagerJson } from "./manager-store.mjs";
import { importTournamentJson } from "./tournament-store.mjs";
import { championshipStandings } from "./tournament.mjs";
import { verifyCupDocument } from "./cup-store.mjs";
import { resolveCupTie } from "./cup.mjs";
import { createMatchRecord } from "./team-history.mjs";
import { createPlayer } from "./team-catalogue.mjs";
import { RATING_FIELDS, createPlayerRatings } from "./player-ratings.mjs";
import { extraordinaryEventCatalogue } from "./extraordinary-events.mjs";

export const GLOBAL_SAVE_SCHEMA_VERSION = 1;

export class GlobalSaveError extends TypeError {
  constructor(path, reason = "invalid value") {
    super(`${path}: ${reason}`);
    this.name = "GlobalSaveError";
    this.path = path;
  }
}

const own = (value, key) => Object.hasOwn(value, key);
const object = value => value !== null && typeof value === "object" && !Array.isArray(value);
const text = value => typeof value === "string" && value.trim().length > 0;
const natural = value => Number.isSafeInteger(value) && value >= 0;
const identifier = value => text(value) || (natural(value) && value > 0);
const color = value => typeof value === "string" && /^#[0-9a-f]{6}$/i.test(value);
const seat = value => value === 0 || value === 1;
const shirt = value => Number.isSafeInteger(value) && value > 0;
const competitions = ["135:2024", "136:2024", "39:2024", "40:2024", "61:2024",
  "78:2024", "140:2024", "79:2024", "62:2024", "141:2024"];
const extraordinary = new Map(extraordinaryEventCatalogue.map(event => [event.type, event]));

function check(condition, path, reason) {
  if (!condition) throw new GlobalSaveError(path, reason);
}
function pair(value, path, predicate) {
  check(Array.isArray(value) && value.length === 2, path, "expected two seats");
  value.forEach((item, index) => check(predicate(item), `${path}[${index}]`));
}
function uniqueArray(value, path, predicate) {
  check(Array.isArray(value) && value.every(predicate) && new Set(value).size === value.length, path);
}
function freeze(value) {
  if (value && typeof value === "object") {
    Object.values(value).forEach(freeze);
    Object.freeze(value);
  }
  return value;
}

// Accept only JSON data; do not invoke accessors/toJSON or silently coerce corrupt
// numbers, undefined, sparse arrays, Sets, Dates or cyclic object input.
function copyJson(value, path = "$", ancestors = new Set()) {
  if (value === null || typeof value === "string" || typeof value === "boolean") return value;
  if (typeof value === "number") {
    check(Number.isFinite(value), path);
    return value;
  }
  check(object(value) || Array.isArray(value), path, "expected JSON data");
  check([Object.prototype, Array.prototype, null].includes(Object.getPrototypeOf(value)) &&
    !ancestors.has(value), path, "expected acyclic plain JSON data");
  ancestors.add(value);
  const result = Array.isArray(value) ? [] : {};
  const keys = Object.keys(value);
  if (Array.isArray(value)) check(keys.length === value.length &&
    keys.every((key, i) => key === String(i)), path, "sparse or decorated array");
  check(Reflect.ownKeys(value).length === keys.length + (Array.isArray(value) ? 1 : 0), path);
  for (const key of keys) {
    const descriptor = Object.getOwnPropertyDescriptor(value, key);
    check(own(descriptor, "value"), `${path}.${key}`, "accessor is not JSON data");
    Object.defineProperty(result, key, {
      value: copyJson(descriptor.value, `${path}.${key}`, ancestors),
      enumerable: true, writable: true, configurable: true,
    });
  }
  ancestors.delete(value);
  return result;
}

function player(value, path) {
  check(object(value), path);
  createPlayer(value);
  if (own(value, "shirtNumber")) check(value.shirtNumber === null || shirt(value.shirtNumber), path);
}
function team(value, path, requirePlayers = false, participant = false) {
  check(object(value) && identifier(value.providerTeamId) &&
    ((participant && !own(value, "name")) || text(value.name)) &&
    ((participant && !own(value, "provider")) || text(value.provider)), path);
  if (requirePlayers || own(value, "players")) {
    check(Array.isArray(value.players) && (!requirePlayers || value.players.length >= 11), `${path}.players`);
    const ids = new Set();
    value.players.forEach((entry, i) => {
      player(entry, `${path}.players[${i}]`);
      const id = String(entry.providerPlayerId);
      check(!ids.has(id), path, "duplicate player ID");
      ids.add(id);
    });
  }
}
function identity(value, path) {
  if (value !== null) player(value, path);
}
function event(value, path) {
  check(object(value) && seat(value.team) && natural(value.minute) && value.minute <= 90, path);
  const narrative = extraordinary.get(value.type);
  check(narrative || ["goal", "penalty", "yellow", "red", "substitution"].includes(value.type), path);
  if (narrative) {
    check(value.family === narrative.family && value.classification === "narrative", path);
    if (narrative.requiresPlayer) check(shirt(value.number), path);
  } else if (value.type === "substitution") {
    check(shirt(value.out) && shirt(value.in) && value.out !== value.in &&
      ["injury", "tactical"].includes(value.reason), path);
  } else {
    check(value.number === null || shirt(value.number), `${path}.number`);
    if (["yellow", "red"].includes(value.type)) check(shirt(value.number) && text(value.reason), path);
    if (value.type === "penalty") check(FACES.penalty.includes(value.outcome), path);
  }
  if (own(value, "assistNumber")) check(shirt(value.assistNumber) && value.assistNumber !== value.number, path);
  for (const key of ["player", "outPlayer", "inPlayer", "assistPlayer"]) {
    if (own(value, key)) identity(value[key], `${path}.${key}`);
  }
}
function events(values, path) {
  check(Array.isArray(values), path);
  values.forEach((entry, i) => event(entry, `${path}[${i}]`));
}
function rolls(values, path) {
  check(Array.isArray(values) && values.every(value => object(value) && text(value.face)), path);
  // The domain normalizer deliberately accepts translated historical faces when
  // a valid semantic kind is already present. Never reinterpret that kind.
  return normalizeRollEvents(values);
}
function ratings(values, path) {
  check(Array.isArray(values), path);
  values.forEach((entry, i) => {
    check(object(entry), `${path}[${i}]`);
    const fields = own(entry, "ratings") ? entry.ratings : entry;
    check(object(fields), path);
    for (const key of RATING_FIELDS) {
      if (own(fields, key)) check(fields[key] !== null, `${path}.${key}`);
    }
    createPlayerRatings(fields);
  });
}
function record(value, path) {
  check(object(value) && text(value.matchId) &&
    (!own(value, "schemaVersion") || value.schemaVersion === 1), path);
  team(value.home, `${path}.home`);
  team(value.away, `${path}.away`);
  check(String(value.home.providerTeamId) !== String(value.away.providerTeamId), path);
  check(object(value.score), `${path}.score`);
  createMatchRecord({
    ...value, homeGoals: value.score.home, awayGoals: value.score.away,
  });
  if (own(value, "events")) events(value.events, `${path}.events`);
  if (own(value, "rollEvents")) rolls(value.rollEvents, `${path}.rollEvents`);
  if (own(value, "ratings")) ratings(value.ratings, `${path}.ratings`);
  if (own(value, "statistics")) {
    const stats = value.statistics;
    check(object(stats) || (Array.isArray(stats) && stats.length === 2), `${path}.statistics`);
    for (const row of Array.isArray(stats) ? stats : [stats]) {
      check(object(row) && Object.values(row).every(natural), `${path}.statistics`);
    }
  }
  if (own(value, "participation")) {
    check(Array.isArray(value.participation), `${path}.participation`);
    const ids = new Set();
    for (const entry of value.participation) {
      check(object(entry) && identifier(entry.providerPlayerId) && text(entry.name) &&
        ["starter", "bench", "not-selected"].includes(entry.selection) &&
        natural(entry.minutesPlayed) && entry.minutesPlayed <= 90 &&
        [entry.enteredMinute, entry.exitedMinute].every(at => at === null || (natural(at) && at <= 90)) &&
        [null, "red", "injury", "substitution"].includes(entry.exitReason) &&
        !ids.has(String(entry.providerPlayerId)), `${path}.participation`);
      ids.add(String(entry.providerPlayerId));
    }
  }
}
function management(value, path) {
  importManagementJson(JSON.stringify(value));
  ratings(value.ratings, `${path}.ratings`);
  const index = new Map();
  value.matches.forEach((entry, i) => {
    record(entry, `${path}.matches[${i}]`);
    check(!index.has(entry.matchId), path, "duplicate canonical match ID");
    index.set(entry.matchId, entry);
  });
  return index;
}
function sameSport(a, b) {
  return ["home", "away"].every(side =>
    String(a[side].providerTeamId) === String(b[side].providerTeamId) &&
    a[side].provider === b[side].provider && a.score[side] === b.score[side]);
}

function graph(persistent) {
  const matches = management(persistent.management, "persistent.management");
  const manager = persistent.manager;
  if (manager !== null) {
    importManagerJson(JSON.stringify(manager));
    team(manager.club, "manager.club", true);
    const playerIds = manager.club.players.map(p => String(p.providerPlayerId));
    check(new Set(playerIds).size === playerIds.length, "manager.club.players");
    const career = management(manager.management, "manager.management");
    for (const [id, entry] of career) {
      if (matches.has(id)) check(sameSport(entry, matches.get(id)), "manager.management", "conflicting canonical mirror");
    }
    if (!own(manager, "nextMatch")) manager.nextMatch = null;
  }
  const tournament = persistent.tournament;
  if (tournament !== null) {
    importTournamentJson(JSON.stringify(tournament));
    tournament.participants.forEach((entry, i) => team(entry, `tournament.participants[${i}]`, false, true));
  }
  check(persistent.cup === null || tournament === null, "persistent", "incompatible current competitions");
  check(Array.isArray(persistent.cupArchive), "persistent.cupArchive");
  const cupIds = new Set();
  const cups = persistent.cup === null ? [] : [persistent.cup];
  cups.push(...persistent.cupArchive);
  for (const cup of cups) {
    verifyCupDocument(cup, persistent.management.matches);
    check(!cupIds.has(cup.cupId), "persistent.cupArchive", "duplicate Cup ID");
    cupIds.add(cup.cupId);
    cup.participants.forEach((entry, i) => team(entry, `cup.participants[${i}]`, false, true));
    // Also check completed ties in the last saved round: progression validates
    // preceding rounds, but must not advance this saved document to check it.
    const ties = new Map();
    for (const fixture of cup.fixtures) {
      const key = `r${fixture.round}-p${fixture.pair}`;
      if (!ties.has(key)) ties.set(key, []);
      ties.get(key).push(fixture);
    }
    for (const [key, fixtures] of ties) {
      if (fixtures.every(f => f.matchId !== null)) resolveCupTie({
        fixtures, matches: persistent.management.matches, penalties: cup.shootouts?.[key] ?? null,
      });
    }
  }
  for (const cup of persistent.cupArchive) {
    check(own(cup, "championProviderTeamId"), "persistent.cupArchive", "archived Cup is unfinished");
  }
  const owners = new Map();
  for (const competition of [tournament, ...cups].filter(Boolean)) {
    for (const fixture of competition.fixtures) {
      if (fixture.matchId === null) continue;
      const match = matches.get(fixture.matchId);
      check(match && !owners.has(fixture.matchId), "fixtures.matchId", "missing or multiply owned canonical match");
      check(String(match.home.providerTeamId) === String(fixture.homeProviderTeamId) &&
        String(match.away.providerTeamId) === String(fixture.awayProviderTeamId), "fixtures.matchId", "team orientation mismatch");
      owners.set(fixture.matchId, fixture.fixtureId);
    }
  }
  if (tournament !== null) {
    const standings = championshipStandings(tournament, persistent.management.matches);
    check(standings.every(row => Object.entries(row).every(([key, value]) =>
      key === "team" || (key === "goalDifference" ? Number.isSafeInteger(value) : natural(value)))),
    "tournament", "standings arithmetic overflow");
  }
  return matches;
}

function matchState(value, path) {
  check(object(value) && Number.isSafeInteger(value.handsPerTeam) && value.handsPerTeam >= 1 &&
    value.handsPerTeam <= 1000 && seat(value.player) &&
    ["team", "pink", "result", "penalty", "finished"].includes(value.phase), path);
  pair(value.hands, `${path}.hands`, n => natural(n) && n <= value.handsPerTeam);
  pair(value.goals, `${path}.goals`, (n) => natural(n));
  check(value.goals.every((n, i) => n <= value.hands[i]) &&
    value.hands[0] >= value.hands[1] && value.hands[0] - value.hands[1] <= 1 &&
    value.player === (value.hands[0] === value.hands[1] ? 0 : 1) &&
    (value.phase === "finished") === value.hands.every(n => n === value.handsPerTeam) &&
    (value.lastRoll === null || text(value.lastRoll)), path);
}
function sameState(a, b) {
  return a.handsPerTeam === b.handsPerTeam && a.player === b.player && a.phase === b.phase &&
    a.hands.every((n, i) => n === b.hands[i]) && a.goals.every((n, i) => n === b.goals[i]);
}
function replayRuntime(value) {
  let state = newMatch(value.state.handsPerTeam);
  for (const [index, roll] of value.rollEvents.entries()) {
    check(roll.seat === state.player && roll.die === state.phase, `runtime.rollEvents[${index}]`, "invalid engine prefix");
    // Replay the authoritative kind using the existing engine, including when
    // historical presentation text differs. This creates no new match rules.
    const face = FACES[roll.die].find(face => rollEventKind(roll.die, face) === roll.kind);
    const before = state;
    state = applyRoll(state, face);
    if (value.lastCommentary?.index === index) {
      check(sameState(before, value.lastCommentary.before) && sameState(state, value.lastCommentary.after),
        "runtime.lastCommentary", "commentary states disagree with engine history");
    }
  }
  check(sameState(state, value.state), "runtime.state", "state disagrees with engine history");
}
function roster(value, path) {
  check(object(value), path);
  const keys = ["active", "bench", "departed", "sentOff", "substitutesIn"];
  for (const key of keys) uniqueArray(value[key], `${path}.${key}`, shirt);
  const all = keys.slice(0, 4).flatMap(key => value[key]);
  check(new Set(all).size === all.length && value.active.length <= 11 && value.bench.length <= 9 &&
    value.active.length + value.sentOff.length === 11 &&
    natural(value.substitutions) && value.substitutions <= 5 &&
    value.departed.length === value.substitutions && value.substitutesIn.length === value.substitutions &&
    value.substitutesIn.every(n => value.active.includes(n) || value.departed.includes(n) || value.sentOff.includes(n)), path);
  if (own(value, "identities")) {
    check(object(value.identities), `${path}.identities`);
    for (const [key, entry] of Object.entries(value.identities)) {
      check(String(Number(key)) === key && shirt(Number(key)), `${path}.identities`);
      player(entry, `${path}.identities.${key}`);
    }
    check(all.every(n => own(value.identities, String(n))), `${path}.identities`, "missing player identity");
    const ids = Object.values(value.identities).map(p => String(p.providerPlayerId));
    check(new Set(ids).size === ids.length, `${path}.identities`, "duplicate player identity");
  }
}
function numericControl(value, path, min, max, integer = false) {
  check(typeof value === "string" && value.trim() !== "" && Number.isFinite(Number(value)) &&
    Number(value) >= min && Number(value) <= max && (!integer || Number.isSafeInteger(Number(value))), path);
}
function configuration(value, availableTeams) {
  check(object(value), "configuration");
  check(competitions.includes(value.competition) || availableTeams.some(team =>
    team.competitionId != null && team.season != null &&
    `${team.competitionId}:${team.season}` === value.competition), "configuration.competition");
  numericControl(value.hands, "configuration.hands", 1, 1000, true);
  if (!own(value, "randomHands")) value.randomHands = false;
  check(typeof value.randomHands === "boolean" && typeof value.commentaryEnabled === "boolean", "configuration");
  pair(value.teams, "configuration.teams", id => typeof id === "string");
  pair(value.colors, "configuration.colors", entry => object(entry) && color(entry.primary) &&
    color(entry.secondary) && typeof entry.secondaryEnabled === "boolean");
  check(object(value.auto) && ["all", "team-0", "team-1"].includes(value.auto.mode), "configuration.auto");
  if (value.auto.handLimit !== "all") numericControl(value.auto.handLimit, "configuration.auto.handLimit", 1, 1000, true);
  numericControl(value.auto.rollDelay, "configuration.auto.rollDelay", 0, 5);
  numericControl(value.auto.handDelay, "configuration.auto.handDelay", 0, 10);
  const resolved = value.teams.map(id => id === "" ? null : availableTeams.find(t =>
    String(t.providerTeamId) === id && Array.isArray(t.players)));
  check(resolved.every(t => t !== undefined), "configuration.teams", "unresolved team snapshot; supply catalogueTeams");
  resolved.forEach((t, i) => { if (t !== null) team(t, `configuration.teams[${i}]`, true); });
  return resolved;
}
function runtime(value, config, persistent, legacy, matches) {
  check(object(value), "runtime");
  pair(value.names, "runtime.names", text);
  pair(value.colors, "runtime.colors", color);
  pair(value.teamPalettes, "runtime.teamPalettes", p => p === null ||
    (Array.isArray(p) && p.length >= 1 && p.length <= 2 && p.every(color)));
  pair(value.selectedMatchTeams, "runtime.selectedMatchTeams", t => t === null || object(t));
  value.selectedMatchTeams.forEach((t, i) => { if (t !== null) team(t, `runtime.selectedMatchTeams[${i}]`, true); });
  check(Array.isArray(value.rosters) && value.rosters.length === 2, "runtime.rosters");
  value.rosters.forEach((r, i) => roster(r, `runtime.rosters[${i}]`));
  value.selectedMatchTeams.forEach((t, i) => {
    if (t !== null) {
      const identities = value.rosters[i].identities;
      check(object(identities) && Object.entries(identities).every(([number, p]) => {
        const expected = t.players[Number(number) - 1];
        return expected && String(expected.providerPlayerId) === String(p.providerPlayerId) && expected.provider === p.provider;
      }), `runtime.rosters[${i}].identities`, "roster disagrees with selected team");
    }
  });
  check(Array.isArray(value.refereeStates) && value.refereeStates.length === 2, "runtime.refereeStates");
  value.refereeStates.forEach((r, i) => {
    check(object(r), `runtime.refereeStates[${i}]`);
    uniqueArray(r.yellowed, `runtime.refereeStates[${i}].yellowed`, shirt);
    const roster = value.rosters[i];
    check(r.yellowed.every(n => ["active", "bench", "departed", "sentOff"].some(key => roster[key].includes(n))),
      `runtime.refereeStates[${i}].yellowed`, "unknown roster number");
  });
  check(object(value.extraordinaryState), "runtime.extraordinaryState");
  uniqueArray(value.extraordinaryState.occurredFamilies, "runtime.extraordinaryState.occurredFamilies",
    v => ["weather", "external", "technical"].includes(v));
  uniqueArray(value.extraordinaryState.occurredTypes, "runtime.extraordinaryState.occurredTypes", v => extraordinary.has(v));
  check(value.extraordinaryState.occurredTypes.every(type => {
    const family = extraordinary.get(type).family;
    return family === "injury" || value.extraordinaryState.occurredFamilies.includes(family);
  }) && value.extraordinaryState.occurredFamilies.every(family =>
    value.extraordinaryState.occurredTypes.some(type => extraordinary.get(type).family === family)), "runtime.extraordinaryState");
  value.rollEvents = rolls(value.rollEvents, "runtime.rollEvents");
  events(value.matchEvents, "runtime.matchEvents");
  check(typeof value.matchStarted === "boolean" && typeof value.matchRecorded === "boolean", "runtime");
  check(value.matchStarted || (value.rollEvents.length === 0 && value.matchEvents.length === 0), "runtime.matchStarted");
  check(value.state === null || object(value.state), "runtime.state");
  if (value.state === null) {
    check(!value.matchStarted && !value.matchRecorded && value.rollEvents.length === 0 && value.matchEvents.length === 0, "runtime.state");
    value.state = newMatch(Number(config.hands));
  } else matchState(value.state, "runtime.state");
  check(!value.matchRecorded || (value.matchStarted && value.state.phase === "finished"), "runtime.matchRecorded");
  for (const key of ["lastRefereeEvent", "lastExtraordinaryEvent"]) {
    if (value[key] !== null) event(value[key], `runtime.${key}`);
  }
  check(value.lastGoalScorer === null || shirt(value.lastGoalScorer), "runtime.lastGoalScorer");
  check(text(value.messageKey) && object(value.messageValues) && Object.values(value.messageValues).every(v =>
    typeof v === "string" || typeof v === "number"), "runtime.messageValues");
  if (value.lastCommentary !== null) {
    const c = value.lastCommentary;
    check(object(c) && natural(c.index) && c.index < value.rollEvents.length, "runtime.lastCommentary");
    rolls([c.event], "runtime.lastCommentary.event");
    matchState(c.before, "runtime.lastCommentary.before");
    matchState(c.after, "runtime.lastCommentary.after");
    pair(c.names, "runtime.lastCommentary.names", text);
  }
  replayRuntime(value);
  const projection = own(value, "activeTournamentFixtureId") ? value.activeTournamentFixtureId : null;
  check(projection === null || text(projection), "runtime.activeTournamentFixtureId");
  if (!own(value, "competitionRef")) {
    const candidates = [];
    if (projection !== null) {
      if (persistent.tournament?.fixtures.some(f => f.fixtureId === projection)) candidates.push({
        kind: "tournament", championshipId: persistent.tournament.championshipId, fixtureId: projection,
      });
      if (!legacy && persistent.cup?.fixtures.some(f => f.fixtureId === projection)) candidates.push({
        kind: "cup", cupId: persistent.cup.cupId, fixtureId: projection,
      });
      check(candidates.length === 1, "runtime.competitionRef", "cannot infer a unique current owner");
    }
    value.competitionRef = candidates[0] ?? null;
  }
  const ref = value.competitionRef;
  if (ref === null) {
    check(projection === null, "runtime.competitionRef", "contradictory fixture projection");
  } else {
    check(object(ref) && ["cup", "tournament"].includes(ref.kind) && text(ref.fixtureId), "runtime.competitionRef");
    const key = ref.kind === "cup" ? "cupId" : "championshipId";
    check(Object.keys(ref).length === 3 && text(ref[key]) && !(legacy && ref.kind === "cup"), "runtime.competitionRef");
    const owner = ref.kind === "cup" ? persistent.cup : persistent.tournament;
    check(owner && owner[key] === ref[key] && value.matchStarted &&
      (projection === null || projection === ref.fixtureId), "runtime.competitionRef", "wrong current owner or idle runtime");
    const fixture = owner.fixtures.find(f => f.fixtureId === ref.fixtureId);
    check(fixture, "runtime.competitionRef.fixtureId");
    const ids = [fixture.homeProviderTeamId, fixture.awayProviderTeamId].map(String);
    check(ids.every((id, i) => config.teams[i] === id &&
      String(value.selectedMatchTeams[i]?.providerTeamId) === id), "runtime.selectedMatchTeams", "fixture teams disagree");
    if (value.matchRecorded) {
      const match = matches.get(fixture.matchId);
      check(match && value.state.goals[0] === match.score.home && value.state.goals[1] === match.score.away,
        "runtime.matchRecorded", "runtime disagrees with canonical finished match");
    } else check(fixture.matchId === null, "runtime.matchRecorded", "fixture already recorded");
  }
  value.activeTournamentFixtureId = ref?.fixtureId ?? null;
  if (!own(value, "tournamentViewedMatchday")) value.tournamentViewedMatchday = null;
  const viewed = value.tournamentViewedMatchday;
  check(viewed === null || (Number.isSafeInteger(viewed) && viewed >= 1 &&
    (persistent.tournament ?? persistent.cup)?.fixtures.some(f => (f.matchday ?? f.round) === viewed)), "runtime.tournamentViewedMatchday");
}

/**
 * Pure V1 reader. `source` is JSON text or a plain JSON object. The frozen result
 * contains a complete replacement `document`, aliases for all persistent values,
 * normalized hydration `runtime`, and resolved `configurationTeams` snapshots.
 * Sets are deliberately kept as validated arrays so the plan is deeply immutable.
 *
 * `catalogueTeams` is optional detached, already available catalogue data used to
 * resolve controls absent from the portable team snapshots; no fetch is made.
 * `localState`, when supplied, must explicitly contain cup/cupArchive plus the
 * management (and optionally manager/tournament) needed to verify them. Non-null
 * readError, pendingAttempt, cupRecovery, tournamentRecovery or globalTransaction
 * block preparation. Omission of localState
 * means an empty destination for standalone validation, NOT permission to skip
 * the coordinator's authoritative read/recovery/ownership/generation recheck.
 * Reprepare under the persistence gate with that snapshot before any writes.
 */
export function prepareGlobalSave(source, options = {}) {
  const document = copyJson(typeof source === "string" ? JSON.parse(source) : source);
  check(object(options), "options");
  check(object(document) && document.schemaVersion === GLOBAL_SAVE_SCHEMA_VERSION &&
    object(document.persistent) && object(document.configuration) && object(document.runtime), "$", "invalid Global Save V1 envelope");
  check(["it", "en"].includes(document.locale) && text(document.savedAt), "locale/savedAt");
  const persistent = document.persistent;
  check(Object.keys(persistent).every(key =>
    ["management", "manager", "tournament", "cup", "cupArchive"].includes(key)), "persistent", "unsupported portable field");
  const cupPresent = own(persistent, "cup");
  check(cupPresent === own(persistent, "cupArchive"), "persistent", "partial Cup field presence");
  const legacy = !cupPresent;
  const local = own(options, "localState") ? copyJson(options.localState) : {
    cup: null, cupArchive: [], management: { schemaVersion: 1, matches: [], ratings: [] },
  };
  check(object(local) && own(local, "cup") && own(local, "cupArchive"), "localState", "explicit Cup state required");
  for (const key of ["readError", "pendingAttempt", "cupRecovery", "tournamentRecovery", "globalTransaction"]) {
    check(!own(local, key) || local[key] === null, `localState.${key}`, "resolve local blocker before preparing");
  }
  graph({ management: own(local, "management") ? local.management : { schemaVersion: 1, matches: [], ratings: [] },
    manager: own(local, "manager") ? local.manager : null,
    tournament: own(local, "tournament") ? local.tournament : null,
    cup: local.cup, cupArchive: local.cupArchive });
  if (legacy) {
    check(local.cup === null, "localState.cup", "legacy import is blocked by a current Cup");
    persistent.cup = null;
    persistent.cupArchive = local.cupArchive;
  }
  const matches = graph(persistent);
  const catalogue = own(options, "catalogueTeams") ? copyJson(options.catalogueTeams) : [];
  check(Array.isArray(catalogue), "catalogueTeams");
  catalogue.forEach((t, i) => team(t, `catalogueTeams[${i}]`, true));
  const availableTeams = [
    ...(Array.isArray(document.runtime.selectedMatchTeams) ? document.runtime.selectedMatchTeams.filter(Boolean) : []),
    ...(persistent.manager ? [persistent.manager.club] : []),
    ...(persistent.cup?.participants ?? []), ...(persistent.tournament?.participants ?? []),
    ...persistent.cupArchive.flatMap(cup => cup.participants),
    ...persistent.management.matches.flatMap(match => [match.home, match.away]), ...catalogue,
  ];
  const configurationTeams = configuration(document.configuration, availableTeams);
  runtime(document.runtime, document.configuration, persistent, legacy, matches);
  return freeze({
    legacy, document, configuration: document.configuration, configurationTeams,
    runtime: document.runtime, management: persistent.management, manager: persistent.manager,
    tournament: persistent.tournament, cup: persistent.cup, cupArchive: persistent.cupArchive,
  });
}
