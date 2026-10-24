import {
  initial, player, legalMoves, applyMove, territoryScore, outcome, chooseMove
} from "./engine.mjs";

const $ = id => document.getElementById(id);
const board = $("board");
let state = initial();
let selected = null;
let mode = "ai";
let aiPending = false;
let aiTimer = null;
const AI_DELAY_MS = 700;

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

  board.replaceChildren();
  for (let i = 0; i < 16; i++) {
    const cell = document.createElement("button");
    cell.type = "button";
    cell.className = "cell";
    if (i === selected) cell.classList.add("selected");
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
  state = applyMove(state, move);
  selected = null;

  if (mode === "ai" && legalMoves(state).length > 0) {
    if (player(state) !== "south") throw new Error("Invalid AI turn");

    aiPending = true;
    render();

    aiTimer = window.setTimeout(() => {
      aiTimer = null;
      state = applyMove(state, chooseMove(state, 2));
      aiPending = false;
      render();
    }, AI_DELAY_MS);
    return;
  }

  render();
}

function reset() {
  if (aiTimer !== null) {
    window.clearTimeout(aiTimer);
    aiTimer = null;
  }
  aiPending = false;
  mode = $("mode").value;
  state = initial();
  selected = null;
  render();
}

$("new").addEventListener("click", reset);
$("mode").addEventListener("change", reset);
reset();
