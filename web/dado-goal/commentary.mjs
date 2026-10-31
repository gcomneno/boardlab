/**
 * Optional deterministic football commentary.
 *
 * Consumes observed game transitions; never changes game state.
 * No network, randomness, audio or external providers.
 */

const lines = {
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

export function describeRoll(event, before, after, teamNames = ["Squadra 1", "Squadra 2"], index = 0) {
  if (!event || !before || !after ||
      ![0, 1].includes(event.seat) ||
      event.seat !== before.player ||
      !Number.isSafeInteger(index) || index < 0 ||
      !Array.isArray(teamNames) || teamNames.length !== 2 ||
      !teamNames.every(name => typeof name === "string")) {
    throw new RangeError("Invalid commentary input");
  }

  const choices = lines[event.face];
  if (!choices) throw new RangeError("Unknown commentary event");

  const name = teamNames[event.seat];
  const phrase = choices[index % choices.length];

  if (after.phase === "finished") {
    const [a, b] = after.goals;
    const ending = a === b
      ? `Triplice fischio! Pareggio ${a} a ${b}!`
      : `Triplice fischio! Vince ${teamNames[a > b ? 0 : 1]}! ${a} a ${b}!`;
    return `${phrase} ${ending}`;
  }

  if (event.face === "GOAL!" && after.goals[event.seat] > before.goals[event.seat]) {
    return `${phrase} Segna ${name}!`;
  }

  return `${name}: ${phrase}`;
}
