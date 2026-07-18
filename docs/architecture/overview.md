# Panoramica architetturale

## Obiettivo

BoardLab separa il dominio dei giochi dagli algoritmi che scelgono le mosse.

Il motore coordina una partita, ma non contiene regole appartenenti a uno
specifico gioco.

## Responsabilità principali

### Game

Definisce le regole e crea lo stato iniziale di un gioco.

### GameState

Rappresenta una configurazione completa e valida della partita.

### Move

Descrive una possibile azione senza imporre una rappresentazione comune a
tutti i giochi.

### Player

Identifica uno dei partecipanti alla partita.

### Strategy

Sceglie una mossa tra quelle legali usando esclusivamente il contratto pubblico
del gioco.

### Match

Coordina turni, strategie, terminazione e risultato della partita.

## Dipendenze

```text
Match
  |
  +-- Game
  |     |
  |     +-- GameState
  |     +-- Move
  |     +-- Player
  |
  +-- Strategy
        |
        +-- statistiche di ricerca
```

## Regole architetturali

1. Il motore non importa implementazioni concrete dei giochi.
2. Le strategie non conoscono Tre Sigilli o altri giochi specifici.
3. Uno stato non viene modificato clandestinamente durante una ricerca.
4. Le statistiche sono risultati espliciti, non variabili globali.
5. Ottimizzazioni e cache devono preservare il comportamento osservabile.
6. La CLI utilizza il dominio, ma il dominio non dipende dalla CLI.

## Contratto concettuale minimo

Un gioco dovrà permettere almeno di:

- ottenere le mosse legali;
- applicare una mossa;
- verificare se lo stato è terminale;
- valutare uno stato dal punto di vista di un giocatore;
- produrre uno stato indipendente utilizzabile durante la ricerca.

La forma definitiva delle interfacce verrà decisa prima di implementare gli
algoritmi.
