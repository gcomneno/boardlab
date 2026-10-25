import {
  initial, player, legalMoves, applyMove, territoryScore, outcome
} from "./engine.mjs";
import { validateStrategy } from "./strategies.mjs";

const $ = id => document.getElementById(id);
const board = $("board");
let state = initial();
let selected = null;
let mode = "ai";
let aiPending = false;
let aiTimer = null;
const AI_DELAY_MS = 700;
let activeConfig = null;
let aiWorker = null;
let workerReady = false;
let workerRequestId = 0;
let workerGeneration = 0;
let pendingAiRequest = false;

let guidedEnabled = false;
let suggestion = null;
let suggestionRequestId = null;
let suggestionPosition = null;
let explanationVisible = false;

function clearSuggestion() {
  suggestion = null;
  suggestionRequestId = null;
  suggestionPosition = null;
  explanationVisible = false;
  $("guided-explanation").textContent = "";
}

function requestSuggestion() {
  if (!guidedEnabled || mode !== "ai" || aiPending ||
      !workerReady || aiWorker === null ||
      legalMoves(state).length === 0 ||
      player(state) !== "north" ||
      suggestionRequestId !== null) return;

  clearSuggestion();

  suggestionPosition = JSON.stringify(state);
  suggestionRequestId = ++workerRequestId;

  aiWorker.postMessage({
    id: suggestionRequestId,
    type: "suggest",
    state: structuredClone(state)
  });

  $("guided-explanation").textContent = "Analizzo la posizione…";
  render();
}

function acceptSuggestion() {
  if (!guidedEnabled || !suggestion || aiPending ||
      mode !== "ai" || legalMoves(state).length === 0 ||
      player(state) !== "north") return;

  const move = suggestion.move;

  if (!legalMoves(state).some(candidate =>
    JSON.stringify(candidate) === JSON.stringify(move))) {
    clearSuggestion();
    render();
    return;
  }

  selected = null;

  if (move.type === "place") {
    tap(move.cell);
  } else {
    tap(move.origin);
    tap(move.destination);
  }
}

function updateGuide() {
  const active = guidedEnabled && mode === "ai";

  $("guided-controls").hidden = !active;
  $("guided-toggle").textContent = active
    ? "Esci dalla prova guidata"
    : "Prova il gioco";
  $("guided-toggle").setAttribute(
    "aria-expanded", String(active)
  );

  if (!active) return;

  const finished = legalMoves(state).length === 0;
  const humanTurn = !finished && player(state) === "north";
  const ready = humanTurn && !aiPending && workerReady;
  const waiting = suggestionRequestId !== null;

  $("guided-suggest").disabled = !ready || waiting;
  $("guided-explain").disabled = !ready || !suggestion;
  $("guided-accept").disabled = !ready || !suggestion;

  $("guided-instruction").textContent = finished
    ? "Partita conclusa. Confronta i territori dei giocatori."
    : !humanTurn || aiPending
      ? "Osserva la risposta del computer."
      : state.turn < 6
        ? "Colloca un sigillo in una casella vuota. " +
          "Puoi chiedere un suggerimento oppure giocare da solo."
        : "Sposta un tuo sigillo in una casella " +
          "ortogonalmente adiacente e libera.";

  if (suggestion && explanationVisible) {
    const e = suggestion.explanation;
    $("guided-explanation").textContent =
      [e.description, e.summary, e.detail, e.disclaimer].join(" ");
  }
}


function parseNumber(id) {
  const input = $(id);
  if (!input.value.trim()) throw new Error(`Parametro obbligatorio: ${id}`);
  return Number(input.value);
}

function selectedConfiguration() {
  const algorithm = $("algorithm").value;
  const parameters = {};

  if (algorithm === "alpha_beta" || algorithm === "minimax") {
    parameters.depth = parseNumber("depth");
  } else if (algorithm === "mcts") {
    parameters.simulations = parseNumber("simulations");
    parameters.exploration = parseNumber("exploration");
    parameters.seed = parseNumber("seed");
  } else if (algorithm === "random") {
    parameters.seed = parseNumber("seed");
  }

  return validateStrategy({ algorithm, parameters });
}

function showStrategyFields() {
  const algorithm = $("algorithm").value;
  for (const field of document.querySelectorAll(".strategy-parameter")) {
    field.hidden = !field.dataset.for.split(" ").includes(algorithm);
  }
}

function describeStrategy(config) {
  const { algorithm, parameters } = config;
  if (algorithm === "random") {
    return `Casuale · seed ${parameters.seed}`;
  }
  if (algorithm === "mcts") {
    return `MCTS · ${parameters.simulations} simulazioni · ` +
      `UCT ${parameters.exploration} · seed ${parameters.seed}`;
  }
  return `${algorithm === "minimax" ? "Minimax" : "Alpha-Beta"} · ` +
    `profondità ${parameters.depth}`;
}

function showAiError(message) {
  $("strategy-error").textContent = `Errore IA: ${message}`;
  aiPending = false;
  pendingAiRequest = false;

  if (aiTimer !== null) {
    window.clearTimeout(aiTimer);
    aiTimer = null;
  }

  if (aiWorker !== null) {
    aiWorker.terminate();
    aiWorker = null;
  }

  workerReady = false;
  render();
}

function requestAiMove() {
  if (!aiPending || !pendingAiRequest || !workerReady ||
      aiWorker === null) return;

  if (mode !== "ai" || legalMoves(state).length === 0 ||
      player(state) !== "south") {
    throw new Error("Invalid AI request state");
  }

  pendingAiRequest = false;
  aiWorker.postMessage({
    id: ++workerRequestId,
    type: "choose",
    state: structuredClone(state)
  });
}

function createGameWorker() {
  return new Worker(
    new URL("./strategy-worker.mjs", import.meta.url),
    { type: "module" }
  );
}

function bindWorker(worker) {
  const generation = workerGeneration;
  workerReady = false;
  workerRequestId = 0;

  worker.onmessage = ({ data }) => {
    if (generation !== workerGeneration || worker !== aiWorker) return;

    if (!data || !Number.isSafeInteger(data.id)) {
      showAiError("Risposta non valida dal motore");
      return;
    }

    if (data.type === "configured") {
      if (data.id !== 0) {
        showAiError("Configurazione inattesa");
        return;
      }
      workerReady = true;
      try {
        requestAiMove();
      } catch (error) {
        showAiError(error.message);
      }
      return;
    }

    if (data.type === "move") {
      if (!aiPending || data.id !== workerRequestId) return;

      try {
        if (mode !== "ai" || player(state) !== "south") {
          throw new Error("Risposta IA fuori turno");
        }

        state = applyMove(state, data.move);
        aiPending = false;
        render();
      } catch (error) {
        showAiError(error.message);
      }
      return;
    }

    if (data.type === "suggestion") {
      if (data.id !== suggestionRequestId ||
          suggestionPosition !== JSON.stringify(state) ||
          !guidedEnabled || mode !== "ai" || aiPending ||
          legalMoves(state).length === 0 ||
          player(state) !== "north") return;

      suggestionRequestId = null;
      suggestionPosition = null;

      const valid = legalMoves(state).some(move =>
        JSON.stringify(move) === JSON.stringify(data.move)
      );

      if (!valid || !data.explanation || !data.analysis) {
        $("guided-explanation").textContent =
          "Suggerimento non valido.";
        render();
        return;
      }

      suggestion = data;
      $("guided-explanation").textContent =
        "Mossa evidenziata. Puoi seguirla o giocare liberamente.";
      render();
      return;
    }

    if (data.type === "error") {
      if (data.id === suggestionRequestId) {
        clearSuggestion();
        $("guided-explanation").textContent =
          data.message ?? "Suggerimento non disponibile";
        render();
        return;
      }

      // Ignore errors from requests invalidated by another move.
      if (data.id !== 0 &&
          (!aiPending || data.id !== workerRequestId)) return;

      showAiError(data.message ?? "Errore del motore");
      return;
    }

    showAiError("Comando sconosciuto dal motore");
  };

  worker.onerror = event => {
    if (generation !== workerGeneration || worker !== aiWorker) return;
    event.preventDefault();
    showAiError("Impossibile eseguire il motore");
  };
}

function render() {
  const moves = legalMoves(state);
  const result = outcome(state);
  const score = territoryScore(state);
  $("north-score").textContent = score.north;
  $("south-score").textContent = score.south;

  if (result !== null) {
    $("status").textContent = result === "draw"
      ? "Partita terminata · Pareggio"
      : `Partita terminata · Vince ${result === "north" ? "Avorio" : "Terracotta"}!`;
    $("hint").textContent = "Premi Nuova partita per giocare ancora.";
  } else if (aiPending) {
    $("status").textContent = "Il computer sta pensando…";
    $("hint").textContent = "La tua mossa è stata eseguita. Attendi la risposta.";
  } else {
    $("status").textContent =
      `Turno ${state.turn + 1} di 18 · ${player(state) === "north" ? "Avorio" : "Terracotta"}`;
    $("hint").textContent = state.turn < 6
      ? "Scegli una casella vuota per collocare un sigillo."
      : "Seleziona un tuo sigillo, poi una casella adiacente libera.";
  }

  const targets = new Set(
    state.turn < 6
      ? moves.map(m => m.cell)
      : selected === null
        ? moves.map(m => m.origin)
        : moves.filter(m => m.origin === selected).map(m => m.destination)
  );

  updateGuide();
  board.replaceChildren();
  for (let i = 0; i < 16; i++) {
    const cell = document.createElement("button");
    cell.type = "button";
    cell.className = "cell";
    if (i === selected) cell.classList.add("selected");

    if (guidedEnabled && suggestion) {
      const hint = suggestion.move;

      if (hint.type === "place" && hint.cell === i) {
        cell.classList.add("guided-target");
      } else if (hint.type === "slide") {
        if (hint.origin === i) cell.classList.add("guided-origin");
        if (hint.destination === i) cell.classList.add("guided-target");
      }
    }
    if (targets.has(i) && result === null && !aiPending) cell.classList.add("legal");
    cell.disabled = aiPending;

    const occupant = state.cells[i];
    cell.setAttribute("aria-label",
      `Riga ${Math.floor(i / 4) + 1}, colonna ${i % 4 + 1}, ` +
      (occupant === null ? "libera" : occupant === "north" ? "Avorio" : "Terracotta")
    );
    cell.setAttribute("aria-pressed", String(i === selected));

    if (occupant !== null) {
      const seal = document.createElement("span");
      seal.className = `seal ${occupant}`;
      seal.setAttribute("aria-hidden", "true");
      cell.append(seal);
    }

    cell.addEventListener("click", () => tap(i));
    board.append(cell);
  }
}

function tap(cell) {
  const moves = legalMoves(state);
  if (aiPending || !moves.length ||
      (mode === "ai" && player(state) !== "north")) return;

  let move = null;
  if (state.turn < 6) {
    move = moves.find(m => m.type === "place" && m.cell === cell);
  } else {
    if (selected !== null) {
      move = moves.find(m => m.type === "slide" &&
        m.origin === selected && m.destination === cell);
    }
    if (!move) {
      selected = moves.some(m => m.origin === cell) ? cell : null;
      render();
      return;
    }
  }

  if (!move) return;
  clearSuggestion();
  state = applyMove(state, move);
  selected = null;

  if (mode === "ai" && legalMoves(state).length > 0) {
    if (player(state) !== "south") throw new Error("Invalid AI turn");

    aiPending = true;
    render();

    aiTimer = window.setTimeout(() => {
      aiTimer = null;
      pendingAiRequest = true;

      try {
        requestAiMove();
      } catch (error) {
        showAiError(error.message);
      }
    }, AI_DELAY_MS);
    return;
  }

  render();
}

function reset() {
  const nextMode = $("mode").value;
  let nextConfig = null;
  let nextWorker = null;

  try {
    if (nextMode === "ai") {
      nextConfig = selectedConfiguration();
      nextWorker = createGameWorker();
    }
  } catch (error) {
    $("strategy-error").textContent = error.message;
    return;
  }

  if (aiTimer !== null) {
    window.clearTimeout(aiTimer);
    aiTimer = null;
  }

  // Invalidate all callbacks from the previous match.
  workerGeneration++;

  if (aiWorker !== null) {
    aiWorker.terminate();
  }

  aiWorker = nextWorker;
  aiPending = false;
  pendingAiRequest = false;
  clearSuggestion();
  mode = nextMode;
  activeConfig = nextConfig;
  state = initial();
  selected = null;

  $("strategy-error").textContent = "";
  $("strategy-active").textContent = activeConfig
    ? `Avversario attivo: ${describeStrategy(activeConfig)}`
    : "Modalità: due giocatori locali";

  if (aiWorker !== null) {
    // Capture the generation assigned to this active worker.
    // The worker's listener is installed below by bindWorker().
    bindWorker(aiWorker);
    aiWorker.postMessage({
      id: 0,
      type: "configure",
      config: activeConfig
    });
  }

  render();
}

$("guided-toggle").addEventListener("click", () => {
  guidedEnabled = !guidedEnabled;
  if (guidedEnabled) $("mode").value = "ai";
  reset();
});

$("guided-suggest").addEventListener("click", requestSuggestion);

$("guided-explain").addEventListener("click", () => {
  if (!suggestion) return;
  explanationVisible = true;
  updateGuide();
});

$("guided-accept").addEventListener("click", acceptSuggestion);

$("new").addEventListener("click", reset);
$("mode").addEventListener("change", () => {
  if ($("mode").value !== "ai") guidedEnabled = false;
  reset();
});
$("algorithm").addEventListener("change", showStrategyFields);
showStrategyFields();
reset();
