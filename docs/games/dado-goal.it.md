# Dado Goal — Regole originali

[English](dado-goal.md) | [Italiano](dado-goal.it.md)

## Origine e paternità

Dado Goal è un gioco originale ideato da Giancarlo Cicellyn Comneno
durante la sua infanzia. L'autore ha conservato i dadi artigianali originali,
documentati fotograficamente.

Questa ricostruzione riporta regole confermate direttamente dall'autore.
La fotografia dei dadi originali non è ancora stata acquisita come materiale
del progetto; questa modifica non include fotografie.

## Identità e ambito

- Dado Goal Originale: ricostruzione fedele del gioco d'infanzia.
- Dado Goal Digitale: adattamento informatico delle regole originali.
- Dado Goal Evolution: nuove meccaniche e varianti dichiarate esplicitamente.

Le innovazioni successive non devono essere presentate come regole storiche
senza conferma dell'autore. Questo documento descrive le regole, non
un'implementazione giocabile.

## Componenti

Due giocatori utilizzano quattro dadi a sei facce. Ogni giocatore possiede
un dado squadra con i colori sociali scelti. Questi due dadi hanno identica
distribuzione delle facce. I giocatori condividono il dado rosa dello sviluppo
e il dado rosso/blu dell'esito.

## Dadi squadra

Ogni dado squadra presenta queste sei facce:

| Faccia | Quantità | Effetto |
|---|---:|---|
| Tiro in porta! | 2 | Lancio del dado rosa |
| Passaggio intercettato! | 2 | Fine mano |
| Cross! | 1 | Lancio del dado rosa |
| Passaggio | 1 | Lo stesso giocatore rilancia il proprio dado squadra |

Le scritte delle facce restano in italiano in entrambe le versioni.
Tiro in porta! e Cross! richiedono entrambi il dado rosa, senza saltarlo.

## Dado rosa dello sviluppo

Ogni risultato elencato occupa una faccia.

| Faccia | Effetto |
|---|---|
| Palla sul fondo | Fine mano |
| Mezza rovesciata | Lancio del dado finale |
| Palla alta sulla traversa | Fine mano |
| Palla intercettata | Fine mano |
| Rovesciata | Lancio del dado finale |
| Colpo di testa | Lancio del dado finale |

## Dado rosso/blu dell'esito

Ogni risultato elencato occupa una faccia.

| Faccia | Effetto |
|---|---|
| Respinto! | Fine mano senza goal |
| GOAL! | Un goal al giocatore di mano e fine mano |
| FUORI! | Fine mano senza goal |
| PALO/TRAVERSA | Fine mano senza goal |
| RIGORE | Lo stesso giocatore rilancia questo dado finale |
| Parato! | Fine mano senza goal |

RIGORE ripetuti comportano ulteriori lanci del dado finale.
Solo GOAL! incrementa il punteggio. Queste regole confermate non prevedono
ulteriori procedure per ribattute o rigori.

## Primo giocatore e alternanza

Storicamente il primo giocatore veniva scelto con il "tocco" napoletano:
i giocatori mostravano un numero di dita, le sommavano e contavano
per stabilire chi iniziasse. La convenzione esatta della conta non è
specificata qui.

Ogni fine mano passa il gioco all'avversario, anche dopo un goal.
Ogni nuova mano inizia dal dado squadra del giocatore interessato.
I rilanci per Passaggio e RIGORE restano nella stessa mano.

## Durata e risultato della partita

I giocatori concordano un numero intero positivo N di mani complete a testa.
Una mano conta anche quando termina prima del dado finale.
I singoli lanci e rilanci non costituiscono mani aggiuntive.

Quando entrambi hanno completato N mani, vince chi ha segnato più goal.
A parità di goal la partita termina in pareggio, senza spareggio.

## Punteggio attuale dei campionati

Il sistema attualmente approvato assegna tre punti per la vittoria,
uno per il pareggio e zero per la sconfitta. Questi punti di classifica
sono distinti dai goal segnati durante una partita.

Si tratta di una scelta attuale di configurazione; il suo utilizzo storico
nel gioco d'infanzia non è stato confermato. Calendari e criteri di spareggio
della classifica non sono definiti da questo documento.

## Confini dello sviluppo digitale

La posizione prevista per la versione browser è accanto a Tre Sigilli,
in `web/dado-goal`. Questo documento non crea tale implementazione.

Le regole originali non prevedono decisioni strategiche durante una mano.
Una simulazione casuale non deve essere presentata come IA strategica.
Le strategie BoardLab non devono essere incorporate automaticamente.
Tre Sigilli e gli altri giochi restano fuori da questa modifica.

Lo sviluppo futuro deve prevedere test riproducibili, preservare quantità
delle facce e transizioni confermate e distinguere le ipotesi del modello
matematico dai fatti storici. In particolare, lanci equi e indipendenti
sarebbero un'ipotesi del modello digitale, non una proprietà misurata
dei dadi artigianali.

## Dettagli di ricostruzione ancora aperti

- Acquisire la fotografia dei dadi originali come riferimento visivo.
- Confermare la conta esatta del tocco, se riprodotta digitalmente.
- Non dedurre l'utilizzo storico dell'attuale punteggio dei campionati.
- Conservare eventuali nuove regole ricordate come integrazioni confermate dall'autore.
