const TEXT = Object.freeze({
  it: Object.freeze({
    "light-knock":
      ({ minute, number }) =>
        `${minute}′ Problema fisico per il numero ${number}: qualche istante per riprendersi, poi può continuare.`,
    "heavy-rain":
      ({ minute }) =>
        `${minute}′ Pioggia molto intensa: il campo si fa pesante, ma si continua a giocare.`,
    "strong-wind":
      ({ minute }) =>
        `${minute}′ Raffiche di vento sul terreno di gioco: condizioni complicate per qualche momento.`,
    "snow-poor-visibility":
      ({ minute }) =>
        `${minute}′ Neve e visibilità ridotta: scenario insolito, ma la partita può proseguire.`,
    "pitch-invasion":
      ({ minute }) =>
        `${minute}′ Invasione di campo! Breve interruzione mentre viene liberato il terreno di gioco.`,
    "object-on-pitch":
      ({ minute }) =>
        `${minute}′ Un oggetto finisce sul terreno di gioco: pausa necessaria per rimuoverlo.`,
    "animal-on-pitch":
      ({ minute }) =>
        `${minute}′ Ospite inatteso in campo! Un animale attraversa il terreno di gioco prima di essere accompagnato fuori.`,
    "floodlight-failure":
      ({ minute }) =>
        `${minute}′ Problema all'illuminazione: breve sospensione tecnica, poi si può riprendere.`,
    "damaged-goal-net":
      ({ minute }) =>
        `${minute}′ La rete della porta richiede un piccolo intervento: gioco momentaneamente fermo.`,
    "referee-equipment-problem":
      ({ minute }) =>
        `${minute}′ Problema tecnico per l'arbitro: attrezzatura sistemata e partita pronta a riprendere.`,
  }),
  en: Object.freeze({
    "light-knock":
      ({ minute, number }) =>
        `${minute}′ A knock for number ${number}: a moment to recover, then the player can continue.`,
    "heavy-rain":
      ({ minute }) =>
        `${minute}′ Heavy rain is falling: the pitch is getting difficult, but play continues.`,
    "strong-wind":
      ({ minute }) =>
        `${minute}′ Strong gusts sweep across the pitch: tricky conditions for a few moments.`,
    "snow-poor-visibility":
      ({ minute }) =>
        `${minute}′ Snow and reduced visibility: unusual conditions, but the match can continue.`,
    "pitch-invasion":
      ({ minute }) =>
        `${minute}′ Pitch invasion! A brief interruption while the field is cleared.`,
    "object-on-pitch":
      ({ minute }) =>
        `${minute}′ An object lands on the pitch: play pauses while it is removed.`,
    "animal-on-pitch":
      ({ minute }) =>
        `${minute}′ An unexpected visitor! An animal crosses the pitch before being guided away.`,
    "floodlight-failure":
      ({ minute }) =>
        `${minute}′ Floodlight problem: a brief technical stoppage before play resumes.`,
    "damaged-goal-net":
      ({ minute }) =>
        `${minute}′ The goal net needs a quick repair: play is briefly stopped.`,
    "referee-equipment-problem":
      ({ minute }) =>
        `${minute}′ The referee has an equipment problem: it is fixed and play can resume.`,
  }),
});

export function describeExtraordinaryEvent(event, locale = "it") {
  if (!event || !Number.isInteger(event.minute) ||
      event.minute < 0 || event.minute > 90) {
    throw new RangeError("Invalid extraordinary commentary input");
  }

  const table = TEXT[locale] ?? TEXT.it;
  const describe = table[event.type];

  if (!describe) {
    throw new RangeError("Unknown extraordinary event");
  }

  if (event.type === "light-knock" &&
      !Number.isInteger(event.number)) {
    throw new RangeError("Invalid extraordinary event player");
  }

  return describe(event);
}
