import {
  autoAllowsTeam,
  autoQuotaReached,
  createAutoConfig,
  createInterruptibleDelay,
  remainingHands
} from "./auto-play.mjs";

import { applyRoll, newMatch, rollFace } from "./engine.mjs";
import { matchStatistics } from "./statistics.mjs";
import { matchClock } from "./match-clock.mjs";
import { createRoster, pickScorer } from "./match-roster.mjs";
import { importCatalogueJson } from "./catalogue-json.mjs";
import { createMatchRoster } from "./catalogue-roster.mjs";
import {
  createManagerClub,
  managerClubMatchTeam,
  updateManagerSelection,
  updateManagerPlayers,
} from "./manager-profile.mjs";
import { createNextMatch } from "./manager-next-match.mjs";
import {
  createManagerDocument,
  exportManagerJson,
  importManagerJson,
  loadManager,
  saveManager,
  clearManager,
} from "./manager-store.mjs";
import { createMatchRecord } from "./team-history.mjs";
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
import { DICE_ART } from "./dice-art.mjs";
import { initializeLocale, getLocale, translate, formatMessage } from "./i18n.mjs";
import { socialColors } from "./team-colors.mjs";

const byId = (id) => document.getElementById(id);
const phaseNames = {
  team: "teamDie",
  pink: "pinkDie",
  result: "finalDie",
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
});

function displayDieFace(face) {
  const key = dieFaceMessageKeys[face];
  if (!key) throw new RangeError("Unknown die face");
  return translate(key);
}

function diceKind(phase, player) {
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
  const art = DICE_ART[kind];

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

  byId("manager-enabled").disabled =
    matchStarted && state.phase !== "finished";

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
  byId("guide-result").classList.toggle("active", state.phase === "result");

  const [home, away] = matchStatistics(rollEvents);
  const metrics = [
    ["goals", "goals"],
    ["shots", "shots"],
    ["crosses", "crosses"],
    ["passes", "passes"],
    ["saves", "saves"],
    ["woodwork", "woodwork"],
    ["penalties", "penalties"],
  ];
  byId("match-stats").replaceChildren(
    ...metrics.map(([label, key]) => {
      const row = document.createElement("div");
      row.className = "stat-row stat-comparison";

      const values = document.createElement("div");
      values.className = "stat-values";

      const left = document.createElement("strong");
      left.textContent = String(home[key]);

      const name = document.createElement("span");
      name.textContent = translate(label);

      const right = document.createElement("strong");
      right.textContent = String(away[key]);

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

  // Match configuration is immutable from kick-off to the final whistle.
  byId("competition").disabled = playing;
  byId("hands").disabled = playing;

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

  byId("new").disabled = isRolling || (!playing && sameSelectedTeam);
  byId("new").textContent = translate(
    playing ? "cancelMatch" : "newGame"
  );

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



function recordCompletedMatch() {
  if (matchRecorded || state.phase !== "finished") return;

  // Management history is opt-in: at least one imported real team
  // must have been selected for this match.
  if (!selectedMatchTeams.some(Boolean)) return;

  const teamSnapshot = (team, seat) => team ?? {
    provider: "local",
    providerTeamId: `local-${seat}`,
    name: names[seat],
    players: [],
  };

  const record = createMatchRecord({
    matchId: crypto.randomUUID(),
    playedAt: new Date().toISOString(),
    home: teamSnapshot(selectedMatchTeams[0], 0),
    away: teamSnapshot(selectedMatchTeams[1], 1),
    homeGoals: state.goals[0],
    awayGoals: state.goals[1],
    events: matchEvents,
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

  try {
    saveManagement(window.localStorage, nextState);
    managementState = nextState;
  } catch {
    // Local persistence failure must never affect gameplay.
  }

  if (managerState &&
      byId("manager-enabled").checked &&
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
  const enabled = byId("manager-enabled").checked;
  const club = managerState?.club ?? null;

  syncNewManagerControls();

  byId("manager-empty").hidden = enabled && Boolean(club);
  byId("manager-dashboard").hidden = !enabled || !club;

  if (!enabled || !club) return;

  byId("manager-club-name").textContent = club.name;

  const matches = managerClubMatches();
  byId("manager-match-count").textContent = String(matches.length);

  const opponentSelect = byId("manager-opponent");
  const preparedOpponentId =
    managerState.nextMatch?.opponentProviderTeamId ?? "";

  opponentSelect.replaceChildren(
    new Option("Scegli un avversario", "")
  );

  for (const team of [...(footballCatalogue?.teams ?? [])]
      .sort((left, right) => left.name.localeCompare(right.name, "it"))) {
    if (String(team.providerTeamId) === String(club.providerTeamId)) {
      continue;
    }

    opponentSelect.append(
      new Option(team.name, String(team.providerTeamId))
    );
  }

  opponentSelect.value = preparedOpponentId;
  const hasPreparedOpponent = opponentSelect.value !== "";

  byId("manager-play-next").disabled = !hasPreparedOpponent;
  byId("manager-next-action").textContent = hasPreparedOpponent
    ? "Gioca la prossima partita"
    : "Scegli il prossimo avversario";

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

  const history = byId("manager-history");
  history.replaceChildren();

  for (const match of matches.slice(-5).reverse()) {
    const item = document.createElement("li");
    item.textContent =
      `${match.home.name} ${match.score.home}–${match.score.away} ` +
      `${match.away.name}`;
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

function syncNewManagerControls() {
  const managerEnabled = byId("manager-enabled").checked;

  for (const seat of [0, 1]) {
    const button = byId(`manager-new-${seat}`);
    button.hidden = managerEnabled;
    button.disabled =
      managerEnabled || catalogueTeam(`team-${seat}`) === null;
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
  byId("manager-enabled").checked = true;
  renderManagerDashboard();
}

function exportManagerSave() {
  if (!managerState) return;

  const source = exportManagerJson(managerState);
  const blob = new Blob([source], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");

  const clubSlug = managerState.club.name
    .normalize("NFKD")
    .replace(/[\u0300-\u036f]/g, "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "") || "squadra";

  link.href = url;
  link.download = `dado-goal-manager-${clubSlug}.json`;
  link.click();

  URL.revokeObjectURL(url);
}

async function importManagerSave(file) {
  if (!file) return;

  const imported = importManagerJson(await file.text());
  const managedId = String(imported.club.providerTeamId);
  const managedTeam = allCatalogueTeams().find(
    team => String(team.providerTeamId) === managedId
  );

  if (!managedTeam) {
    throw new Error(
      "La squadra del salvataggio Manager non è disponibile nei cataloghi."
    );
  }

  managerState = imported;
  saveManager(window.localStorage, managerState);

  byId("manager-enabled").checked = true;

  const competitionValue =
    `${managedTeam.competitionId}:${managedTeam.season}`;

  if (competitions.some(({ value }) => value === competitionValue)) {
    byId("competition").value = competitionValue;
  }

  renderCatalogueTeams();

  const managedSelect = byId("team-0");
  if (![...managedSelect.options].some(option => option.value === managedId)) {
    managedSelect.append(new Option(managedTeam.name, managedId));
  }
  managedSelect.value = managedId;

  applySelectedTeamColor(0);
  syncNewManagerControls();

  // A Manager save stores the career, not an in-progress match.
  // Reset the match presentation so no pre-import state remains visible.
  start(false);

  render();
  renderManagerDashboard();
}

function prepareManagerOpponent() {
  if (!managerState) return;

  const opponentProviderTeamId = byId("manager-opponent").value;

  managerState = createManagerDocument({
    club: managerState.club,
    management: managerState.management,
    nextMatch: opponentProviderTeamId === ""
      ? null
      : createNextMatch({
          managedProviderTeamId: managerState.club.providerTeamId,
          opponentProviderTeamId,
        }),
  });

  persistManagerState();
}

function playManagerNextMatch() {
  if (!managerState?.nextMatch) return;

  const managedId = String(managerState.club.providerTeamId);
  const opponentId = String(
    managerState.nextMatch.opponentProviderTeamId
  );

  const managedTeam = footballCatalogue?.teams.find(
    team => String(team.providerTeamId) === managedId
  );
  const opponentTeam = footballCatalogue?.teams.find(
    team => String(team.providerTeamId) === opponentId
  );

  if (!managedTeam || !opponentTeam) {
    window.alert(
      "Squadra Manager o avversario non disponibili nel catalogo."
    );
    return;
  }

  for (const [seat, team] of [
    [0, managedTeam],
    [1, opponentTeam],
  ]) {
    const select = byId(`team-${seat}`);
    const teamId = String(team.providerTeamId);

    if (![...select.options].some(option => option.value === teamId)) {
      select.append(new Option(team.name, teamId));
    }

    select.value = teamId;
  }

  applySelectedTeamColor(0);
  applySelectedTeamColor(1);

  start(true);
}

function initializeManagerUi() {
  const toggle = byId("manager-enabled");

  toggle.addEventListener("change", () => {
    if (toggle.checked && !managerState) {
      const team = catalogueTeam("team-0");

      if (team) {
        managerState = createManagerDocument({
          club: createManagerClub(team),
          management: createManagementDocument(),
        });

        try {
          persistManagerState();
        } catch {
          // Manager persistence failure must never block quick match.
        }
      }
    }

    renderManagerDashboard();
  });

  byId("manager-edit").addEventListener("click", editManagerSelection);
  byId("manager-opponent").addEventListener(
    "change",
    prepareManagerOpponent
  );
  byId("manager-play-next").addEventListener(
    "click",
    playManagerNextMatch
  );
  for (const seat of [0, 1]) {
  byId(`manager-new-${seat}`).addEventListener(
    "click",
    () => newManagerCareer(seat)
  );
}
  byId("manager-export").addEventListener("click", exportManagerSave);

  byId("manager-import").addEventListener("click", () => {
    byId("manager-import-file").click();
  });

  byId("manager-import-file").addEventListener("change", async event => {
    try {
      await importManagerSave(event.target.files?.[0]);
    } catch (caught) {
      window.alert(
        caught instanceof Error
          ? caught.message
          : "Salvataggio Manager non valido."
      );
    } finally {
      event.target.value = "";
    }
  });

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
  }

  if (selector.options.length !== competitions.length) {
    throw new Error("Competition selector synchronization failed");
  }
}

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
  const filteredTeams = selectedCompetitionTeams().sort(
    (left, right) => left.name.localeCompare(
      right.name, undefined, { sensitivity: "base" }
    )
  );
  const allTeams = allCatalogueTeams();

  for (let seat = 0; seat < 2; seat += 1) {
    const select = byId(`team-${seat}`);
    const current = select.value;
    const currentTeam = allTeams.find(team =>
      String(team.providerTeamId) === current
    );

    const visibleTeams = [...filteredTeams];

    // Preserve an already-selected team from another competition so the
    // two seats may intentionally represent different championships.
    if (currentTeam &&
        !visibleTeams.some(team =>
          String(team.providerTeamId) === String(currentTeam.providerTeamId)
        )) {
      visibleTeams.push(currentTeam);
    }

    visibleTeams.sort(
      (left, right) => left.name.localeCompare(
        right.name, undefined, { sensitivity: "base" }
      )
    );

    select.replaceChildren(new Option(`Squadra ${seat + 1}`, ""));

    for (const team of visibleTeams) {
      select.append(new Option(team.name, String(team.providerTeamId)));
    }

    if ([...select.options].some(option => option.value === current)) {
      select.value = current;
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
}


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

byId("competition").addEventListener("change", () => {
  renderCatalogueTeams();
  syncNewManagerControls();
  renderManagerDashboard();
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
    return;
  }

  const selectedTeamIds = [0, 1].map(
    (i) => byId(`team-${i}`).value
  );

  if (activate &&
      selectedTeamIds[0] !== "" &&
      selectedTeamIds[0] === selectedTeamIds[1]) {
    showMessage("sameTeamConflict");
    render();
    return;
  }

  const selectedTeams = [0, 1].map(
    (i) => catalogueTeam(`team-${i}`)
  );

  const mixedCatalogueMatch =
    selectedTeams.some(Boolean) && !selectedTeams.every(Boolean);

  if (activate && mixedCatalogueMatch) {
    showMessage("mixedTeamConflict");
    render();
    return;
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

  const realTeamKitConflict =
    selectedTeams.every(Boolean) &&
    palettesConflict(candidatePalettes[0], candidatePalettes[1]);

  if (activate && realTeamKitConflict) {
    showMessage("kitColorConflict");
    render();
    return;
  }

  if (activate &&
      managerState &&
      byId("manager-enabled").checked) {
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

    return byId("manager-enabled").checked &&
      String(team.providerTeamId) ===
        String(managerState.club.providerTeamId)
      ? managerClubMatchTeam(managerState.club)
      : team;
  });
  matchRecorded = false;

  for (const seat of [0, 1]) {
    teamPalettes[seat] = candidatePalettes[seat];
  }

  state = newMatch(n);
  matchStarted = activate;
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
  const penaltyResolved =
    phase === "result" &&
    face !== "RIGORE" &&
    rollEvents.at(-1)?.die === "result" &&
    rollEvents.at(-1)?.face === "RIGORE";
  state = applyRoll(state, face);
  rollEvents.push({ seat: player, die: phase, face });
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
        matchEvents.push(enrichMatchEvent(rosters[player], {
          type: "goal",
          team: player,
          minute,
          number: lastGoalScorer
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

byId("new").addEventListener("click", () => {
  if (isRolling) {
    return;
  }

  if (matchStarted && state.phase !== "finished") {
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
start(false);
initializeLocale(() => {
  syncCompetitionSelector();
  render();
  renderManagerDashboard();
});

syncCompetitionSelector();
void reloadLiveCatalogue();
