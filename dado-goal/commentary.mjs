/**
 * Optional deterministic football commentary.
 *
 * Consumes observed game transitions; never changes game state.
 * No network, randomness, audio or external providers.
 */

const linesIt = {
  "Passaggio": [
    "La squadra fa girare il pallone, cerca uno spiraglio!",
    "Possesso palla, si costruisce con pazienza!",
  ],
  "Passaggio intercettato!": [
    "Pallone intercettato! L'azione sfuma!",
    "La difesa legge tutto e recupera!",
  ],
  "Cross!": [
    "Cross in area! Attenzione al pallone che arriva!",
    "Palla dentro! La difesa deve stare attenta!",
  ],
  "Tiro in porta!": [
    "Si va alla conclusione! Attenzione!",
    "C'è spazio per il tiro! Parte la conclusione!",
  ],
  "GOAL!": [
    "GOOOOOOL! La rete si gonfia! Esplode lo stadio!",
    "RETEEEE! Pallone dentro, che emozione!",
  ],
  "PALO/TRAVERSA": [
    "CLAMOROSO! Il pallone si stampa sul legno!",
    "PALO O TRAVERSA! Il gol era a un soffio!",
  ],
  "Parato!": [
    "CHE PARATA! Il portiere salva il risultato!",
    "Intervento decisivo del portiere! Niente gol!",
  ],
  "RIGORE": [
    "Calcio di rigore! Momento delicatissimo!",
    "L'arbitro indica il dischetto! Sale la tensione!",
  ],
  "FUORI!": [
    "Fuori! Il pallone termina lontano dalla porta!",
  ],
  "Respinto!": [
    "Conclusione respinta! La difesa resiste!",
  ],
  "Palla sul fondo": [
    "La palla termina sul fondo. Occasione sfumata!",
  ],
  "Palla alta sulla traversa": [
    "La palla sorvola la traversa! Nulla di fatto!",
  ],
  "Palla intercettata": [
    "Intercettata! La difesa interrompe l'azione!",
  ],
  "Mezza rovesciata": [
    "Mezza rovesciata! Che gesto acrobatico!",
  ],
  "Rovesciata": [
    "ROVESCIATA! Lo stadio trattiene il respiro!",
  ],
  "Colpo di testa": [
    "Colpo di testa! Attenzione alla conclusione!",
  ],
};


const linesEn = {
  "Passaggio": [
    "The team moves the ball around, looking for an opening!",
    "Patient build-up play! They keep possession!",
  ],
  "Passaggio intercettato!": [
    "INTERCEPTED! The attack breaks down!",
    "The defence reads it perfectly and wins the ball!",
  ],
  "Cross!": [
    "A cross into the box! Here comes the ball!",
    "Ball into the area! The defence is under pressure!",
  ],
  "Tiro in porta!": [
    "A shot on goal! Here we go!",
    "There's room to shoot! The strike is coming!",
  ],
  "GOAL!": [
    "GOOOOOAL! The net bulges! The crowd erupts!",
    "IT'S IN! What an incredible moment!",
  ],
  "PALO/TRAVERSA": [
    "INCREDIBLE! The ball crashes against the woodwork!",
    "POST OR CROSSBAR! So close to a goal!",
  ],
  "Parato!": [
    "WHAT A SAVE! The keeper keeps them in it!",
    "A crucial save by the goalkeeper! No goal!",
  ],
  "RIGORE": [
    "Penalty kick! What a tense moment!",
    "The referee points to the spot! The tension rises!",
  ],
  "FUORI!": [
    "Wide! The ball misses the target!",
  ],
  "Respinto!": [
    "Blocked shot! The defence holds firm!",
  ],
  "Palla sul fondo": [
    "The ball goes out of play. The chance is gone!",
  ],
  "Palla alta sulla traversa": [
    "Over the crossbar! No goal this time!",
  ],
  "Palla intercettata": [
    "Intercepted! The defence stops the move!",
  ],
  "Mezza rovesciata": [
    "A scissor kick! What an acrobatic effort!",
  ],
  "Rovesciata": [
    "BICYCLE KICK! The crowd holds its breath!",
  ],
  "Colpo di testa": [
    "A header! This could be dangerous!",
  ],
};

export function describeRoll(event, before, after, teamNames = ["Squadra 1", "Squadra 2"], index = 0, locale = "it") {
  if (!event || !before || !after ||
      ![0, 1].includes(event.seat) ||
      event.seat !== before.player ||
      !Number.isSafeInteger(index) || index < 0 ||
      !Array.isArray(teamNames) || teamNames.length !== 2 ||
      !teamNames.every(name => typeof name === "string")) {
    throw new RangeError("Invalid commentary input");
  }

  const choices = (locale === "it" ? linesIt : linesEn)[event.face];
  if (!choices) throw new RangeError("Unknown commentary event");

  const name = teamNames[event.seat];
  const phrase = choices[index % choices.length];

  if (after.phase === "finished") {
    const [a, b] = after.goals;
    const ending = locale === "it"
      ? (a === b
        ? `Triplice fischio! Pareggio ${a} a ${b}!`
        : `Triplice fischio! Vince ${teamNames[a > b ? 0 : 1]}! ${a} a ${b}!`)
      : (a === b
        ? `Final whistle! A ${a}-${b} draw!`
        : `Final whistle! ${teamNames[a > b ? 0 : 1]} win! ${a}-${b}!`);
    return `${phrase} ${ending}`;
  }

  if (event.face === "GOAL!" && after.goals[event.seat] > before.goals[event.seat]) {
    return locale === "it" ? `${phrase} Segna ${name}!` : `${phrase} ${name} scores!`;
  }

  return `${name}: ${phrase}`;
}

/**
 * Describe a referee event without modifying the original roll commentary.
 * All text is derived from event data, so changing locale is reversible.
 */
export function describeMatchEvent(event, teamNames, locale = "it") {
  if (!event || !["yellow", "red", "substitution"].includes(event.type) ||
      ![0, 1].includes(event.team) ||
      !Array.isArray(teamNames) || teamNames.length !== 2 ||
      !teamNames.every(name => typeof name === "string") ||
      !Number.isInteger(event.minute) ||
      event.minute < 0 || event.minute > 90) {
    throw new RangeError("Invalid referee commentary input");
  }

  const name = teamNames[event.team];
  const minute = event.minute;

  if (event.type === "substitution") {
    if (!Number.isInteger(event.out) || !Number.isInteger(event.in)) {
      throw new RangeError("Invalid substitution numbers");
    }
    return locale === "it"
      ? `${minute}′ Cambio per ${name}! Esce il numero ${event.out}, entra il numero ${event.in}.`
      : `${minute}′ Substitution for ${name}! Number ${event.out} off, number ${event.in} on.`;
  }

  if (!Number.isInteger(event.number)) {
    throw new RangeError("Invalid shirt number");
  }

  if (event.type === "yellow") {
    return locale === "it"
      ? `${minute}′ Cartellino giallo! Ammonito il numero ${event.number} di ${name}.`
      : `${minute}′ Yellow card! Number ${event.number} of ${name} is booked.`;
  }

  return locale === "it"
    ? `${minute}′ Cartellino rosso! Espulso il numero ${event.number} di ${name}!`
    : `${minute}′ Red card! Number ${event.number} of ${name} is sent off!`;
}
