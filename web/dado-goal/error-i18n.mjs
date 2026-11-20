import { getLocale } from "./i18n.mjs";

const errors = Object.freeze({
  "manager lineup must contain exactly 11 players": [
    "The lineup must contain exactly 11 starters.",
    "La formazione deve contenere esattamente 11 titolari.",
  ],
  "manager bench cannot contain more than 9 players": [
    "The bench cannot contain more than 9 players.",
    "La panchina non può contenere più di 9 giocatori.",
  ],
  "lineup and bench must not overlap": [
    "A player cannot be both a starter and a substitute.",
    "Un giocatore non può essere contemporaneamente titolare e in panchina.",
  ],
  "MATCH_NOT_FOUND": [
    "Match not found.",
    "Partita non trovata.",
  ],
  "MATCH_EXPIRED": [
    "The multiplayer match has expired.",
    "La partita multiplayer è scaduta.",
  ],
  "UNAUTHORIZED": [
    "You are not authorized for this match.",
    "Non sei autorizzato per questa partita.",
  ],
  "BAD_REQUEST": [
    "Invalid multiplayer request.",
    "Richiesta multiplayer non valida.",
  ],
  "INTERNAL_ERROR": [
    "The multiplayer service encountered an error.",
    "Si è verificato un errore nel servizio multiplayer.",
  ],
  "HTTP_ERROR": [
    "Unable to complete the multiplayer request.",
    "Impossibile completare la richiesta multiplayer.",
  ],
  "Invalid invitation": [
    "Invalid invitation.",
    "Invito non valido.",
  ],
  "Invalid match identifier": [
    "Invalid match identifier.",
    "Identificativo partita non valido.",
  ],
  "No active multiplayer session": [
    "No active multiplayer session.",
    "Nessuna sessione multiplayer attiva.",
  ],
  "Memorizzazione browser non disponibile": [
    "Browser storage is unavailable.",
    "Memorizzazione browser non disponibile.",
  ],
});

const fallback = Object.freeze({
  general: [
    "Unable to complete the operation.",
    "Impossibile completare l'operazione.",
  ],
  manager: [
    "Invalid lineup or squad selection.",
    "Formazione o selezione della rosa non valida.",
  ],
  save: [
    "Invalid or unsupported Dado Goal save.",
    "Salvataggio Dado Goal non valido o non supportato.",
  ],
  multiplayer: [
    "Unable to complete the multiplayer operation.",
    "Impossibile completare l'operazione multiplayer.",
  ],
});

export function localizedError(error, context = "general") {
  const index = getLocale() === "it" ? 1 : 0;
  const key = typeof error?.code === "string"
    ? error.code
    : typeof error?.message === "string"
      ? error.message
      : "";
  return (errors[key] ?? fallback[context] ?? fallback.general)[index];
}
