import {
  MultiplayerClient,
  MultiplayerAPIError,
} from "./multiplayer-client.mjs";

const $ = id => document.getElementById(id);
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

  if (!state) return;

  $("score").textContent =
    `Risultato: ${state.goals[0]} – ${state.goals[1]}`;
  $("hands-done").textContent =
    `Mani: ${state.hands[0]} – ${state.hands[1]} / ${state.handsPerTeam}`;
  $("turn").textContent = status === "completed"
    ? "Fischio finale!"
    : `Giocatore ${state.player + 1} — ` +
      (myTurn ? "tocca a te" : "attendi l'avversario");
  $("die").textContent =
    `Dado: ${dice[state.phase] ?? state.phase}`;
  $("last-roll").textContent =
    `Ultimo lancio: ${lastFace ?? "—"}`;

  if (status === "completed") {
    message("Partita conclusa.");
  } else if (status === "waiting") {
    message("Partita creata: in attesa dell'amico.");
  } else {
    message(`Partita attiva — revisione ${snapshot.revision}.`);
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
      render();
    }
  } catch (error) {
    message(errorMessage(error));
  } finally {
    polling = false;
  }
}

$("login").addEventListener("click", () => work(async () => {
  const accessKey = $("access-key").value;
  if (!accessKey.trim()) throw new Error("Inserisci la chiave di accesso.");

  const response = await globalThis.fetch("/auth/login", {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ accessKey }),
    credentials: "same-origin",
    cache: "no-store",
  });
  const result = await response.json();
  if (!response.ok || typeof result.csrfToken !== "string") {
    throw new Error("Accesso rifiutato. Verifica la chiave.");
  }

  csrfToken = result.csrfToken;
  authenticated = true;
  $("access-key").value = "";
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
  $("match-id").value = "";
  $("invite").value = "";
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
  $("match-id").value = created.matchId;
  $("invite").value = created.inviteToken;
  snapshot = created.snapshot;
  lastFace = null;
  if ($("remember").checked) client.remember();
}));

$("join").addEventListener("click", () => work(async () => {
  const id = $("match-id").value.trim();
  const invite = $("invite").value.trim();

  snapshot = await client.join(id, invite);
  $("invite").value = "";
  lastFace = null;
  if ($("remember").checked) client.remember();
}));

$("roll").addEventListener("click", () => work(async () => {
  if (!snapshot || snapshot.status !== "active" ||
      snapshot.state.player !== client.seat) return;

  const requestId = crypto.randomUUID();
  const revision = snapshot.revision;

  const result = await client.roll(revision, requestId);
  lastFace = result.face;

  snapshot = {
    revision: result.revision,
    status: result.status,
    seat: client.seat,
    state: result.state,
  };
}));

$("refresh").addEventListener("click", () => {
  void synchronize();
});

$("leave").addEventListener("click", () => {
  if (busy) return;
  client.leave();
  snapshot = null;
  lastFace = null;
  $("invite").value = "";
  $("match-id").value = "";
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
    lastFace = null;
    $("remember").checked = true;
    $("match-id").value = client.matchId;
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
