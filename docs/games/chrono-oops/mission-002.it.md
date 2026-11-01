# ChronoOops! — Missione 002

[English](mission-002.md) | [Italiano](mission-002.it.md)

## Il prestito di domani

- Progetto: GiadaWare BoardLab
- Documento: Game Design Specification M002
- Versione: 0.1
- Stato: proposta progettuale, non implementata
- Base: Missione 001, motore deterministico C11

## 1. Obiettivo

Nel 2025 il Cronofono di Sottosopra rischia di spegnersi.
B deve riattivarlo entro il turno 8 usando un modulo temporale.
Il modulo può essere anticipato dal futuro, ma il suo utilizzo
crea un debito causale che A deve rendere soddisfacibile nel 1985.

Il mondo conserva una sola cronologia modificabile.

## 2. Regole conservate

- Turni alternati A (1985) e B (2025).
- Due punti azione per turno.
- Otto turni complessivi.
- Le azioni previste costano un punto azione.
- Propagazione causale alla fine del turno.
- Determinismo e assenza di universi paralleli.
- Azioni rifiutate senza alterare stato o punti azione.

## 3. Prestito temporale

B può anticipare un solo modulo temporale nella missione.

L'anticipo crea:
- disponibilità provvisoria del modulo nel 2025;
- un debito causale aperto;
- l'obbligo di stabilire un'origine storica compatibile.

Un debito aperto non costituisce di per sé un paradosso.

A estingue il debito approvando nel 1985 la documentazione
necessaria alla produzione del modulo. L'effetto viene
propagato alla fine del proprio turno.

Non esistono interessi, casualità o debiti multipli nel pilota.

## 4. Prevenzione del paradosso

Finché il debito è aperto, A non può distruggere l'unica
documentazione capace di giustificare l'origine del modulo.

Il tentativo:
- restituisce un risultato distinto da un prerequisito mancante;
- lascia invariati stato, inventario e punti azione;
- identifica il debito e la causa incompatibile.

La missione non introduce cronologie alternative né consente
paradossi temporaneamente irrisolti.

## 5. Sequenza nominale

| Turno | Agente | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Consulta archivio | Individua progetto del modulo |
| 2 | B | Esamina Cronofono | Anticipa modulo |
| 3 | A | Registra origine | Approva produzione |
| 4 | B | Installa modulo | Riattiva Cronofono |
| 5 | A | Verifica documentazione | Attende |
| 6 | B | Conferma stabilità | Completa missione |

Il debito nasce al turno 2 e viene estinto dalla propagazione
dell'approvazione storica alla fine del turno 3.

La vittoria nominale avviene al turno 6, con debito estinto
e Cronofono funzionante.

## 6. Sequenza incompatibile

Dopo l'anticipo del turno 2, A tenta di distruggere il solo
progetto che può giustificare l'esistenza del modulo.

Il motore rifiuta l'azione al turno 3, senza consumare PA.
A può ancora approvare la produzione e continuare.

La spiegazione distingue:
- causa: distruzione del progetto;
- vincolo: origine del modulo ancora dovuta;
- effetto impedito: impossibilità di restituire il debito.

## 7. Vittoria e sconfitta

- Vittoria: Cronofono riattivato e debito estinto entro turno 8.
- Sconfitta: obiettivo incompleto alla fine del turno 8.
- Blocco: nessuna sequenza legale può ancora completare l'obiettivo.
- L'anticipo non equivale automaticamente alla restituzione.

## 8. Contratto di verifica

- Alternanza corretta dei giocatori e limite di 2 PA.
- Impossibilità di anticipare il modulo due volte.
- Debito aperto dopo l'anticipo.
- Debito estinto soltanto dopo propagazione storica.
- Tentativo incompatibile rifiutato atomicamente.
- Percorso nominale vincente al turno 6.
- Nessuna vittoria con debito ancora aperto.
- Risultati riproducibili a parità di decisioni.
- Spiegazioni basate sulle transizioni calcolate.

## 9. Confini di implementazione

Missione 001 resta invariata.

L'estensione C11 dovrà distinguere chiaramente le regole
di scenario M001 e M002, riutilizzando i principi del motore
senza introdurre un framework generale dei paradossi.

Esclusi: multiplayer di rete, IA, combattimento, inventario
generale, prestiti multipli e universi paralleli.

## 10. Qualificazione

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- PUBLICATION: NOT_STARTED
