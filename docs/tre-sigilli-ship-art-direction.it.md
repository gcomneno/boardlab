# Tre Sigilli — SHIP-02 Gaming UX & Art Direction

Languages: [English](tre-sigilli-ship-art-direction.md) | [Italiano](tre-sigilli-ship-art-direction.it.md)

Stato: baseline visiva Free approvata; implementazione e QA da validare

## Obiettivo

Definire una identità visiva originale, riconoscibile,
accessibile e coerente con la strategia Free/Premium.

## Direzioni candidate

### A — Sigilli Imperiali
Blu notte, oro satinato, geometrie incise.
Eleganza e atmosfera da gioco da tavolo da collezione.

### B — Geometria Essenziale
Avorio, grafite, terracotta.
Massima leggibilità, forme semplici, interazioni pulite.

### C — Nexus
Grafite, turchese, viola.
Atmosfera tecnologica e laboratorio strategico.

## Principi Gaming UX

- Sigilli distinguibili anche senza colore.
- Feedback chiaro per selezione e mosse legali.
- Territori controllati, contesi e neutrali leggibili.
- Conteggio dell'influenza esplorabile per singola casella.
- Tutorial accessibile prima e durante la partita.
- Spiegazione visiva del risultato finale.
- Contrasti e bersagli touch da verificare su Android.
- Animazioni discrete, senza rallentare le mosse.

## Free/Premium

Free: identità visiva completa, sigilli originali e tema base.
Premium: temi e collezioni alternative senza vantaggi di gioco.

## Strategia delle skin

- Geometria Essenziale: skin Free predefinita.
- Sigilli Imperiali: skin Premium candidata.
- Nexus: skin Premium candidata.
- Le skin modificano soltanto la presentazione.
- Regole, punteggi e AI restano invariati.
- Ogni skin deve rispettare accessibilità e leggibilità.
- Lo sblocco Premium resta subordinato alla validazione commerciale.


## Baseline UX Free — proposta da validare

### Identità grafica

- Tema predefinito: Geometria Essenziale.
- Palette candidata: avorio #F5F0E6, grafite #33434B,
  terracotta #A65E49, sabbia #D7C7AD.
- Sigillo Nord: stella geometrica a quattro punte.
- Sigillo Sud: esagono con centro circolare.
- Simboli leggibili indipendentemente dal colore.
- Contrasti da verificare prima dell'implementazione.

### Interazione

- Modalità Gioca: selezione e mosse legali.
- Modalità Esplora territori: nessuna mossa eseguita;
  selezione della casella e conteggio influenza Nord/Sud.
- Fine partita: territorio di ciascun giocatore visibile,
  con spiegazione del risultato.
- Tutorial accessibile senza abbandonare la partita.
- Comandi comprensibili e utilizzabili su schermi piccoli.

### Confini tecnici

- Motore e regole canoniche invariati.
- Presentazione indipendente dalla skin selezionata.
- Nessuna logica di scoring duplicata nell'interfaccia.
- Modalità di esplorazione separata dalle azioni di gioco.
- Persistenza e ripristino dello stato da verificare.

Le scelte grafiche restano candidate fino all'approvazione
del mockup e alla validazione di leggibilità.

## Baseline visiva approvata — SHIP-02

### Identità

- Geometria Essenziale è la skin Free predefinita.
- Palette: avorio #F5F0E6, grafite #33434B,
  terracotta #A65E49, sabbia #D7C7AD.
- Nord: sigillo circolare grafite con stella a quattro punte.
- Sud: sigillo circolare terracotta con esagono e punto centrale.
- I simboli restano distinguibili senza il colore.
- Sigilli Imperiali e Nexus sono candidati Premium separati.

### Layout di gioco

- Titolo Tre Sigilli e identità BoardLab/GiadaWare.
- Punteggi territoriali per entrambi i giocatori.
- Turno, fase e istruzioni contestuali.
- Scacchiera classica 4×4 con bersagli touch adeguati.
- Modalità Gioca e Mostra territori distinguibili.
- Pannello di dettaglio della casella selezionata.
- Conteggi di influenza Nord/Sud e spiegazione del punto.

### Contratto di interazione

- In modalità Gioca il tap conserva la semantica canonica.
- In modalità Esplora il tap non modifica lo stato di gioco.
- I dati territoriali derivano dal dominio, senza duplicare
  le regole di influenza nell'interfaccia.
- L'esplorazione non altera il turno né avvia mosse AI.
- Il risultato finale resta spiegabile.
- Il ripristino di partita e modalità richiede test dedicati.

### Criteri di accettazione implementativa

- Nessuna modifica alle regole Tre Sigilli v0.2.
- Parità Python/Kotlin preservata.
- Nessuna regressione delle modalità di gioco esistenti.
- Segni, colori e stati riconoscibili e accessibili.
- Leggibilità su schermi Android piccoli.
- Test di selezione, esplorazione e ripristino.
- Nessuna dipendenza Premium per la UX fondamentale.
- Nessuna integrazione di pagamenti in SHIP-02.

## Prossimo gate

Validare Geometria Essenziale come tema Free predefinito.
Solo successivamente progettare asset finali e modifiche Compose.

Nessun asset, prezzo o rilascio è approvato qui.
