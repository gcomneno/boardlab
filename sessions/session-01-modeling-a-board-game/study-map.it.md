# Sessione 01 — Mappa di studio

[English](study-map.md) | [Italiano](study-map.it.md)

## Scopo

Questa mappa definisce la sequenza didattica della Sessione 01 prima della
scrittura delle singole unità.

La sequenza procede dalla rappresentazione di una situazione di gioco fino al
coordinamento di una partita completa.

Ogni unità dipende soltanto da concetti introdotti precedentemente nella
sequenza.

## Sequenza didattica

### Unità 01 — Stato del gioco

Domanda principale:

Quali informazioni devono esistere per descrivere una situazione completa di
gioco?

Focus didattico:

- distinguere il gioco da una sua particolare situazione;
- identificare le informazioni che appartengono a uno stato completo;
- ragionare su validità e indipendenza degli stati;
- capire perché gli algoritmi di ricerca richiedono stati espliciti invece di
  contesto mutabile nascosto.

Perché viene per prima:

Ogni concetto successivo fa riferimento a uno stato. Mosse legali, transizioni,
condizioni terminali e decisioni strategiche non hanno un significato preciso
finché la situazione corrente non può essere rappresentata esplicitamente.

### Unità 02 — Mosse legali

Domanda principale:

Dato uno stato, quali azioni sono consentite?

Focus didattico:

- distinguere azioni possibili da azioni legali;
- comprendere che la legalità dipende dallo stato corrente e dalle regole;
- separare la rappresentazione della mossa dall'applicazione delle regole;
- riconoscere perché una strategia deve scegliere soltanto tra mosse legali.

Perché viene per seconda:

Una mossa può essere giudicata soltanto rispetto a uno stato già definito.

### Unità 03 — Transizioni di stato

Domanda principale:

Cosa significa applicare una mossa legale?

Focus didattico:

- modellare il gameplay come transizioni da uno stato a un altro;
- distinguere la mossa dallo stato risultante;
- ragionare sui cambiamenti deterministici dello stato a livello di modello di
  dominio;
- comprendere perché la ricerca richiede stati successori indipendenti.

Perché viene per terza:

Una volta definiti stati e mosse legali, la dipendenza successiva è
l'operazione che li collega.

### Unità 04 — Stati terminali e risultato

Domanda principale:

Quando è terminato il gioco e cosa significa quello stato?

Focus didattico:

- distinguere stati in corso e stati terminali;
- identificare le condizioni terminali come regole del gioco;
- separare la terminazione dalla valutazione strategica;
- ragionare sui risultati dal punto di vista di un giocatore.

Perché viene per quarta:

La terminazione è una proprietà degli stati raggiunti tramite transizioni.
Introdurla prima richiederebbe concetti non ancora stabiliti.

### Unità 05 — Giocatore e strategia

Domanda principale:

Chi partecipa al gioco e cosa sceglie una mossa?

Focus didattico:

- distinguere l'identità di un giocatore dall'algoritmo che seleziona le azioni;
- comprendere `Strategy` come consumatore del contratto pubblico del gioco;
- impedire che regole specifiche del gioco penetrino negli algoritmi decisionali
  generici;
- ragionare su strategie intercambiabili.

Perché viene per quinta:

Una strategia richiede stati e mosse legali prima che la sua responsabilità
possa essere definita con precisione.

### Unità 06 — Coordinamento della partita e confini delle responsabilità

Domanda principale:

Cosa coordina una partita completa senza possedere regole specifiche del gioco
o logica decisionale?

Focus didattico:

- comprendere la responsabilità di `Match`;
- collegare giocatori, strategie, stati, mosse e terminazione;
- preservare la direzione delle dipendenze tra motore, gioco e strategia;
- riconoscere violazioni dei confini architetturali;
- formare il modello mentale completo della Sessione 01.

Perché viene per ultima:

Il coordinamento della partita compone tutti i concetti precedenti. È il primo
punto in cui il modello astratto completo può essere ragionato come sistema.

## Catena delle dipendenze

La dipendenza concettuale prevista è:

Stato del gioco
→ mosse legali
→ transizione di stato
→ condizione terminale e risultato
→ giocatore e strategia
→ coordinamento della partita

Le unità successive possono riprendere concetti precedenti ma non devono
richiedere conoscenze appartenenti a unità future.

## Schema didattico di ogni unità

Quando appropriato, ogni unità dovrebbe contenere:

- learning objectives;
- riferimenti alle fonti;
- modello mentale intuitivo;
- spiegazione tecnica;
- un esempio concreto originale;
- errori comuni di modellazione;
- scenari di errore o problema;
- takeaway pratico;
- una domanda o un esercizio per il learner.

Gli esercizi non devono includere automaticamente la loro soluzione.

## Implementazione rimandata

La sequenza prepara soltanto il modello concettuale.

Non richiede ancora:

- protocolli Python definitivi o abstract base class;
- implementazione di Tre Sigilli;
- implementazione degli algoritmi di ricerca;
- benchmark.

Queste attività restano fuori dal confine di preparazione della Sessione 01
definito dalla source coverage map.

## Stato della preparazione

Questa study map definisce l'ordine didattico previsto.

Le singole unità non sono ancora state scritte e non è stata completata alcuna
attività del learner.
