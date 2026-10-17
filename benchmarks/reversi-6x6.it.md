# Reversi 6x6 — Protocollo sperimentale dei benchmark

[English](reversi-6x6.md) | [Italiano](reversi-6x6.it.md)

## Obiettivo

Confrontare Random, Minimax, Alpha-Beta e MCTS su Reversi 6x6
attraverso partite riproducibili e budget computazionali dichiarati.

Questo documento definisce il protocollo sperimentale. Non presenta
risultati sperimentali.

## Configurazione pilota

| Parametro | Valore |
| --- | --- |
| Gioco | Reversi 6x6 |
| Baseline | Random |
| Profondità Minimax | 2 |
| Profondità Alpha-Beta | 2 |
| Simulazioni MCTS per decisione | 32 |
| Seed | 0, 1, 2, 3, 4 |
| Ruoli | Entrambi i colori, invertiti per ogni seed |
| Valutazione della ricerca deterministica | Differenza dei dischi |
| Esplorazione MCTS | Valore predefinito della strategia |
| Generatori casuali | Seed espliciti |

I valori sono parametri pilota, non configurazioni ottimizzate.
Profondità di ricerca e numero di simulazioni non rappresentano
budget computazionali equivalenti.

## Disegno delle partite

Ogni strategia di ricerca affronta la baseline Random con entrambi
i colori per ciascun seed. Il pilota comprende quindi 10 partite per
strategia e 30 partite complessive.

Usare generatori casuali indipendenti per ciascun partecipante
stocastico. Registrare configurazione, seed, colore assegnato ed
esito. Applicare lo stesso protocollo a tutti i confronti.

Un torneo completo fra tutte le strategie è escluso dal pilota.

## Misure

Registrare vittorie, sconfitte, pareggi e numero di azioni per partita.
Misurare il tempo trascorso separatamente dagli esiti deterministici.

Per Minimax e Alpha-Beta raccogliere le statistiche disponibili,
compresi i nodi visitati. Il numero di simulazioni MCTS rappresenta
un budget configurato e non equivale ai nodi visitati.

Presentare gli aggregati insieme alle osservazioni delle singole
partite. Non considerare i tempi grezzi riproducibili fra macchine.

## Riproducibilità

Sono obbligatori seed fissi, parametri dichiarati, inversione dei
ruoli e registrazione della revisione del codice.

Esecuzioni identiche devono produrre gli stessi esiti e numeri di
azioni con la stessa configurazione software. Il tempo trascorso
è escluso dalle verifiche di uguaglianza esatta.

## Criteri di accettazione del pilota

- Tutte le partite terminano con esiti validi.
- Entrambi i ruoli sono esercitati per ogni seed e strategia.
- Configurazioni identiche riproducono esiti e numeri di azioni.
- Gli aggregati corrispondono alle osservazioni individuali.
- Il tempo di esecuzione resta praticabile localmente.
- La QA del repository passa senza introdurre esperimenti lunghi
  nella suite di test predefinita.

## Confini

Il motore generico e le strategie esistenti rimangono invariati.
Non sono previste nuove dipendenze runtime.

Implementazione e calibrazione dell'harness appartengono alle
milestone successive. Il documento non dichiara risultati né
superiorità comparative.
