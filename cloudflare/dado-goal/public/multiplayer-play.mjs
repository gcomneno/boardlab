import {
  MultiplayerClient,
  MultiplayerAPIError,
} from "./multiplayer-client.mjs";
import { matchClock } from "./match-clock.mjs";
import { DICE_ART } from "./dice-art.mjs";

const $ = id => document.getElementById(id);

// One user-facing invitation; two independent server credentials.
function encodeInvite(matchId, inviteToken) {
  return `DG1.${matchId}.${inviteToken}`;
}

function decodeInvite(code) {
  const match = /^DG1\.([A-Za-z0-9_-]{20,128})\.([A-Za-z0-9_-]{20,256})$/
    .exec(code.trim());

  if (!match) {
    throw new Error("Codice invito non valido.");
  }

  return { matchId: match[1], inviteToken: match[2] };
}

let browserStorage = null;
try {
  browserStorage = window.localStorage;
} catch {
  // Private browsing or browser policy may disable storage.
}

let csrfToken = null;
let authenticated = false;

const client = new MultiplayerClient({
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

let snapshot = null;
let lastFace = null;
let busy = false;
let polling = false;

const dice = {
  team: "Dado squadra",
  pink: "Dado rosa",
  result: "Dado finale",
};

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

function renderDie(lastRoll, phase) {
  const element = $("die");

  if (!lastRoll) {
    element.textContent = "🎲";
    element.setAttribute("aria-label", dice[phase] ?? "Dado corrente");
    return;
  }

  const { seat, die, face } = lastRoll;
  const kind = die === "team"
    ? (seat === 0 ? "yellow" : "blue")
    : die === "pink" ? "pink" : "final";
  const index = diceFaces[die]?.indexOf(face) ?? -1;
  const svg = index >= 0 ? DICE_ART[kind]?.[index] : null;

  if (!svg) {
    element.textContent = face ?? "🎲";
    element.setAttribute("aria-label", "Ultimo lancio");
    return;
  }

  element.innerHTML = svg;
  element.setAttribute("aria-label", `Dado ${kind}: ${face}`);
}

function message(value) {
  $("status").textContent = value;
}

function errorMessage(error) {
  if (error instanceof MultiplayerAPIError) {
    return `Errore server: ${error.code} (${error.status})`;
  }
  return error instanceof Error
    ? error.message
    : "Errore sconosciuto";
}

function render() {
  const active = Boolean(client.seatToken && snapshot);
  const state = snapshot?.state;
  const status = snapshot?.status;
  const myTurn =
    status === "active" && state?.player === client.seat;

  $("roll").disabled = !myTurn || busy;
  $("refresh").disabled = !active || busy;
  $("leave").disabled = !client.seatToken || busy;
  $("create").disabled = !authenticated || Boolean(client.seatToken) || busy;
  $("join").disabled = !authenticated || Boolean(client.seatToken) || busy;
  $("copy-invite").disabled =
    !$("invite-code").value.trim() || busy;

  if (!state) {
    $("match-clock").textContent = "CRONOMETRO · —";
    $("match-clock").classList.remove("halftime");
    $("match-events").replaceChildren();
    $("match-stats").replaceChildren();
    return;
  }

  renderMatchEvents(snapshot.rollEvents);
  renderStatistics(snapshot.statistics);

  const clock = matchClock(state);
  $("match-clock").textContent = clock.finished
    ? `FISCHIO FINALE · ${clock.label}`
    : `${clock.half}° TEMPO · ${clock.label}`;
  $("match-clock").classList.toggle("halftime", clock.halftime);

  $("score").textContent =
    `Risultato: ${state.goals[0]} – ${state.goals[1]}`;
  $("hands-done").textContent =
    `Mani: ${state.hands[0]} – ${state.hands[1]} / ${state.handsPerTeam}`;
  $("turn").textContent = status === "completed"
    ? "Fischio finale!"
    : `Giocatore ${state.player + 1} — ` +
      (myTurn ? "tocca a te" : "attendi l'avversario");
  renderDie(snapshot.lastRoll, state.phase);
  $("last-roll").textContent =
    `Ultimo lancio: ${lastFace ?? "—"}`;

  if (status === "completed") {
    message("Partita conclusa.");
  } else if (status === "waiting") {
    message("Partita creata: in attesa dell'amico.");
  } else {
    message(
      `Partita attiva — lancio ${Math.max(0, snapshot.revision - 1)}.`
    );
  }
}

function renderMatchEvents(rollEvents) {
  const container = $("match-events");
  container.replaceChildren();

  if (!Array.isArray(rollEvents)) return;

  for (const event of rollEvents) {
    if (!event?.goal) continue;

    const item = document.createElement("li");
    item.textContent =
      `${event.goal.minute}' — GOAL! Giocatore ${event.goal.team + 1}`;
    container.append(item);
  }
}

const statisticsMetrics = [
  ["Gol", "goals"],
  ["Tiri in porta", "shots"],
  ["Cross", "crosses"],
  ["Passaggi", "passes"],
  ["Parate", "saves"],
  ["Pali/traverse", "woodwork"],
  ["Rigori", "penalties"],
];

function renderStatistics(statistics) {
  const container = $("match-stats");
  container.replaceChildren();

  if (!Array.isArray(statistics) || statistics.length !== 2) return;

  for (const [label, key] of statisticsMetrics) {
    const home = statistics[0][key];
    const away = statistics[1][key];
    const total = home + away;

    const row = document.createElement("div");
    row.className = "mp-stat-row";

    const values = document.createElement("div");
    values.className = "mp-stat-values";

    for (const value of [home, label, away]) {
      const cell = document.createElement("span");
      cell.textContent = String(value);
      values.append(cell);
    }

    const bars = document.createElement("div");
    bars.className = "mp-stat-bars";
    bars.setAttribute("aria-hidden", "true");

    for (const [value, side] of [[home, "home"], [away, "away"]]) {
      const track = document.createElement("div");
      track.className = `mp-stat-track ${side}`;

      const fill = document.createElement("div");
      fill.className = "mp-stat-fill";
      fill.style.width = total === 0 ? "0%" : `${value / total * 100}%`;

      track.append(fill);
      bars.append(track);
    }

    row.append(values, bars);
    container.append(row);
  }
}

async function work(operation) {
  if (busy) return;
  busy = true;
  render();

  try {
    await operation();
  } catch (error) {
    message(errorMessage(error));
  } finally {
    busy = false;
    render();
  }
}

async function synchronize() {
  if (!client.seatToken || polling || busy) return;
  polling = true;

  try {
    const next = await client.snapshot();
    if (!snapshot || next.revision >= snapshot.revision) {
      snapshot = next;
      if (next.lastRoll) {
        lastFace =
          `Giocatore ${next.lastRoll.seat + 1}: ${next.lastRoll.face}`;
      }
      render();
    }
  } catch (error) {
    message(errorMessage(error));
  } finally {
    polling = false;
  }
}

$("login").addEventListener("click", () => work(async () => {
  const response = await globalThis.fetch("/auth/guest", {
    method: "POST",
    headers: {
      Accept: "application/json",
    },
    credentials: "same-origin",
    cache: "no-store",
  });
  const result = await response.json();
  if (!response.ok || typeof result.csrfToken !== "string") {
    throw new Error("Accesso non disponibile. Riprova più tardi.");
  }

  csrfToken = result.csrfToken;
  authenticated = true;
  $("auth-status").textContent = "Accesso effettuato.";
  $("login").disabled = true;
  $("logout").disabled = false;
  render();
}));

$("logout").addEventListener("click", () => work(async () => {
  const response = await globalThis.fetch("/auth/logout", {
    method: "POST",
    headers: {
      Accept: "application/json",
      "X-Dado-CSRF": csrfToken,
    },
    credentials: "same-origin",
    cache: "no-store",
  });
  if (!response.ok) throw new Error("Impossibile terminare la sessione.");

  csrfToken = null;
  authenticated = false;
  client.leave();
  snapshot = null;
  lastFace = null;
  $("invite-code").value = "";
  $("auth-status").textContent = "Sessione terminata.";
  $("login").disabled = false;
  $("logout").disabled = true;
  render();
}));

$("create").addEventListener("click", () => work(async () => {
  const raw = $("hands").value.trim();
  if (!/^\d+$/.test(raw)) {
    throw new RangeError("Numero di mani non valido");
  }

  const created = await client.create(Number(raw));
  $("invite-code").value = encodeInvite(
    created.matchId, created.inviteToken
  );
  snapshot = created.snapshot;
  lastFace = null;
  if ($("remember").checked) client.remember();
}));


$("join").addEventListener("click", () => work(async () => {
  const { matchId, inviteToken } = decodeInvite(
    $("invite-code").value
  );
  snapshot = await client.join(matchId, inviteToken);
  lastFace = null;
  if ($("remember").checked) client.remember();
}));

$("roll").addEventListener("click", () => work(async () => {
  if (!snapshot || !Number.isSafeInteger(snapshot.revision)) {
    throw new Error("Stato della partita non disponibile.");
  }

  const requestId = crypto.randomUUID();
  const result = await client.roll(snapshot.revision, requestId);

  lastFace = `Giocatore ${client.seat + 1}: ${result.face}`;

  const next = await client.snapshot();
  if (next.revision < result.revision) {
    throw new Error("Snapshot autorevole non aggiornato.");
  }
  snapshot = next;
}));

$("invite-code").addEventListener("input", render);

let copyResetTimer = null;

$("copy-invite").addEventListener("click", async () => {
  const button = $("copy-invite");
  const code = $("invite-code").value.trim();

  if (!code || busy) return;

  if (copyResetTimer !== null) {
    clearTimeout(copyResetTimer);
    copyResetTimer = null;
  }

  button.classList.remove("copy-success");
  button.textContent = "Copia codice invito";

  try {
    await navigator.clipboard.writeText(code);
    button.textContent = "✓ Copiato!";
    button.classList.add("copy-success");

    copyResetTimer = setTimeout(() => {
      button.textContent = "Copia codice invito";
      button.classList.remove("copy-success");
      copyResetTimer = null;
      render();
    }, 1300);
  } catch {
    message("Impossibile copiare: seleziona il codice e copialo manualmente.");
  }
});

$("leave").addEventListener("click", () => {
  if (busy) return;
  client.leave();
  snapshot = null;
  lastFace = null;
  $("invite-code").value = "";
  $("remember").checked = false;
  message("Postazione dimenticata e sessione locale abbandonata.");
  render();
});

$("remember").addEventListener("change", () => {
  if (!$("remember").checked) {
    try {
      client.forget();
    } catch (error) {
      message(errorMessage(error));
    }
    return;
  }

  if (client.seatToken) {
    try {
      client.remember();
    } catch (error) {
      $("remember").checked = false;
      message(errorMessage(error));
    }
  }
});

async function recoverOnLoad() {
  try {
    const restored = await client.restore();
    if (!restored) return;
    snapshot = restored;
    lastFace = restored.lastRoll
      ? `Giocatore ${restored.lastRoll.seat + 1}: ${restored.lastRoll.face}`
      : null;
    $("remember").checked = true;
    $("invite-code").value = "";
    render();
  } catch (error) {
    message(errorMessage(error));
  }
}

void recoverOnLoad();

window.setInterval(() => {
  if (client.seatToken &&
      snapshot?.status !== "completed") {
    void synchronize();
  }
}, 1500);

render();
