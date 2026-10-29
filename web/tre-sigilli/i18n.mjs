export const supportedLocales = Object.freeze(["en", "it"]);

export const messages = Object.freeze({
  en: {
    randomOption: "Random",
    explainPlace: "Place a seal on square {cell}.",
    explainSlide: "Move the seal from square {origin} to square {destination}.",
    explainScore: "Territory after the move: Ivory {north} ({northDelta}), Terracotta {south} ({southDelta}).",
    explainZero: "No squares change control immediately.",
    explainOne: "One square changes control immediately.",
    explainMany: "{count} squares change control immediately.",
    explainDisclaimer: "This is the immediate territorial consequence, not a guarantee of victory.",
    guidedOn: "Exit guided play",
    guidedFinished: "Game over. Compare the players' territories.",
    guidedWait: "Watch the computer's response.",
    aiMoveLabel: "Computer move analyzed:",
    guidedPlace: "Place a seal on an empty square. You can ask for a suggestion or play on your own.",
    guidedSlide: "Move one of your seals to an adjacent empty square.",
    analyzing: "Analyzing the position…",
    invalidSuggestion: "Invalid suggestion.",
    suggestionReady: "Suggested move highlighted. You can follow it or play freely.",
    suggestionUnavailable: "Suggestion unavailable",
    aiError: "AI error: {message}",
    engineError: "Engine error",
    unknownCommand: "Unknown engine command",
    engineUnavailable: "Unable to run the engine",
    requiredParameter: "Required parameter: {name}",
    strategyRandom: "Random · seed {seed}",
    strategyMcts: "MCTS · {simulations} simulations · UCT {exploration} · seed {seed}",
    strategyDepth: "{algorithm} · depth {depth}",
    activeAi: "Active opponent: {strategy}",
    activeLocal: "Mode: two local players",
    activeAiVsAi: "Ivory: {north} · Terracotta: {south}",

    title: "Tre Sigilli — Play free",
    description: "Tre Sigilli: a free browser strategy game.",
    subtitle: "Three seals each. Sixteen squares. Strategy decides.",
    settings: "Game settings",
    language: "Language",
    mode: "Mode",
    aiMode: "Against the computer",
    localMode: "Two players",
    aiVsAiMode: "Computer vs computer",
    northAlgorithm: "Ivory strategy",
    northStrategyHelp: "Ivory uses the algorithm defaults. Terracotta uses the settings below.",
    newGame: "New game",
    strategySummary: "Configure artificial intelligence",
    strategyHelp: "Choose an algorithm and its settings, then press New game to apply them.",
    algorithm: "Algorithm",
    depth: "Search depth (1–4)",
    simulations: "MCTS simulations (1–3000)",
    exploration: "UCT exploration (≥ 0)",
    seed: "Random seed (0–4294967295)",
    guided: "Try the game",
    suggest: "Suggest a move",
    explain: "Why this move?",
    accept: "Play suggested move",
    game: "Game",
    board: "4 by 4 board",
    ivory: "Ivory",
    terracotta: "Terracotta",
    gameTurn: "Turn {turn} of 18 · {player}",
    gameDraw: "Game over · Draw",
    gameWin: "Game over · {player} wins!",
    gameAgain: "Press New game to play again.",
    gameThinking: "The computer is thinking…",
    gameWait: "Your move is complete. Wait for the response.",
    gamePlace: "Choose an empty square to place a seal.",
    gameSlide: "Select your seal, then an adjacent empty square.",
    cell: "Row {row}, column {column}, {occupant}",
    free: "empty",
    rulesIntro: "Players alternate turns. During the first six turns, each player places three seals on empty squares.",
    rulesMoves: "After placement, move one of your seals to an adjacent empty square horizontally or vertically.",
    rulesEnd: "The game ends after 18 total moves or when the current player cannot move.",
    rulesHeading: "How are points scored?",
    rulesPoints: "Each square is worth 1 point to the player controlling it, even when empty.",
    rulesInfluence: "Count the seals on that square and its four orthogonal neighbors. Diagonals do not count.",
    rulesExample: "Example: if two Ivory seals and one Terracotta seal influence a square, Ivory scores 1 point. Equal influence means no points.",
    rulesCurrent: "Scores show territory controlled at the moment. They can rise or fall after every move and are not cumulative.",
    rulesWinner: "The player controlling more squares wins. Equal scores result in a draw.",
    footer: "Free · Offline after loading · No account · GiadaWare",
    rules: "How to play?"
  },
  it: {
    randomOption: "Casuale",
    explainPlace: "Colloca un sigillo nella casella {cell}.",
    explainSlide: "Sposta il sigillo dalla casella {origin} alla casella {destination}.",
    explainScore: "Territorio dopo la mossa: Avorio {north} (variazione {northDelta}), Terracotta {south} (variazione {southDelta}).",
    explainZero: "Nessuna casella cambia controllo immediatamente.",
    explainOne: "Una casella cambia controllo immediatamente.",
    explainMany: "{count} caselle cambiano controllo immediatamente.",
    explainDisclaimer: "Questa è la conseguenza territoriale immediata, non una garanzia di vittoria.",
    guidedOn: "Esci dalla prova guidata",
    guidedFinished: "Partita conclusa. Confronta i territori dei giocatori.",
    guidedWait: "Osserva la risposta del computer.",
    aiMoveLabel: "Analisi della mossa del computer:",
    guidedPlace: "Colloca un sigillo in una casella vuota. Puoi chiedere un suggerimento oppure giocare da solo.",
    guidedSlide: "Sposta un tuo sigillo in una casella ortogonalmente adiacente e libera.",
    analyzing: "Analizzo la posizione…",
    invalidSuggestion: "Suggerimento non valido.",
    suggestionReady: "Mossa evidenziata. Puoi seguirla o giocare liberamente.",
    suggestionUnavailable: "Suggerimento non disponibile",
    aiError: "Errore IA: {message}",
    engineError: "Errore del motore",
    unknownCommand: "Comando sconosciuto dal motore",
    engineUnavailable: "Impossibile eseguire il motore",
    requiredParameter: "Parametro obbligatorio: {name}",
    strategyRandom: "Casuale · seed {seed}",
    strategyMcts: "MCTS · {simulations} simulazioni · UCT {exploration} · seed {seed}",
    strategyDepth: "{algorithm} · profondità {depth}",
    activeAi: "Avversario attivo: {strategy}",
    activeLocal: "Modalità: due giocatori locali",
    activeAiVsAi: "Avorio: {north} · Terracotta: {south}",

    title: "Tre Sigilli — Gioca gratis",
    description: "Tre Sigilli: gioco strategico gratuito, direttamente nel browser.",
    subtitle: "Tre sigilli a testa. Sedici caselle. La strategia decide.",
    settings: "Impostazioni partita",
    language: "Lingua",
    mode: "Modalità",
    aiMode: "Contro il computer",
    localMode: "Due giocatori",
    aiVsAiMode: "Computer contro computer",
    northAlgorithm: "Strategia Avorio",
    northStrategyHelp: "Avorio usa i parametri predefiniti. Terracotta usa quelli configurati qui sotto.",
    newGame: "Nuova partita",
    strategySummary: "Configura l'intelligenza artificiale",
    strategyHelp: "Scegli l'algoritmo e i parametri, poi premi Nuova partita per applicarli.",
    algorithm: "Algoritmo",
    depth: "Profondità di ricerca (1–4)",
    simulations: "Simulazioni MCTS (1–3000)",
    exploration: "Esplorazione UCT (≥ 0)",
    seed: "Seed casuale (0–4294967295)",
    guided: "Prova il gioco",
    suggest: "Suggerisci una mossa",
    explain: "Perché questa mossa?",
    accept: "Gioca la mossa suggerita",
    game: "Partita",
    board: "Scacchiera 4 per 4",
    ivory: "Avorio",
    terracotta: "Terracotta",
    gameTurn: "Turno {turn} di 18 · {player}",
    gameDraw: "Partita terminata · Pareggio",
    gameWin: "Partita terminata · Vince {player}!",
    gameAgain: "Premi Nuova partita per giocare ancora.",
    gameThinking: "Il computer sta pensando…",
    gameWait: "La tua mossa è stata eseguita. Attendi la risposta.",
    gamePlace: "Scegli una casella vuota per collocare un sigillo.",
    gameSlide: "Seleziona un tuo sigillo, poi una casella adiacente libera.",
    cell: "Riga {row}, colonna {column}, {occupant}",
    free: "libera",
    rulesIntro: "Si alternano i giocatori. Nei primi sei turni ciascuno colloca tre sigilli nelle caselle libere.",
    rulesMoves: "Poi ogni turno permette di spostare un proprio sigillo di una casella in orizzontale o verticale, senza occupare una casella già piena.",
    rulesEnd: "La partita termina dopo 18 mosse complessive o quando il giocatore di turno non può muovere.",
    rulesHeading: "Come si conquistano i punti?",
    rulesPoints: "Ogni casella vale 1 punto per il giocatore che la controlla, anche quando è vuota.",
    rulesInfluence: "Per stabilire chi la controlla, conta i sigilli nella casella stessa e nelle caselle immediatamente sopra, sotto, a destra e a sinistra. Le diagonali non contano.",
    rulesExample: "Esempio: se una casella riceve influenza da 2 sigilli Avorio e 1 Terracotta, assegna 1 punto ad Avorio. Se le influenze sono uguali, non assegna punti a nessuno.",
    rulesCurrent: "I punteggi mostrano il territorio controllato in quel momento: possono aumentare o diminuire dopo ogni mossa. Non sono punti cumulativi.",
    rulesWinner: "Alla fine vince chi controlla più caselle. In caso di parità, la partita termina in pareggio.",
    footer: "Gratuito · Offline dopo il caricamento · Nessun account · GiadaWare",
    rules: "Come si gioca?"
  }
});

let locale = "it";

export function getLocale() {
  return locale;
}

export function translate(key) {
  return messages[locale][key] ?? messages.en[key] ?? key;
}

export function formatMessage(key, values = {}) {
  return translate(key).replace(/\{([a-z][a-zA-Z]*)\}/g, (match, name) =>
    Object.hasOwn(values, name) ? String(values[name]) : match
  );
}

export function explainAnalysis(analysis) {
  const move = analysis.move;

  const description = move.type === "place"
    ? formatMessage("explainPlace", {
        cell: move.cell + 1
      })
    : formatMessage("explainSlide", {
        origin: move.origin + 1,
        destination: move.destination + 1
      });

  const signed = value => value >= 0 ? `+${value}` : String(value);

  const summary = formatMessage("explainScore", {
    north: analysis.scoreAfter.north,
    south: analysis.scoreAfter.south,
    northDelta: signed(analysis.scoreDelta.north),
    southDelta: signed(analysis.scoreDelta.south)
  });

  const count = analysis.changedCells.length;
  const detail = formatMessage(
    count === 0 ? "explainZero"
      : count === 1 ? "explainOne" : "explainMany",
    { count }
  );

  return [
    description,
    summary,
    detail,
    translate("explainDisclaimer")
  ].join(" ");
}

export function setLocale(value) {
  locale = supportedLocales.includes(value) ? value : "en";
}

const bindings = Object.freeze([
  [".subtitle", "subtitle"],
  ['label[for="language"]', "language"],
  ['label[for="mode"]', "mode"],
  ['#mode option[value="ai"]', "aiMode"],
  ['#mode option[value="local"]', "localMode"],
  ['#mode option[value="ai_vs_ai"]', "aiVsAiMode"],
  ['label[for="north-algorithm"]', "northAlgorithm"],
  ["#north-strategy-help", "northStrategyHelp"],
  ["#new", "newGame"],
  ["#strategy-help", "strategyHelp"],
  ['#algorithm option[value="random"]', "randomOption"],
  ["#strategy-panel > summary", "strategySummary"],
  ['label[for="algorithm"]', "algorithm"],
  ['label[for="depth"]', "depth"],
  ['label[for="simulations"]', "simulations"],
  ['label[for="exploration"]', "exploration"],
  ['label[for="seed"]', "seed"],
  ["#guided-suggest", "suggest"],
  ["#guided-explain", "explain"],
  ["#guided-accept", "accept"],
  ...[
    "rulesIntro", "rulesMoves", "rulesEnd", "rulesPoints",
    "rulesInfluence", "rulesExample", "rulesCurrent", "rulesWinner"
  ].map((key, index) => [
    `main.shell > details:last-of-type > p:nth-of-type(${index + 1})`,
    key
  ]),
  ["main.shell > details:last-of-type > h3", "rulesHeading"],
  ["main.shell > footer", "footer"],
  [".scores .north", null],
  [".scores .south", null],
  ["main.shell > details:last-of-type > summary", "rules"]
]);

export function applyLocale() {
  document.documentElement.lang = locale;
  document.title = translate("title");

  const meta = document.querySelector('meta[name="description"]');
  if (meta) meta.setAttribute("content", translate("description"));

  for (const [selector, key] of bindings) {
    if (key === null) continue;
    const element = document.querySelector(selector);
    if (!element) throw new Error(`Missing i18n target: ${selector}`);
    element.textContent = translate(key);
  }

  for (const [selector, key] of [
    [".panel", "settings"],
    [".guided-panel", "guided"],
    [".game", "game"],
    ["#board", "board"]
  ]) {
    const element = document.querySelector(selector);
    if (!element) throw new Error(`Missing i18n target: ${selector}`);
    element.setAttribute("aria-label", translate(key));
  }

  const legendNorth = document.querySelector(".scores .north");
  const legendSouth = document.querySelector(".scores .south");

  if (legendNorth?.nextSibling?.nodeType === 3) {
    legendNorth.nextSibling.textContent = ` ${translate("ivory")} `;
  }
  if (legendSouth?.nextSibling?.nodeType === 3) {
    legendSouth.nextSibling.textContent = ` ${translate("terracotta")} `;
  }
}

export function initializeLocale(onChange) {
  const selector = document.getElementById("language");
  if (!selector) throw new Error("Missing language selector");

  selector.value = locale;
  applyLocale();

  selector.addEventListener("change", () => {
    setLocale(selector.value);
    applyLocale();
    onChange();
  });
}
