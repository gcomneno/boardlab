import { applyRoll, newMatch, rollFace } from "./engine.mjs";

const byId = (id) => document.getElementById(id);
const phaseNames = {
  team: "Dado squadra",
  pink: "Dado rosa",
  result: "Dado finale rosso/blu",
};

const dieFrames = ["⚀", "⚁", "⚂", "⚃", "⚄", "⚅"];

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
  die.textContent = "🎲";
  die.classList.remove("rolling");
  byId("result").textContent = "Calcio d'inizio!";
  byId("message").textContent =
    "Primo turno: squadra 1 (convenzione digitale provvisoria).";
  render();
}

async function animateRoll(finalFace) {
  const die = byId("die");
  die.classList.add("rolling");

  for (let index = 0; index < 10; index += 1) {
    die.textContent = dieFrames[index % dieFrames.length];
    await sleep(70);
  }

  die.classList.remove("rolling");
  die.textContent = "🎲";
  byId("result").textContent = finalFace;
}

async function roll() {
  if (state.phase === "finished" || isRolling) return;

  isRolling = true;
  render();

  const player = state.player;
  const face = rollFace(state.phase);

  await animateRoll(face);
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
