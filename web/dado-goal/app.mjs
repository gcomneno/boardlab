import { applyRoll, newMatch, rollFace } from "./engine.mjs";
import { DICE_ART } from "./dice-art.mjs";

const byId = (id) => document.getElementById(id);
const phaseNames = {
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

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function render() {
  for (let player = 0; player < 2; player += 1) {
    byId(`name-${player}`).textContent = names[player];
    byId(`goals-${player}`).textContent = state.goals[player];
    byId(`hands-${player}`).textContent =
      `${state.hands[player]} / ${state.handsPerTeam} mani`;
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

  const finished = state.phase === "finished";
  byId("roll").disabled = finished || isRolling;
  byId("phase").textContent = finished
    ? "PARTITA TERMINATA"
    : phaseNames[state.phase];
  byId("turn").textContent = finished
    ? "Fischio finale!"
    : `Tocca a ${names[state.player]}`;
  byId("roll").textContent = finished
    ? "Partita conclusa"
    : isRolling
      ? "Lancio in corso..."
      : `Lancia: ${phaseNames[state.phase]}`;
}

function start() {
  const rawHands = byId("hands").value;
  const n = Number(rawHands);

  if (
    !/^\d+$/.test(rawHands.trim()) ||
    !Number.isSafeInteger(n) ||
    n < 1 ||
    n > 1000
  ) {
    byId("message").textContent = "Scegli da 1 a 1000 mani per squadra.";
    return;
  }

  names = [0, 1].map((i) => byId(`team-${i}`).value.trim() || `Squadra ${i + 1}`);
  colors = [0, 1].map((i) => byId(`color-${i}`).value);
  state = newMatch(n);
  isRolling = false;

  const die = byId("die");
  die.classList.remove("rolling");
  die.textContent = "⚽";
  die.setAttribute("aria-label", "In attesa del primo lancio");
  byId("result").textContent = "Calcio d'inizio!";
  byId("message").textContent =
    "Primo turno: squadra 1 (convenzione digitale provvisoria).";
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
  if (state.phase === "finished" || isRolling) return;

  isRolling = true;
  render();

  const player = state.player;
  const face = rollFace(state.phase);

  await animateRoll(state.phase, player, face);
  state = applyRoll(state, face);

  const handEnded = state.phase === "finished" || state.player !== player;

  if (handEnded) {
    const scored = face === "GOAL!";
    byId("message").textContent = scored
      ? `⚽ GOAL di ${names[player]}!`
      : `Fine mano di ${names[player]}.`;
  } else if (face === "Passaggio" || face === "RIGORE") {
    byId("message").textContent = "Si rilancia lo stesso dado!";
  } else {
    byId("message").textContent = "L'azione continua!";
  }

  if (state.phase === "finished") {
    const [a, b] = state.goals;
    byId("message").textContent = a === b
      ? `Pareggio! ${a} a ${b}.`
      : `Vince ${names[a > b ? 0 : 1]}! ${a} a ${b}.`;
  }

  isRolling = false;
  render();
}

byId("new").addEventListener("click", start);
byId("roll").addEventListener("click", () => {
  void roll();
});
start();
