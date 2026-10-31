import { applyRoll, newMatch, rollFace } from "./engine.mjs";
import { matchStatistics } from "./statistics.mjs";
import { matchClock } from "./match-clock.mjs";
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
let rollEvents = [];
let matchStarted = false;
let autoRunning = false;

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function render() {
  const clock = matchClock(state);
  byId("match-clock").textContent = clock.finished
    ? `FISCHIO FINALE · ${clock.label}`
    : `${clock.half}° TEMPO · ${clock.label}`;
  byId("match-clock").classList.toggle("halftime", clock.halftime);
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

  const [home, away] = matchStatistics(rollEvents);
  const metrics = [
    ["Gol", "goals"],
    ["Tiri in porta", "shots"],
    ["Cross", "crosses"],
    ["Passaggi", "passes"],
    ["Parate", "saves"],
    ["Pali/traverse", "woodwork"],
    ["Rigori", "penalties"],
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
      name.textContent = label;

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
  byId("new").textContent = playing
    ? "Annulla l'incontro"
    : "Nuova partita";

  const multiplayer = document.querySelector(".multiplayer-button");
  multiplayer.classList.toggle("is-disabled", playing);
  multiplayer.setAttribute("aria-disabled", String(playing));
  multiplayer.tabIndex = playing ? -1 : 0;
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

function start(activate = true) {
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
  matchStarted = activate;
  autoRunning = false;
  isRolling = false;
  rollEvents = [];

  const die = byId("die");
  die.classList.remove("rolling");
  die.textContent = "⚽";
  die.setAttribute("aria-label", "In attesa del primo lancio");
  byId("result").textContent = "Calcio d'inizio!";
  byId("message").textContent = activate
    ? `Calcio d'inizio! Tocca a ${names[0]}.`
    : "Configura le squadre e avvia una nuova partita.";
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
  state = applyRoll(state, face);
  rollEvents.push({ seat: player, die: phase, face });

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
    if (!window.confirm("L'arbitro decide di annullare l'incontro in corso?")) return;
    start(false);
    byId("message").textContent = "Incontro annullato.";
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
start(false);
