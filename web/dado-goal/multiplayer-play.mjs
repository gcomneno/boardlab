import {
  MultiplayerClient,
  MultiplayerAPIError,
} from "./multiplayer-client.mjs";

import {
  initializeMultiplayerLocale,
  translate,
  formatMessage,
} from "./i18n.mjs";

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
let lastFace = null; // { player, face } — canonical face unchanged
let busy = false;
let polling = false;

const dice = {
  team: "mpTeamDie",
  pink: "mpPinkDie",
  result: "mpFinalDie",
};

let statusNotice = null;

function message(value) {
  $("status").textContent = value;
}

function showNotice(key, values = {}) {
  statusNotice = { key, values };
  message(formatMessage(key, values));
}

const localErrors = Object.freeze({
  "Codice invito non valido.": "mpInvalidInvite",
  "Accesso non disponibile. Riprova più tardi.": "mpLoginUnavailable",
  "Impossibile terminare la sessione.": "mpLogoutFailed",
  "Numero di mani non valido": "mpInvalidHands",
  "Stato della partita non disponibile.": "mpStateUnavailable"
});

function showError(error) {
  if (error instanceof MultiplayerAPIError) {
    showNotice("mpServerError", {
      code: error.code,
      status: error.status
    });
  } else if (error instanceof Error) {
    const key = localErrors[error.message];
    if (key) showNotice(key);
    else {
      // Unknown technical errors retain their original diagnostic text.
      statusNotice = { raw: error.message };
      message(error.message);
    }
  } else {
    showNotice("mpUnknownError");
  }
}

function restoreNotice() {
  if (!statusNotice) return;
  if ("raw" in statusNotice) message(statusNotice.raw);
  else message(formatMessage(statusNotice.key, statusNotice.values));
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
    restoreNotice();
    $("score").textContent = formatMessage("mpScore", {
      home: "—", away: "—"
    });
    $("hands-done").textContent = formatMessage("mpHands", {
      home: "—", away: "—", total: "—"
    });
    $("turn").textContent = formatMessage("mpTurn", {
      player: "—", detail: "—"
    });
    $("die").textContent = formatMessage("mpDie", { die: "—" });
    $("last-roll").textContent = formatMessage("mpLastRoll", { face: "—" });
    return;
  }

  $("score").textContent = formatMessage("mpScore", {
    home: state.goals[0], away: state.goals[1]
  });
  $("hands-done").textContent = formatMessage("mpHands", {
    home: state.hands[0], away: state.hands[1],
    total: state.handsPerTeam
  });
  $("turn").textContent = status === "completed"
    ? translate("mpFinalWhistle")
    : formatMessage("mpTurn", {
      player: state.player + 1,
      detail: translate(myTurn ? "mpYourTurn" : "mpWaitTurn")
    });
  $("die").textContent = formatMessage("mpDie", {
    die: dice[state.phase] ? translate(dice[state.phase]) : state.phase
  });
  $("last-roll").textContent = formatMessage("mpLastRoll", {
    face: lastFace
      ? formatMessage("mpPlayerFace", lastFace)
      : "—"
  });

  if (statusNotice) {
    restoreNotice();
  } else if (status === "completed") {
    message(translate("mpCompleted"));
  } else if (status === "waiting") {
    message(translate("mpWaiting"));
  } else {
    message(formatMessage("mpActive", {
      number: Math.max(0, snapshot.revision - 1)
    }));
  }
}

async function work(operation) {
  if (busy) return;
  busy = true;
  render();

  try {
    await operation();
    statusNotice = null;
  } catch (error) {
    showError(error);
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
        lastFace = {
          player: next.lastRoll.seat + 1,
          face: next.lastRoll.face
        };
      }
      render();
    }
  } catch (error) {
    showError(error);
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
  $("auth-status").dataset.i18nDynamic = "mpAuthSuccess";
  $("auth-status").textContent = translate("mpAuthSuccess");
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
  $("auth-status").dataset.i18nDynamic = "mpAuthEnded";
  $("auth-status").textContent = translate("mpAuthEnded");
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

  snapshot = {
    ...snapshot,
    revision: result.revision,
    state: result.state,
    status: result.status,
  };
  lastFace = {
    player: client.seat + 1,
    face: result.face
  };
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
  button.dataset.i18n = "mpCopyInvite";
  button.textContent = translate("mpCopyInvite");

  try {
    await navigator.clipboard.writeText(code);
    button.dataset.i18n = "mpCopied";
    button.textContent = translate("mpCopied");
    button.classList.add("copy-success");

    copyResetTimer = setTimeout(() => {
      button.dataset.i18n = "mpCopyInvite";
  button.textContent = translate("mpCopyInvite");
      button.classList.remove("copy-success");
      copyResetTimer = null;
      render();
    }, 1300);
  } catch {
    showNotice("mpCopyFailed");
  }
});

$("leave").addEventListener("click", () => {
  if (busy) return;
  client.leave();
  snapshot = null;
  lastFace = null;
  $("invite-code").value = "";
  $("remember").checked = false;
  showNotice("mpLeft");
  render();
});

$("remember").addEventListener("change", () => {
  if (!$("remember").checked) {
    try {
      client.forget();
    } catch (error) {
      showError(error);
    }
    return;
  }

  if (client.seatToken) {
    try {
      client.remember();
    } catch (error) {
      $("remember").checked = false;
      showError(error);
    }
  }
});

async function recoverOnLoad() {
  try {
    const restored = await client.restore();
    if (!restored) return;
    snapshot = restored;
    lastFace = restored.lastRoll
      ? {
        player: restored.lastRoll.seat + 1,
        face: restored.lastRoll.face
      }
      : null;
    $("remember").checked = true;
    $("invite-code").value = "";
    render();
  } catch (error) {
    showError(error);
  }
}

initializeMultiplayerLocale(() => {
  for (const element of document.querySelectorAll("[data-i18n-dynamic]")) {
    element.textContent = translate(element.dataset.i18nDynamic);
  }
  render();
  restoreNotice();
});
void recoverOnLoad();

window.setInterval(() => {
  if (client.seatToken &&
      snapshot?.status !== "completed") {
    void synchronize();
  }
}, 1500);

render();
