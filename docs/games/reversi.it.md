# Reversi 6x6 — Specifica del gioco v1.0

[English](reversi.md) | [Italiano](reversi.it.md)

## Stato e ambito

Questo documento descrive le regole Reversi 6x6 implementate in BoardLab e i relativi criteri di accettazione. Documenta il gioco concreto, non un nuovo motore generico né una dimostrazione di equilibrio strategico.

## Tabellone e partecipanti

Il tabellone è una griglia sei per sei di 36 celle, numerate da zero a 35 per righe. Ogni cella è vuota oppure occupata da una pedina BLACK o WHITE.

BLACK inizia. La configurazione iniziale contiene pedine WHITE agli indici 14 e 21 e pedine BLACK agli indici 15 e 20. Le regole non prevedono casualità.

## Azioni e catture

Un piazzamento usa `Place(cell)` su una cella vuota. È legale soltanto se almeno una linea retta nelle otto direzioni contiene una o più pedine avversarie consecutive seguite da una pedina del partecipante attivo.

Tutte le pedine avversarie così delimitate vengono capovolte. Le linee non possono attraversare i bordi del tabellone. I piazzamenti legali sono restituiti in ordine crescente di indice.

Quando il partecipante attivo non può piazzare ma l'avversario può farlo, l'unica azione legale è `Pass()`. Il passaggio conserva il tabellone e trasferisce il turno. Passare è vietato quando esiste un piazzamento.

## Terminazione ed esito

La partita termina quando nessuno dei partecipanti dispone di un piazzamento legale, anche se rimangono celle vuote.

Vince chi possiede più pedine. Quantità uguali producono un pareggio. Gli esiti sono espressi dal punto di vista del partecipante richiesto come vittoria, sconfitta o pareggio.

Non è possibile richiedere un esito prima della terminazione. Negli stati terminali `current_player` non restituisce un partecipante attivo e non sono consentite ulteriori azioni.

## Contratto dello stato e delle transizioni

Uno stato contiene una tupla immutabile di 36 celle e un partecipante attivo esplicito. `Place` e `Pass` sono tipi di azione immutabili e distinti.

Un'azione valida produce un nuovo stato senza modificare quello precedente. Stati malformati e azioni illegali vengono rifiutati.

La validazione strutturale controlla forma del tabellone, valori delle celle e partecipante attivo. Non dimostra che ogni posizione strutturalmente valida sia raggiungibile attraverso una storia legale.

Il gioco implementa il contratto generico `Game` esistente e funziona con `Match`, `RandomStrategy`, `MinimaxStrategy`, `AlphaBetaStrategy` e `MCTSStrategy` senza modifiche al motore generico.

## Criteri di accettazione

- L'apertura standard contiene le quattro pedine previste e BLACK è il primo partecipante.
- Piazzamenti e capovolgimenti legali sono deterministici e rispettano le otto direzioni e i bordi.
- I piazzamenti senza catture e le mosse su celle occupate vengono rifiutati.
- Il passaggio esplicito è obbligatorio solo quando l'avversario può piazzare e il partecipante attivo non può.
- La terminazione avviene quando nessuno può piazzare, anche con tabellone non pieno.
- Gli esiti terminali dipendono dal numero di pedine e comprendono vittorie, sconfitte e pareggi.
- Stati invalidi, mosse illegali e richieste premature dell'esito vengono rifiutati senza modificare l'input.
- Il gioco concreto è compatibile con il coordinamento delle partite e le strategie di ricerca generiche esistenti.

## Evidenze e limiti

I test automatici coprono modello a valori, azioni legali, catture avversariali, esiti terminali, partite complete con seed, passaggi forzati e compatibilità con le strategie generiche.

Sono verificate la riproducibilità di Monte Carlo Tree Search con seed e la concordanza fra Minimax e Alpha-Beta a determinate profondità di ricerca. Queste verifiche non dimostrano ottimalità strategica, equità o prestazioni con budget di ricerca maggiori.

Questa specifica non include interfaccia grafica, cronologia persistente delle partite o varianti delle regole.
