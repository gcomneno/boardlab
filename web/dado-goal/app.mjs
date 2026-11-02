import { applyRoll, newMatch, rollFace } from "./engine.mjs";
import { matchStatistics } from "./statistics.mjs";
import { matchClock } from "./match-clock.mjs";
import { createRoster, pickScorer } from "./match-roster.mjs";
import { importCatalogueJson } from "./catalogue-json.mjs";
import { createMatchRoster } from "./catalogue-roster.mjs";
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
import { describeRoll, describeMatchEvent } from "./commentary.mjs";
import { DICE_ART } from "./dice-art.mjs";
import { initializeLocale, getLocale, translate, formatMessage } from "./i18n.mjs";

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

function diceKind(phase, player) {
  return phase === "team"
    ? (player === 0 ? "yellow" : "blue")
    : phase === "pink" ? "pink" : "final";
}

function shade(hex, factor) {
  const channels = [1, 3, 5].map((i) =>
    Math.round(parseInt(hex.slice(i, i + 2), 16) * factor),
  );
  return "#" + channels.map((n) =>
    n.toString(16).padStart(2, "0")).join("");
}

function displayFace(phase, player, index) {
  const die = byId("die");
  const kind = diceKind(phase, player);
  const svg = DICE_ART[kind][index];
  if (!svg) throw new RangeError("Invalid die face");

  const color = colors[player];
  die.style.removeProperty("--die-light");
  die.style.removeProperty("--die-dark");
  die.style.removeProperty("--die-ink");

  if (phase === "team") {
    die.style.setProperty("--die-light", color);
    die.style.setProperty("--die-dark", shade(color, 0.65));
    die.style.setProperty("--die-ink", shade(color, 0.28));
  }

  // SVG controllati dal repository, incorporati affinché
  // ereditino i colori CSS della squadra.
  die.innerHTML = svg;
  die.setAttribute(
    "aria-label",
    `Dado ${kind}: ${diceFaces[phase][index]}`,
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
let commentaryEnabled = true;
let lastCommentary = null;
let lastRefereeEvent = null;
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

      const description = document.createElement("span");
      description.textContent = formatMessage(eventKeys[event.type], {
        ...eventDisplayValues(event),
        team: names[event.team]
      });

      item.append(minute, description);
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
          : null
      ].filter(Boolean).join(" ")
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
    panel.style.setProperty("--team-color", colors[player]);
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
  byId("roll").disabled = !playing || isRolling || autoRunning;
  byId("auto").disabled = !playing;
  byId("auto").textContent = autoRunning ? "■ STOP" : "▶ AUTO";
  byId("auto").setAttribute("aria-pressed", String(autoRunning));
  byId("new").disabled = isRolling;
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
let matchRecorded = false;

try {
  managementState = loadManagement(window.localStorage);
} catch {
  managementState = createManagementDocument();
}

function renderManagementStatus() {
  byId("management-status").textContent =
    `${managementState.matches.length} partite archiviate localmente`;
}

function downloadManagementJson() {
  const blob = new Blob(
    [exportManagementJson(managementState)],
    { type: "application/json" }
  );
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = "dado-goal-management.json";
  link.click();
  URL.revokeObjectURL(url);
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

  try {
    saveManagement(window.localStorage, nextState);
    managementState = nextState;
    matchRecorded = true;
  } catch {
    // Local persistence failure must never affect gameplay.
  }
}


function catalogueTeam(selectId) {
  if (!footballCatalogue) return null;

  const value = byId(selectId).value;
  if (value === "") return null;

  return footballCatalogue.teams.find(
    (team) => team.providerTeamId === value
  ) ?? null;
}

function ratingPlayerKey(team, player) {
  return `${team.providerTeamId}:${player.providerPlayerId}`;
}

function ratingEntry(key) {
  return managementState.ratings.find(entry => entry.key === key) ?? null;
}

function renderRatingPlayers() {
  const select = byId("ratings-player");
  select.replaceChildren();

  const teams = [0, 1]
    .map(i => catalogueTeam(`catalogue-team-${i}`))
    .filter(Boolean);

  if (teams.length === 0) {
    select.append(new Option("Seleziona una squadra reale", ""));
    select.disabled = true;
    byId("ratings-save").disabled = true;
    byId("ratings-fields").replaceChildren();
    return;
  }

  select.append(new Option("Seleziona giocatore", ""));

  for (const team of teams) {
    for (const player of team.players) {
      const key = ratingPlayerKey(team, player);
      select.append(new Option(
        `${team.name} · ${player.name}`,
        key
      ));
    }
  }

  select.disabled = false;
  renderRatingFields();
}

function renderRatingFields() {
  const key = byId("ratings-player").value;
  const container = byId("ratings-fields");
  container.replaceChildren();
  byId("ratings-save").disabled = !key;

  if (!key) return;

  const saved = ratingEntry(key);
  const ratings = createPlayerRatings(saved?.ratings);

  for (const field of RATING_FIELDS) {
    const label = document.createElement("label");
    label.textContent = field;

    const select = document.createElement("select");
    select.dataset.ratingField = field;

    for (const level of RATING_LEVELS) {
      select.append(new Option(level, level));
    }

    select.value = ratings[field];
    label.append(select);
    container.append(label);
  }
}

function saveCurrentRating() {
  const key = byId("ratings-player").value;
  if (!key) return;

  const [teamId, playerId] = key.split(":");
  const team = footballCatalogue?.teams.find(
    candidate => candidate.providerTeamId === teamId
  );
  const player = team?.players.find(
    candidate => candidate.providerPlayerId === playerId
  );
  if (!team || !player) return;

  const values = {};
  for (const select of document.querySelectorAll("[data-rating-field]")) {
    values[select.dataset.ratingField] = select.value;
  }

  const entry = {
    key,
    providerTeamId: team.providerTeamId,
    providerPlayerId: player.providerPlayerId,
    playerName: player.name,
    ratings: createPlayerRatings(values),
  };

  const ratings = managementState.ratings.filter(item => item.key !== key);
  const nextState = createManagementDocument({
    ratings: [...ratings, entry],
    matches: managementState.matches,
  });

  saveManagement(window.localStorage, nextState);
  managementState = nextState;
  byId("ratings-status").textContent =
    `Valutazione salvata per ${player.name}`;
  renderManagementStatus();
}

function renderCatalogueTeams() {
  for (let seat = 0; seat < 2; seat += 1) {
    const select = byId(`catalogue-team-${seat}`);
    select.replaceChildren(new Option("Partita rapida", ""));

    for (const team of footballCatalogue?.teams ?? []) {
      select.append(new Option(team.name, team.providerTeamId));
    }

    select.disabled = !footballCatalogue;
  }

  renderRatingPlayers();
}

async function loadCatalogueFile(file) {
  const source = await file.text();
  footballCatalogue = importCatalogueJson(source);
  renderCatalogueTeams();
  byId("catalogue-status").textContent =
    `${footballCatalogue.teams.length} squadre importate`;
}

function start(activate = true) {
  const rawHands = byId("hands").value;
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

  const selectedTeams = [0, 1].map(
    (i) => catalogueTeam(`catalogue-team-${i}`)
  );

  selectedMatchTeams = selectedTeams;
  matchRecorded = false;

  names = [0, 1].map((i) =>
    selectedTeams[i]?.name ||
    byId(`team-${i}`).value.trim() ||
    `Squadra ${i + 1}`
  );
  colors = [0, 1].map((i) => byId(`color-${i}`).value);
  state = newMatch(n);
  matchStarted = activate;
  autoRunning = false;
  isRolling = false;
  rollEvents = [];
  matchEvents = [];
  rosters = selectedTeams.map((team) =>
    createMatchRoster({ team })
  );
  refereeStates = [createEventState(), createEventState()];
  lastCommentary = null;
  lastRefereeEvent = null;
  lastGoalScorer = null;

  const die = byId("die");
  die.classList.remove("rolling");
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

async function animateRoll(phase, player, finalFace) {
  const die = byId("die");
  const finalIndex = diceFaces[phase].indexOf(finalFace);
  if (finalIndex < 0) throw new RangeError("Unknown die result");

  if (!window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
    die.classList.add("rolling");
    for (let index = 0; index < 10; index += 1) {
      displayFace(phase, player, index % 6);
      await sleep(70);
    }
    die.classList.remove("rolling");
  }

  displayFace(phase, player, finalIndex);
  byId("result").textContent = finalFace;
}

async function roll() {
  if (!matchStarted || state.phase === "finished" || isRolling) return;

  isRolling = true;
  render();

  const player = state.player;
  const phase = state.phase;
  const face = rollFace(phase);

  await animateRoll(state.phase, player, face);
  const before = state;
  state = applyRoll(state, face);
  rollEvents.push({ seat: player, die: phase, face });
  lastRefereeEvent = null;
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

    // The score comes exclusively from the dice engine.
    if (state.goals[player] > before.goals[player]) {
      lastGoalScorer = pickScorer(rosters[player]);
      matchEvents.push(enrichMatchEvent(rosters[player], {
        type: "goal",
        team: player,
        minute,
        number: lastGoalScorer
      }));
    }

    // Only the team completing its hand gets an event opportunity.
    const refereeEvent = evaluateHandEvent(
      rosters[player], refereeStates[player], minute
    );

    if (refereeEvent) {
      lastRefereeEvent = enrichMatchEvent(
        rosters[player],
        { ...refereeEvent, team: player }
      );
      matchEvents.push(lastRefereeEvent);
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
  if (state.phase === "finished") autoRunning = false;
  render();
}

async function runAuto() {
  if (autoRunning || !matchStarted || state.phase === "finished" || isRolling) return;

  autoRunning = true;
  render();

  while (autoRunning && matchStarted && state.phase !== "finished") {
    await roll();
    if (!autoRunning || state.phase === "finished") break;
    await sleep(350);
  }

  autoRunning = false;
  render();
}

byId("commentary-toggle").addEventListener("click", () => {
  commentaryEnabled = !commentaryEnabled;
  render();
});

byId("auto").addEventListener("click", () => {
  if (autoRunning) {
    autoRunning = false;
    render();
  } else {
    void runAuto();
  }
});

byId("new").addEventListener("click", () => {
  if (isRolling) return;

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
byId("catalogue-team-0").addEventListener("change", renderRatingPlayers);
byId("catalogue-team-1").addEventListener("change", renderRatingPlayers);
byId("ratings-player").addEventListener("change", renderRatingFields);
byId("ratings-save").addEventListener("click", saveCurrentRating);

byId("management-export").addEventListener("click", () => {
  downloadManagementJson();
});

byId("management-import").addEventListener("change", async (event) => {
  const [file] = event.target.files;
  if (!file) return;

  try {
    const imported = importManagementJson(await file.text());
    saveManagement(window.localStorage, imported);
    managementState = imported;
    renderManagementStatus();
    renderRatingPlayers();
  } catch {
    byId("management-status").textContent = "Dati gestione non validi";
  }

  event.target.value = "";
});

byId("management-clear").addEventListener("click", () => {
  if (!window.confirm("Cancellare tutti i dati di gestione locali?")) return;

  try {
    clearManagement(window.localStorage);
    managementState = createManagementDocument();
    renderManagementStatus();
    renderRatingPlayers();
  } catch {
    byId("management-status").textContent =
      "Impossibile cancellare i dati locali";
  }
});

byId("catalogue-file").addEventListener("change", async (event) => {
  const [file] = event.target.files;
  if (!file) return;

  try {
    await loadCatalogueFile(file);
  } catch {
    footballCatalogue = null;
    renderCatalogueTeams();
    byId("catalogue-status").textContent = "Catalogo non valido";
  }
});

renderManagementStatus();
start(false);
initializeLocale(render);
