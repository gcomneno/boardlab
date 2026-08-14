# Sessione 01 — Review della copertura

[English](coverage-review.md) | [Italiano](coverage-review.it.md)

## Scopo

Questa review verifica se la preparazione dei contenuti della Sessione 01 è
completa prima che il repository possa marcare la sessione come **Prepared**.

Completezza dei contenuti e readiness del repository sono gate separati.

Questo documento può concludere che la preparazione dei contenuti è completa
mentre la sessione rimane in **Preparation in progress** fino al superamento
della validazione finale repository-wide.

## Obiettivo della sessione

La Sessione 01 prepara il modello computazionale necessario per ragionare su un
gioco da tavolo prima di implementare un gioco concreto o un algoritmo di
ricerca.

L'ambito concettuale previsto è:

- stato del gioco;
- mosse legali;
- transizioni di stato;
- stati terminali e risultato;
- identità del giocatore;
- comportamento della strategia;
- coordinamento della partita;
- confini delle responsabilità e delle dipendenze.

## Copertura obiettivi-unità

| Obiettivo | Unità primaria | Copertura |
|---|---|---|
| Stato completo e valido | Unità 01 | Covered |
| Mosse legali e autorità delle regole | Unità 02 | Covered |
| Transizioni e indipendenza dei successori | Unità 03 | Covered |
| Distinzione tra terminalità, risultato e valutazione | Unità 04 | Covered |
| Identità del giocatore rispetto alla strategia | Unità 05 | Covered |
| Coordinamento di Match e confini delle responsabilità | Unità 06 | Covered |

Tutti gli obiettivi concettuali previsti possiedono una unità didattica
primaria.

## Copertura unità-assessment

| Unità | Assessment | Domande dirette | Answer key |
|---|---|---:|---|
| Unità 01 — Stato del gioco | Quiz 01 | 2 | Present |
| Unità 02 — Mosse legali | Quiz 01 | 2 | Present |
| Unità 03 — Transizioni di stato | Quiz 01 | 2 | Present |
| Unità 04 — Stati terminali e risultato | Quiz 02 | 2 | Present |
| Unità 05 — Giocatore e strategia | Quiz 02 | 2 | Present |
| Unità 06 — Coordinamento della partita | Quiz 02 | 2 | Present |

Ogni unità possiede copertura diretta nell'assessment.

Quiz 01 contiene 6 domande.

Quiz 02 contiene 6 domande.

L'assessment completo della Sessione 01 contiene quindi 12 domande.

## Copertura delle misconception

### Stato

Misconception coperte:

- la rappresentazione visibile del tabellone è automaticamente uno stato
  completo;
- il contesto globale nascosto del gameplay è accettabile;
- gli stati ipotetici possono condividere tranquillamente dati mutabili.

Coperte da:

- Unità 01;
- domande 1 e 2 del Quiz 01.

### Legalità delle mosse

Misconception coperte:

- qualsiasi azione rappresentabile è legale;
- le strategie dovrebbero implementare autonomamente le regole concrete di
  legalità.

Coperte da:

- Unità 02;
- domande 3 e 4 del Quiz 01.

### Transizioni di stato

Misconception coperte:

- una mossa legale garantisce automaticamente un successore valido;
- i rami ipotetici della ricerca possono modificare lo stesso stato corrente.

Coperte da:

- Unità 03;
- domande 5 e 6 del Quiz 01.

### Terminalità e valutazione

Misconception coperte:

- uno stato con valutazione forte deve essere terminale;
- un risultato terminale ha significato identico per ogni giocatore.

Coperte da:

- Unità 04;
- domande 1 e 2 del Quiz 02.

### Giocatore e strategia

Misconception coperte:

- identità del giocatore e comportamento della strategia sono una sola
  responsabilità;
- le strategie generiche dovrebbero contenere regole concrete del gioco.

Coperte da:

- Unità 05;
- domande 3 e 4 del Quiz 02.

### Coordinamento di Match

Misconception coperte:

- `Match` dovrebbe possedere regole specifiche o algoritmi decisionali;
- aggiungere un nuovo gioco dovrebbe normalmente richiedere modifiche
  all'orchestrazione generica.

Coperte da:

- Unità 06;
- domande 5 e 6 del Quiz 02.

Tutti i gruppi di misconception definiti dal quiz plan ricevono copertura
diretta nell'assessment.

## Completezza delle answer key

Ognuna delle 12 domande possiede materiale di review separato.

Per ogni domanda, la answer key include:

- risposta attesa;
- ragionamento;
- misconception tipica;
- takeaway pratico.

Le answer key sono conservate separatamente dai file dei quiz.

Sono destinate alla review dopo un tentativo reale del learner.

## Separazione dei quiz

I file dei quiz contengono soltanto domande e opzioni di risposta.

Devono rimanere privi di:

- marker della risposta corretta;
- sezioni di answer key;
- sezioni di soluzione;
- spiegazioni che rivelino direttamente la risposta.

Questa separazione deve essere verificata automaticamente prima che la sessione
diventi **Prepared**.

## Copertura delle fonti

La sessione utilizza attualmente le fonti del repository BoardLab già
identificate nella source coverage map.

Fonti primarie:

- `docs/architecture/overview.md`;
- `README.md`.

Fonti di supporto:

- `docs/roadmap.md`;
- `docs/architecture/adr/0001-python-toolchain.md`.

Il materiale didattico preparato non richiede regolamenti commerciali esterni,
manuali protetti, libri di testo o materiale relativo a giochi commerciali.

Gli esempi originali come Three Stones sono artefatti didattici BoardLab.

## Confine di pubblicazione

La sessione non richiede la pubblicazione di:

- PDF di regolamenti commerciali;
- scansioni;
- artwork protetto;
- raccolte sostanziali di testi delle carte;
- regolamenti commerciali copiati;
- trascrizioni delle fonti;
- corpus di studio privati.

Il materiale privato, se mai necessario in seguito, appartiene a
`sources/private/` e deve restare fuori dalla cronologia Git.

Il contenuto corrente della Sessione 01 non dipende da file sorgente privati.

## Materiale deliberatamente rimandato

Restano fuori dalla Sessione 01:

- interfacce Python definitive;
- implementazione concreta del dominio;
- regole e implementazione di Tre Sigilli;
- implementazione della strategia casuale;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- benchmark prestazionali;
- UI o rappresentazione grafica;
- meccaniche avanzate specifiche dei giochi.

Nessuna unità o assessment preparato richiede questi argomenti.

## Navigazione canonica

Il percorso di navigazione previsto è:

README root
→ indice sessions
→ README Sessione 01
→ source coverage map
→ study map
→ unità didattiche
→ quiz
→ answer key dopo un tentativo reale
→ coverage review

Non è stato identificato alcun percorso didattico attivo concorrente per la
Sessione 01.

`docs/roadmap.md` rimane la roadmap software.

`docs/progress.md` rimane il tracker del learner e della readiness del materiale
didattico.

## Preparazione rispetto allo studio

La preparazione del repository e l'attività del learner rimangono separate.

Stato corrente del learner:

- unità studiate: 0 di 6;
- quiz affrontati: 0 di 2;
- review delle risposte completate: 0 di 2.

Creare o pubblicare il materiale didattico non deve modificare questi valori.

La checkbox di studio della Sessione 01 rimane quindi incompleta.

## Decisione sul materiale da colloquio

Un interview bank dedicato non è necessario per questa sessione fondamentale.

La Sessione 01 è focalizzata sulla costruzione del modello mentale del dominio
BoardLab e dei confini delle responsabilità.

Materiale orientato ai colloqui potrà essere introdotto successivamente quando
l'ambito includerà review architetturale, trade-off algoritmici o discussioni
simili al System Design.

La sua assenza è deliberata e non rappresenta un artefatto di preparazione
mancante.

## Risultato della preparazione dei contenuti

Sono presenti source map, study map, sei unità didattiche, quiz plan, due quiz,
due answer key separate e questa coverage review.

Obiettivi, misconception, copertura dell'assessment, confine di pubblicazione,
materiale rimandato e navigazione canonica risultano considerati.

**Content preparation complete.**

Questa dichiarazione non marca la sessione come **Prepared**.

Lo stato Prepared richiede ancora validazione finale repository-wide, controllo
dell'inventario, verifica della navigazione, enforcement anti-leakage dei quiz,
controlli di pubblicazione, validazione whitespace, ispezione dello stato Git e
nuova validazione dopo il cambio di stato.
