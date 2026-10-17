# Tre Sigilli — Specifica del gioco v0.2

[English](tre-sigilli.md) | [Italiano](tre-sigilli.it.md)

## Stato e ambito

Regole approvate per il primo gioco originale di BoardLab. Questo documento definisce comportamento e criteri di accettazione, non un'implementazione. L'equilibrio strategico non è ancora dimostrato.

## Tabellone e partecipanti

Il tabellone è una griglia quattro per quattro di sedici celle. Due partecipanti, NORTH e SOUTH, controllano tre sigilli ciascuno. NORTH inizia. I turni si alternano e un turno corrisponde a un'azione completata. Non esiste casualità.

Le celle sono numerate da zero a quindici per righe. Due celle sono ortogonalmente adiacenti se condividono un lato; le diagonali non contano.

## Fasi e azioni

### Piazzamento

I turni da zero a cinque sono turni di piazzamento. Il partecipante attivo colloca uno dei propri sigilli non ancora piazzati in una cella libera. NORTH agisce nei turni pari e SOUTH nei turni dispari.

Dopo sei turni completati, ciascun partecipante possiede esattamente tre sigilli sul tabellone.

### Manovra

I turni da sei a diciassette sono turni di manovra. Il partecipante attivo sposta esattamente uno dei propri sigilli in una cella libera ortogonalmente adiacente. È illegale spostare un sigillo avversario, muoversi diagonalmente, occupare una cella già occupata o passare.

Dopo la terminazione non è consentita alcuna azione.

## Influenza e territorio

Ogni sigillo influenza la cella occupata e tutte le celle ortogonalmente adiacenti esistenti.

Per ogni cella si confronta il numero di sigilli NORTH influenti con quello dei sigilli SOUTH influenti.

Una cella appartiene al partecipante con influenza strettamente maggiore e contribuisce con un punto al suo punteggio territoriale.

Un'influenza positiva e uguale rende la cella contesa. L'assenza di influenza da entrambi i partecipanti la rende neutrale. Le celle contese e neutrali valgono entrambe zero punti.

I punteggi vengono ricalcolati dalla configurazione corrente, senza accumulo fra turni.

## Terminazione ed esito

La partita termina dopo diciotto turni completati oppure quando il partecipante attivo non dispone di mosse legali di manovra dopo la conclusione del piazzamento.

Un partecipante bloccato non perde automaticamente. Alla terminazione vince chi ha il punteggio territoriale maggiore. Punteggi uguali producono un pareggio.

L'esito è espresso dal punto di vista del partecipante richiesto come vittoria, sconfitta o pareggio. Non è possibile richiedere l'esito di una partita in corso.

## Contratto dello stato e delle transizioni

Uno stato completo contiene l'occupazione immutabile del tabellone e il numero di turni completati. Il partecipante attivo si ricava dalla parità del turno negli stati non terminali.

Le azioni di piazzamento e di manovra sono tipi di mossa distinti. Una transizione valida restituisce uno stato successore indipendente senza modificare quello originale.

Stati non validi e azioni illegali vengono rifiutati. Una transizione rifiutata non produce effetti osservabili sullo stato originale.

La validazione strutturale richiede sedici celle valide, proprietà disgiunte, un numero intero di turni fra zero e diciotto e quantità di sigilli coerenti con la fase di piazzamento e con quella successiva di manovra.

La validità strutturale non implica che ogni configurazione accettata sia raggiungibile attraverso una storia di gioco legale.

## Criteri di accettazione

- Il tabellone iniziale è vuoto e NORTH è il primo partecipante.
- Il piazzamento alterna i partecipanti per sei turni e non sovrascrive celle occupate.
- La manovra conserva esattamente tre sigilli per partecipante e richiede una destinazione libera ortogonalmente adiacente.
- Le azioni legali sono deterministiche e non modificano lo stato.
- La terminazione viene rilevata al limite di diciotto turni oppure quando il partecipante attivo non può manovrare.
- Il partecipante bloccato viene valutato dal punteggio territoriale, senza sconfitta automatica.
- Il territorio distingue celle controllate, contese e neutrali.
- Stati non validi, azioni illegali e richieste premature dell'esito vengono rifiutati.
- Il gioco concreto rispetta il contratto generico esistente senza modificare il motore.

## Evidenze e limiti

Audit read-only in memoria hanno dimostrato configurazioni bloccate raggiungibili dopo sei, sette e otto turni completati, rispettando l'ordine dei partecipanti.

Sono state verificate geometricamente tutte le 160.160 configurazioni disgiunte con tre sigilli per partecipante. Quattro bloccano NORTH, quattro bloccano SOUTH e nessuna blocca entrambi contemporaneamente.

La distribuzione geometrica dei punteggi dopo il piazzamento è simmetrica fra i partecipanti. Questo non dimostra equilibrio strategico, vantaggio del primo giocatore o frequenza degli esiti nelle partite effettive.

La versione 0.2 non prevede regole sulle ripetizioni, passaggi volontari o bonus speciali per immobilizzazione.
