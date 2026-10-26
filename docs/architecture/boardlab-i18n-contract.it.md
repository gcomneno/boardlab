# BoardLab — Contratto di internazionalizzazione browser (M0)

[English](boardlab-i18n-contract.md) | [Italiano](boardlab-i18n-contract.it.md)

Stato: PROPOSTO — BoardLab #33

## 1. Scopo

Fornire interfacce in inglese e italiano per i giochi browser BoardLab senza modificare le regole.

Destinatari iniziali:

- Tre Sigilli.
- Dado Goal.
- Interfaccia multiplayer sperimentale di Dado Goal.

## 2. Lingue

Lingue supportate:

- `en`: riferimento canonico delle chiavi.
- `it`: traduzione italiana.

Le chiavi devono essere stabili e indipendenti dal testo visualizzato.

Le interfacce attualmente pubblicate iniziano in italiano, salvo scelta esplicita dell'utente.

Le lingue sconosciute utilizzano il fallback inglese (`en`).

## 3. Selezione e persistenza

L'interfaccia presenta un selettore accessibile EN/IT.

Cambiare lingua deve:

- Aggiornare etichette, istruzioni, messaggi e testi accessibili.
- Conservare partita e impostazioni.
- Non riavviare motori, IA o sessioni multiplayer.
- Funzionare senza connessione.

La scelta esplicita della lingua (`it`) prevale sulla lingua iniziale.

L'eventuale persistenza locale è facoltativa, documentata e tollera l'indisponibilità dello storage.

## 4. Localizzazione runtime

Utilizzare un modulo leggero senza dipendenze esterne.

Requisiti:

- Dizionari espliciti per `en` e `it`.
- Insiemi identici di chiavi obbligatorie.
- Messaggi parametrizzati per valori dinamici.
- Rendering testuale sicuro come impostazione predefinita.
- Nessuna iniezione HTML mediante traduzioni.
- Rilevazione nei test delle chiavi mancanti.
- Fallback inglese deterministico (`en`).

I moduli possono essere condivisi tra giochi, ma i motori non devono dipendere dalla localizzazione.

## 5. Confini

L'internazionalizzazione non deve modificare:

- Regole, punteggi o transizioni deterministiche.
- Algoritmi IA, protocolli dei worker o parametri di ricerca.
- Autorizzazioni di rete o identità delle sessioni multiplayer.
- Formati persistenti degli stati di gioco.

Gli identificatori dei protocolli rimangono indipendenti dalla lingua.

## 6. Documentazione

Restano valide le convenzioni Markdown esistenti:

- Inglese canonico: `document.md`.
- Italiano: `document.it.md`.
- Manifest e validatore bilingue esistenti.

La localizzazione runtime è indipendente dal validatore Markdown.

## 7. Verifica

Controlli automatici richiesti:

- Parità delle chiavi EN/IT.
- Risoluzione delle chiavi utilizzate.
- Interpolazione e gestione delle chiavi mancanti.
- Cambio lingua senza alterare la partita.
- Comportamento equivalente nelle due lingue.
- Funzionamento offline.
- Accessibilità ed etichetta della lingua HTML coerenti.
- Regressioni di gioco, IA e multiplayer superate.

## 8. Sequenza

M0: Inventario, contratto e test del contratto.

M1: Localizzazione Tre Sigilli.

M2: Localizzazione Dado Goal e multiplayer.

M3: Documentazione e navigazione bilingue.

M4: Metadati pubblici e pubblicazione bilingue itch.io.

M0 non autorizza modifiche o rilasci dell'interfaccia pubblica.
