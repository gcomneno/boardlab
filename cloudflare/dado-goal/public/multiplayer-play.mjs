
import { newMatch, applyRoll } from "./engine.mjs";
import {
  MultiplayerClient,
  MultiplayerAPIError,
} from "./multiplayer-client.mjs";
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
  describeMatchEvent
} from "./commentary.mjs";
import { DICE_ART } from "./dice-art.mjs";
import { initializeLocale, getLocale, translate, formatMessage } from "./i18n.mjs";
import { socialColors } from "./team-colors.mjs";

const byId = (id) => document.getElementById(id);

function encodeInvite(matchId, inviteToken) {
  return `DG1.${matchId}.${inviteToken}`;
}

function decodeInvite(code) {
  const match = /^DG1\.([A-Za-z0-9_-]{20,128})\.([A-Za-z0-9_-]{20,256})$/
    .exec(code.trim());
  if (!match) throw new Error("Codice invito non valido.");
  return { matchId: match[1], inviteToken: match[2] };
}

let browserStorage = null;
try {
  browserStorage = window.localStorage;
} catch {}

let csrfToken = null;
let authenticated = false;
let snapshot = null;
let multiplayerBusy = false;

const multiplayerClient = new MultiplayerClient({
  storage: browserStorage,
  fetchImpl: (input, init = {}) => {
    const headers = new Headers(init.headers);
    if (csrfToken && init.method && init.method !== "GET") {
      headers.set("X-Dado-CSRF", csrfToken);
    }
    return globalThis.fetch(input, {
      ...init,
      headers,
      credentials: "same-origin",
    });
  },
});
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
  return teamPalettes[player] ??
    (colors[player] ? [colors[player]] : ["#f2c94c"]);
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

  const color = visualTeamPalette(player)[0];
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
let autoEnabled = false;
let autoRunning = false;
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

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function syncMultiplayerControls() {
  const hasSeat = Boolean(multiplayerClient.seatToken);
  const invite = byId("invite-code").value.trim();
  let validTeams = false;

  try {
    selectedMultiplayerTeams();
    validTeams = true;
  } catch {}

  byId("login").disabled = authenticated || multiplayerBusy;
  byId("logout").disabled = !authenticated || multiplayerBusy;
  byId("create").disabled =
    !authenticated || hasSeat || multiplayerBusy || !validTeams;
  byId("join").disabled =
    !authenticated || hasSeat || multiplayerBusy || !invite;
  byId("copy-invite").disabled = !invite || multiplayerBusy;
}

function render() {
  syncMultiplayerControls();

  if (!state) {
    byId("roll").disabled = true;
    byId("auto").disabled = true;
    byId("commentary-toggle").disabled = true;
    byId("result").textContent = "⚽";
    byId("match-clock").textContent = "—";
    return;
  }
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
  const localSeat = snapshot?.seat;
  const localTurn =
    localSeat === 0 || localSeat === 1
      ? state.player === localSeat
      : false;
  byId("roll").disabled = !playing || isRolling || !localTurn;
  byId("commentary-toggle").disabled = !playing;

  const autoButton = byId("auto");
  autoButton.disabled =
    !playing || isRolling || (!localTurn && !autoEnabled);
  autoButton.textContent = autoEnabled ? "■ AUTO" : "▶ AUTO";
  autoButton.setAttribute("aria-pressed", String(autoEnabled));
  const selectedTeamIds = [0, 1].map(
    seat => byId(`team-${seat}`).value
  );
  const sameSelectedTeam =
    selectedTeamIds[0] !== "" &&
    selectedTeamIds[0] === selectedTeamIds[1];

  byId("phase").textContent = finished
    ? translate("matchFinished")
    : phaseName(state.phase);
  const activeTeamName = names[state.player] ||
    `Squadra ${state.player + 1}`;
  byId("turn").textContent = finished
    ? translate("finalWhistle")
    : formatMessage("matchTurn", { name: activeTeamName });
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
    (team) => String(team.providerTeamId) === value
  ) ?? null;
}






function renderCatalogueTeams() {
  const teams = [...(footballCatalogue?.teams ?? [])].sort(
    (left, right) => left.name.localeCompare(
      right.name, undefined, { sensitivity: "base" }
    )
  );

  for (let seat = 0; seat < 2; seat += 1) {
    const select = byId(`team-${seat}`);
    const current = select.value;
    select.replaceChildren(new Option(`Squadra ${seat + 1}`, ""));

    for (const team of teams) {
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
  footballCatalogue = importCatalogueJson(source);
  renderCatalogueTeams();
}

const competitionsByLocale = Object.freeze({
  it: Object.freeze([
    Object.freeze({ value: "135:2024", label: "Serie A" }),
    Object.freeze({ value: "136:2024", label: "Serie B" }),
  ]),
  en: Object.freeze([
    Object.freeze({ value: "39:2024", label: "Premier League" }),
    Object.freeze({ value: "40:2024", label: "Championship" }),
  ]),
});

function syncCompetitionSelector() {
  const selector = byId("competition");
  const language = byId("language").value;
  const competitions = competitionsByLocale[language] ??
    competitionsByLocale.en;
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

async function reloadLiveCatalogue() {
  footballCatalogue = null;
  renderCatalogueTeams();

  try {
    await loadLiveCatalogue();
  } catch {
    footballCatalogue = null;
    renderCatalogueTeams();
  }
}

async function loadLiveCatalogue() {
  const [competitionId, season] = byId("competition").value.split(":");
  const catalogueUrls = Object.freeze({
    "135:2024": "./catalogues/serie-a-2024.json",
    "136:2024": "./catalogues/serie-b-2024.json",
    "39:2024": "./catalogues/premier-league-2024.json",
    "40:2024": "./catalogues/championship-2024.json",
  });
  const catalogueUrl = catalogueUrls[`${competitionId}:${season}`];

  if (!catalogueUrl) {
    throw new Error(
      `unsupported football catalogue ${competitionId}:${season}`
    );
  }

  const response = await fetch(catalogueUrl);

  if (!response.ok) {
    throw new Error(`catalogue HTTP ${response.status}`);
  }

  footballCatalogue = importCatalogueJson(await response.text());
  renderCatalogueTeams();
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
  void reloadLiveCatalogue();
});

for (const seat of [0, 1]) {
  byId(`team-${seat}`).addEventListener("change", () => {
    applySelectedTeamColor(seat);
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

  selectedMatchTeams = selectedTeams;
  matchRecorded = false;

  for (const seat of [0, 1]) {
    teamPalettes[seat] = candidatePalettes[seat];
  }

  state = newMatch(n);
  matchStarted = activate;
  autoEnabled = false;
  autoRunning = false;
  isRolling = false;
  rollEvents = [];
  matchEvents = [];
  rosters = selectedTeams.map((team) =>
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
  if (
    !snapshot ||
    snapshot.status !== "active" ||
    state.phase === "finished" ||
    snapshot.seat !== state.player ||
    isRolling ||
    multiplayerBusy
  ) {
    return false;
  }

  const player = state.player;

  isRolling = true;
  multiplayerBusy = true;
  render();

  try {
    const result = await multiplayerClient.roll(
      snapshot.revision,
      crypto.randomUUID()
    );

    await animateRoll(result.die, player, result.face);

    snapshot = {
      ...snapshot,
      revision: result.revision,
      status: result.status,
      state: result.state,
      lastRoll: {
        revision: result.revision,
        seat: player,
        die: result.die,
        face: result.face,
      },
    };

    state = result.state;

    const refreshedSnapshot = await multiplayerClient.snapshot();
    adoptAuthoritativeSnapshot(refreshedSnapshot);

    return state.phase === "finished" || state.player !== player;
  } finally {
    isRolling = false;
    multiplayerBusy = false;
    render();
  }
}

async function runAutoForLocalTurn() {
  if (
    autoRunning ||
    !autoEnabled ||
    !snapshot ||
    snapshot.status !== "active" ||
    state.phase === "finished" ||
    snapshot.seat !== state.player
  ) {
    return;
  }

  const localSeat = snapshot.seat;
  autoRunning = true;
  render();

  try {
    while (
      autoEnabled &&
      snapshot &&
      snapshot.status === "active" &&
      state.phase !== "finished" &&
      snapshot.seat === localSeat &&
      state.player === localSeat
    ) {
      const turnEnded = await roll();
      if (turnEnded) break;
    }
  } finally {
    autoRunning = false;
    if (!snapshot || snapshot.status !== "active" ||
        state.phase === "finished") {
      autoEnabled = false;
    }
    render();
  }
}

async function toggleAutoRoll() {
  if (autoEnabled) {
    autoEnabled = false;
    render();
    return;
  }

  if (
    !snapshot ||
    snapshot.status !== "active" ||
    state.phase === "finished" ||
    snapshot.seat !== state.player
  ) {
    return;
  }

  autoEnabled = true;
  render();
  await runAutoForLocalTurn();
}

function selectedMultiplayerTeam(seat) {
  const team = selectedCatalogueTeam(seat);
  if (!team || !footballCatalogue) {
    throw new Error("Seleziona entrambe le squadre");
  }

  return {
    provider: String(footballCatalogue.provider),
    competitionId: String(footballCatalogue.competitionId),
    season: String(footballCatalogue.season),
    providerTeamId: String(team.providerTeamId),
  };
}

function selectedMultiplayerTeams() {
  const teams = [
    selectedMultiplayerTeam(0),
    selectedMultiplayerTeam(1),
  ];

  if (
    teams[0].provider === teams[1].provider &&
    teams[0].providerTeamId === teams[1].providerTeamId
  ) {
    throw new Error("Le due squadre devono essere diverse");
  }

  return teams;
}

function adoptAuthoritativeSnapshot(nextSnapshot) {
  snapshot = nextSnapshot;
  if (
    Array.isArray(snapshot.teamNames) &&
    snapshot.teamNames.length === 2 &&
    snapshot.teamNames.every(name =>
      typeof name === "string" && name.trim().length > 0)
  ) {
    names = snapshot.teamNames.map(name => name.trim());
  } else {
    names = names.map((name, index) =>
      typeof name === "string" && name.trim()
        ? name
        : `Squadra ${index + 1}`
    );
  }
  if (
    Array.isArray(snapshot.teams) &&
    snapshot.teams.length === 2
  ) {
    snapshot.teams.forEach((team, seat) => {
      if (!team || typeof team !== "object") return;

      if (Array.isArray(team.colors) && team.colors.length > 0) {
        teamPalettes[seat] = [...team.colors];
        colors[seat] = team.colors[0];
      }

      const logo = byId(`pitch-logo-${seat}`);
      if (logo && typeof team.logo === "string" && team.logo) {
        logo.src = team.logo;
        logo.alt = team.name ?? names[seat] ?? "";
        logo.hidden = false;
      }
    });
  }

  state = snapshot.state;
  matchStarted = snapshot.status === "active" ||
    snapshot.status === "finished";

  if (snapshot.status === "finished" || state.phase === "finished") {
    autoEnabled = false;
  }

  rollEvents = Array.isArray(snapshot.rollEvents)
    ? snapshot.rollEvents
    : [];

  matchEvents = rollEvents.flatMap(event => [
    ...(event.penalty ? [event.penalty] : []),
    ...(event.matchEvent ? [event.matchEvent] : []),
  ]);

  lastCommentary = null;
  if (rollEvents.length > 0) {
    let replayState = newMatch(state.handsPerTeam);

    rollEvents.forEach((event, index) => {
      const before = replayState;
      replayState = applyRoll(replayState, event.face);

      if (index === rollEvents.length - 1) {
        lastCommentary = {
          event: {
            seat: event.seat,
            die: event.die,
            face: event.face,
          },
          before,
          after: replayState,
          names: [...names],
          index,
        };
      }
    });
  }

  if (snapshot.matchEventContext?.rosters) {
    rosters = snapshot.matchEventContext.rosters;
  }

  if (snapshot.matchEventContext?.refereeStates) {
    refereeStates = snapshot.matchEventContext.refereeStates.map(
      refereeState => ({
        ...refereeState,
        yellowed: new Set(refereeState.yellowed ?? []),
      })
    );
  }

  render();
}

let multiplayerSyncTimer = null;

function startMultiplayerSync() {
  if (multiplayerSyncTimer !== null) return;

  multiplayerSyncTimer = window.setInterval(async () => {
    if (!snapshot || multiplayerBusy || isRolling) return;

    try {
      const refreshed = await multiplayerClient.snapshot();
      if (refreshed.revision !== snapshot.revision ||
          refreshed.status !== snapshot.status) {
        adoptAuthoritativeSnapshot(refreshed);
      }

      if (autoEnabled &&
          snapshot?.status === "active" &&
          snapshot.seat === state.player) {
        void runAutoForLocalTurn().catch(showMultiplayerError);
      }
    } catch (error) {
      console.error(error);
    }
  }, 1000);
}

function stopMultiplayerSync() {
  if (multiplayerSyncTimer === null) return;
  window.clearInterval(multiplayerSyncTimer);
  multiplayerSyncTimer = null;
}

async function multiplayerCreate() {
  if (!authenticated || multiplayerBusy) return;

  multiplayerBusy = true;
  try {
    const hands = Number(byId("hands").value);
    if (!Number.isInteger(hands) || hands < 1) {
      throw new RangeError("Numero di mani non valido");
    }

    const teamSelections = selectedMultiplayerTeams();
    const created = await multiplayerClient.create(hands, teamSelections);

    byId("invite-code").value = encodeInvite(
      created.matchId,
      created.inviteToken
    );

    if (byId("remember").checked) {
      multiplayerClient.remember();
    }

    adoptAuthoritativeSnapshot(created.snapshot);
    startMultiplayerSync();
  } finally {
    multiplayerBusy = false;
    syncMultiplayerControls();
  }
}

async function multiplayerJoin() {
  if (!authenticated || multiplayerBusy) return;

  multiplayerBusy = true;
  try {
    const { matchId, inviteToken } = decodeInvite(
      byId("invite-code").value
    );
    const joinedSnapshot = await multiplayerClient.join(
      matchId,
      inviteToken
    );

    if (byId("remember").checked) {
      multiplayerClient.remember();
    }

    adoptAuthoritativeSnapshot(joinedSnapshot);
    startMultiplayerSync();
  } finally {
    multiplayerBusy = false;
    syncMultiplayerControls();
  }
}

async function multiplayerLogin() {
  if (multiplayerBusy) return;
  multiplayerBusy = true;
  try {
    const response = await fetch("/auth/guest", {
      method: "POST",
      credentials: "same-origin",
    });
    if (!response.ok) {
      throw new Error(`Authentication failed (${response.status})`);
    }
    const result = await response.json();
    csrfToken = result.csrfToken;
    authenticated = true;
    byId("auth-status").textContent = translate("mpAuthSuccess");
    render();
  } finally {
    multiplayerBusy = false;
  }
}

async function multiplayerLogout() {
  if (multiplayerBusy) return;
  multiplayerBusy = true;
  try {
    await fetch("/auth/logout", {
      method: "POST",
      headers: csrfToken ? { "X-Dado-CSRF": csrfToken } : {},
      credentials: "same-origin",
    });
    csrfToken = null;
    authenticated = false;
    snapshot = null;
    stopMultiplayerSync();
    multiplayerClient.leave();
    byId("auth-status").textContent = translate("mpAuthEnded");
    render();
  } finally {
    multiplayerBusy = false;
  }
}

function showMultiplayerError(error) {
  const message = error instanceof MultiplayerAPIError
    ? `${error.code}: ${error.message}`
    : error?.message || String(error);

  byId("auth-status").textContent = message;
  console.error(error);
}

byId("create").addEventListener("click", () => {
  void multiplayerCreate().catch(showMultiplayerError);
});

byId("invite-code").addEventListener(
  "input", syncMultiplayerControls
);

byId("join").addEventListener("click", () => {
  void multiplayerJoin().catch(showMultiplayerError);
});

async function copyTextToClipboard(text) {
  let clipboardError = null;

  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
      return;
    }
  } catch (error) {
    clipboardError = error;
  }

  const field = byId("invite-code");
  field.focus();
  field.select();
  field.setSelectionRange(0, field.value.length);

  let copied = false;
  try {
    copied = document.execCommand("copy");
  } catch {}

  if (!copied) {
    throw clipboardError ?? new Error("Clipboard unavailable");
  }
}

let copyFeedbackTimer = null;

byId("copy-invite").addEventListener("click", async () => {
  const invite = byId("invite-code").value.trim();
  if (!invite) return;

  const button = byId("copy-invite");

  try {
    await copyTextToClipboard(invite);

    button.textContent = translate("mpCopied");
    button.classList.add("copy-success");

    if (copyFeedbackTimer !== null) {
      window.clearTimeout(copyFeedbackTimer);
    }

    copyFeedbackTimer = window.setTimeout(() => {
      button.textContent = translate("mpCopyInvite");
      button.classList.remove("copy-success");
      copyFeedbackTimer = null;
    }, 1800);
  } catch {
    showMultiplayerError(new Error(translate("mpCopyFailed")));
  }
});

byId("login").addEventListener("click", () => {
  void multiplayerLogin().catch(showMultiplayerError);
});

byId("logout").addEventListener("click", () => {
  void multiplayerLogout().catch(showMultiplayerError);
});

byId("roll").addEventListener("click", () => {
  void roll().catch(showMultiplayerError);
});

byId("auto").addEventListener("click", () => {
  void toggleAutoRoll().catch(showMultiplayerError);
});

byId("commentary-toggle").addEventListener("click", () => {
  commentaryEnabled = !commentaryEnabled;
  render();
});

initializeLocale(() => {
  syncCompetitionSelector();
  render();
  void reloadLiveCatalogue();
});

syncCompetitionSelector();
void reloadLiveCatalogue();
