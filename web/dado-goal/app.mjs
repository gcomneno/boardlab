import {
  autoAllowsTeam,
  autoQuotaReached,
  createAutoConfig,
  createInterruptibleDelay,
  remainingHands
} from "./auto-play.mjs";

import { FACES, applyRoll, newMatch, rollFace } from "./engine.mjs";
import { matchStatistics } from "./statistics.mjs";
import { createRollEvent, normalizeRollEvents } from "./roll-event.mjs";
import {
  managerPlayerStatistics,
} from "./player-statistics.mjs";
import { nextStatisticsSort, sortStatisticsRows } from "./statistics-sort.mjs";
import { simulateMatch } from "./match-simulator.mjs";
import { matchClock } from "./match-clock.mjs";
import { createRoster, pickScorer, pickAssist } from "./match-roster.mjs";
import { importCatalogueJson } from "./catalogue-json.mjs";
import { createMatchRoster } from "./catalogue-roster.mjs";
import {
  createManagerClub,
  managerClubMatchTeam,
  updateManagerSelection,
  updateManagerPlayers,
} from "./manager-profile.mjs";
import {
  createManagerDocument,
  exportManagerJson,
  importManagerJson,
  loadManager,
  saveManager,
  clearManager,
} from "./manager-store.mjs";
import { createMatchRecord, teamStatistics } from "./team-history.mjs";
import {
  CUP_STORAGE_KEY,
  createCupShootoutCoordinator,
  createCupDocument,
  loadVerifiedCup,
  saveVerifiedCup,
  loadCupRecovery,
  saveCupRecovery,
  clearCupRecovery,
  archiveCompletedCup,
  loadCupArchive,
  clearCup,
} from "./cup-store.mjs";
import { advanceCupIfReady } from "./cup.mjs";
import { CupShootoutError, replayCupShootout } from "./cup-shootout.mjs";
import { cupFixturePresentation } from "./cup-view.mjs";
import { recordCupShootout } from "./cup-store.mjs";
import { completeCupFixture } from "./cup.mjs";
import { managerMatchParticipation } from "./player-participation.mjs";
import {
  championshipStandings,
  completeFixture,
} from "./tournament.mjs";
import {
  clearTournament,
  clearTournamentRecovery,
  createTournamentDocument,
  exportTournamentJson,
  importTournamentJson,
  loadTournament,
  loadTournamentRecovery,
  saveTournament,
  saveTournamentRecovery,
} from "./tournament-store.mjs";
import {
  createPlayerRatings,
  RATING_FIELDS,
  RATING_LEVELS,
} from "./player-ratings.mjs";
import {
  clearManagement,
  createManagementDocument,
  exportManagementJson,
  importManagementJson,
  loadManagement,
  saveManagement,
} from "./team-management-store.mjs";
import {
  enrichMatchEvent,
  eventDisplayValues,
} from "./match-player-identity.mjs";
import {
  createEventState, evaluateHandEvent
} from "./match-events.mjs";
import {
  createExtraordinaryEventState,
  evaluateExtraordinaryEvent,
} from "./extraordinary-events.mjs";
import {
  describeExtraordinaryEvent,
} from "./extraordinary-commentary.mjs";
import {
  describeEventReason,
  describeRoll,
  describeMatchEvent,
  describeFinalWhistle
} from "./commentary.mjs";
import { DICE_ART, PENALTY_DICE_ART } from "./dice-art.mjs";
import {
  applyLocale,
  formatMessage,
  getLocale,
  initializeLocale,
  setLocale,
  translate,
} from "./i18n.mjs";
import { socialColors } from "./team-colors.mjs";

const byId = (id) => document.getElementById(id);


const DADO_GOAL_LOCAL_STORAGE_KEYS = Object.freeze([
  "dado-goal.tournament.v1",
  "dado-goal.tournament-recovery.v1",
  "dado-goal.manager.v1",
  "dado-goal.team-management.v1",
  "dado-goal.multiplayer-seat.v1",
]);

function resetLocalData() {
  const confirmed = window.confirm(
    "Azzera tutti i dati locali di Dado Goal? " +
    "Torneo, manager, gestione squadra e recovery saranno eliminati."
  );

  if (!confirmed) return;

  for (const key of DADO_GOAL_LOCAL_STORAGE_KEYS) {
    window.localStorage.removeItem(key);
  }

  window.location.reload();
}

const phaseNames = {
  team: "teamDie",
  pink: "pinkDie",
  result: "finalDie",
  penalty: "penaltyDie",
};

const phaseName = phase => translate(phaseNames[phase]);

const diceFaces = {
  team: [
    "Tiro in porta!", "Tiro in porta!",
    "Passaggio intercettato!", "Passaggio intercettato!",
    "Cross!", "Passaggio",
  ],
  pink: [
    "Palla sul fondo", "Mezza rovesciata",
    "Palla alta sulla traversa", "Palla intercettata",
    "Rovesciata", "Colpo di testa",
  ],
  result: [
    "Respinto!", "GOAL!", "FUORI!",
    "PALO/TRAVERSA", "RIGORE", "Parato!",
  ],
  penalty: [
    "GOAL!", "GOAL!", "GOAL!",
    "PALO/TRAVERSA", "PARATO/RESPINTO", "FUORI",
  ],
};

const dieFaceMessageKeys = Object.freeze({
  "Tiro in porta!": "faceShotOnTarget",
  "Passaggio intercettato!": "faceInterceptedPass",
  "Cross!": "faceCross",
  "Passaggio": "facePass",
  "Palla sul fondo": "faceWide",
  "Mezza rovesciata": "faceScissorKick",
  "Palla alta sulla traversa": "faceOverTheBar",
  "Palla intercettata": "faceBallIntercepted",
  "Rovesciata": "faceBicycleKick",
  "Colpo di testa": "faceHeader",
  "Respinto!": "faceBlocked",
  "GOAL!": "faceGoal",
  "FUORI!": "faceOut",
  "PALO/TRAVERSA": "faceWoodwork",
  "RIGORE": "facePenalty",
  "Parato!": "faceSaved",
  "PARATO/RESPINTO": "facePenaltySaved",
  "FUORI": "facePenaltyWide",
});

function displayDieFace(face) {
  const key = dieFaceMessageKeys[face];
  if (!key) throw new RangeError("Unknown die face");
  return translate(key);
}

function diceKind(phase, player) {
  if (phase === "penalty") return "penalty";
  return phase === "team"
    ? (player === 0 ? "yellow" : "blue")
    : phase === "pink" ? "pink" : "final";
}

const teamPalettes = [null, null];

function visualTeamPalette(player) {
  return teamPalettes[player] ?? [colors[player]];
}

function shade(hex, factor) {
  const channels = [1, 3, 5].map((i) =>
    Math.round(parseInt(hex.slice(i, i + 2), 16) * factor),
  );
  return "#" + channels.map((n) =>
    n.toString(16).padStart(2, "0")).join("");
}

const dieFaceTransforms = [
  "rotateY(0deg) translateZ(var(--die-half))",
  "rotateY(90deg) translateZ(var(--die-half))",
  "rotateY(180deg) translateZ(var(--die-half))",
  "rotateY(-90deg) translateZ(var(--die-half))",
  "rotateX(90deg) translateZ(var(--die-half))",
  "rotateX(-90deg) translateZ(var(--die-half))",
];

const dieFinalRotations = [
  "rotateX(0deg) rotateY(0deg)",
  "rotateX(0deg) rotateY(-90deg)",
  "rotateX(0deg) rotateY(-180deg)",
  "rotateX(0deg) rotateY(90deg)",
  "rotateX(-90deg) rotateY(0deg)",
  "rotateX(90deg) rotateY(0deg)",
];

function buildDie3d(phase, player) {
  const die = byId("die");
  die.classList.remove("kickoff-ball");
  const kind = diceKind(phase, player);
  const art = kind === "penalty" ? PENALTY_DICE_ART : DICE_ART[kind];

  if (!Array.isArray(art) || art.length !== 6) {
    throw new RangeError("Invalid 3D die faces");
  }

  const color = colors[player];
  die.style.removeProperty("--die-light");
  die.style.removeProperty("--die-dark");
  die.style.removeProperty("--die-ink");

  if (phase === "team") {
    die.style.setProperty("--die-light", color);
    die.style.setProperty("--die-dark", shade(color, 0.65));
    die.style.setProperty("--die-ink", shade(color, 0.28));
  }

  const cube = document.createElement("div");
  cube.className = "die-cube";

  art.forEach((svg, index) => {
    const face = document.createElement("div");
    face.className = `die-face die-face-${index}`;
    face.style.transform = dieFaceTransforms[index];
    face.innerHTML = svg;
    cube.append(face);
  });

  die.replaceChildren(cube);
  return cube;
}

function displayFace(phase, player, index) {
  if (!Number.isInteger(index) || index < 0 || index > 5) {
    throw new RangeError("Invalid die face");
  }

  const die = byId("die");
  const cube = buildDie3d(phase, player);
  cube.style.transform = dieFinalRotations[index];

  die.setAttribute(
    "aria-label",
    `${phaseName(phase)}: ${displayDieFace(diceFaces[phase][index])}`,
  );
}

let state;
let names = [];
let colors = [];
let isRolling = false;
let rollEvents = [];
let matchEvents = [];
let rosters = [];
let refereeStates = [];
let extraordinaryState;
let commentaryEnabled = true;
let lastCommentary = null;
let lastRefereeEvent = null;
let lastExtraordinaryEvent = null;
let lastGoalScorer = null;
let messageKey = "setupPrompt";
let messageValues = {};

function showMessage(key, values = {}) {
  messageKey = key;
  messageValues = values;
  byId("message").textContent = formatMessage(key, values);
}

let matchStarted = false;
let autoRunning = false;
let autoConfig = null;
const autoDelay = createInterruptibleDelay(
  window.setTimeout.bind(window),
  window.clearTimeout.bind(window)
);

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function render() {
  byId("message").textContent = formatMessage(messageKey, messageValues);

  // Keep events recorded, but hide their narrative when commentary is OFF.
  byId("match-events-list").closest(".match-events").hidden = !commentaryEnabled;

  // Rebuild presentation from canonical events on every locale change.
  byId("match-events-heading").textContent = translate("eventBoardHeading");
  byId("match-events-empty").textContent = translate("eventBoardEmpty");
  byId("match-events-empty").hidden = matchEvents.length > 0;

  const eventKeys = {
    goal: "eventGoal",
    penalty: "eventPenalty",
    yellow: "eventYellow",
    red: "eventRed",
    substitution: "eventSubstitution"
  };

  byId("match-events-list").replaceChildren(
    ...matchEvents.map(event => {
      const item = document.createElement("li");
      item.classList.add(event.team === 0 ? "event-home" : "event-away");
      const minute = document.createElement("time");
      minute.textContent = `${event.minute}′`;
      minute.className = "event-minute";

      const descriptionStack = document.createElement("span");
      descriptionStack.className = "event-description-stack";

      const description = document.createElement("span");
      const reason = describeEventReason(event.reason, getLocale());
      const eventDescription = event.type === "penalty"
        ? formatMessage("eventPenalty", {
            ...eventDisplayValues(event),
            outcome: displayDieFace(event.outcome)
          })
        : event.classification === "narrative"
          ? describeExtraordinaryEvent(event, getLocale())
              .replace(/^\d+′\s*/, "")
          : formatMessage(eventKeys[event.type], {
              ...eventDisplayValues(event),
              team: names[event.team],
              reason: ""
            });
      description.textContent = eventDescription;
      descriptionStack.append(description);

      if ((event.type === "yellow" || event.type === "red") && reason) {
        const reasonLine = document.createElement("span");
        reasonLine.className = "event-reason";
        reasonLine.textContent = `[${reason}]`;
        descriptionStack.append(reasonLine);
      }

      item.append(minute, descriptionStack);
      return item;
    })
  );
  const commentary = byId("commentary-text");
  commentary.textContent = !commentaryEnabled
    ? translate("commentaryDisabled")
    : lastCommentary
      ? [
        describeRoll(
          lastCommentary.event,
          lastCommentary.before,
          lastCommentary.after,
          lastCommentary.names,
          lastCommentary.index,
          getLocale()
        ),
        lastGoalScorer !== null
          ? formatMessage("goalScorerCall", { number: lastGoalScorer })
          : null,
        lastRefereeEvent
          ? describeMatchEvent(lastRefereeEvent, names, getLocale())
          : null,
        lastExtraordinaryEvent
          ? describeExtraordinaryEvent(
              lastExtraordinaryEvent,
              getLocale()
            )
          : null,
        lastCommentary.after.phase === "finished"
          ? describeFinalWhistle(
              lastCommentary.after,
              lastCommentary.names,
              getLocale()
            )
          : null
      ].filter(Boolean).join("\n")
      : translate("commentaryReady");
  byId("commentary-toggle").textContent = commentaryEnabled
    ? "🎙️ ON"
    : "🎙️ OFF";
  byId("commentary-toggle").setAttribute(
    "aria-pressed", String(commentaryEnabled)
  );
  const clock = matchClock(state);
  const footballMinute = `${Math.floor(clock.seconds / 60)}′`;
  byId("match-clock").textContent = clock.finished
    ? formatMessage("finalClock", { time: footballMinute })
    : formatMessage(clock.half === 1 ? "firstHalf" : "secondHalf",
      { time: footballMinute });
  byId("match-clock").classList.toggle("halftime", clock.halftime);
  for (let player = 0; player < 2; player += 1) {
    byId(`name-${player}`).textContent = names[player];
    byId(`goals-${player}`).textContent = state.goals[player];
    byId(`hands-${player}`).textContent =
      formatMessage("handsProgress", {
        done: state.hands[player], total: state.handsPerTeam
      });
    const panel = byId(`panel-${player}`);
    const palette = visualTeamPalette(player);
    panel.style.setProperty("--team-color", palette[0]);
    panel.style.setProperty("--team-color-secondary", palette[1] ?? palette[0]);
    panel.classList.toggle(
      "active",
      state.phase !== "finished" && state.player === player,
    );
  }

  for (let player = 0; player < 2; player += 1) {
    const guide = byId(`guide-team-${player}`);
    guide.textContent = names[player];
    guide.style.borderLeft = `5px solid ${colors[player]}`;
    guide.classList.toggle(
      "active",
      state.phase === "team" && state.player === player,
    );
  }

  byId("guide-pink").classList.toggle("active", state.phase === "pink");
  byId("guide-result").classList.toggle(
    "active",
    state.phase === "result" || state.phase === "penalty",
  );

  const [home, away] = matchStatistics(rollEvents);
  const metrics = [
    ["goals", "goals"],
    ["possession", "possession"],
    ["shots", "shots"],
    ["wide", "wide"],
    ["blocked", "blocked"],
    ["saves", "saves"],
    ["woodwork", "woodwork"],
    ["corners", "corners"],
    ["freeKicks", "freeKicks"],
    ["penalties", "penalties"],
  ];
  byId("match-stats").replaceChildren(
    ...metrics.map(([label, key]) => {
      const row = document.createElement("div");
      row.className = "stat-row stat-comparison";

      const values = document.createElement("div");
      values.className = "stat-values";

      const left = document.createElement("strong");
      left.textContent = key === "possession"
        ? `${home[key]}%`
        : String(home[key]);

      const name = document.createElement("span");
      name.textContent = translate(label);

      const right = document.createElement("strong");
      right.textContent = key === "possession"
        ? `${away[key]}%`
        : String(away[key]);

      values.append(left, name, right);

      const bars = document.createElement("div");
      bars.className = "stat-bars";
      bars.setAttribute("aria-hidden", "true");

      const total = home[key] + away[key];

      for (const [value, color] of [
        [home[key], colors[0]],
        [away[key], colors[1]],
      ]) {
        const track = document.createElement("div");
        track.className = "stat-track";

        const fill = document.createElement("div");
        fill.className = "stat-fill";
        fill.style.width = total === 0
          ? "0%"
          : `${(value / total) * 100}%`;
        fill.style.backgroundColor = color;

        track.append(fill);
        bars.append(track);
      }

      row.append(values, bars);
      return row;
    }),
  );

  byId("stats-team-0").textContent = names[0];
  byId("stats-team-1").textContent = names[1];

  const finished = state.phase === "finished";
  const playing = matchStarted && !finished;
  const friendlyPlaying = playing && activeTournamentFixtureId === null;

  const tournamentDisclosure = byId("tournament-disclosure");
  if (friendlyPlaying) tournamentDisclosure.open = false;
  tournamentDisclosure.querySelector("summary").setAttribute(
    "aria-disabled", String(friendlyPlaying)
  );

  // Match configuration is immutable from kick-off to the final whistle.
  byId("competition").disabled = playing;
  byId("tournament-create").disabled = playing;
  byId("tournament-close").disabled = playing;
  byId("manager-close").disabled = playing;
  byId("hands").disabled = playing;
  byId("random-hands").disabled = playing;

  for (const seat of [0, 1]) {
    byId(`team-${seat}`).disabled = playing;
    byId(`color-${seat}`).disabled = playing;
    byId(`color-${seat}-enabled`).disabled = playing;
    byId(`color-${seat}-secondary`).disabled =
      playing || !byId(`color-${seat}-enabled`).checked;
  }

  byId("roll").disabled = !playing || isRolling || autoRunning;
  byId("auto").disabled = !playing;
  byId("commentary-toggle").disabled = !playing;
  byId("auto").textContent = autoRunning ? "■ STOP" : "▶ AUTO";
  byId("auto").setAttribute("aria-pressed", String(autoRunning));
  const selectedTeamIds = [0, 1].map(
    seat => byId(`team-${seat}`).value
  );
  const sameSelectedTeam =
    selectedTeamIds[0] !== "" &&
    selectedTeamIds[0] === selectedTeamIds[1];

  const tournamentPlaying = playing &&
    activeTournamentFixtureId !== null;
  byId("new").disabled = isRolling || tournamentPlaying ||
    (!playing && sameSelectedTeam);
  byId("new").textContent = friendlyPlaying
    ? translate("cancelMatch")
    : "Gioca amichevole";

  const multiplayer = document.querySelector(".multiplayer-button");
  multiplayer.classList.toggle("is-disabled", playing);
  multiplayer.setAttribute("aria-disabled", String(playing));
  multiplayer.tabIndex = playing ? -1 : 0;
  byId("phase").textContent = finished
    ? translate("matchFinished")
    : phaseName(state.phase);
  byId("turn").textContent = finished
    ? translate("finalWhistle")
    : formatMessage("matchTurn", { name: names[state.player] });
  byId("roll").textContent = finished
    ? translate("matchFinished")
    : isRolling
      ? translate("rolling")
      : formatMessage("rollPhase", { phase: phaseName(state.phase) });
}

let footballCatalogue = null;
let selectedMatchTeams = [null, null];
let managementState = createManagementDocument();
let managerState = null;
let tournamentState = null;
let cupState = null;
let cupLoadError = null;
let cupLock = null;
let cupLockRequest = null;
let cupCoordinator = null;
let cupShootoutBusy = false;
let cupShootoutPendingSave = false;
let cupShootoutError = null;
let cupShootoutNotice = null;
let activeTournamentFixtureId = null;
let tournamentViewedMatchday = null;
let managerPlayerSorting = null;
let matchRecorded = false;

try {
  managementState = loadManagement(window.localStorage);
} catch {
  managementState = createManagementDocument();
}

try {
  managerState = loadManager(window.localStorage);
} catch {
  managerState = null;
}

try {
  tournamentState = loadTournament(window.localStorage);
} catch {
  tournamentState = null;
}

try {
  cupState = loadVerifiedCup(window.localStorage, managementState.matches);
} catch (failure) {
  cupLoadError = failure.code ?? "CUP_SHOOTOUT_INVALID_STATE";
}

// Cup recovery writes run only after acquiring the active Cup's Web Lock.
function reconcileCupRecovery() {
  requireCupOwnership();
  const recovery = loadCupRecovery(window.localStorage);

  if (recovery) {
    const canonicalMatch = managementState.matches.find(
      match => match.matchId === recovery.matchId
    );
    const sameCup = cupState?.cupId === recovery.cupId;
    const fixture = sameCup
      ? cupState.fixtures.find(
          candidate => candidate.fixtureId === recovery.fixtureId
        )
      : null;

    if (canonicalMatch && fixture?.matchId === null) {
      const homeMatches =
        String(canonicalMatch.home.providerTeamId) ===
        String(fixture.homeProviderTeamId);
      const awayMatches =
        String(canonicalMatch.away.providerTeamId) ===
        String(fixture.awayProviderTeamId);

      if (homeMatches && awayMatches) {
        const reconciled = completeCupFixture(
          cupState, recovery.fixtureId, recovery.matchId,
        );
        const advancement = advanceCupIfReady(
          reconciled,
          managementState.matches,
          reconciled.shootouts ?? {},
        );
        saveVerifiedCup(
          window.localStorage, advancement.cup, managementState.matches,
        );
        cupState = advancement.cup;
        clearCupRecovery(window.localStorage);
        if (activeTournamentFixtureId === recovery.fixtureId) activeTournamentFixtureId = null;
      } else {
        throw new CupShootoutError("CUP_SHOOTOUT_INVALID_STATE");
      }
    } else if (
      canonicalMatch &&
      fixture?.matchId === recovery.matchId
    ) {
      clearCupRecovery(window.localStorage);
      if (activeTournamentFixtureId === recovery.fixtureId) activeTournamentFixtureId = null;
    } else if (!canonicalMatch || !sameCup) {
      clearCupRecovery(window.localStorage);
    } else {
      // Do not enable new Cup writes over an unresolved canonical association.
      throw new CupShootoutError("CUP_SHOOTOUT_INVALID_STATE");
    }
  }
}

try {
  const recovery = loadTournamentRecovery(window.localStorage);

  if (recovery) {
    const canonicalMatchExists = managementState.matches.some(
      match => match.matchId === recovery.matchId
    );
    const sameChampionship =
      tournamentState?.championshipId === recovery.championshipId;
    const fixture = sameChampionship
      ? tournamentState.fixtures.find(
          candidate => candidate.fixtureId === recovery.fixtureId
        )
      : null;

    if (canonicalMatchExists && fixture?.matchId === null) {
      const reconciled = completeFixture(
        tournamentState,
        recovery.fixtureId,
        recovery.matchId,
      );
      saveTournament(window.localStorage, reconciled);
      tournamentState = reconciled;
      clearTournamentRecovery(window.localStorage);
    } else if (
      canonicalMatchExists &&
      fixture?.matchId === recovery.matchId
    ) {
      clearTournamentRecovery(window.localStorage);
    } else if (!canonicalMatchExists || !sameChampionship) {
      clearTournamentRecovery(window.localStorage);
    }
  }
} catch {
  // Keep recoverable persisted state untouched for a later reload.
}

function recordCompletedMatch() {
  if (matchRecorded || state.phase !== "finished") return;

  const completedTournamentFixture =
    tournamentState && activeTournamentFixtureId
      ? tournamentState.fixtures.find(
          fixture => fixture.fixtureId === activeTournamentFixtureId
        ) ?? null
      : null;
  const managedTournamentId = managerTournamentTeamId();
  const completedManagerMatchday =
    completedTournamentFixture &&
    managedTournamentId !== null &&
    fixtureIncludesTeam(completedTournamentFixture, managedTournamentId)
      ? completedTournamentFixture.matchday
      : null;

  // Management history is opt-in: at least one imported real team
  // must have been selected for this match.
  if (!selectedMatchTeams.some(Boolean)) return;

  const teamSnapshot = (team, seat) => team ?? {
    provider: "local",
    providerTeamId: `local-${seat}`,
    name: names[seat],
    players: [],
  };

  const managedParticipation =
    managerState
      ? (() => {
          const managedId = String(managerState.club.providerTeamId);
          const managedSeat = selectedMatchTeams.findIndex(team =>
            team &&
            String(team.providerTeamId) === managedId
          );

          return managedSeat < 0
            ? []
            : managerMatchParticipation(
                managerState.club,
                managedSeat,
                matchEvents,
              );
        })()
      : [];

  const record = createMatchRecord({
    matchId: crypto.randomUUID(),
    playedAt: new Date().toISOString(),
    home: teamSnapshot(selectedMatchTeams[0], 0),
    away: teamSnapshot(selectedMatchTeams[1], 1),
    homeGoals: state.goals[0],
    awayGoals: state.goals[1],
    events: matchEvents,
    rollEvents,
    participation: managedParticipation,
    statistics: matchStatistics(rollEvents),
    ratings: [],
  });

  const nextState = createManagementDocument({
    ratings: managementState.ratings,
    matches: [...managementState.matches, record],
  });

  // A completed match is processed at most once even when persistence
  // is unavailable. Generic and Manager saves remain independent.
  matchRecorded = true;

  let canonicalMatchPersisted = false;
  let tournamentRecoveryPrepared = false;
  let cupRecoveryPrepared = false;

  if (cupState && !tournamentState && activeTournamentFixtureId) {
    try {
      requireCupOwnership();
      saveCupRecovery(window.localStorage, {
        cupId: cupState.cupId,
        fixtureId: activeTournamentFixtureId,
        matchId: record.matchId,
      });
      cupRecoveryPrepared = true;
    } catch {
      // Never associate a cup fixture without durable recovery intent.
    }
  }

  if (tournamentState && activeTournamentFixtureId) {
    try {
      saveTournamentRecovery(window.localStorage, {
        championshipId: tournamentState.championshipId,
        fixtureId: activeTournamentFixtureId,
        matchId: record.matchId,
      });
      tournamentRecoveryPrepared = true;
    } catch {
      // Gameplay remains valid even when recovery persistence is unavailable.
    }
  }

  const cupPersistenceReady =
    !cupState || !activeTournamentFixtureId || cupRecoveryPrepared;

  if (cupPersistenceReady) {
    try {
      saveManagement(window.localStorage, nextState);
      managementState = nextState;
      canonicalMatchPersisted = true;
      markGlobalSaveDirty();
    } catch {
      // Local persistence failure must never affect gameplay.
    }
  }

  if (!canonicalMatchPersisted && cupRecoveryPrepared) {
    try {
      clearCupRecovery(window.localStorage);
    } catch {
      // Reload discards a journal without a canonical match.
    }
  }

  if (!canonicalMatchPersisted && tournamentRecoveryPrepared) {
    try {
      clearTournamentRecovery(window.localStorage);
    } catch {
      // A stale journal without a canonical match is discarded on reload.
    }
  }

  if (canonicalMatchPersisted &&
      tournamentState &&
      activeTournamentFixtureId) {
    try {
      const nextTournament = completeFixture(
        tournamentState,
        activeTournamentFixtureId,
        record.matchId,
      );
      saveTournament(window.localStorage, nextTournament);
      tournamentState = nextTournament;
      activeTournamentFixtureId = null;

      if (tournamentRecoveryPrepared) {
        clearTournamentRecovery(window.localStorage);
      }
    } catch {
      // A durable journal reconciles the canonical match on next reload.
    }
  }

  if (canonicalMatchPersisted &&
      cupState &&
      !tournamentState &&
      activeTournamentFixtureId &&
      cupRecoveryPrepared) {
    try {
      requireCupOwnership();
      const completedCup = completeCupFixture(
        cupState, activeTournamentFixtureId, record.matchId,
      );
      const advancement = advanceCupIfReady(
        completedCup,
        managementState.matches,
        completedCup.shootouts ?? {},
      );
      const nextCup = advancement.cup;
      saveVerifiedCup(
        window.localStorage, nextCup, managementState.matches,
      );
      cupState = nextCup;
      if (advancement.status === "advanced") {
        tournamentViewedMatchday = Math.max(
          ...nextCup.fixtures.map(fixture => fixture.round),
        );
      }
      activeTournamentFixtureId = null;
      clearCupRecovery(window.localStorage);
    } catch (failure) {
      // Offer the same lock-protected recovery without requiring a page reload.
      // The canonical match and its journal remain authoritative.
      cupLoadError = failure.code ?? "CUP_SHOOTOUT_STORAGE_UNAVAILABLE";
    }
  }

  if (
    canonicalMatchPersisted &&
    completedManagerMatchday !== null &&
    tournamentState
  ) {
    const remainingMatchdayFixtures = tournamentState.fixtures.filter(
      fixture =>
        fixture.matchId === null &&
        fixture.matchday === completedManagerMatchday &&
        !fixtureIncludesTeam(fixture, managedTournamentId)
    );

    for (const fixture of remainingMatchdayFixtures) {
      if (!simulateTournamentFixture(fixture)) break;
    }

    tournamentViewedMatchday = completedManagerMatchday;
  }

  if (managerState &&
      selectedMatchTeams.some(team =>
        team &&
        String(team.providerTeamId) ===
          String(managerState.club.providerTeamId)
      )) {
    try {
      managerState = createManagerDocument({
        club: managerState.club,
        management: createManagementDocument({
          ratings: managerState.management.ratings,
          matches: [...managerState.management.matches, record],
        }),
        nextMatch: null,
      });
      saveManager(window.localStorage, managerState);
    } catch {
      // Manager persistence failure must never affect gameplay.
    }
  }

  renderTournament();
  renderManagerDashboard();
}


function managerPlayerPosition(position) {
  if (!position) return "";

  const key = {
    Goalkeeper: "playerPositionGoalkeeper",
    Defender: "playerPositionDefender",
    Midfielder: "playerPositionMidfielder",
    Attacker: "playerPositionAttacker",
  }[position];

  return key ? translate(key) : position;
}

function managerPlayerLabel(player) {
  const shirt = player.shirtNumber == null ? "—" : player.shirtNumber;
  const position = player.position
    ? ` · ${managerPlayerPosition(player.position)}`
    : "";
  return `#${shirt} ${player.name}${position}`;
}

function managerClubMatches() {
  if (!managerState) return [];

  const teamId = String(managerState.club.providerTeamId);
  return managerState.management.matches.filter(match =>
    String(match.home?.providerTeamId) === teamId ||
    String(match.away?.providerTeamId) === teamId
  );
}

function renderManagerDashboard() {
  const managerSummary = byId("manager-disclosure").querySelector("summary");
  managerSummary.setAttribute("aria-disabled", "false");
  const club = managerState?.club ?? null;

  syncNewManagerControls();

  byId("manager-dashboard").hidden = !club;
  const emptyState = byId("manager-empty-state");
  emptyState.hidden = Boolean(club);
  if (!club) {
    const italian = document.documentElement.lang?.toLowerCase().startsWith("it");
    byId("manager-empty-title").textContent = italian
      ? "Inizia la tua carriera"
      : "Start your career";
    byId("manager-empty-description").textContent = italian
      ? "Scegli una squadra in Partita e premi +Manager per avviare la carriera."
      : "Choose a team in Match and press +Manager to start your career.";
  }


  if (!club) return;

  byId("manager-club-name").textContent = club.name;

  const matches = managerClubMatches();
  byId("manager-match-count").textContent = String(matches.length);

  const players = new Map(
    club.players.map(player => [String(player.providerPlayerId), player])
  );

  const renderPlayers = (elementId, playerIds) => {
    const list = byId(elementId);
    list.replaceChildren();

    for (const playerId of playerIds) {
      const player = players.get(String(playerId));
      if (!player) continue;

      const item = document.createElement("li");
      item.className = "manager-player-row";

      const identity = document.createElement("span");
      identity.className = "manager-player-identity";

      const number = document.createElement("span");
      number.className = "manager-shirt-number";
      number.textContent = player.shirtNumber == null
        ? "—"
        : String(player.shirtNumber);

      const name = document.createElement("span");
      name.className = "manager-player-name";
      name.textContent = player.name;

      identity.append(number, name);

      const role = document.createElement("span");
      role.className = "manager-player-role";

      const roleIcon = document.createElement("span");
      roleIcon.className = "manager-role-icon";
      roleIcon.setAttribute("aria-hidden", "true");
      roleIcon.textContent = {
        Goalkeeper: "◆",
        Defender: "⬟",
        Midfielder: "●",
        Attacker: "▲",
      }[player.position] ?? "•";

      const roleText = document.createElement("span");
      roleText.textContent = managerPlayerPosition(player.position);

      role.append(roleIcon, roleText);
      item.append(identity, role);
      list.append(item);
    }
  };

  renderPlayers("manager-lineup", club.lineup);
  renderPlayers("manager-bench", club.bench);

  const cumulativePlayers = managerPlayerStatistics(
    matches,
    club.providerTeamId,
    club.players,
  );

  byId("manager-player-statistics-title").textContent =
    translate("managerPlayerStatistics");

  const managerPlayerHeaders = {
    "manager-player-stat-player": "managerStatPlayer",
    "manager-player-stat-apps": "managerStatAppearances",
    "manager-player-stat-minutes": "managerStatMinutes",
    "manager-player-stat-starts": "managerStatStarts",
    "manager-player-stat-bench": "managerStatBenchUnused",
    "manager-player-stat-not-selected": "managerStatNotSelected",
    "manager-player-stat-goals": "managerStatGoals",
    "manager-player-stat-assists": "managerStatAssists",
    "manager-player-stat-penalties": "managerStatPenalties",
    "manager-player-stat-yellow": "managerStatYellow",
    "manager-player-stat-red": "managerStatRed",
    "manager-player-stat-on": "managerStatSubbedOn",
    "manager-player-stat-off": "managerStatSubbedOff",
    "manager-player-stat-injury": "managerStatInjuryOff",
  };

  for (const [elementId, key] of Object.entries(managerPlayerHeaders)) {
    const header = byId(elementId);
    const label = translate(key);
    const descriptions = {
      en: {
        managerStatAppearances: "Appearances",
        managerStatMinutes: "Minutes played",
        managerStatStarts: "Starting appearances",
        managerStatBenchUnused: "Unused substitute",
        managerStatNotSelected: "Not selected",
        managerStatGoals: "Goals",
        managerStatAssists: "Assists",
        managerStatPenalties: "Penalties",
        managerStatYellow: "Yellow cards",
        managerStatRed: "Red cards",
      },
      it: {
        managerStatAppearances: "Presenze",
        managerStatMinutes: "Minuti giocati",
        managerStatStarts: "Presenze da titolare",
        managerStatBenchUnused: "Panchina senza ingresso",
        managerStatNotSelected: "Non convocato",
        managerStatGoals: "Gol",
        managerStatAssists: "Assist",
        managerStatPenalties: "Rigori",
        managerStatYellow: "Cartellini gialli",
        managerStatRed: "Cartellini rossi",
      },
    };
    const locale = document.documentElement.lang === "en" ? "en" : "it";
    const fullLabel = key.endsWith("SubbedOn") ||
      key.endsWith("SubbedOff") ||
      key.endsWith("InjuryOff")
      ? translate(`${key}Tooltip`)
      : descriptions[locale][key] ?? label;

    header.title = fullLabel;
    header.setAttribute("aria-label", fullLabel);

    const iconName = {
      managerStatMinutes: "clock",
      managerStatStarts: "shirt-lineup",
      managerStatBenchUnused: "shirt-bench",
      managerStatNotSelected: "shirt-out",
      managerStatGoals: "ball",
      managerStatPenalties: "target",
      managerStatYellow: "yellow",
      managerStatRed: "red",
      managerStatSubbedOn: "on",
      managerStatSubbedOff: "off",
      managerStatInjuryOff: "injury",
    }[key];

    header.replaceChildren();
    if (iconName) {
      const icon = document.createElement("span");
      icon.className = `manager-stat-icon manager-stat-icon--${iconName}`;
      icon.setAttribute("aria-hidden", "true");
      header.append(icon);
    } else {
      header.textContent = label;
    }
  }

  const managerPlayerStatsBody = byId("manager-player-statistics");
  managerPlayerStatsBody.replaceChildren();

  const lineupIds = new Set(club.lineup.map(String));
  const benchIds = new Set(club.bench.map(String));
  const shirtStatusLabels = document.documentElement.lang === "en"
    ? {
        lineup: "Starting eleven",
        bench: "Substitute bench",
        out: "Outside match squad",
      }
    : {
        lineup: "Titolare",
        bench: "Panchina",
        out: "Fuori formazione",
      };

  const managerPlayerRows = new Map();
  for (const player of cumulativePlayers) {
    const row = document.createElement("tr");

    const identity = document.createElement("td");
    const number = player.number == null ? "—" : String(player.number);
    const shirt = document.createElement("span");
    const playerId = player.providerPlayerId == null
      ? null
      : String(player.providerPlayerId);
    const shirtStatus = playerId !== null && lineupIds.has(playerId)
      ? "lineup"
      : playerId !== null && benchIds.has(playerId)
        ? "bench"
        : "out";
    shirt.className = `manager-player-shirt manager-player-shirt--${shirtStatus}`;
    shirt.setAttribute("aria-hidden", "true");
    shirt.title = shirtStatusLabels[shirtStatus];
    const shirtNumber = document.createElement("span");
    shirtNumber.textContent = number;
    shirt.append(shirtNumber);

    const playerName = document.createElement("span");
    playerName.className = "manager-player-name";
    playerName.textContent = player.name || number;

    identity.classList.add("manager-player-identity");
    identity.setAttribute(
      "aria-label",
      `${player.name ? `${number} ${player.name}` : number} — ${shirtStatusLabels[shirtStatus]}`
    );
    identity.title = shirtStatusLabels[shirtStatus];
    identity.append(shirt, playerName);

    const values = [
      player.appearances,
      player.minutesPlayed,
      player.starts,
      player.benchUnused,
      player.notSelected,
      player.goals,
      player.assists,
      player.penalties,
      player.yellowCards,
      player.redCards,
      player.subbedOn,
      player.subbedOff,
      player.injurySubbedOff,
    ];

    row.append(identity);

    for (const value of values) {
      const cell = document.createElement("td");
      cell.textContent = String(value);
      row.append(cell);
    }

    managerPlayerRows.set(player, row);
    managerPlayerStatsBody.append(row);
  }

  const sortableColumns = [
    "name", "appearances", "minutesPlayed", "starts",
    "benchUnused", "notSelected", "goals", "assists", "penalties",
    "yellowCards", "redCards", "subbedOn", "subbedOff",
    "injurySubbedOff",
  ];

  const applyManagerPlayerSort = () => {
    const ordered = sortStatisticsRows(
      cumulativePlayers, managerPlayerSorting,
    );
    managerPlayerStatsBody.replaceChildren(
      ...ordered.map(player => managerPlayerRows.get(player)),
    );

    Object.keys(managerPlayerHeaders).forEach((id, index) => {
      const header = byId(id);
      const direction = managerPlayerSorting?.column ===
        sortableColumns[index]
        ? managerPlayerSorting.direction : null;

      if (direction) {
        header.setAttribute(
          "aria-sort",
          direction === "asc" ? "ascending" : "descending",
        );
      } else {
        header.removeAttribute("aria-sort");
      }

      header.querySelector(".manager-stat-sort-indicator")?.remove();
      if (direction) {
        const indicator = document.createElement("span");
        indicator.className = "manager-stat-sort-indicator";
        indicator.textContent = direction === "asc" ? " ↑" : " ↓";
        indicator.setAttribute("aria-hidden", "true");
        header.append(indicator);
      }
    });
  };

  Object.keys(managerPlayerHeaders).forEach((id, index) => {
    const header = byId(id);
    const column = sortableColumns[index];
    header.tabIndex = 0;
    header.style.cursor = "pointer";

    const toggleSort = () => {
      managerPlayerSorting = nextStatisticsSort(
        managerPlayerSorting, column,
      );
      applyManagerPlayerSort();
    };

    // Replace handlers on each render; never accumulate stale closures.
    header.onclick = toggleSort;
    header.onkeydown = event => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        toggleSort();
      }
    };
  });
  applyManagerPlayerSort();

  const career = teamStatistics(
    matches,
    club.providerTeamId,
  );

  const careerLabels = document.documentElement.lang === "en"
    ? {
        played: "Played",
        won: "Wins",
        drawn: "Draws",
        lost: "Losses",
        goalsFor: "Goals for",
        goalsAgainst: "Goals against",
      }
    : {
        played: "Giocate",
        won: "Vinte",
        drawn: "Pareggiate",
        lost: "Perse",
        goalsFor: "Gol fatti",
        goalsAgainst: "Gol subiti",
      };

  for (const key of [
    "played",
    "won",
    "drawn",
    "lost",
    "goalsFor",
    "goalsAgainst",
  ]) {
    byId(`manager-career-${key}-label`).textContent =
      careerLabels[key];
    byId(`manager-career-${key}`).textContent =
      String(career[key]);
  }

  const history = byId("manager-history");
  history.replaceChildren();

  for (const match of matches.slice(-5).reverse()) {
    const item = document.createElement("li");
    const managedHome =
      String(match.home.providerTeamId) === String(club.providerTeamId);
    const goalsFor = managedHome ? match.score.home : match.score.away;
    const goalsAgainst = managedHome ? match.score.away : match.score.home;
    const outcome = goalsFor > goalsAgainst
      ? (document.documentElement.lang === "en" ? "W" : "V")
      : goalsFor < goalsAgainst
        ? (document.documentElement.lang === "en" ? "L" : "S")
        : "P";

    item.textContent =
      `${outcome} · ${match.home.name} ` +
      `${match.score.home}–${match.score.away} ${match.away.name}`;
    history.append(item);
  }

  if (history.childElementCount === 0) {
    const item = document.createElement("li");
    item.textContent = "Nessuna partita registrata.";
    history.append(item);
  }
}

function persistManagerState() {
  if (!managerState) return;
  saveManager(window.localStorage, managerState);
  markGlobalSaveDirty();
  renderManagerDashboard();
}

function editManagerSelection() {
  const club = managerState?.club ?? null;
  if (!club) return;

  const currentLineup = new Set(club.lineup.map(String));
  const currentBench = new Set(club.bench.map(String));

  const dialog = document.createElement("dialog");
  dialog.className = "manager-editor";

  const form = document.createElement("form");
  form.method = "dialog";

  const title = document.createElement("h3");
  title.textContent = `Formazione · ${club.name}`;
  form.append(title);

  const hint = document.createElement("p");
  hint.textContent =
    "Seleziona esattamente 11 titolari e fino a 9 giocatori in panchina.";
  form.append(hint);

  const rows = document.createElement("div");
  rows.className = "manager-editor-players";

  const header = document.createElement("div");
  header.className = "manager-editor-player manager-editor-header";

  for (const caption of ["Giocatore", "N°", "Ruolo", "Impiego"]) {
    const cell = document.createElement("strong");
    cell.textContent = caption;
    header.append(cell);
  }

  rows.append(header);

  for (const player of club.players) {
    const row = document.createElement("div");
    row.className = "manager-editor-player";

    const playerId = String(player.providerPlayerId);

    const name = document.createElement("input");
    name.type = "text";
    name.value = player.name;
    name.dataset.playerName = playerId;
    name.setAttribute("aria-label", `Nome ${player.name}`);

    const shirtNumber = document.createElement("input");
    shirtNumber.type = "number";
    shirtNumber.min = "1";
    shirtNumber.max = "99";
    shirtNumber.step = "1";
    shirtNumber.value = player.shirtNumber ?? "";
    shirtNumber.dataset.playerShirtNumber = playerId;
    shirtNumber.setAttribute(
      "aria-label",
      `Numero di maglia ${player.name}`
    );

    const position = document.createElement("select");
    position.dataset.playerPosition = playerId;
    position.setAttribute("aria-label", `Ruolo ${player.name}`);

    for (const value of [
      "Goalkeeper",
      "Defender",
      "Midfielder",
      "Attacker",
    ]) {
      position.append(
        new Option(managerPlayerPosition(value), value)
      );
    }
    position.value = player.position;

    const assignment = document.createElement("select");
    assignment.dataset.playerId = playerId;
    assignment.setAttribute("aria-label", `Impiego ${player.name}`);

    for (const [value, caption] of [
      ["out", "Fuori rosa partita"],
      ["lineup", "Titolare"],
      ["bench", "Panchina"],
    ]) {
      assignment.append(new Option(caption, value));
    }

    assignment.value = currentLineup.has(playerId)
      ? "lineup"
      : currentBench.has(playerId)
        ? "bench"
        : "out";

    row.append(name, shirtNumber, position, assignment);
    rows.append(row);
  }

  const refreshLineupNumbers = () => {
    let lineupNumber = 0;

    for (const select of rows.querySelectorAll("select[data-player-id]")) {
      const lineupOption = [...select.options].find(
        option => option.value === "lineup"
      );

      if (!lineupOption) continue;

      if (select.value === "lineup") {
        lineupNumber += 1;
        lineupOption.textContent = `Titolare ${lineupNumber}`;
      } else {
        lineupOption.textContent = "Titolare";
      }
    }
  };

  rows.addEventListener("change", event => {
    if (event.target.matches("select[data-player-id]")) {
      refreshLineupNumbers();
    }
  });

  refreshLineupNumbers();

  form.append(rows);

  const error = document.createElement("p");
  error.className = "manager-editor-error";
  error.setAttribute("role", "alert");
  form.append(error);

  const actions = document.createElement("div");
  actions.className = "manager-actions";

  const cancel = document.createElement("button");
  cancel.type = "button";
  cancel.textContent = "Annulla";
  cancel.addEventListener("click", () => dialog.close());

  const save = document.createElement("button");
  save.type = "button";
  save.textContent = "Salva rosa e formazione";
  save.addEventListener("click", () => {
    const lineup = [];
    const bench = [];

    for (const select of rows.querySelectorAll("select[data-player-id]")) {
      if (select.value === "lineup") lineup.push(select.dataset.playerId);
      if (select.value === "bench") bench.push(select.dataset.playerId);
    }

    try {
      const edits = club.players.map(player => {
        const playerId = String(player.providerPlayerId);

        return {
          providerPlayerId: playerId,
          name: rows.querySelector(
            `[data-player-name="${playerId}"]`
          ).value,
          shirtNumber: rows.querySelector(
            `[data-player-shirt-number="${playerId}"]`
          ).value,
          position: rows.querySelector(
            `[data-player-position="${playerId}"]`
          ).value,
        };
      });

      const editedClub = updateManagerPlayers(club, edits);
      const nextClub = updateManagerSelection(
        editedClub,
        { lineup, bench }
      );

      managerState = createManagerDocument({
        club: nextClub,
        management: managerState.management,
        nextMatch: managerState.nextMatch,
      });
      persistManagerState();
      dialog.close();
    } catch (caught) {
      const message = caught instanceof Error
        ? caught.message
        : "Formazione non valida.";

      const italianErrors = {
        "manager lineup must contain exactly 11 players":
          "La formazione deve contenere esattamente 11 titolari.",
        "manager bench cannot contain more than 9 players":
          "La panchina non può contenere più di 9 giocatori.",
        "lineup and bench must not overlap":
          "Un giocatore non può essere contemporaneamente titolare e in panchina.",
      };

      error.textContent = document.documentElement.lang === "it"
        ? (italianErrors[message] ?? message)
        : message;
    }
  });

  actions.append(cancel, save);
  form.append(actions);
  dialog.append(form);
  document.body.append(dialog);

  dialog.addEventListener("close", () => dialog.remove(), { once: true });
  dialog.showModal();
}

function setManagerPanelExpanded(expanded) {
  byId("manager-disclosure").open = Boolean(expanded);
}

function syncNewManagerControls() {
  const managerActive = Boolean(managerState);

  for (const seat of [0, 1]) {
    const button = byId(`manager-new-${seat}`);
    button.hidden = managerActive;
    button.disabled =
      managerActive || catalogueTeam(`team-${seat}`) === null;
  }
}

function newManagerCareer(seat) {
  const team = catalogueTeam(`team-${seat}`);
  if (!team) return;

  clearManager(window.localStorage);

  managerState = createManagerDocument({
    club: createManagerClub(team),
    management: createManagementDocument(),
  });

  persistManagerState();
  setManagerPanelExpanded(true);
  renderManagerDashboard();
}

function closeManagerCareer() {
  if (!managerState ||
      (matchStarted && state.phase !== "finished")) return;

  if (!window.confirm(
    "Terminare la carriera Manager? Il torneo corrente e lo storico generale resteranno invariati."
  )) {
    return;
  }

  clearManager(window.localStorage);
  managerState = null;
  setManagerPanelExpanded(false);
  renderManagerDashboard();
  renderTournament();
}

const GLOBAL_SAVE_SCHEMA_VERSION = 1;

let globalSaveDirty = false;

function syncGlobalSaveDirtyUi() {
  byId("game-export").disabled = !globalSaveDirty;
}

function markGlobalSaveDirty() {
  globalSaveDirty = true;
  syncGlobalSaveDirtyUi();
}

function markGlobalSaveClean() {
  globalSaveDirty = false;
  syncGlobalSaveDirtyUi();
}

function serializedRefereeStates() {
  return refereeStates.map(refereeState => ({
    ...structuredClone(refereeState),
    yellowed: [...(refereeState.yellowed ?? [])],
  }));
}

function serializedExtraordinaryState() {
  return {
    occurredFamilies: [...(extraordinaryState?.occurredFamilies ?? [])],
    occurredTypes: [...(extraordinaryState?.occurredTypes ?? [])],
  };
}

function globalSaveDocument() {
  return {
    schemaVersion: GLOBAL_SAVE_SCHEMA_VERSION,
    savedAt: new Date().toISOString(),
    locale: getLocale(),

    configuration: {
      competition: byId("competition").value,
      hands: byId("hands").value,
      randomHands: byId("random-hands").checked,
      teams: [0, 1].map(seat => byId(`team-${seat}`).value),
      colors: [0, 1].map(seat => ({
        primary: byId(`color-${seat}`).value,
        secondaryEnabled: byId(`color-${seat}-enabled`).checked,
        secondary: byId(`color-${seat}-secondary`).value,
      })),
      commentaryEnabled,
      auto: {
        mode: byId("auto-mode").value,
        handLimit: byId("auto-hand-limit").value,
        rollDelay: byId("auto-roll-delay").value,
        handDelay: byId("auto-hand-delay").value,
      },
    },

    persistent: {
      management: JSON.parse(exportManagementJson(managementState)),
      manager: managerState
        ? JSON.parse(exportManagerJson(managerState))
        : null,
      tournament: tournamentState
        ? JSON.parse(exportTournamentJson(tournamentState))
        : null,
    },

    runtime: {
      state: state == null ? null : structuredClone(state),
      names: structuredClone(names),
      colors: structuredClone(colors),
      teamPalettes: structuredClone(teamPalettes),
      rollEvents: structuredClone(rollEvents),
      matchEvents: structuredClone(matchEvents),
      rosters: structuredClone(rosters),
      refereeStates: serializedRefereeStates(),
      extraordinaryState: serializedExtraordinaryState(),
      lastCommentary: structuredClone(lastCommentary),
      lastRefereeEvent: structuredClone(lastRefereeEvent),
      lastExtraordinaryEvent: structuredClone(lastExtraordinaryEvent),
      lastGoalScorer,
      messageKey,
      messageValues: structuredClone(messageValues),
      matchStarted,
      selectedMatchTeams: structuredClone(selectedMatchTeams),
      activeTournamentFixtureId,
      tournamentViewedMatchday,
      matchRecorded,
    },
  };
}

function validateGlobalSave(source) {
  if (typeof source !== "string") {
    throw new TypeError("Il salvataggio deve essere un file JSON.");
  }

  let document;
  try {
    document = JSON.parse(source);
  } catch {
    throw new TypeError("JSON del salvataggio non valido.");
  }

  if (
    !document ||
    typeof document !== "object" ||
    Array.isArray(document) ||
    document.schemaVersion !== GLOBAL_SAVE_SCHEMA_VERSION ||
    !document.configuration ||
    !document.persistent ||
    !document.runtime
  ) {
    throw new TypeError("Formato salvataggio Dado Goal non valido.");
  }

  const management = importManagementJson(
    JSON.stringify(document.persistent.management)
  );

  const manager = document.persistent.manager === null
    ? null
    : importManagerJson(JSON.stringify(document.persistent.manager));

  const tournament = document.persistent.tournament === null
    ? null
    : importTournamentJson(JSON.stringify(document.persistent.tournament));

  const runtime = document.runtime;

  if (
    !Array.isArray(runtime.names) ||
    !Array.isArray(runtime.colors) ||
    !Array.isArray(runtime.teamPalettes) ||
    !Array.isArray(runtime.rollEvents) ||
    !Array.isArray(runtime.matchEvents) ||
    !Array.isArray(runtime.rosters) ||
    !Array.isArray(runtime.refereeStates) ||
    !Array.isArray(runtime.selectedMatchTeams) ||
    !runtime.extraordinaryState ||
    !Array.isArray(runtime.extraordinaryState.occurredFamilies) ||
    !Array.isArray(runtime.extraordinaryState.occurredTypes)
  ) {
    throw new TypeError("Stato partita nel salvataggio non valido.");
  }

  for (const refereeState of runtime.refereeStates) {
    if (!refereeState || !Array.isArray(refereeState.yellowed)) {
      throw new TypeError("Stato arbitrale nel salvataggio non valido.");
    }
  }

  const configuration = document.configuration;

  if (
    !Array.isArray(configuration.teams) ||
    configuration.teams.length !== 2 ||
    !Array.isArray(configuration.colors) ||
    configuration.colors.length !== 2 ||
    typeof configuration.hands !== "string"
  ) {
    throw new TypeError("Configurazione nel salvataggio non valida.");
  }

  return {
    document,
    management,
    manager,
    tournament,
  };
}

function globalSaveFilename(save, now = new Date()) {
  const slug = value => String(value ?? "")
    .normalize("NFKD")
    .replace(/[\\u0300-\\u036f]/g, "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "");

  const pad = value => String(value).padStart(2, "0");
  const timestamp = [
    now.getFullYear(),
    pad(now.getMonth() + 1),
    pad(now.getDate()),
  ].join("") + "-" + [
    pad(now.getHours()),
    pad(now.getMinutes()),
    pad(now.getSeconds()),
  ].join("");

  const managerClub = save.persistent?.manager?.club;
  const teamName = managerClub?.name
    || save.runtime?.names?.find(name =>
      name && !/^squadra\\s*[12]$/i.test(name)
    );

  const championshipId = save.persistent?.tournament?.championshipId;
  const competition = championshipId
    ? slug(championshipId)
    : save.runtime?.matchStarted
      ? "amichevole"
      : "generale";

  const parts = ["dado-goal"];
  if (slug(teamName)) parts.push(slug(teamName));
  parts.push(competition, timestamp);

  return `${parts.join("-")}.json`;
}

function exportGlobalSave() {
  if (isRolling) {
    byId("game-save-status").textContent =
      "Attendi la conclusione del lancio prima di salvare.";
    return;
  }

  const save = globalSaveDocument();
  const source = JSON.stringify(save, null, 2);
  const blob = new Blob([source], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");

  link.href = url;
  link.download = globalSaveFilename(save);
  link.click();

  URL.revokeObjectURL(url);
  markGlobalSaveClean();
  byId("game-save-status").textContent = "Salvataggio creato.";
}

function restoreControlValue(id, value) {
  const element = byId(id);
  if (!element || value == null) return;
  element.value = String(value);
}

function restoreGlobalRuntime(validated) {
  const { document, management, manager, tournament } = validated;
  const { configuration, runtime } = document;

  managementState = management;
  managerState = manager;
  tournamentState = tournament;

  saveManagement(window.localStorage, managementState);

  if (managerState) {
    saveManager(window.localStorage, managerState);
  } else {
    clearManager(window.localStorage);
  }

  if (tournamentState) {
    saveTournament(window.localStorage, tournamentState);
  } else {
    clearTournament(window.localStorage);
  }

  // A recovery journal is an internal crash-consistency artifact.
  // A portable save contains the canonical tournament + runtime directly.
  clearTournamentRecovery(window.localStorage);

  if (typeof document.locale === "string") {
    setLocale(document.locale);
    restoreControlValue("language", getLocale());
    applyLocale();
  }

  restoreControlValue("competition", configuration.competition);
  renderCatalogueTeams();

  restoreControlValue("hands", configuration.hands);
  byId("random-hands").checked = Boolean(configuration.randomHands);

  for (const seat of [0, 1]) {
    const teamId = configuration.teams[seat];

    if (
      teamId &&
      ![...byId(`team-${seat}`).options].some(
        option => option.value === String(teamId)
      )
    ) {
      const team = allCatalogueTeams().find(
        candidate =>
          String(candidate.providerTeamId) === String(teamId)
      );

      if (team) {
        byId(`team-${seat}`).append(
          new Option(team.name, String(team.providerTeamId))
        );
      }
    }

    restoreControlValue(`team-${seat}`, teamId);

    const color = configuration.colors[seat];
    restoreControlValue(`color-${seat}`, color?.primary);
    byId(`color-${seat}-enabled`).checked =
      Boolean(color?.secondaryEnabled);
    restoreControlValue(`color-${seat}-secondary`, color?.secondary);
    syncSecondaryColorControl(seat);
    renderSelectedTeamLogo(seat);
  }

  restoreControlValue("auto-mode", configuration.auto?.mode);
  restoreControlValue("auto-hand-limit", configuration.auto?.handLimit);
  restoreControlValue("auto-roll-delay", configuration.auto?.rollDelay);
  restoreControlValue("auto-hand-delay", configuration.auto?.handDelay);

  state = runtime.state === null
    ? newMatch(Number(configuration.hands))
    : structuredClone(runtime.state);

  names = structuredClone(runtime.names);
  colors = structuredClone(runtime.colors);

  const restoredTeamPalettes = structuredClone(runtime.teamPalettes);
  teamPalettes.splice(
    0,
    teamPalettes.length,
    ...restoredTeamPalettes,
  );

  rollEvents = normalizeRollEvents(runtime.rollEvents);
  matchEvents = structuredClone(runtime.matchEvents);
  rosters = structuredClone(runtime.rosters);

  refereeStates = runtime.refereeStates.map(refereeState => ({
    ...structuredClone(refereeState),
    yellowed: new Set(refereeState.yellowed),
  }));

  extraordinaryState = {
    occurredFamilies: new Set(
      runtime.extraordinaryState.occurredFamilies
    ),
    occurredTypes: new Set(
      runtime.extraordinaryState.occurredTypes
    ),
  };

  lastCommentary = structuredClone(runtime.lastCommentary);
  lastRefereeEvent = structuredClone(runtime.lastRefereeEvent);
  lastExtraordinaryEvent =
    structuredClone(runtime.lastExtraordinaryEvent);
  lastGoalScorer = runtime.lastGoalScorer ?? null;
  messageKey = typeof runtime.messageKey === "string"
    ? runtime.messageKey
    : "setupPrompt";
  messageValues = runtime.messageValues &&
      typeof runtime.messageValues === "object"
    ? structuredClone(runtime.messageValues)
    : {};

  matchStarted = Boolean(runtime.matchStarted);
  selectedMatchTeams = structuredClone(runtime.selectedMatchTeams);
  activeTournamentFixtureId =
    runtime.activeTournamentFixtureId ?? null;
  tournamentViewedMatchday =
    runtime.tournamentViewedMatchday ?? null;
  matchRecorded = Boolean(runtime.matchRecorded);

  commentaryEnabled =
    configuration.commentaryEnabled !== false;

  // Never resume asynchronous activity automatically after a load.
  isRolling = false;
  autoRunning = false;
  autoConfig = null;
  autoDelay.cancel();

  if (matchStarted) {
    capturePitchTeamLogos();
  }

  const lastRoll = rollEvents.at(-1);
  const die = byId("die");

  if (lastRoll && diceFaces[lastRoll.die]) {
    const faceIndex = diceFaces[lastRoll.die].indexOf(lastRoll.face);
    if (faceIndex >= 0) {
      displayFace(lastRoll.die, lastRoll.seat, faceIndex);
      byId("result").textContent = displayDieFace(lastRoll.face);
    }
  } else {
    die.classList.remove("rolling");
    die.classList.add("kickoff-ball");
    die.textContent = "⚽";
    die.setAttribute("aria-label", "In attesa del primo lancio");
  }

  render();
  renderTournament();
  renderManagerDashboard();
  syncNewManagerControls();
}

async function importGlobalSave(file) {
  if (!file) return;

  if (isRolling) {
    throw new Error(
      "Attendi la conclusione del lancio prima di caricare."
    );
  }

  if (cupGlobalImportBlocked()) throw new CupShootoutError("CUP_GLOBAL_IMPORT_BLOCKED");
  const validated = validateGlobalSave(await file.text());
  // Recheck after file I/O: another tab may have stored a Cup meanwhile.
  if (cupGlobalImportBlocked()) throw new CupShootoutError("CUP_GLOBAL_IMPORT_BLOCKED");

  // Validate first, mutate only after the whole document is accepted.
  const keys = [
    "dado-goal.tournament.v1",
    "dado-goal.tournament-recovery.v1",
    "dado-goal.manager.v1",
    "dado-goal.team-management.v1",
  ];

  const previous = new Map(
    keys.map(key => [key, window.localStorage.getItem(key)])
  );

  try {
    restoreGlobalRuntime(validated);
  } catch (error) {
    for (const [key, value] of previous) {
      if (value === null) {
        window.localStorage.removeItem(key);
      } else {
        window.localStorage.setItem(key, value);
      }
    }
    throw error;
  }

  markGlobalSaveClean();
  byId("game-save-status").textContent =
    "Salvataggio caricato.";
}

function initializeGlobalSaveUi() {
  markGlobalSaveClean();
  byId("game-export").addEventListener("click", exportGlobalSave);

  byId("game-import").addEventListener("click", () => {
    byId("game-import-file").click();
  });

  byId("game-import-file").addEventListener(
    "change",
    async event => {
      try {
        await importGlobalSave(event.target.files?.[0]);
      } catch (caught) {
        window.alert(
          caught instanceof CupShootoutError
            ? cupMessage(caught.code)
            : caught instanceof Error
              ? caught.message
              : "Salvataggio Dado Goal non valido."
        );
      } finally {
        event.target.value = "";
      }
    }
  );
}

function initializeManagerUi() {
  byId("manager-edit").addEventListener("click", editManagerSelection);

  for (const seat of [0, 1]) {
    byId(`manager-new-${seat}`).addEventListener(
      "click",
      () => newManagerCareer(seat)
    );
  }

  renderManagerDashboard();
}


function catalogueTeam(selectId) {
  if (!footballCatalogue) return null;

  const value = byId(selectId).value;
  if (value === "") return null;

  return footballCatalogue.teams.find(
    (team) => String(team.providerTeamId) === value
  ) ?? null;
}






const competitions = Object.freeze([
  Object.freeze({
    value: "135:2024",
    label: "Serie A",
    url: "./catalogues/serie-a-2024.json",
  }),
  Object.freeze({
    value: "136:2024",
    label: "Serie B",
    url: "./catalogues/serie-b-2024.json",
  }),
  Object.freeze({
    value: "39:2024",
    label: "Premier League",
    url: "./catalogues/premier-league-2024.json",
  }),
  Object.freeze({
    value: "40:2024",
    label: "Championship",
    url: "./catalogues/championship-2024.json",
  }),
  Object.freeze({
    value: "61:2024",
    label: "Ligue 1",
    url: "./catalogues/ligue-1-2024.json",
  }),
  Object.freeze({
    value: "78:2024",
    label: "Bundesliga",
    url: "./catalogues/bundesliga-2024.json",
  }),
  Object.freeze({
    value: "140:2024",
    label: "La Liga",
    url: "./catalogues/la-liga-2024.json",
  }),
  Object.freeze({
    value: "79:2024",
    label: "2. Bundesliga",
    url: "./catalogues/2-bundesliga-2024.json",
  }),
  Object.freeze({
    value: "62:2024",
    label: "Ligue 2",
    url: "./catalogues/ligue-2-2024.json",
  }),
  Object.freeze({
    value: "141:2024",
    label: "Segunda División",
    url: "./catalogues/segunda-division-2024.json",
  }),
]);

let footballCatalogues = [];

function syncCompetitionSelector() {
  const selector = byId("competition");
  const previous = selector.value;

  selector.replaceChildren(
    ...competitions.map(({ value, label }) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = label;
      return option;
    }),
  );

  if (competitions.some(({ value }) => value === previous)) {
    selector.value = previous;
  } else if (competitions.length > 0) {
    selector.value = competitions[0].value;
  }

  if (selector.options.length !== competitions.length) {
    throw new Error("Competition selector synchronization failed");
  }
}

const tournamentSelectedTeamIds = new Set();

function allCatalogueTeams() {
  return footballCatalogues.flatMap(catalogue => catalogue.teams);
}

function selectedCompetitionTeams() {
  const selected = byId("competition").value;

  const [competitionId, season] = selected.split(":");

  return allCatalogueTeams().filter(team =>
    String(team.competitionId) === String(competitionId) &&
    String(team.season) === String(season)
  );
}

function renderCatalogueTeams() {
  const friendlyTeams = allCatalogueTeams().sort(
    (left, right) => left.name.localeCompare(
      right.name, undefined, { sensitivity: "base" }
    )
  );
  for (let seat = 0; seat < 2; seat += 1) {
    const select = byId(`team-${seat}`);
    const current = select.value;

    const country = byId(`team-country-${seat}`).value;
    const visibleTeams = friendlyTeams.filter(team =>
      !country || team.enrichment?.country === country
    );
    select.replaceChildren(new Option(`Squadra ${seat + 1}`, ""));

    for (const team of visibleTeams) {
      select.append(new Option(team.name, String(team.providerTeamId)));
    }

    if ([...select.options].some(option => option.value === current)) {
      select.value = current;
    } else {
      select.value = "";
    }

    renderSelectedTeamLogo(seat);
  }
}

async function loadCatalogueFile(file) {
  const source = await file.text();
  const catalogue = importCatalogueJson(source);
  footballCatalogues = [catalogue];
  footballCatalogue = {
    ...catalogue,
    teams: [...catalogue.teams],
  };
  renderCatalogueTeams();
}

async function reloadLiveCatalogue() {
  footballCatalogue = null;
  footballCatalogues = [];
  renderCatalogueTeams();

  try {
    await loadLiveCatalogue();
  } catch {
    footballCatalogue = null;
    footballCatalogues = [];
    renderCatalogueTeams();
  }
}

async function loadLiveCatalogue() {
  const loaded = await Promise.all(
    competitions.map(async ({ url }) => {
      const response = await fetch(url);

      if (!response.ok) {
        throw new Error(`catalogue HTTP ${response.status}`);
      }

      return importCatalogueJson(await response.text());
    })
  );

  footballCatalogues = loaded;

  const teams = allCatalogueTeams();
  const ids = new Set(teams.map(team => String(team.providerTeamId)));

  if (ids.size !== teams.length) {
    throw new Error("duplicate provider team id across catalogues");
  }

  footballCatalogue = {
    schemaVersion: loaded[0]?.schemaVersion ?? 1,
    teams,
  };

  renderCatalogueTeams();
  syncNewManagerControls();
  renderManagerDashboard();
  renderTournament();
}


function tournamentTeamName(teamId) {
  return allCatalogueTeams().find(team =>
    String(team.providerTeamId) === String(teamId)
  )?.name ?? String(teamId);
}

function managerTournamentTeamId() {
  if (!managerState || !tournamentState) return null;

  const managedId = String(managerState.club.providerTeamId);
  return tournamentState.participants.some(
    team => String(team.providerTeamId) === managedId
  )
    ? managedId
    : null;
}

function fixtureIncludesTeam(fixture, teamId) {
  if (!fixture || teamId === null) return false;

  return String(fixture.homeProviderTeamId) === String(teamId) ||
    String(fixture.awayProviderTeamId) === String(teamId);
}

function nextTournamentFixture() {
  return tournamentState?.fixtures.find(fixture => fixture.matchId === null) ?? null;
}

function nextPlayableTournamentFixture() {
  const managedId = managerTournamentTeamId();

  if (managedId === null) {
    return nextTournamentFixture();
  }

  return tournamentState.fixtures.find(
    fixture =>
      fixture.matchId === null &&
      fixtureIncludesTeam(fixture, managedId)
  ) ?? null;
}

function nextSimulatedTournamentFixture() {
  const managedId = managerTournamentTeamId();

  if (managedId === null) return null;

  return tournamentState.fixtures.find(
    fixture =>
      fixture.matchId === null &&
      !fixtureIncludesTeam(fixture, managedId)
  ) ?? null;
}

function selectTournamentCompetitionParticipants() {
  if (tournamentState) return;

  const list = byId("tournament-participants");
  const createButton = byId("tournament-create");
  const competition = byId("competition").value;

  list.disabled = false;
  createButton.disabled = false;

  const selectedIds = new Set(
    selectedCompetitionTeams().map(
      team => String(team.providerTeamId)
    )
  );

  tournamentSelectedTeamIds.clear();
  for (const id of selectedIds) tournamentSelectedTeamIds.add(id);

  for (const option of list.options) {
    option.selected = selectedIds.has(option.value);
  }
}

// SP-01 app adapter: sporting state and draws remain in the coordinator.
function cupText(it, en) {
  return getLocale() === "en" ? en : it;
}

function cupMessage(code) {
  const messages = {
    CUP_SHOOTOUT_NOT_ELIGIBLE: ["Rigori non disponibili per questa eliminatoria.", "A shootout is not available for this tie."],
    CUP_SHOOTOUT_STALE: ["Lo stato della Coppa è cambiato. Ricarica i dati salvati.", "The Cup state changed. Reload the saved data."],
    CUP_SHOOTOUT_DECIDED: ["Questa eliminatoria è già decisa.", "This tie is already decided."],
    CUP_SHOOTOUT_MANUAL_LOCKED: ["I rigori sono già iniziati: riprendi i tentativi salvati.", "The shootout has started: resume the saved attempts."],
    CUP_SHOOTOUT_INVALID_SCORE: ["Inserisci due punteggi interi, non negativi e diversi.", "Enter two different nonnegative whole-number scores."],
    CUP_SHOOTOUT_DRAW_INVALID: ["Impossibile risolvere il rigore. Nessun tentativo salvato.", "Unable to resolve the penalty. No attempt was saved."],
    CUP_SHOOTOUT_SAVE_FAILED: ["Salvataggio non completato. Riprova prima di continuare. Ricaricare perde solo il tentativo non salvato.", "Save did not complete. Retry before continuing. Reloading loses only the unsaved attempt."],
    CUP_SHOOTOUT_STORAGE_UNAVAILABLE: ["Salvataggio non disponibile. Ricarica i dati Coppa prima di continuare.", "Saving is unavailable. Reload Cup data before continuing."],
    CUP_SHOOTOUT_INVALID_STATE: ["Dati della Coppa non validi. I dati salvati sono stati conservati.", "Invalid Cup data. The saved data has been preserved."],
    CUP_SHOOTOUT_UNSUPPORTED_VERSION: ["Versione dei rigori non supportata. I dati salvati sono stati conservati.", "Unsupported shootout version. The saved data has been preserved."],
    CUP_SHOOTOUT_OTHER_TAB: ["Coppa aperta in un'altra scheda. Questa vista è in sola lettura.", "The Cup is open in another tab. This view is read-only."],
    CUP_SHOOTOUT_LOCK_UNAVAILABLE: ["Questo browser non supporta l'accesso esclusivo alla Coppa. Modifica non disponibile.", "This browser does not support exclusive Cup access. Editing is unavailable."],
    CUP_GLOBAL_IMPORT_BLOCKED: ["Impossibile importare un salvataggio globale mentre è presente una Coppa.", "A global save cannot be imported while a Cup is stored."],
  };
  return cupText(...(messages[code] ?? messages.CUP_SHOOTOUT_INVALID_STATE));
}

function cupGlobalImportBlocked() {
  try { return Boolean(cupState || cupLoadError || window.localStorage.getItem(CUP_STORAGE_KEY) !== null); }
  catch { return true; }
}

function ownsCupLock() {
  return Boolean(cupState && cupLock?.cupId === cupState.cupId);
}

function requireCupOwnership() {
  if (cupLoadError) throw new CupShootoutError(cupLoadError);
  if (!window.navigator?.locks?.request) throw new CupShootoutError("CUP_SHOOTOUT_LOCK_UNAVAILABLE");
  if (!ownsCupLock()) throw new CupShootoutError("CUP_SHOOTOUT_OTHER_TAB");
}

function reloadCupForOwnership() {
  if (cupShootoutPendingSave) return;
  try {
    const management = loadManagement(window.localStorage);
    const cup = loadVerifiedCup(window.localStorage, management.matches);
    managementState = management;
    cupState = cup;
    if (cupLock && cupLock.cupId !== cup?.cupId) releaseCupOwnership();
    cupLoadError = null;
    cupShootoutError = null;
  } catch (failure) {
    cupLoadError = failure.code ?? "CUP_SHOOTOUT_INVALID_STATE";
  }
}

function refreshOwnedCup() {
  reloadCupForOwnership();
  if (!ownsCupLock() || cupLoadError || cupShootoutPendingSave) return;
  try {
    reconcileCupRecovery();
    cupCoordinator = createCupShootoutCoordinator(window.localStorage, managementState.matches, {
      ownsLock: ownsCupLock,
    });
    cupShootoutNotice = Object.values(cupState.shootoutSessions ?? {})
      .some(session => session.status !== "complete") ? "resumed" : null;
  } catch (failure) {
    // Retain the journal and block new writes until recovery can finish.
    cupLoadError = failure.code ?? "CUP_SHOOTOUT_STORAGE_UNAVAILABLE";
  }
}

function releaseCupOwnership() {
  const owned = cupLock;
  cupLock = null;
  owned?.release();
}

function requestCupOwnership() {
  if (!cupState || cupLoadError || cupLockRequest || ownsCupLock()) return;
  if (!window.navigator?.locks?.request) {
    cupShootoutError = "CUP_SHOOTOUT_LOCK_UNAVAILABLE";
    return;
  }
  const cupId = cupState.cupId;
  const request = { cupId };
  cupLockRequest = request;
  // ifAvailable keeps a second tab read-only; its explicit retry requests handover.
  void window.navigator.locks.request(`dado-goal.cup:${cupId}`, {
    mode: "exclusive", ifAvailable: true,
  }, async lock => {
    if (!lock || cupState?.cupId !== cupId) {
      cupShootoutError = "CUP_SHOOTOUT_OTHER_TAB";
      return;
    }
    let release;
    const held = new Promise(resolve => { release = resolve; });
    cupLock = { cupId, release };
    try {
      refreshOwnedCup();
      renderTournament();
      await held;
    } finally {
      if (cupLock?.release === release) cupLock = null;
    }
  }).catch(() => {
    cupShootoutError = "CUP_SHOOTOUT_LOCK_UNAVAILABLE";
  }).finally(() => {
    if (cupLockRequest === request) cupLockRequest = null;
    renderTournament();
  });
}

function renderCupStorageStatus() {
  const error = cupLoadError ?? (cupState && !ownsCupLock()
    ? (cupShootoutError === "CUP_SHOOTOUT_LOCK_UNAVAILABLE" || !window.navigator?.locks?.request
      ? "CUP_SHOOTOUT_LOCK_UNAVAILABLE" : "CUP_SHOOTOUT_OTHER_TAB")
    : ["CUP_SHOOTOUT_STORAGE_UNAVAILABLE", "CUP_SHOOTOUT_STALE", "CUP_SHOOTOUT_INVALID_STATE",
      "CUP_SHOOTOUT_UNSUPPORTED_VERSION", "CUP_SHOOTOUT_SAVE_FAILED"].includes(cupShootoutError)
      ? cupShootoutError : null);
  let notice = "";
  const manualKey = cupShootoutNotice?.manualKey;
  const manualResult = cupState?.shootouts?.[manualKey];
  if (manualResult) {
    const firstLeg = cupState.fixtures.find(f => `r${f.round}-p${f.pair}` === manualKey && f.leg === 1);
    const name = id => cupState.participants.find(team => String(team.providerTeamId) === id)?.name ?? id;
    const winnerId = manualResult.home > manualResult.away ? firstLeg.homeProviderTeamId : firstLeg.awayProviderTeamId;
    notice = cupText("Risultato rigori registrato manualmente: ", "Manually recorded shootout: ") +
      `${name(firstLeg.homeProviderTeamId)} ${manualResult.home}–${manualResult.away} ${name(firstLeg.awayProviderTeamId)}. ` +
      `${name(winnerId)} ` + cupText("vince ai rigori.", "wins the shootout.");
  }
  byId("cup-storage-status").textContent = error ? cupMessage(error) : notice;
  const retry = byId("cup-storage-retry");
  retry.hidden = !error;
  retry.disabled = cupShootoutBusy || cupShootoutPendingSave;
  retry.textContent = cupText("Ricarica dati / acquisisci Coppa", "Reload data / acquire Cup");
  byId("cup-global-omission").textContent = cupText(
    "Il salvataggio globale non include la Coppa.", "The global save does not include the Cup.",
  );
}

function cupAttemptOutcome(attempt) {
  const outcomes = {
    "penalty-goal": ["Goal", "Goal"],
    "penalty-woodwork": ["Palo/traversa · sbagliato", "Post/crossbar · missed"],
    "penalty-saved": ["Parato/respinto · sbagliato", "Saved/blocked · missed"],
    "penalty-wide": ["Fuori · sbagliato", "Wide · missed"],
  };
  return cupText(...outcomes[attempt.kind]);
}

function renderCupShootout(cup, teamName) {
  const ready = advanceCupIfReady(cup, managementState.matches, cup.shootouts ?? {});
  const needsPenalties = ready.status === "pending-penalties";
  const panel = byId("cup-shootout");
  const history = Object.values(cup.shootoutSessions ?? {});
  const session = needsPenalties ? cup.shootoutSessions?.[`r${ready.round}-p${ready.pair}`]
    : history.at(-1);
  panel.hidden = !needsPenalties && !session;
  if (panel.hidden) return;
  panel.setAttribute("aria-busy", String(cupShootoutBusy));
  panel.dataset.round = needsPenalties ? String(ready.round) : "";
  panel.dataset.pair = needsPenalties ? String(ready.pair) : "";
  const fixture = cup.fixtures.find(f => f.leg === 1 &&
    f.round === (needsPenalties ? ready.round : session.round) &&
    f.pair === (needsPenalties ? ready.pair : session.pair));
  const teams = { home: teamName(fixture.homeProviderTeamId), away: teamName(fixture.awayProviderTeamId) };
  const matchView = cupFixturePresentation(cup, fixture, managementState.matches);
  byId("cup-shootout-title").textContent = cupText("Rigori di spareggio", "Penalty shootout");
  byId("cup-shootout-description").textContent = `${teams.home} – ${teams.away} · ` +
    cupText("Risultato partita", "Match score") + ` ${matchView.score}` +
    (matchView.aggregate === null ? "" : ` · ${cupText("Aggregato", "Aggregate")} ${matchView.aggregate}`);
  byId("cup-shootout-rules").textContent = cupText(
    "Dado verde Extended v1: 3 facce Goal su 6. Cinque rigori alternati, poi oltranza a coppie. I rigori restano separati dal risultato partita.",
    "Extended v1 green die: 3 Goal faces out of 6. Five alternating penalties, then paired sudden death. Shootout goals remain separate from the match score.",
  );
  const projection = session ? replayCupShootout(session) : null;
  const lastAttempt = session?.attempts.at(-1);
  const die = byId("cup-shootout-die");
  die.hidden = !lastAttempt;
  if (lastAttempt) {
    // Display only the committed canonical face, in its own image. The regular
    // match die, event stream, scorers and statistics are never touched.
    die.src = `data:image/svg+xml,${encodeURIComponent(PENALTY_DICE_ART[FACES.penalty.indexOf(lastAttempt.face)])}`;
    die.alt = cupText("Dado verde: ", "Green die: ") + cupAttemptOutcome(lastAttempt);
  }
  const editable = ownsCupLock() && !cupLoadError && !cupShootoutBusy &&
    !cupShootoutPendingSave && !tournamentState;
  const blocked = ["CUP_SHOOTOUT_STORAGE_UNAVAILABLE", "CUP_SHOOTOUT_STALE", "CUP_SHOOTOUT_INVALID_STATE",
    "CUP_SHOOTOUT_UNSUPPORTED_VERSION"].includes(cupShootoutError);
  const start = byId("cup-shootout-start");
  start.hidden = Boolean(session) || !needsPenalties;
  start.disabled = !editable || blocked;
  start.textContent = cupText("Gioca i rigori", "Play shootout");
  const manual = byId("cup-shootout-manual");
  manual.hidden = Boolean(session) || !needsPenalties;
  manual.disabled = !editable || blocked;
  byId("cup-shootout-manual-title").textContent = cupText("Inserisci risultato manuale", "Enter manual result");
  byId("cup-penalties-home-label").textContent = teams.home;
  byId("cup-penalties-away-label").textContent = teams.away;
  byId("cup-shootout-confirm").textContent = cupText("Conferma risultato rigori", "Confirm shootout result");
  byId("cup-shootout-confirm").disabled = Boolean(session) || !editable || blocked;
  const retry = byId("cup-shootout-retry");
  retry.hidden = !cupShootoutPendingSave;
  retry.disabled = !ownsCupLock() || cupShootoutBusy || Boolean(cupLoadError);
  retry.textContent = cupText("Riprova salvataggio", "Retry save");
  const kick = byId("cup-shootout-kick");
  kick.hidden = !projection || projection.status === "complete";
  kick.disabled = !editable || blocked || !projection?.nextTeam;
  kick.onclick = null;
  const goals = projection?.goals ?? { home: 0, away: 0 };
  const counts = projection?.attemptsByTeam ?? { home: 0, away: 0 };
  byId("cup-shootout-score").textContent = `${cupText("Rigori", "Penalties")}: ` +
    `${teams.home} ${goals.home} – ${goals.away} ${teams.away} · ` +
    `${cupText("Tentativi", "Attempts")}: ${counts.home}–${counts.away}`;
  let status = cupText("Parità: scegli i rigori giocabili o il risultato manuale.", "Scores level: choose a playable shootout or a manual result.");
  if (projection?.winner) {
    status = `${teams[projection.winner]} ` + cupText("vince ai rigori", "wins the shootout") + ` ${goals.home}–${goals.away}.`;
  } else if (projection) {
    const team = teams[projection.nextTeam];
    const count = projection.attemptsByTeam[projection.nextTeam];
    status = projection.status === "initial"
      ? cupText(`Serie iniziale · ${team} · rigore ${count + 1} di 5`, `Initial series · ${team} · attempt ${count + 1} of 5`)
      : cupText(`Oltranza · coppia ${count - 4} · tira ${team}`, `Sudden death · pair ${count - 4} · ${team} to shoot`);
    if (projection.status === "sudden-death" && session.attempts.length % 2 === 1) {
      status += cupText(`. Nessun vincitore prima del rigore di ${team}.`, `. No winner before ${team}'s penalty.`);
    }
    kick.textContent = cupText(`Tira il dado verde per ${team}`, `Roll the green die for ${team}`);
    // Capture exactly the displayed revision, including for delayed/duplicate handlers.
    const expectedRevision = session.attempts.length;
    const command = { cupId: cup.cupId, shootoutId: session.shootoutId, expectedRevision,
      attemptId: JSON.stringify([cup.cupId, `r${session.round}-p${session.pair}`, expectedRevision + 1]) };
    kick.onclick = event => {
      if (event.detail > 1) return;
      runCupShootoutAction("kick", command);
    };
  }
  if (cupShootoutNotice === "resumed") status = cupText(
    "Rigori ripresi dall'ultimo tentativo salvato. ", "Shootout resumed from the last saved attempt. ",
  ) + status;
  const savedSession = cup.shootoutSessions?.[cupShootoutNotice?.key];
  const savedAttempt = savedSession?.attempts.find(attempt => attempt.attemptId === cupShootoutNotice.attemptId);
  if (savedAttempt) {
    const savedState = replayCupShootout(savedSession);
    const savedTeams = {
      home: teamName(savedSession.homeProviderTeamId),
      away: teamName(savedSession.awayProviderTeamId),
    };
    let announcement = cupText("Rigore salvato. ", "Penalty saved. ") +
      `${savedTeams[savedAttempt.team]}: ${cupAttemptOutcome(savedAttempt)}. ` +
      `${savedTeams.home} ${savedState.goals.home}–${savedState.goals.away} ${savedTeams.away}. `;
    if (savedState.winner) {
      announcement += `${savedTeams[savedState.winner]} ` + cupText("vince ai rigori. ", "wins the shootout. ");
    }
    status = announcement + status;
  }
  if (cupShootoutBusy) status = cupText("Salvataggio del rigore…", "Saving penalty…");
  byId("cup-shootout-turn").textContent = status;
  const sequence = byId("cup-shootout-sequence");
  sequence.setAttribute("aria-label", cupText("Sequenza rigori", "Penalty sequence"));
  sequence.replaceChildren(...(session?.attempts ?? []).map(attempt => {
    const item = document.createElement("li");
    item.textContent = `${teams[attempt.team]} · ${attempt.index}: ${cupAttemptOutcome(attempt)}`;
    return item;
  }));
  byId("cup-shootout-error").textContent = cupShootoutError ? cupMessage(cupShootoutError) : "";
}

async function runCupShootoutAction(action, command) {
  if (cupShootoutBusy || (cupShootoutPendingSave && action !== "retry")) return;
  cupShootoutBusy = true;
  cupShootoutError = null;
  cupShootoutNotice = null;
  renderTournament();
  let accepted = false;
  try {
    requireCupOwnership();
    // Match completion replaces the matches array. Start a coordinator with that
    // canonical input; never replace one retaining a prepared attempt.
    if (action === "start") {
      cupCoordinator = createCupShootoutCoordinator(window.localStorage, managementState.matches, { ownsLock: ownsCupLock });
    }
    if (!cupCoordinator) throw new CupShootoutError("CUP_SHOOTOUT_STALE");
    const result = action === "start" ? { cup: cupCoordinator.startCupShootout(command) }
      : action === "retry" ? cupCoordinator.retryCupShootoutSave()
      : cupCoordinator.confirmCupShootoutAttempt(command);
    // Commit has succeeded. Presentation below cannot turn it into a failed save.
    cupState = result.cup;
    cupShootoutPendingSave = false;
    accepted = true;
    cupShootoutNotice = result.attempt
      ? { key: JSON.parse(result.attempt.attemptId)[1], attemptId: result.attempt.attemptId }
      : "resumed";
    tournamentViewedMatchday = Math.max(...cupState.fixtures.map(f => f.round));
  } catch (failure) {
    cupShootoutError = failure.code ?? "CUP_SHOOTOUT_INVALID_STATE";
    if (action === "kick" && failure.code === "CUP_SHOOTOUT_SAVE_FAILED") cupShootoutPendingSave = true;
  } finally {
    // Keep repeated activation disabled through the next paint; this never draws.
    await new Promise(resolve => window.requestAnimationFrame(resolve));
    cupShootoutBusy = false;
  }
  try { renderTournament(); }
  catch (failure) {
    if (!accepted) throw failure;
    // A committed result remains authoritative even if its first render fails.
    refreshOwnedCup();
    renderTournament();
  }
  if (accepted) {
    const target = ["cup-shootout-kick", "cup-shootout-start", "tournament-play-next"]
      .map(byId).find(button => !button.disabled && !button.hidden)
      ?? byId("cup-shootout-turn");
    target.focus();
  }
}

// SP-01 app adapter end.

function renderCupBracket() {
  const cup = cupState;
  byId("tournament-close").textContent = cupText("Archivia Coppa", "Archive Cup");
  byId("tournament-close").disabled =
    !ownsCupLock() || cupLoadError !== null || cupShootoutPendingSave || cupShootoutBusy ||
    cup.championProviderTeamId === undefined ||
    (matchStarted && state.phase !== "finished");
  const rounds = Math.log2(cup.participants.length);
  const currentRound = Math.max(...cup.fixtures.map(f => f.round));
  const viewedRound = Math.min(
    Math.max(tournamentViewedMatchday ?? currentRound, 1),
    currentRound,
  );
  tournamentViewedMatchday = viewedRound;

  const records = new Map(
    managementState.matches.map(match => [match.matchId, match]),
  );
  const teamName = id =>
    cup.participants.find(team =>
      String(team.providerTeamId) === String(id)
    )?.name ?? String(id);

  const fixtures = cup.fixtures.filter(f => f.round === viewedRound);
  const completed = cup.fixtures.filter(f => f.matchId !== null).length;
  const next = cup.fixtures.find(f => f.matchId === null);
  const roundNames = {
    1: cup.participants.length === 4 ? cupText("Semifinali", "Semi-finals") :
       cup.participants.length === 8 ? cupText("Quarti di finale", "Quarter-finals") :
       cup.participants.length === 16 ? cupText("Ottavi di finale", "Round of 16") :
       cupText("Sedicesimi di finale", "Round of 32"),
  };
  const roundName = viewedRound === rounds
    ? cupText("Finale", "Final")
    : viewedRound === rounds - 1
      ? cupText("Semifinali", "Semi-finals")
      : viewedRound === rounds - 2
        ? cupText("Quarti di finale", "Quarter-finals")
        : viewedRound === rounds - 3
          ? cupText("Ottavi di finale", "Round of 16")
          : roundNames[viewedRound] ?? cupText(`Turno ${viewedRound}`, `Round ${viewedRound}`);

  byId("tournament-progress").textContent =
    `${completed} / ${(cup.participants.length - 1) * cup.legs}`;
  byId("tournament-status").textContent =
    cup.championProviderTeamId !== undefined
      ? `${cupText("Campione", "Champion")}: ${teamName(cup.championProviderTeamId)}`
      : `${cup.name ?? cupText("Coppa", "Cup")} · ` + cupText(`turno ${currentRound} di ${rounds}`, `round ${currentRound} of ${rounds}`);
  byId("tournament-next").textContent = next
    ? `${cupText("Prossima", "Next")}: ${teamName(next.homeProviderTeamId)} – ${teamName(next.awayProviderTeamId)}`
    : cup.championProviderTeamId !== undefined
      ? cupText("Coppa conclusa", "Cup completed")
      : cupText("Turno da risolvere", "Round awaiting a decision");
  byId("tournament-play-next").textContent = cupText("Gioca partita della Coppa", "Play Cup match");
  byId("tournament-matchday-prev").setAttribute("aria-label", cupText("Turno precedente", "Previous round"));
  byId("tournament-matchday-next").setAttribute("aria-label", cupText("Turno successivo", "Next round"));
  byId("tournament-matchday-label").textContent = roundName;
  byId("tournament-matchday-prev").disabled = viewedRound <= 1;
  byId("tournament-matchday-next").disabled = viewedRound >= currentRound;
  byId("tournament-play-next").disabled =
    !ownsCupLock() || cupLoadError !== null || cupShootoutPendingSave || cupShootoutBusy ||
    activeTournamentFixtureId !== null ||
    !next || (matchStarted && state.phase !== "finished");

  renderCupShootout(cup, teamName);

  const renderFixture = fixture => {
    const item = document.createElement("li");
    item.className = "tournament-fixture";
    const view = cupFixturePresentation(
      cup, fixture, managementState.matches,
    );
    const home = document.createElement("span");
    const away = document.createElement("span");
    const score = document.createElement("span");
    const detail = document.createElement("span");
    home.className = "tournament-fixture-name home";
    away.className = "tournament-fixture-name away";
    score.className = "tournament-fixture-score";
    detail.className = "tournament-fixture-detail";
    home.textContent = teamName(fixture.homeProviderTeamId);
    away.textContent = teamName(fixture.awayProviderTeamId);
    score.textContent = view.score;
    item.dataset.status = view.status;

    if (view.status === "pending") {
      detail.textContent = cupText("Da giocare", "To be played");
    } else if (view.status === "penalties") {
      detail.textContent = `${cupText("Aggregato", "Aggregate")} ${view.aggregate ?? view.score} · ` +
        cupText("Rigori necessari", "Shootout required");
    } else if (view.winnerProviderTeamId !== null) {
      const winner = teamName(view.winnerProviderTeamId);
      detail.textContent =
        `${view.aggregate === null ? "" : `${cupText("Aggregato", "Aggregate")} ${view.aggregate} · `}` +
        `${cupText("Qualificata", "Qualified")}: ${winner}`;
      if (view.decidedBy === "penalties") {
        const key = `r${fixture.round}-p${fixture.pair}`;
        const penalties = cup.shootouts[key];
        const firstLeg = cup.fixtures.find(f => f.round === fixture.round && f.pair === fixture.pair && f.leg === 1);
        detail.textContent += ` · ${cupText("Rigori", "Penalties")}: ` +
          `${teamName(firstLeg.homeProviderTeamId)} ${penalties.home}–${penalties.away} ${teamName(firstLeg.awayProviderTeamId)}` +
          (cup.shootoutSessions?.[key] ? "" : cupText(" (risultato manuale)", " (manual result)"));
      }
      const winnerId = String(view.winnerProviderTeamId);
      if (String(fixture.homeProviderTeamId) === winnerId) {
        home.classList.add("cup-qualified");
      } else {
        away.classList.add("cup-qualified");
      }
    } else {
      detail.textContent = cupText("Giocata · Eliminatoria in corso", "Played · Tie in progress");
    }

    item.append(home, score, away, detail);
    return item;
  };

  const results = fixtures.filter(f => f.matchId !== null)
    .map(renderFixture);
  const pending = fixtures.filter(f => f.matchId === null)
    .map(renderFixture);
  byId("tournament-results").replaceChildren(...results);
  byId("tournament-results-empty").hidden = results.length !== 0;
  byId("tournament-pending").replaceChildren(...pending);
  byId("tournament-pending-empty").hidden = pending.length !== 0;
}

function confirmCupShootout() {
  if (!cupState || tournamentState || cupShootoutBusy || cupShootoutPendingSave) return;
  const panel = byId("cup-shootout");
  if (panel.hidden) return;
  cupShootoutNotice = null;
  const homeRaw = byId("cup-penalties-home").value.trim();
  const awayRaw = byId("cup-penalties-away").value.trim();
  const home = Number(homeRaw);
  const away = Number(awayRaw);
  const round = Number(panel.dataset.round);
  const pair = Number(panel.dataset.pair);
  if (!/^\d+$/.test(homeRaw) || !/^\d+$/.test(awayRaw) ||
      !Number.isSafeInteger(home) || !Number.isSafeInteger(away) ||
      home < 0 || away < 0 || home === away) {
    cupShootoutError = "CUP_SHOOTOUT_INVALID_SCORE";
    renderTournament();
    return;
  }
  let accepted = false;
  try {
    requireCupOwnership();
    const latest = loadVerifiedCup(window.localStorage, managementState.matches);
    if (latest?.cupId !== cupState.cupId) throw new CupShootoutError("CUP_SHOOTOUT_STALE");
    const decided = recordCupShootout(
      latest, managementState.matches, round, pair, { home, away },
    );
    const advancement = advanceCupIfReady(
      decided, managementState.matches, decided.shootouts ?? {},
    );
    saveVerifiedCup(window.localStorage, advancement.cup, managementState.matches);
    cupState = advancement.cup;
    accepted = true;
    cupShootoutError = null;
    cupShootoutNotice = { manualKey: `r${round}-p${pair}` };
    tournamentViewedMatchday = Math.max(...cupState.fixtures.map(f => f.round));
    byId("cup-penalties-home").value = "";
    byId("cup-penalties-away").value = "";
  } catch (failure) {
    cupShootoutError = failure.code ?? "CUP_SHOOTOUT_SAVE_FAILED";
  }
  renderTournament();
  if (accepted) {
    const target = ["cup-shootout-start", "tournament-play-next"]
      .map(byId).find(button => !button.disabled && !button.hidden)
      ?? byId("cup-storage-status");
    target.focus();
  }
}

byId("cup-shootout-confirm").addEventListener("click", confirmCupShootout);
byId("cup-storage-retry").addEventListener("click", () => {
  if (cupShootoutPendingSave) return;
  if (ownsCupLock()) refreshOwnedCup();
  else {
    reloadCupForOwnership();
    requestCupOwnership();
  }
  renderTournament();
});
byId("cup-shootout-start").addEventListener("click", event => {
  if (event.detail > 1) return;
  const panel = byId("cup-shootout");
  runCupShootoutAction("start", {
    cupId: cupState?.cupId, round: Number(panel.dataset.round), pair: Number(panel.dataset.pair),
  });
});
byId("cup-shootout-retry").addEventListener("click", () => runCupShootoutAction("retry"));

// One deliberate key press requests one command, even when focus moves from a
// finished shootout to the next tie or fixture while Enter is still held down.
for (const id of ["cup-shootout-start", "cup-shootout-kick", "cup-shootout-confirm",
  "cup-shootout-retry", "tournament-play-next"]) {
  byId(id).addEventListener("keydown", event => {
    if (event.repeat && (event.key === "Enter" || event.key === " ")) event.preventDefault();
  });
}

function renderCupArchive() {
  const section = byId("cup-archive");
  const list = byId("cup-archive-list");
  byId("cup-archive-title").textContent = cupText("Coppe archiviate", "Archived Cups");
  let archive;

  try {
    archive = loadCupArchive(
      window.localStorage, managementState.matches,
    );
  } catch (failure) {
    section.hidden = false;
    list.replaceChildren();
    const warning = document.createElement("p");
    warning.textContent = cupText("Archivio Coppe non disponibile. ", "Cup archive unavailable. ") +
      cupMessage(failure.code ?? "CUP_SHOOTOUT_INVALID_STATE");
    list.append(warning);
    return;
  }

  section.hidden = archive.length === 0;
  const records = new Map(
    managementState.matches.map(match => [match.matchId, match]),
  );

  const entries = archive.map(cup => {
    const details = document.createElement("details");
    const summary = document.createElement("summary");
    const name = id => cup.participants.find(team =>
      String(team.providerTeamId) === String(id)
    )?.name ?? String(id);

    summary.textContent =
      `${cup.name ?? cupText("Coppa", "Cup")} — ${cupText("Campione", "Champion")}: ` +
      `${name(cup.championProviderTeamId)} ` +
      `(${cup.legs === 2 ? cupText("andata e ritorno", "two legs") : cupText("gara unica", "single leg")})`;
    details.append(summary);

    const matches = document.createElement("ol");
    for (const fixture of cup.fixtures) {
      const record = records.get(fixture.matchId);
      const item = document.createElement("li");
      item.textContent =
        cupText(`Turno ${fixture.round}, gara ${fixture.leg}: `, `Round ${fixture.round}, leg ${fixture.leg}: `) +
        `${name(fixture.homeProviderTeamId)} ` +
        `${record.score.home}–${record.score.away} ` +
        `${name(fixture.awayProviderTeamId)}`;
      const penalties = cup.shootouts?.[`r${fixture.round}-p${fixture.pair}`];
      if (penalties && fixture.leg === cup.legs) {
        const firstLeg = cup.fixtures.find(f => f.round === fixture.round && f.pair === fixture.pair && f.leg === 1);
        item.textContent += ` · ${cupText("Rigori", "Penalties")}: ` +
          `${name(firstLeg.homeProviderTeamId)} ${penalties.home}–${penalties.away} ${name(firstLeg.awayProviderTeamId)}`;
      }
      matches.append(item);
    }
    details.append(matches);
    return details;
  });

  list.replaceChildren(...entries);
}

function renderTournament() {
  const setup = byId("tournament-setup");
  const dashboard = byId("tournament-dashboard");
  if (!setup || !dashboard) return;
  renderCupArchive();
  renderCupStorageStatus();
  if (!cupState) byId("cup-shootout").hidden = true;

  setup.hidden = Boolean(tournamentState || cupState);
  dashboard.hidden = !tournamentState && !cupState;
  const cupMode = byId("tournament-mode").value === "cup";
  const competitionExists = Boolean(tournamentState || cupState || cupLoadError);
  // Keep the competition selector available internally, but never display it.
  byId("competition").closest(".locale-setting").hidden = true;
  byId("cup-leg-setting").hidden =
    Boolean(tournamentState) || !cupMode;
  byId("tournament-create").disabled = competitionExists;
  if (!tournamentState) {
    byId("tournament-error").textContent = "";
  }

  if (cupState) {
    renderCupBracket();
    return;
  }

  // Restore shared controls after leaving the Cup dashboard.
  byId("tournament-play-next").textContent = cupText("Gioca partita del torneo", "Play tournament match");
  byId("tournament-matchday-prev").setAttribute("aria-label", cupText("Giornata precedente", "Previous matchday"));
  byId("tournament-matchday-next").setAttribute("aria-label", cupText("Giornata successiva", "Next matchday"));
  byId("tournament-close").textContent = "Abbandona torneo";

  if (!tournamentState) {
    tournamentViewedMatchday = null;

    const country = byId("tournament-country").value;
    const teams = allCatalogueTeams()
      .filter(team => !country || team.enrichment?.country === country)
      .sort(
      (left, right) => left.name.localeCompare(
        right.name, undefined, { sensitivity: "base" }
      )
    );
    const list = byId("tournament-participants");
    const selected = new Set([
      ...[...list.selectedOptions].map(option => option.value),
      ...tournamentSelectedTeamIds,
    ]);
    tournamentSelectedTeamIds.clear();
    for (const id of selected) tournamentSelectedTeamIds.add(id);
    const selectedOutsideFilter = allCatalogueTeams().filter(team =>
      selected.has(String(team.providerTeamId)) &&
      !teams.some(visible =>
        String(visible.providerTeamId) === String(team.providerTeamId)
      )
    );
    teams.push(...selectedOutsideFilter);
    list.replaceChildren(...teams.map(team => {
      const option = new Option(team.name, String(team.providerTeamId));
      option.selected = selected.has(option.value);
      return option;
    }));

    return;
  }

  const standings = championshipStandings(
    {
      ...tournamentState,
      fixtures: tournamentState.fixtures.map(fixture => ({
        ...fixture,
        matchId: fixture.matchday <= tournamentViewedMatchday
          ? fixture.matchId
          : null,
      })),
    },
    managementState.matches,
  );
  const body = byId("tournament-standings");
  const managedTeamId = managerState
    ? String(managerState.club.providerTeamId)
    : null;

  body.replaceChildren(...standings.map(row => {
    const tr = document.createElement("tr");

    if (
      managedTeamId !== null &&
      String(row.team.providerTeamId) === managedTeamId
    ) {
      tr.classList.add("tournament-manager-row");
    }

    const position = document.createElement("td");
    position.textContent = String(row.position);
    tr.append(position);

    const teamCell = document.createElement("td");
    teamCell.className = "tournament-team-cell";

    const logo = document.createElement("img");
    logo.className = "tournament-team-logo";
    logo.src = row.team.logo;
    logo.alt = "";
    logo.setAttribute("aria-hidden", "true");

    const name = document.createElement("span");
    name.textContent = row.team.name;

    teamCell.append(logo, name);
    tr.append(teamCell);

    for (const value of [
      row.played,
      row.won,
      row.drawn,
      row.lost,
      row.goalsFor,
      row.goalsAgainst,
      row.goalDifference,
      row.points,
    ]) {
      const td = document.createElement("td");
      td.textContent = String(value);
      tr.append(td);
    }

    return tr;
  }));

  const completedFixtures = tournamentState.fixtures.filter(
    fixture => fixture.matchId !== null
  );
  const completed = completedFixtures.length;
  const next = nextTournamentFixture();
  const total = tournamentState.fixtures.length;
  const finalMatchday = Math.max(
    ...tournamentState.fixtures.map(fixture => fixture.matchday)
  );
  const currentMatchday = next?.matchday ?? finalMatchday;

  if (
    tournamentViewedMatchday === null ||
    tournamentViewedMatchday < 1 ||
    tournamentViewedMatchday > finalMatchday
  ) {
    tournamentViewedMatchday = currentMatchday;
  }

  const viewedMatchday = tournamentViewedMatchday;
  const matchdayFixtures = tournamentState.fixtures.filter(
    fixture => fixture.matchday === viewedMatchday
  );

  byId("tournament-matchday-label").textContent =
    `Giornata ${viewedMatchday} di ${finalMatchday}`;
  byId("tournament-matchday-prev").disabled = viewedMatchday <= 1;
  byId("tournament-matchday-next").disabled =
    viewedMatchday >= finalMatchday;

  const recordsById = new Map(
    managementState.matches.map(record => [record.matchId, record])
  );

  byId("tournament-progress").textContent = `${completed} / ${total}`;
  byId("tournament-status").textContent = next
    ? `In corso · giornata ${currentMatchday} di ${finalMatchday}`
    : `Completato · ${finalMatchday} giornate`;

  byId("tournament-next").textContent = next
    ? `Prossima: ${tournamentTeamName(next.homeProviderTeamId)} – ` +
      `${tournamentTeamName(next.awayProviderTeamId)}`
    : "Nessuna partita pendente";

  const renderFixtureItem = (fixture, record = null) => {
    const item = document.createElement("li");
    item.className = "tournament-fixture";

    const teams = allCatalogueTeams();
    const findTeam = id => teams.find(
      team => String(team.providerTeamId) === String(id)
    );

    const home = findTeam(fixture.homeProviderTeamId);
    const away = findTeam(fixture.awayProviderTeamId);

    const name = (team, id, side) => {
      const element = document.createElement("span");
      element.className = `tournament-fixture-name ${side}`;
      element.textContent = team?.name ?? tournamentTeamName(id);
      return element;
    };

    const crest = team => {
      const element = document.createElement("span");
      element.className = "tournament-fixture-crest";
      if (team?.logo) {
        const image = document.createElement("img");
        image.src = team.logo;
        image.alt = "";
        image.setAttribute("aria-hidden", "true");
        image.loading = "lazy";
        element.append(image);
      }
      return element;
    };

    const score = document.createElement("span");
    score.className = "tournament-fixture-score";
    score.textContent = record
      ? `${record.score.home}–${record.score.away}`
      : "–";

    item.append(
      name(home, fixture.homeProviderTeamId, "home"),
      crest(home),
      score,
      crest(away),
      name(away, fixture.awayProviderTeamId, "away")
    );

    return item;
  };

  const resultItems = matchdayFixtures
    .filter(fixture => fixture.matchId !== null)
    .map(fixture => renderFixtureItem(
      fixture, recordsById.get(fixture.matchId)
    ));

  const pendingItems = matchdayFixtures
    .filter(fixture => fixture.matchId === null)
    .map(fixture => renderFixtureItem(fixture));

  byId("tournament-results").replaceChildren(...resultItems);
  byId("tournament-results-empty").hidden = resultItems.length !== 0;
  byId("tournament-pending").replaceChildren(...pendingItems);
  byId("tournament-pending-empty").hidden = pendingItems.length !== 0;

  byId("tournament-play-next").disabled =
    !next || activeTournamentFixtureId !== null ||
    (matchStarted && state.phase !== "finished");
}

function closeTournament() {
  if (matchStarted && state.phase !== "finished") return;

  if (cupState && !tournamentState) {
    if (!ownsCupLock() || cupLoadError || cupShootoutPendingSave || cupShootoutBusy ||
        cupState.championProviderTeamId === undefined) return;
    if (!window.confirm(
      cupText("Archiviare questa Coppa completata? I risultati saranno conservati.",
        "Archive this completed Cup? Its results will be retained.")
    )) return;

    try {
      requireCupOwnership();
      archiveCompletedCup(
        window.localStorage, cupState, managementState.matches,
      );
      clearCup(window.localStorage);
      clearCupRecovery(window.localStorage);
      cupState = null;
      releaseCupOwnership();
      activeTournamentFixtureId = null;
      tournamentViewedMatchday = null;
      renderTournament();
      renderManagerDashboard();
    } catch (failure) {
      window.alert(cupText("Archiviazione Coppa non completata. ", "Cup archive did not complete. ") +
        cupMessage(failure.code ?? "CUP_SHOOTOUT_SAVE_FAILED"));
    }
    return;
  }

  if (!tournamentState) return;
  if (!window.confirm(
    "Abbandonare il torneo corrente? La carriera Manager resterà attiva."
  )) {
    return;
  }

  clearTournament(window.localStorage);
  clearTournamentRecovery(window.localStorage);
  tournamentState = null;
  activeTournamentFixtureId = null;
  tournamentViewedMatchday = null;
  renderTournament();
  renderManagerDashboard();
}

async function createTournamentFromSelection() {
  if (cupLoadError) return;
  if (tournamentState || cupState) {
    byId("tournament-error").textContent =
      "Esiste già una competizione salvata.";
    return;
  }
  if (matchStarted && state.phase !== "finished") return;
  const participants = [...tournamentSelectedTeamIds]
    .map(id => allCatalogueTeams().find(team =>
      String(team.providerTeamId) === id
    ))
    .filter(Boolean);

  try {
    if (byId("tournament-mode").value === "cup") {
      const document = createCupDocument({
        cupId: crypto.randomUUID(),
        participants,
        legs: Number(byId("cup-legs").value),
        name: byId("tournament-name").value.trim() || "Coppa personalizzata",
      });
      if (!window.navigator?.locks?.request) throw new CupShootoutError("CUP_SHOOTOUT_LOCK_UNAVAILABLE");
      // Serialize creation too: independent new Cup IDs cannot protect one key.
      await window.navigator.locks.request("dado-goal.cup.create", { mode: "exclusive" }, () => {
        if (window.localStorage.getItem(CUP_STORAGE_KEY) !== null) {
          throw new CupShootoutError("CUP_SHOOTOUT_STALE");
        }
        saveVerifiedCup(window.localStorage, document, managementState.matches);
      });
      cupState = document;
      activeTournamentFixtureId = null;
      requestCupOwnership();
      renderTournament();
      return;
    }

    const document = createTournamentDocument({
      championshipId: crypto.randomUUID(),
      participants,
      name: byId("tournament-name").value.trim() || "Torneo personalizzato",
    });
    saveTournament(window.localStorage, document);
    tournamentState = document;
    activeTournamentFixtureId = null;
    tournamentViewedMatchday = null;
    byId("tournament-error").textContent = "";
    renderTournament();
  } catch (error) {
    byId("tournament-error").textContent = error instanceof CupShootoutError
      ? cupMessage(error.code) : error.message;
  }
}

function simulateTournamentFixture(fixture) {
  if (!fixture || fixture.matchId !== null) return false;

  const home = allCatalogueTeams().find(
    team => String(team.providerTeamId) ===
      String(fixture.homeProviderTeamId)
  );
  const away = allCatalogueTeams().find(
    team => String(team.providerTeamId) ===
      String(fixture.awayProviderTeamId)
  );

  if (!home || !away) return false;

  const rawHands = byId("hands").value;
  const handsPerTeam = Number(rawHands);

  if (
    !/^\d+$/.test(rawHands.trim()) ||
    !Number.isSafeInteger(handsPerTeam) ||
    handsPerTeam < 1 ||
    handsPerTeam > 1000
  ) {
    showMessage("invalidHands");
    return false;
  }

  const simulatedHandsPerTeam = byId("random-hands").checked
    ? 1 + Math.floor(Math.random() * handsPerTeam)
    : handsPerTeam;

  const simulated = simulateMatch({
    handsPerTeam: simulatedHandsPerTeam,
  });
  const record = createMatchRecord({
    matchId: crypto.randomUUID(),
    playedAt: new Date().toISOString(),
    home,
    away,
    homeGoals: simulated.homeGoals,
    awayGoals: simulated.awayGoals,
    events: [],
    rollEvents: simulated.rolls,
    statistics: matchStatistics(simulated.rolls),
    ratings: [],
  });

  const nextManagement = createManagementDocument({
    ratings: managementState.ratings,
    matches: [...managementState.matches, record],
  });
  const nextTournament = completeFixture(
    tournamentState,
    fixture.fixtureId,
    record.matchId,
  );

  try {
    saveTournamentRecovery(window.localStorage, {
      championshipId: tournamentState.championshipId,
      fixtureId: fixture.fixtureId,
      matchId: record.matchId,
    });
  } catch {
    // A background simulation must not start without durable recovery intent.
    return false;
  }

  try {
    saveManagement(window.localStorage, nextManagement);
    managementState = nextManagement;
    markGlobalSaveDirty();
  } catch {
    try {
      clearTournamentRecovery(window.localStorage);
    } catch {
      // Reload discards a journal whose canonical match does not exist.
    }
    return false;
  }

  try {
    saveTournament(window.localStorage, nextTournament);
    tournamentState = nextTournament;
  } catch {
    // The canonical match is already durable. Advance only the in-memory
    // tournament state so this fixture cannot be simulated twice before reload.
    // Keep the journal: reload will persist the same association.
    tournamentState = nextTournament;
    return false;
  }

  try {
    clearTournamentRecovery(window.localStorage);
  } catch {
    // Reload clears an already reconciled journal idempotently.
  }

  return true;
}

function playTournamentFixture(fixture) {
  if (!fixture) return;
  if (cupState && !tournamentState) {
    try { requireCupOwnership(); } catch { return false; }
    if (cupShootoutPendingSave || cupShootoutBusy) return false;
  }

  const home = allCatalogueTeams().find(team =>
    String(team.providerTeamId) === fixture.homeProviderTeamId
  );
  const away = allCatalogueTeams().find(team =>
    String(team.providerTeamId) === fixture.awayProviderTeamId
  );
  if (!home || !away) return;

  for (const [seat, team] of [[0, home], [1, away]]) {
    const select = byId(`team-${seat}`);
    const teamId = String(team.providerTeamId);

    if (![...select.options].some(option => option.value === teamId)) {
      select.append(new Option(team.name, teamId));
    }

    select.value = teamId;
    applySelectedTeamColor(seat);
  }

  const started = start(true);
  if (started !== true) return false;

  activeTournamentFixtureId = fixture.fixtureId;
  renderTournament();
  renderManagerDashboard();
  return true;
}

function playNextTournamentFixture() {
  if (matchStarted && state.phase !== "finished") return;

  if (cupState && !tournamentState) {
    const currentRound = Math.max(...cupState.fixtures.map(f => f.round));
    const nextCupFixture = cupState.fixtures.find(
      fixture => fixture.round === currentRound && fixture.matchId === null,
    );
    if (nextCupFixture) playTournamentFixture(nextCupFixture);
    return;
  }

  const managedId = managerTournamentTeamId();

  if (managedId === null) {
    playTournamentFixture(nextTournamentFixture());
    return;
  }

  const next = nextTournamentFixture();

  if (!next) {
    renderTournament();
    return;
  }

  const currentMatchday = next.matchday;
  const managedFixture = tournamentState.fixtures.find(
    fixture =>
      fixture.matchId === null &&
      fixture.matchday === currentMatchday &&
      fixtureIncludesTeam(fixture, managedId)
  ) ?? null;

  if (managedFixture) {
    playTournamentFixture(managedFixture);
    return;
  }

  // Recovery path: if the Manager fixture is already complete but some
  // matches from the same day remain pending, close that day before moving on.
  for (const fixture of tournamentState.fixtures.filter(
    fixture =>
      fixture.matchId === null &&
      fixture.matchday === currentMatchday
  )) {
    if (!simulateTournamentFixture(fixture)) break;
  }

  renderTournament();
}

byId("tournament-matchday-prev").addEventListener("click", () => {
  if (!tournamentState || tournamentViewedMatchday === null) return;

  if (tournamentViewedMatchday > 1) {
    tournamentViewedMatchday -= 1;
    renderTournament();
  }
});

byId("tournament-matchday-next").addEventListener("click", () => {
  if (!tournamentState || tournamentViewedMatchday === null) return;

  const finalMatchday = Math.max(
    ...tournamentState.fixtures.map(fixture => fixture.matchday)
  );

  if (tournamentViewedMatchday < finalMatchday) {
    tournamentViewedMatchday += 1;
    renderTournament();
  }
});

byId("tournament-disclosure").querySelector("summary")
  .addEventListener("click", event => {
    const finished = state.phase === "finished";
    if (matchStarted && !finished && activeTournamentFixtureId === null) {
      event.preventDefault();
    }
  });

byId("tournament-create").addEventListener(
  "click", createTournamentFromSelection
);
byId("tournament-play-next").addEventListener(
  "click", playNextTournamentFixture
);
byId("tournament-close").addEventListener(
  "click", closeTournament
);

function manualTeamPalette(seat) {
  const primary = byId(`color-${seat}`).value;
  const enabled = byId(`color-${seat}-enabled`).checked;
  return enabled
    ? [primary, byId(`color-${seat}-secondary`).value]
    : [primary];
}

function syncSecondaryColorControl(seat) {
  const enabled = byId(`color-${seat}-enabled`).checked;
  byId(`color-${seat}-secondary`).disabled = !enabled;
}

function selectedCatalogueTeam(seat) {
  const teamId = byId(`team-${seat}`).value;

  if (!teamId) {
    return null;
  }

  return footballCatalogue?.teams.find(
    candidate => String(candidate.providerTeamId) === String(teamId)
  ) ?? null;
}

function renderSelectedTeamLogo(seat) {
  const logo = byId(`team-logo-${seat}`);
  const team = selectedCatalogueTeam(seat);

  if (!team) {
    logo.hidden = true;
    logo.removeAttribute("src");
    logo.alt = "";
    return;
  }

  logo.src = team.logo;
  logo.alt = team.name;
  logo.hidden = false;
}

function capturePitchTeamLogos() {
  for (const seat of [0, 1]) {
    const logo = byId(`pitch-logo-${seat}`);
    const team = selectedCatalogueTeam(seat);

    if (!team) {
      logo.hidden = true;
      logo.removeAttribute("src");
      logo.alt = "";
      continue;
    }

    logo.src = team.logo;
    logo.alt = team.name;
    logo.hidden = false;
  }
}

function applySelectedTeamColor(seat) {
  const team = selectedCatalogueTeam(seat);

  if (!team) {
    teamPalettes[seat] = null;
    renderSelectedTeamLogo(seat);
    return;
  }

  const palette = [...team.colors];
  teamPalettes[seat] = palette;
  byId(`color-${seat}`).value = palette[0];
  byId(`color-${seat}-enabled`).checked = palette.length === 2;
  byId(`color-${seat}-secondary`).value = palette[1] ?? palette[0];
  syncSecondaryColorControl(seat);
  renderSelectedTeamLogo(seat);
}

for (const seat of [0, 1]) {
  byId(`team-country-${seat}`).addEventListener("change", () => {
    renderCatalogueTeams();
  });
}

byId("tournament-mode").addEventListener("change", () => {
  renderTournament();
});

byId("cup-legs").addEventListener("change", () => {
  renderTournament();
});

byId("tournament-country").addEventListener("change", () => {
  renderTournament();
});

byId("tournament-participants").addEventListener("change", () => {
  const list = byId("tournament-participants");
  for (const option of list.options) {
    if (option.selected) {
      tournamentSelectedTeamIds.add(option.value);
    } else {
      tournamentSelectedTeamIds.delete(option.value);
    }
  }
});

byId("competition").addEventListener("change", () => {
  renderCatalogueTeams();

  for (const seat of [0, 1]) {
    applySelectedTeamColor(seat);
  }

  syncNewManagerControls();
  renderManagerDashboard();
  renderTournament();
  selectTournamentCompetitionParticipants();
  render();
});

for (const seat of [0, 1]) {
  byId(`team-${seat}`).addEventListener("change", () => {
    applySelectedTeamColor(seat);
    syncNewManagerControls();
    render();
  });
  byId(`color-${seat}-enabled`).addEventListener(
    "change", () => syncSecondaryColorControl(seat)
  );
  syncSecondaryColorControl(seat);
}

function palettesConflict(left, right) {
  const normalizedLeft = new Set(
    left.map(color => color.trim().toUpperCase())
  );
  return right.some(
    color => normalizedLeft.has(color.trim().toUpperCase())
  );
}

function start(activate = true) {
  const rawHands = byId("hands").value;

  if (activate) {
    capturePitchTeamLogos();
  }
  const n = Number(rawHands);

  if (
    !/^\d+$/.test(rawHands.trim()) ||
    !Number.isSafeInteger(n) ||
    n < 1 ||
    n > 1000
  ) {
    showMessage("invalidHands");
    return false;
  }

  const handsPerTeam = byId("random-hands").checked
    ? 1 + Math.floor(Math.random() * n)
    : n;

  const selectedTeamIds = [0, 1].map(
    (i) => byId(`team-${i}`).value
  );

  if (activate &&
      selectedTeamIds[0] !== "" &&
      selectedTeamIds[0] === selectedTeamIds[1]) {
    showMessage("sameTeamConflict");
    render();
    return false;
  }

  const selectedTeams = [0, 1].map(
    (i) => catalogueTeam(`team-${i}`)
  );

  const mixedCatalogueMatch =
    selectedTeams.some(Boolean) && !selectedTeams.every(Boolean);

  if (activate && mixedCatalogueMatch) {
    showMessage("mixedTeamConflict");
    render();
    return false;
  }

  names = [0, 1].map((i) =>
    selectedTeams[i]?.name ||
    byId(`team-${i}`).value.trim() ||
    `Squadra ${i + 1}`
  );
  colors = [0, 1].map((i) => byId(`color-${i}`).value);

  const candidatePalettes = [0, 1].map((seat) =>
    selectedTeams[seat]
      ? socialColors(selectedTeams[seat].providerTeamId)
      : manualTeamPalette(seat)
  );

  if (activate && managerState) {
    const selectedManagedTeam = selectedTeams.find(team =>
      team &&
      String(team.providerTeamId) ===
        String(managerState.club.providerTeamId)
    );

    if (selectedManagedTeam) {
      try {
        saveManager(window.localStorage, managerState);
      } catch {
        // Manager persistence failure must never affect gameplay.
      }
    }
  }

  selectedMatchTeams = selectedTeams.map(team => {
    if (!team || !managerState) return team;

    return String(team.providerTeamId) ===
        String(managerState.club.providerTeamId)
      ? managerClubMatchTeam(managerState.club)
      : team;
  });
  matchRecorded = false;

  for (const seat of [0, 1]) {
    teamPalettes[seat] = candidatePalettes[seat];
  }

  state = newMatch(handsPerTeam);
  matchStarted = activate;
  byId("match-disclosure").open = activate;
  autoRunning = false;
  isRolling = false;
  rollEvents = [];
  matchEvents = [];
  rosters = selectedMatchTeams.map((team) =>
    createMatchRoster({ team })
  );
  refereeStates = [createEventState(), createEventState()];
  extraordinaryState = createExtraordinaryEventState();
  lastCommentary = null;
  lastRefereeEvent = null;
  lastExtraordinaryEvent = null;
  lastGoalScorer = null;

  const die = byId("die");
  die.classList.remove("rolling");
  die.classList.add("kickoff-ball");
  die.textContent = "⚽";
  die.setAttribute("aria-label", "In attesa del primo lancio");
  byId("result").textContent = "Calcio d'inizio!";
  if (activate) {
    showMessage("kickoffTurn", { name: names[0] });
  } else {
    showMessage("setupPrompt");
  }
  render();
  return true;
}

async function animateRoll(phase, player, finalFace, durationMs = 700) {
  const die = byId("die");
  const finalIndex = diceFaces[phase].indexOf(finalFace);
  if (finalIndex < 0) throw new RangeError("Unknown die result");

  if (!window.matchMedia("(prefers-reduced-motion: reduce)").matches &&
      durationMs > 0) {
    const cube = buildDie3d(phase, player);
    die.classList.add("rolling");
    cube.classList.add("rolling");

    await sleep(durationMs);

    cube.classList.remove("rolling");
    die.classList.remove("rolling");
  }

  displayFace(phase, player, finalIndex);
  byId("result").textContent = displayDieFace(finalFace);
}

async function roll() {
  if (!matchStarted || state.phase === "finished" || isRolling) return false;

  isRolling = true;
  render();

  const player = state.player;
  const phase = state.phase;
  const face = rollFace(phase);

  await animateRoll(state.phase, player, face);
  const before = state;
  const penaltyResolved = phase === "penalty";
  state = applyRoll(state, face);
  rollEvents.push(createRollEvent({ seat: player, die: phase, face }));
  lastRefereeEvent = null;
  lastExtraordinaryEvent = null;
  lastGoalScorer = null;
  if (commentaryEnabled) {
    lastCommentary = {
      event: { seat: player, die: phase, face },
      before,
      after: state,
      names: [...names],
      index: rollEvents.length - 1
    };
  }

  const handEnded = state.phase === "finished" || state.player !== player;

  if (handEnded) {
    const minute = Math.floor(matchClock(state).seconds / 60);

    let penaltyTaker = null;

    if (penaltyResolved) {
      penaltyTaker = pickScorer(rosters[player], Math.random, minute);
      matchEvents.push(enrichMatchEvent(rosters[player], {
        type: "penalty",
        team: player,
        minute,
        outcome: face,
        number: penaltyTaker
      }));
    }

    // The score comes exclusively from the dice engine.
    if (state.goals[player] > before.goals[player]) {
      lastGoalScorer = penaltyTaker ??
        pickScorer(rosters[player], Math.random, minute);

      if (!penaltyResolved) {
        const assistNumber = lastGoalScorer == null
          ? null
          : pickAssist(rosters[player], lastGoalScorer, Math.random);
        matchEvents.push(enrichMatchEvent(rosters[player], {
          type: "goal",
          team: player,
          minute,
          number: lastGoalScorer,
          ...(assistNumber === null ? {} : { assistNumber }),
        }));
      }
    }

    // Only the team completing its hand gets an event opportunity.
    const refereeEvent = evaluateHandEvent(
      rosters[player],
      refereeStates[player],
      minute,
      Math.random,
      { halftime: matchClock(state).halftime }
    );

    if (refereeEvent) {
      lastRefereeEvent = enrichMatchEvent(
        rosters[player],
        { ...refereeEvent, team: player }
      );
      matchEvents.push(lastRefereeEvent);
    } else {
      // Extraordinary events are a separate narrative channel, but a
      // completed hand may still emit at most one auxiliary event.
      const extraordinaryEvent = evaluateExtraordinaryEvent(
        rosters[player],
        extraordinaryState,
        minute,
        Math.random,
        {
          varEligible:
            penaltyResolved ||
            state.goals[player] > before.goals[player]
        }
      );

      if (extraordinaryEvent) {
        lastExtraordinaryEvent = {
          ...extraordinaryEvent,
          team: player
        };
        matchEvents.push(lastExtraordinaryEvent);
      }
    }
  }

  if (handEnded) {
    const scored = face === "GOAL!";
    showMessage(scored ? "goalBy" : "handEnded", {
      name: names[player]
    });
  } else if (face === "Passaggio" || face === "RIGORE") {
    showMessage("rollAgain");
  } else {
    showMessage("actionContinues");
  }

  if (state.phase === "finished") {
    recordCompletedMatch();
    const [a, b] = state.goals;
    if (a === b) {
      showMessage("draw", { a, b });
    } else {
      showMessage("winner", {
        name: names[a > b ? 0 : 1], a, b
      });
    }
  }

  isRolling = false;
  if (state.phase === "finished") {
    autoRunning = false;
    autoConfig = null;
  }
  render();

  if (handEnded &&
      autoConfig &&
      !autoRunning &&
      state.phase !== "finished" &&
      autoAllowsTeam(autoConfig, state.player)) {
    void runAuto();
  }

  return handEnded;
}

async function runAuto() {
  if (autoRunning || !autoConfig || !matchStarted ||
      state.phase === "finished" || isRolling) return;

  let completedHands = 0;
  autoRunning = true;
  render();

  while (autoRunning && matchStarted && state.phase !== "finished") {
    if (autoQuotaReached(autoConfig, completedHands)) {
      break;
    }

    // A team-specific AUTO remains armed while the other team plays
    // manually. Its run resumes from roll() when control reaches the
    // configured team.
    if (!autoAllowsTeam(autoConfig, state.player)) {
      autoRunning = false;
      render();
      return;
    }

    // AUTO reproduces the manual "roll die" action:
    // wait first, then perform every automatic die roll.
    if (!await autoDelay.wait(autoConfig.rollDelay)) break;
    if (!autoRunning || state.phase === "finished") break;

    const handEnded = await roll();

    if (!autoRunning || state.phase === "finished") break;

    if (handEnded) {
      completedHands += 1;

      if (autoQuotaReached(autoConfig, completedHands) ||
          !autoAllowsTeam(autoConfig, state.player)) {
        break;
      }

      if (!await autoDelay.wait(autoConfig.handDelay)) break;
      if (!autoRunning || state.phase === "finished") break;
    }
  }

  autoRunning = false;

  if (state.phase === "finished" ||
      autoQuotaReached(autoConfig, completedHands)) {
    autoConfig = null;
  }

  render();
}

byId("commentary-toggle").addEventListener("click", () => {
  commentaryEnabled = !commentaryEnabled;
  render();
});

const autoDialog = byId("auto-config");
const autoMode = byId("auto-mode");
const autoHandLimitRow = byId("auto-hand-limit-row");
const autoHandLimit = byId("auto-hand-limit");

function updateAutoConfigForm() {
  const maximum = remainingHands(state);
  const previous = autoHandLimit.value;

  autoHandLimit.replaceChildren();

  const all = document.createElement("option");
  all.value = "all";
  all.textContent = "Tutti i lanci rimanenti";
  autoHandLimit.append(all);

  for (let count = 1; count <= maximum; count += 1) {
    const option = document.createElement("option");
    option.value = String(count);
    option.textContent = String(count);
    autoHandLimit.append(option);
  }

  autoHandLimit.value =
    previous !== "" &&
    [...autoHandLimit.options].some(option => option.value === previous)
      ? previous
      : "all";
}

autoMode.addEventListener("change", updateAutoConfigForm);

byId("auto-cancel").addEventListener("click", () => {
  autoDialog.close();
});

byId("auto-config-form").addEventListener("submit", event => {
  event.preventDefault();

  const handLimit = autoHandLimit.value === "all"
    ? null
    : Number(autoHandLimit.value);

  if (handLimit !== null && handLimit > remainingHands(state)) {
    return;
  }

  autoConfig = createAutoConfig({
    mode: autoMode.value,
    handLimit,
    rollDelay: Math.round(Number(byId("auto-roll-delay").value) * 1000),
    handDelay: Math.round(Number(byId("auto-hand-delay").value) * 1000)
  });

  autoDialog.close();
  void runAuto();
});

byId("auto").addEventListener("click", () => {
  if (autoRunning) {
    autoRunning = false;
    autoDelay.cancel();
    render();
    return;
  }

  autoConfig = null;
  updateAutoConfigForm();
  autoDialog.showModal();
});

byId("manager-disclosure").querySelector("summary")
  .addEventListener("click", event => {
    // Career information remains accessible before creation.
  });

byId("manager-close").addEventListener(
  "click", closeManagerCareer
);

byId("reset-local-data").addEventListener("click", resetLocalData);

byId("new").addEventListener("click", () => {
  if (isRolling) {
    return;
  }

  if (matchStarted && state.phase !== "finished") {
    if (activeTournamentFixtureId !== null) return;
    if (!window.confirm(translate("cancelConfirm"))) return;
    start(false);
    showMessage("matchCancelled");
    return;
  }

  start(true);
});

document.querySelector(".multiplayer-button").addEventListener("click", event => {
  if (matchStarted && state.phase !== "finished") {
    event.preventDefault();
  }
});
byId("roll").addEventListener("click", () => {
  void roll();
});
initializeManagerUi();
initializeGlobalSaveUi();
start(false);
initializeLocale(() => {
  syncCompetitionSelector();
  render();
  renderManagerDashboard();
  renderTournament();
});

syncCompetitionSelector();
requestCupOwnership();
void reloadLiveCatalogue();

// The browser releases Web Locks on document destruction. Release explicitly for
// bfcache too, and revalidate storage before enabling a restored page.
window.addEventListener("pagehide", releaseCupOwnership);
window.addEventListener("pageshow", event => {
  if (event.persisted) {
    reloadCupForOwnership();
    requestCupOwnership();
    renderTournament();
  }
});
window.addEventListener("beforeunload", event => {
  if (!cupShootoutPendingSave) return;
  event.preventDefault();
  event.returnValue = "";
});
window.addEventListener("storage", event => {
  if (![CUP_STORAGE_KEY, "dado-goal.team-management.v1"].includes(event.key) && event.key !== null) return;
  if (ownsCupLock() || cupShootoutPendingSave) {
    // A cooperating tab cannot write the owned Cup. Preserve a pending candidate
    // if an older client or an external writer bypasses the lock.
    if (event.key === CUP_STORAGE_KEY || event.key === null) cupShootoutError = "CUP_SHOOTOUT_STALE";
  } else {
    reloadCupForOwnership();
  }
  renderTournament();
});
