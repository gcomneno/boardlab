import {
  initial, player, legalMoves, applyMove, territoryScore, outcome
} from "./engine.mjs";
import { validateStrategy } from "./strategies.mjs";
import { analyzeMove } from "./guided-analysis.mjs";

import { initializeLocale, translate, formatMessage, explainAnalysis } from "./i18n.mjs";

const $ = id => document.getElementById(id);
const board = $("board");
let state = initial();
let selected = null;
let mode = "ai";
let aiPending = false;
let aiTimer = null;
const AI_DELAY_MS = 700;
let activeConfig = null;
let activeNorthConfig = null;
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
let lastAiAnalysis = null;

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

  $("guided-explanation").textContent = translate("analyzing");
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
  $("guided-toggle").textContent = translate(
    active ? "guidedOn" : "guided"
  );
  $("guided-toggle").setAttribute(
    "aria-expanded", String(active)
  );

  if (!active) return;

  $("guided-ai-explanation").textContent = lastAiAnalysis
    ? `${translate("aiMoveLabel")} ${explainAnalysis(lastAiAnalysis)}`
    : "";

  const finished = legalMoves(state).length === 0;
  const humanTurn = !finished && player(state) === "north";
  const ready = humanTurn && !aiPending && workerReady;
  const waiting = suggestionRequestId !== null;

  $("guided-suggest").disabled = !ready || waiting;
  $("guided-explain").disabled = !ready || !suggestion;
  $("guided-accept").disabled = !ready || !suggestion;

  $("guided-instruction").textContent = translate(
    finished ? "guidedFinished"
      : !humanTurn || aiPending ? "guidedWait"
        : state.turn < 6 ? "guidedPlace" : "guidedSlide"
  );

  if (suggestion && explanationVisible) {
    $("guided-explanation").textContent =
      explainAnalysis(suggestion.analysis);
  }
}


function parseNumber(id) {
  const input = $(id);
  if (!input.value.trim()) throw new Error(formatMessage("requiredParameter", { name: id }));
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
    return formatMessage("strategyRandom", { seed: parameters.seed });
  }
  if (algorithm === "mcts") {
    return formatMessage("strategyMcts", parameters);
  }
  return formatMessage("strategyDepth", {
    algorithm: algorithm === "minimax" ? "Minimax" : "Alpha-Beta",
    depth: parameters.depth
  });
}

function scheduleAiTurn() {
  if (legalMoves(state).length === 0) {
    aiPending = false;
    render();
    return;
  }

  aiPending = true;
  render();

  aiTimer = window.setTimeout(() => {
    aiTimer = null;
    if (!aiPending) return;

    pendingAiRequest = true;
    try {
      if (mode === "ai_vs_ai") {
        workerReady = false;
        aiWorker.postMessage({
          id: 0,
          type: "configure",
          config: player(state) === "north"
            ? activeNorthConfig : activeConfig
        });
      } else {
        requestAiMove();
      }
    } catch (error) {
      showAiError(error.message);
    }
  }, AI_DELAY_MS);
}

function showAiError(message) {
  $("strategy-error").textContent = formatMessage("aiError", { message });
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

  if ((mode !== "ai" && mode !== "ai_vs_ai") ||
      legalMoves(state).length === 0 ||
      (mode === "ai" && player(state) !== "south")) {
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
        if ((mode !== "ai" && mode !== "ai_vs_ai") ||
            (mode === "ai" && player(state) !== "south")) {
          throw new Error("Risposta IA fuori turno");
        }

        const analysis = guidedEnabled
          ? analyzeMove(state, data.move)
          : null;
        state = applyMove(state, data.move);
        lastAiAnalysis = analysis;
        aiPending = false;

        if (mode === "ai_vs_ai" && legalMoves(state).length > 0) {
          scheduleAiTurn();
        } else {
          render();
        }
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
          translate("invalidSuggestion");
        render();
        return;
      }

      suggestion = data;
      $("guided-explanation").textContent =
        translate("suggestionReady");
      render();
      return;
    }

    if (data.type === "error") {
      if (data.id === suggestionRequestId) {
        clearSuggestion();
        $("guided-explanation").textContent =
          data.message ?? translate("suggestionUnavailable");
        render();
        return;
      }

      // Ignore errors from requests invalidated by another move.
      if (data.id !== 0 &&
          (!aiPending || data.id !== workerRequestId)) return;

      showAiError(data.message ?? translate("engineError"));
      return;
    }

    showAiError(translate("unknownCommand"));
  };

  worker.onerror = event => {
    if (generation !== workerGeneration || worker !== aiWorker) return;
    event.preventDefault();
    showAiError(translate("engineUnavailable"));
  };
}

function render() {
  const moves = legalMoves(state);
  const result = outcome(state);
  const score = territoryScore(state);
  $("north-score").textContent = score.north;
  $("south-score").textContent = score.south;

  const playerName = side =>
    translate(side === "north" ? "ivory" : "terracotta");

  if (result !== null) {
    $("status").textContent = result === "draw"
      ? translate("gameDraw")
      : formatMessage("gameWin", { player: playerName(result) });
    $("hint").textContent = translate("gameAgain");
  } else if (aiPending) {
    $("status").textContent = translate("gameThinking");
    $("hint").textContent = translate("gameWait");
  } else {
    $("status").textContent = formatMessage("gameTurn", {
      turn: state.turn + 1, player: playerName(player(state))
    });
    $("hint").textContent = translate(
      state.turn < 6 ? "gamePlace" : "gameSlide"
    );
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
    if (targets.has(i) && result === null && !aiPending &&
        mode !== "ai_vs_ai") cell.classList.add("legal");
    cell.disabled = aiPending || mode === "ai_vs_ai";

    const occupant = state.cells[i];
    cell.setAttribute("aria-label", formatMessage("cell", {
      row: Math.floor(i / 4) + 1,
      column: i % 4 + 1,
      occupant: occupant === null ? translate("free") : playerName(occupant)
    }));
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
  if (aiPending || !moves.length || mode === "ai_vs_ai" ||
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
  lastAiAnalysis = null;
  state = applyMove(state, move);
  selected = null;

  if (mode === "ai" && legalMoves(state).length > 0) {
    if (player(state) !== "south") throw new Error("Invalid AI turn");

    scheduleAiTurn();
    return;
  }

  render();
}

function updateStrategyDescription() {
  $("strategy-active").textContent = mode === "ai_vs_ai"
    ? formatMessage("activeAiVsAi", {
        north: describeStrategy(activeNorthConfig),
        south: describeStrategy(activeConfig)
      })
    : activeConfig
      ? formatMessage("activeAi", {
          strategy: describeStrategy(activeConfig)
        })
      : translate("activeLocal");
}

function reset() {
  const nextMode = $("mode").value;
  let nextConfig = null;
  let nextNorthConfig = null;
  let nextWorker = null;

  try {
    if (nextMode === "ai" || nextMode === "ai_vs_ai") {
      nextConfig = selectedConfiguration();
      if (nextMode === "ai_vs_ai") {
        nextNorthConfig = validateStrategy({
          algorithm: $("north-algorithm").value
        });
      }
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
  lastAiAnalysis = null;
  $("guided-ai-explanation").textContent = "";
  mode = nextMode;
  activeConfig = nextConfig;
  activeNorthConfig = nextNorthConfig;
  state = initial();
  selected = null;

  $("strategy-error").textContent = "";
  updateStrategyDescription();

  if (aiWorker !== null) {
    // Capture the generation assigned to this active worker.
    // The worker's listener is installed below by bindWorker().
    bindWorker(aiWorker);
    aiWorker.postMessage({
      id: 0,
      type: "configure",
      config: mode === "ai_vs_ai" ? activeNorthConfig : activeConfig
    });
  }

  if (mode === "ai_vs_ai") {
    scheduleAiTurn();
  } else {
    render();
  }
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
  $("ai-vs-ai-settings").hidden = $("mode").value !== "ai_vs_ai";
  reset();
});
$("algorithm").addEventListener("change", showStrategyFields);
initializeLocale(() => {
  updateStrategyDescription();
  render();

  if (suggestionRequestId !== null) {
    $("guided-explanation").textContent = translate("analyzing");
  } else if (suggestion && !explanationVisible) {
    $("guided-explanation").textContent = translate("suggestionReady");
  }
});
showStrategyFields();
$("ai-vs-ai-settings").hidden = $("mode").value !== "ai_vs_ai";
reset();
