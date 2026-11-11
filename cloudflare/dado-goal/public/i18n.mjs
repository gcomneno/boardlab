/**
 * Dado Goal browser localization.
 *
 * Presentation only: game events and protocol values remain unchanged.
 * No storage, network access or external dependencies.
 */

export const supportedLocales = Object.freeze(["en", "it"]);

export const messages = Object.freeze({
  en: Object.freeze({
    playerPositionGoalkeeper: "Goalkeeper",
    playerPositionDefender: "Defender",
    playerPositionMidfielder: "Midfielder",
    playerPositionAttacker: "Attacker",
    eventBoardHeading: "📋 Match events",
    eventBoardEmpty: "No events recorded.",
    eventGoal: "⚽ GOAL! · #{number}",
    eventPenalty: "🥅 PENALTY · {number} · {outcome}",
    goalScorerCall: "Goal scored by number {number}!",
    eventYellow: "🟨 Yellow card · #{number}{reason}",
    eventRed: "🟥 Red card · #{number}{reason}",
    eventSubstitution: "🔄 Off #{out}, on #{in}{reason}",
    mpTitle: "Dado Goal! — Online match",
    mpSubtitle: "Online multiplayer: two players, one shared match in real time.",
    mpAuthHeading: "Sign in",
    mpLogin: "Start playing",
    mpLogout: "Sign out",
    mpAuthPrompt: "Select “Start playing”, then create a match or enter an invitation.",
    mpAuthSuccess: "Signed in.",
    mpAuthEnded: "Session ended.",
    mpCreateHeading: "Create a match",
    competition: "Competition",
    mpTeam1: "Team 1",
    mpTeam2: "Team 2",
    mpCreate: "Create match",
    mpRemember: "Remember this device",
    mpRememberNote: "Use this option only on a personal device. Saved credentials may be accessible to other people using the same browser.",
    mpInviteHeading: "Invite a friend",
    mpInviteNote: "Share the match invitation code with your opponent.",
    mpInviteCode: "Invitation code",
    mpCopyInvite: "Copy invitation code",
    mpCopied: "✓ Copied!",
    mpJoin: "Join match",
    mpMatchHeading: "Match",
    mpNoMatch: "No active match.",
    mpScore: "Score: {home} – {away}",
    mpHands: "Hands: {home} – {away} / {total}",
    mpTurn: "Player {player} — {detail}",
    mpYourTurn: "your turn",
    mpWaitTurn: "waiting for opponent",
    mpFinalWhistle: "Final whistle!",
    mpDie: "Die: {die}",
    mpLastRoll: "Last roll: {face}",
    mpPlayerFace: "Player {player}: {face}",
    mpPlayerOne: "Player 1",
    mpPlayerTwo: "Player 2",
    mpEventsHeading: "Match events",
    mpStatsHeading: "Match statistics",
    mpClockUnavailable: "MATCH CLOCK · —",
    mpClockHalf: "{half} HALF · {time}",
    mpClockFinal: "FINAL WHISTLE · {time}",
    mpStatGoals: "Goals",
    mpStatShots: "Shots on target",
    mpStatCrosses: "Crosses",
    mpStatPasses: "Passes",
    mpStatSaves: "Saves",
    mpStatWoodwork: "Woodwork",
    mpStatPenalties: "Penalties",
    mpTeamDie: "Team die",
    mpPinkDie: "Pink die",
    mpFinalDie: "Final die",
    mpRoll: "Roll the die",
    mpRefresh: "Refresh status",
    mpLeave: "Leave",
    mpProject: "A GiadaWare · BoardLab project",
    mpCompleted: "Match completed.",
    mpWaiting: "Match created: waiting for your friend.",
    mpActive: "Match active — roll {number}.",
    mpServerError: "Server error: {code} ({status})",
    mpUnknownError: "Unknown error",
    mpInvalidInvite: "Invalid invitation code.",
    mpLoginUnavailable: "Sign-in unavailable. Please try again later.",
    mpLogoutFailed: "Unable to end the session.",
    mpInvalidHands: "Invalid number of hands",
    mpStateUnavailable: "Match state unavailable.",
    mpCopyFailed: "Unable to copy: select the code and copy it manually.",
    mpLeft: "Device forgotten and local match session left.",
    title: "Dado Goal — Play free",
    description: "Dado Goal: the original football dice game by Giancarlo Cicellyn Comneno.",
    multiplayerLink: "Open Dado Goal Multiplayer in a new tab",
    pinkGuide: "🩷 Pink",
    finalGuide: "🎯 Final",
    diceReady: "The match can begin!",
    faceShotOnTarget: "Shot on goal!",
    faceInterceptedPass: "Pass intercepted!",
    faceCross: "Cross!",
    facePass: "Pass",
    faceWide: "Ball goes wide",
    faceScissorKick: "Scissor kick",
    faceOverTheBar: "Over the crossbar",
    faceBallIntercepted: "Ball intercepted",
    faceBicycleKick: "Bicycle kick",
    faceHeader: "Header",
    faceBlocked: "Blocked!",
    faceGoal: "GOAL!",
    faceOut: "WIDE!",
    faceWoodwork: "POST/CROSSBAR",
    facePenalty: "PENALTY",
    faceSaved: "Saved!",
    rulesHeading: "How to play?",
    rules1: "Each player rolls their team die. A shot on goal or a cross leads to the pink die.",
    rules2: "The pink die determines how the attack develops. A scissor kick, bicycle kick or header leads to the final red/blue die.",
    rules3: "The final die can produce a GOAL or end the attack without a goal. A PENALTY means rolling the same die again.",
    rules4: "A pass allows another roll of the team die. At the end of each hand, play passes to the opponent, even after a goal.",
    rules5: "The team with the most goals after the agreed number of hands per team wins. A draw is possible.",
    footer: "Original Dado Goal · Digital adaptation · Two local players · No account · GiadaWare",
    language: "Language",
    competition: "Competition",
    subtitle: "Four dice. Two teams. One goal: GOAL!",
    settings: "Match settings",
    team: "Team {number}",
    color: "Color",
    secondColor: "Second color",
    handsPerTeam: "No. of rolls",
    newGame: "New match",
    cancelMatch: "Cancel match",
    scoreboard: "Scoreboard",
    pitch: "Playing field",
    currentDie: "Current die",
    diceSequence: "Dice sequence",
    teamDie: "Team die",
    pinkDie: "Pink die",
    finalDie: "Final die",
    roll: "Roll the die!",
    rolling: "Rolling...",
    rollPhase: "Roll: {phase}",
    matchFinished: "Match finished",
    matchTurn: "{name}'s turn",
    finalWhistle: "Final whistle!",
    handsProgress: "{done} / {total} hands",
    firstHalf: "1st HALF · {time}",
    secondHalf: "2nd HALF · {time}",
    finalClock: "FULL TIME · {time}",
    commentary: "Commentary",
    commentaryReady: "Commentary ready for kick-off.",
    commentaryDisabled: "Commentary disabled.",
    statistics: "Match statistics",
    metric: "Metric",
    goals: "Goals",
    shots: "Shots on target",
    crosses: "Crosses",
    passes: "Passes",
    saves: "Saves",
    woodwork: "Posts/crossbars",
    penalties: "Penalties",
    invalidHands: "Choose 1 to 1000 hands per team.",
    sameTeamConflict: "The referee cannot start a match between the same team.",
    mixedTeamConflict: "The referee cannot start the match: choose two catalogue teams or two local teams.",
    kitColorConflict: "The referee cannot start the match: the teams share a kit colour.",
    kickoff: "Kick-off!",
    kickoffTurn: "Kick-off! {name} starts.",
    setupPrompt: "Configure the teams and start a new match.",
    goalBy: "⚽ GOAL by {name}!",
    handEnded: "{name}'s hand is over.",
    rollAgain: "Roll the same die again!",
    actionContinues: "The action continues!",
    draw: "Draw! {a}–{b}.",
    winner: "{name} wins! {a}–{b}.",
    cancelConfirm: "Cancel the match in progress?",
    matchCancelled: "Match cancelled."
  }),
  it: Object.freeze({
    playerPositionGoalkeeper: "Portiere",
    playerPositionDefender: "Difensore",
    playerPositionMidfielder: "Centrocampista",
    playerPositionAttacker: "Attaccante",
    eventBoardHeading: "📋 Cronaca della partita",
    eventBoardEmpty: "Nessun evento registrato.",
    eventGoal: "⚽ GOAL! · n° {number}",
    eventPenalty: "🥅 RIGORE · {number} · {outcome}",
    goalScorerCall: "Ha segnato il numero {number}!",
    eventYellow: "🟨 Ammonito il n° {number}{reason}",
    eventRed: "🟥 Espulso il n° {number}{reason}",
    eventSubstitution: "🔄 Esce {out}, entra {in}{reason}",
    mpTitle: "Dado Goal! — Partita online",
    mpSubtitle: "Multiplayer online: due giocatori, una partita condivisa in tempo reale.",
    mpAuthHeading: "Accesso",
    mpLogin: "Inizia a giocare",
    mpLogout: "Esci",
    mpAuthPrompt: "Premi «Inizia a giocare», poi crea una partita o inserisci un invito.",
    mpAuthSuccess: "Accesso effettuato.",
    mpAuthEnded: "Sessione terminata.",
    mpCreateHeading: "Crea una partita",
    competition: "Competizione",
    mpTeam1: "Squadra 1",
    mpTeam2: "Squadra 2",
    mpCreate: "Crea partita",
    mpRemember: "Ricorda questa postazione",
    mpRememberNote: "Usa questa opzione soltanto su un dispositivo personale. Le credenziali salvate possono essere accessibili a chi usa lo stesso browser.",
    mpInviteHeading: "Invita un amico",
    mpInviteNote: "Condividi con l’avversario il codice invito della partita.",
    mpInviteCode: "Codice invito",
    mpCopyInvite: "Copia codice invito",
    mpCopied: "✓ Copiato!",
    mpJoin: "Entra nella partita",
    mpMatchHeading: "Partita",
    mpNoMatch: "Nessuna partita attiva.",
    mpScore: "Risultato: {home} – {away}",
    mpHands: "Mani: {home} – {away} / {total}",
    mpTurn: "Giocatore {player} — {detail}",
    mpYourTurn: "tocca a te",
    mpWaitTurn: "attendi l'avversario",
    mpFinalWhistle: "Fischio finale!",
    mpDie: "Dado: {die}",
    mpLastRoll: "Ultimo lancio: {face}",
    mpPlayerFace: "Giocatore {player}: {face}",
    mpPlayerOne: "Giocatore 1",
    mpPlayerTwo: "Giocatore 2",
    mpEventsHeading: "Eventi della partita",
    mpStatsHeading: "Statistiche della partita",
    mpClockUnavailable: "CRONOMETRO · —",
    mpClockHalf: "{half}° TEMPO · {time}",
    mpClockFinal: "FISCHIO FINALE · {time}",
    mpStatGoals: "Gol",
    mpStatShots: "Tiri in porta",
    mpStatCrosses: "Cross",
    mpStatPasses: "Passaggi",
    mpStatSaves: "Parate",
    mpStatWoodwork: "Pali/traverse",
    mpStatPenalties: "Rigori",
    mpTeamDie: "Dado squadra",
    mpPinkDie: "Dado rosa",
    mpFinalDie: "Dado finale",
    mpRoll: "Lancia il dado",
    mpRefresh: "Aggiorna stato",
    mpLeave: "Esci",
    mpProject: "Un progetto GiadaWare · BoardLab",
    mpCompleted: "Partita conclusa.",
    mpWaiting: "Partita creata: in attesa dell'amico.",
    mpActive: "Partita attiva — lancio {number}.",
    mpServerError: "Errore server: {code} ({status})",
    mpUnknownError: "Errore sconosciuto",
    mpInvalidInvite: "Codice invito non valido.",
    mpLoginUnavailable: "Accesso non disponibile. Riprova più tardi.",
    mpLogoutFailed: "Impossibile terminare la sessione.",
    mpInvalidHands: "Numero di mani non valido",
    mpStateUnavailable: "Stato della partita non disponibile.",
    mpCopyFailed: "Impossibile copiare: seleziona il codice e copialo manualmente.",
    mpLeft: "Postazione dimenticata e sessione locale abbandonata.",
    title: "Dado Goal — Gioca gratis",
    description: "Dado Goal: il gioco calcistico originale di Giancarlo Cicellyn Comneno.",
    multiplayerLink: "Apri Dado Goal Multiplayer in una nuova scheda",
    pinkGuide: "🩷 Rosa",
    finalGuide: "🎯 Finale",
    diceReady: "La partita può iniziare!",
    faceShotOnTarget: "Tiro in porta!",
    faceInterceptedPass: "Passaggio intercettato!",
    faceCross: "Cross!",
    facePass: "Passaggio",
    faceWide: "Palla sul fondo",
    faceScissorKick: "Mezza rovesciata",
    faceOverTheBar: "Palla alta sulla traversa",
    faceBallIntercepted: "Palla intercettata",
    faceBicycleKick: "Rovesciata",
    faceHeader: "Colpo di testa",
    faceBlocked: "Respinto!",
    faceGoal: "GOAL!",
    faceOut: "FUORI!",
    faceWoodwork: "PALO/TRAVERSA",
    facePenalty: "RIGORE",
    faceSaved: "Parato!",
    rulesHeading: "Come si gioca?",
    rules1: "Ogni giocatore lancia il proprio dado squadra. Un tiro in porta o un cross porta al dado rosa.",
    rules2: "Il dado rosa determina lo sviluppo dell'azione. Mezza rovesciata, rovesciata e colpo di testa portano al dado finale rosso/blu.",
    rules3: "Il dado finale può segnare GOAL oppure terminare l'azione senza goal. RIGORE fa rilanciare lo stesso dado.",
    rules4: "Passaggio permette di rilanciare il dado squadra. Ogni fine mano passa il gioco all'avversario, anche dopo un goal.",
    rules5: "Vince chi segna più goal dopo il numero concordato di mani per squadra. Il pareggio è ammesso.",
    footer: "Dado Goal Originale · Adattamento digitale · Due giocatori locali · Nessun account · GiadaWare",
    language: "Lingua",
    competition: "Campionato",
    subtitle: "Quattro dadi. Due squadre. Un solo obiettivo: GOAL!",
    settings: "Impostazioni partita",
    team: "Squadra {number}",
    color: "Colore",
    secondColor: "Secondo colore",
    handsPerTeam: "Num. di lanci",
    newGame: "Nuova partita",
    cancelMatch: "Annulla l'incontro",
    scoreboard: "Tabellone",
    pitch: "Campo di gioco",
    currentDie: "Dado corrente",
    diceSequence: "Sequenza dei dadi",
    teamDie: "Dado squadra",
    pinkDie: "Dado rosa",
    finalDie: "Dado finale",
    roll: "Lancia il dado!",
    rolling: "Lancio in corso...",
    rollPhase: "Lancia: {phase}",
    matchFinished: "Partita conclusa",
    matchTurn: "Tocca a {name}",
    finalWhistle: "Fischio finale!",
    handsProgress: "{done} / {total} mani",
    firstHalf: "1° TEMPO · {time}",
    secondHalf: "2° TEMPO · {time}",
    finalClock: "FISCHIO FINALE · {time}",
    commentary: "Telecronaca",
    commentaryReady: "Telecronaca pronta per il calcio d’inizio.",
    commentaryDisabled: "Telecronaca disattivata.",
    statistics: "Statistiche della partita",
    metric: "Voce",
    goals: "Gol",
    shots: "Tiri in porta",
    crosses: "Cross",
    passes: "Passaggi",
    saves: "Parate",
    woodwork: "Pali/traverse",
    penalties: "Rigori",
    invalidHands: "Scegli da 1 a 1000 mani per squadra.",
    sameTeamConflict: "L'arbitro non può dare il calcio d'inizio: non è possibile giocare contro la stessa squadra.",
    mixedTeamConflict: "L'arbitro non può dare il calcio d'inizio: scegli due squadre del catalogo oppure due squadre locali.",
    kitColorConflict: "L'arbitro non può dare il calcio d'inizio: le squadre condividono un colore della divisa.",
    kickoff: "Calcio d'inizio!",
    kickoffTurn: "Calcio d'inizio! Tocca a {name}.",
    setupPrompt: "Configura le squadre e avvia una nuova partita.",
    goalBy: "⚽ GOAL di {name}!",
    handEnded: "Fine mano di {name}.",
    rollAgain: "Si rilancia lo stesso dado!",
    actionContinues: "L'azione continua!",
    draw: "Pareggio! {a} a {b}.",
    winner: "Vince {name}! {a} a {b}.",
    cancelConfirm: "L'arbitro decide di annullare l'incontro in corso?",
    matchCancelled: "Incontro annullato."
  })
});

let locale = "it";

export function getLocale() {
  return locale;
}

export function setLocale(value) {
  locale = supportedLocales.includes(value) ? value : "en";
}

export function translate(key) {
  return messages[locale][key] ?? messages.en[key] ?? key;
}

export function formatMessage(key, values = {}) {
  return translate(key).replace(/\{([a-z][a-zA-Z]*)\}/g,
    (match, name) => Object.hasOwn(values, name)
      ? String(values[name])
      : match
  );
}


/** Update static labels without touching the match state. */
const bindings = Object.freeze([
  ['label[for="language"] > span', "language"],
  ["header > p:last-child", "subtitle"],
  [".match-stats h3", "statistics"],
  [".stat-header > strong:nth-child(2)", "metric"],
  ['[data-i18n="color"]', "color"],
  ['[data-i18n="handsPerTeam"]', "handsPerTeam"],
  ["#guide-pink", "pinkGuide"],
  ["#guide-result", "finalGuide"],
  ["details.panel > summary", "rulesHeading"],
  ...[1, 2, 3, 4, 5].map(index => [
    `details.panel > p:nth-of-type(${index})`, `rules${index}`
  ]),
  ["main.shell > footer", "footer"]
]);

export function applyLocale() {
  document.documentElement.lang = locale;
  document.title = translate("title");

  const meta = document.querySelector('meta[name="description"]');
  if (meta) meta.setAttribute("content", translate("description"));

  for (const element of document.querySelectorAll("[data-i18n-team]")) {
    element.textContent = formatMessage("team", {
      number: Number(element.dataset.i18nTeam)
    });
  }

  const multiplayer = document.querySelector(".multiplayer-button");
  if (multiplayer) {
    multiplayer.setAttribute("aria-label", translate("multiplayerLink"));
  }

  for (const [selector, key] of bindings) {
    const elements = document.querySelectorAll(selector);
    if (!elements.length) throw new Error(`Missing i18n target: ${selector}`);
    for (const element of elements) element.textContent = translate(key);
  }

  for (const [selector, key] of [
    [".settings", "settings"],
    ['section.panel:has(> .scoreboard)', "scoreboard"],
    [".play", "pitch"],
    ["#die", "currentDie"],
    [".dice-guide", "diceSequence"],
    [".match-commentary", "commentary"]
  ]) {
    const element = document.querySelector(selector);
    if (!element) throw new Error(`Missing i18n target: ${selector}`);
    element.setAttribute("aria-label", translate(key));
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


export function applyMultiplayerLocale() {
  document.documentElement.lang = locale;
  document.title = translate("mpTitle");

  const elements = document.querySelectorAll("[data-i18n]");
  if (!elements.length) throw new Error("Missing multiplayer i18n targets");

  for (const element of elements) {
    element.textContent = translate(element.dataset.i18n);
  }
}

export function initializeMultiplayerLocale(onChange) {
  const selector = document.getElementById("language");
  if (!selector) throw new Error("Missing multiplayer language selector");

  selector.value = locale;
  applyMultiplayerLocale();

  selector.addEventListener("change", () => {
    setLocale(selector.value);
    applyMultiplayerLocale();
    onChange();
  });
}
