# Sessione 01 — Piano dei quiz

[English](quiz-plan.md) | [Italiano](quiz-plan.it.md)

## Scopo

Questo piano definisce la strategia di assessment della Sessione 01 prima della
scrittura delle domande e delle answer key.

I quiz devono misurare se il learner sa ragionare sul modello astratto di gioco
e sui confini delle responsabilità di BoardLab, non soltanto ripetere
definizioni.

I file dei quiz e le answer key devono rimanere rigorosamente separati.

## Obiettivi dell'assessment

L'assessment dovrebbe verificare che il learner sappia:

- identificare cosa appartiene a un `GameState` completo;
- distinguere azioni rappresentabili da `Move` legali;
- ragionare sulle transizioni e sull'indipendenza degli stati successori;
- distinguere terminalità, risultato e valutazione strategica;
- distinguere l'identità di `Player` dal comportamento di `Strategy`;
- spiegare la responsabilità di `Match`;
- identificare la fuoriuscita di regole concrete nei componenti generici;
- ragionare sulla direzione delle dipendenze tra gioco, motore e strategia.

## Struttura dell'assessment

La Sessione 01 userà due quiz.

### Quiz 01 — Stato, legalità e transizioni

Copertura:

- Unità 01 — Stato del gioco;
- Unità 02 — Mosse legali;
- Unità 03 — Transizioni di stato.

Numero di domande:

6 domande.

Obiettivi principali di misurazione:

- completezza dello stato;
- validità dello stato;
- contesto mutabile nascosto;
- rappresentazione della mossa rispetto alla legalità;
- posizione autorevole delle regole del gioco;
- correttezza delle transizioni;
- indipendenza degli stati successori.

### Quiz 02 — Terminazione, strategia e coordinamento

Copertura:

- Unità 04 — Stati terminali e risultato;
- Unità 05 — Giocatore e strategia;
- Unità 06 — Coordinamento della partita e confini delle responsabilità.

Numero di domande:

6 domande.

Obiettivi principali di misurazione:

- terminalità rispetto alla valutazione;
- risultato relativo al giocatore;
- identità del giocatore rispetto al comportamento della strategia;
- intercambiabilità delle strategie;
- responsabilità generiche rispetto a quelle specifiche del gioco;
- confini di orchestrazione di `Match`;
- direzione delle dipendenze.

## Tipologie di domande

I quiz dovrebbero combinare:

- domande concettuali a scelta multipla;
- brevi analisi di scenario;
- domande sull'assegnazione delle responsabilità;
- identificazione di misconception;
- piccoli ragionamenti architetturali.

Il puro richiamo mnemonico di termini dovrebbe essere limitato.

Almeno metà delle domande complessive dei due quiz dovrebbe richiedere
ragionamento su uno scenario concreto invece della semplice ripetizione di una
definizione.

## Misconception da colpire

### Misconception sullo stato

- la sola posizione visibile sul tabellone è sempre uno stato completo;
- il contesto globale nascosto è accettabile se il programma può accedervi;
- gli stati ipotetici della ricerca possono condividere tranquillamente dati
  mutabili del gameplay;
- la rappresentazione UI appartiene automaticamente allo stato del dominio.

### Misconception su mossa e legalità

- qualsiasi mossa rappresentabile è legale;
- le strategie dovrebbero implementare indipendentemente la legalità;
- il motore generico dovrebbe conoscere regole concrete del tabellone;
- la legalità può essere determinata senza tutte le informazioni rilevanti
  dello stato.

### Misconception sulle transizioni

- una mossa e il suo stato successore sono lo stesso concetto;
- una mossa legale garantisce automaticamente uno stato successore corretto;
- modificare lo stato originale è innocuo durante una ricerca ipotetica;
- le strategie dovrebbero eseguire direttamente transizioni specifiche del
  gioco.

### Misconception sugli stati terminali

- assenza di mosse legali significa universalmente fine del gioco;
- ogni stato valutato è terminale;
- rilevamento della terminalità e valutazione strategica sono equivalenti;
- il risultato ha un unico significato universale indipendente dal giocatore.

### Misconception su giocatore e strategia

- identità del giocatore e strategia sono una sola responsabilità;
- le strategie dovrebbero contenere regole concrete del gioco;
- cambiare strategia richiede modificare il dominio del gioco;
- una strategia generica può presumere una prospettiva fissa del giocatore.

### Misconception su Match

- `Match` dovrebbe contenere regole concrete sulle mosse perché controlla i
  turni;
- `Match` dovrebbe scegliere autonomamente le mosse;
- la CLI può essere parte del dominio del gioco;
- aggiungere un nuovo gioco dovrebbe normalmente richiedere modifiche alla
  logica generica di match.

## Matrice di copertura

| Unità | Quiz | Domande previste |
|---|---|---:|
| Unità 01 — Stato del gioco | Quiz 01 | 2 |
| Unità 02 — Mosse legali | Quiz 01 | 2 |
| Unità 03 — Transizioni di stato | Quiz 01 | 2 |
| Unità 04 — Stati terminali e risultato | Quiz 02 | 2 |
| Unità 05 — Giocatore e strategia | Quiz 02 | 2 |
| Unità 06 — Coordinamento della partita | Quiz 02 | 2 |

Ogni unità riceve quindi una copertura diretta nell'assessment.

Le domande possono combinare concetti delle unità precedenti quando utile, ma
nessuna dovrebbe richiedere materiale deliberatamente rimandato oltre la
Sessione 01.

## Regole di pubblicazione dei quiz

I file dei quiz non devono contenere:

- risposta corretta;
- answer key;
- sezioni di soluzione;
- spiegazioni che rivelino direttamente la risposta attesa.

Il validator del repository dovrà infine imporre automaticamente questa
separazione.

## Requisiti delle answer key

Ogni quiz deve avere una answer key separata.

Per ciascuna domanda, la answer key dovrebbe spiegare almeno:

- risposta o ragionamento atteso;
- perché tale ragionamento è difendibile rispetto al materiale della
  Sessione 01;
- misconception tipica oggetto della domanda;
- takeaway pratico.

Le answer key sono materiale di review e dovrebbero essere consultate soltanto
dopo un tentativo reale del quiz durante lo studio attivo.

## Politica linguistica

Ogni quiz e answer key deve essere pubblicato come documento canonico inglese
con controparte italiana `.it.md`.

Numerazione delle domande, identificatori tecnici, struttura delle risposte e
significato degli scenari devono rimanere sincronizzati nelle coppie.

## Confine dell'assessment

I quiz non devono richiedere:

- interfacce Python definitive;
- regole di Tre Sigilli;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- analisi prestazionali;
- conoscenza di giochi da tavolo commerciali.

L'assessment deve rimanere entro source coverage e study map della Sessione 01.

## Stato della preparazione

Questo piano definisce soltanto la struttura dell'assessment.

Non è stata ancora scritta alcuna domanda né answer key.

Nessun assessment è stato affrontato dal learner.
