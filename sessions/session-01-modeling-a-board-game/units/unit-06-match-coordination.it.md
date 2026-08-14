# Unità 06 — Coordinamento della partita e confini delle responsabilità

[English](unit-06-match-coordination.md) | [Italiano](unit-06-match-coordination.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- spiegare la responsabilità di `Match`;
- collegare `Game`, `GameState`, `Move`, `Player` e `Strategy` in un unico flusso
  concettuale completo;
- distinguere il coordinamento dalle regole specifiche del gioco;
- distinguere il coordinamento dalla logica di selezione delle mosse;
- spiegare la direzione prevista delle dipendenze tra motore, gioco e strategia;
- riconoscere violazioni dei confini delle responsabilità.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- `Match` coordina turni, strategie, terminazione e risultato;
- il motore non deve importare implementazioni concrete dei giochi;
- le strategie non devono conoscere giochi concreti;
- la CLI può dipendere dal dominio, mentre il dominio non deve dipendere dalla
  CLI;
- le statistiche devono essere risultati espliciti e non variabili globali.

Riferimento di supporto:

- `README.md`

BoardLab richiede che motore, gioco e strategia rimangano separati.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Un coordinatore della partita è un orchestratore.

Non definisce le regole.

Non inventa le mosse legali.

Non decide come ragiona una strategia.

Coordina partecipanti e operazioni di dominio necessari per avanzare dallo
stato iniziale fino a un risultato terminale.

Un modello mentale semplificato è:

`Game`
→ fornisce le regole e lo stato iniziale

`Match`
→ coordina il flusso

`Player`
→ identifica ogni partecipante

`Strategy`
→ seleziona una mossa legale

`Game`
→ applica la mossa e produce lo stato successivo

Il ciclo continua finché il gioco segnala uno stato terminale.

## Spiegazione tecnica

Concettualmente, un `Match` deve coordinare operazioni come:

1. ottenere il `GameState` iniziale;
2. determinare il `Player` attivo;
3. ottenere la strategia associata a quel giocatore;
4. fornire alla strategia le informazioni pubbliche rilevanti del gioco;
5. ricevere una `Move` legale selezionata;
6. chiedere al dominio del gioco di applicare quella mossa;
7. ottenere il `GameState` successore;
8. determinare se il nuovo stato è terminale;
9. ripetere oppure produrre il risultato finale.

Questa sequenza descrive il coordinamento.

Non implica nomi definitivi di metodi o interfacce Python.

Questi rimangono deliberatamente rimandati.

## Confini delle responsabilità

### `Game`

Possiede la semantica specifica del gioco.

Esempi:

- mosse legali;
- transizioni;
- condizioni terminali;
- comportamento di valutazione del dominio;
- stato iniziale.

### `GameState`

Rappresenta una situazione di gioco completa e valida.

### `Move`

Rappresenta un'azione candidata nel vocabolario di un gioco concreto.

### `Player`

Identifica un partecipante.

### `Strategy`

Sceglie tra opzioni legali usando il contratto pubblico del gioco.

### `Match`

Coordina l'interazione tra queste responsabilità.

Non dovrebbe assorbire le responsabilità degli altri concetti.

## Esempio originale

Continuiamo con **Three Stones**.

Una partita concettuale potrebbe procedere così:

1. il gioco crea lo stato iniziale vuoto;
2. North è il giocatore attivo;
3. la strategia di North riceve le mosse legali disponibili;
4. la strategia seleziona lo spazio 2;
5. il gioco applica la mossa;
6. lo stato risultante indica South come giocatore attivo;
7. la partita controlla se lo stato è terminale;
8. se non è terminale viene invocata la strategia di South;
9. il processo continua finché il gioco segnala la terminazione;
10. il risultato finale viene ottenuto dal dominio del gioco.

Il coordinatore della partita non deve sapere perché lo spazio 2 fosse legale.

Non deve sapere come sia rappresentato il tabellone.

Non deve sapere come Random Legal o Future Minimax effettui la scelta.

Questi dettagli rimangono dietro i rispettivi confini.

## Direzione delle dipendenze

L'architettura prevista da BoardLab mantiene i componenti generici dipendenti
dai contratti invece che dalla conoscenza dei giochi concreti.

Concettualmente:

Match
→ contratto Game
→ GameState / Move / Player

Match
→ contratto Strategy

Strategy
→ contratto pubblico del gioco

Gioco concreto
→ contratti generici del motore

Il motore generico non deve importare un gioco concreto soltanto per comprenderne
le regole.

Un gioco concreto può implementare o dipendere dai contratti generici.

La direzione delle dipendenze preserva la sostituibilità.

## Perché il coordinamento deve rimanere sottile

Un coordinatore vede naturalmente molti componenti.

Questo rende particolarmente facile accumulare responsabilità al suo interno.

Per esempio, si potrebbe essere tentati di inserire dentro `Match`:

- validazione del tabellone;
- controlli di legalità specifici del gioco;
- euristiche della strategia;
- regole terminali;
- formattazione UI;
- statistiche globali nascoste.

Ogni aggiunta rende il coordinatore meno generico e più difficile da testare.

La partita dovrebbe coordinare il comportamento del dominio invece di diventare
il dominio.

## Errori comuni di modellazione

### Match possiede regole concrete

Il coordinatore sa che una specifica cella deve essere vuota.

Conseguenza:

il motore generico diventa dipendente da un gioco.

### Match sceglie autonomamente le mosse

Il coordinatore contiene logica casuale, euristica o di ricerca.

Conseguenza:

coordinamento e strategia vengono fusi.

### Strategy applica direttamente le mosse

L'algoritmo selezionato modifica autonomamente lo stato.

Conseguenza:

comportamento decisionale e transizione di dominio vengono fusi.

### Game controlla l'intero loop della partita

Il gioco concreto orchestra direttamente strategie e partecipanti.

Conseguenza:

regole e coordinamento generico diventano accoppiati.

### La CLI diventa parte del dominio

Gli oggetti del gioco dipendono dall'input o output del terminale.

Conseguenza:

il dominio non può più essere riutilizzato indipendentemente
dall'interfaccia.

## Scenario problematico

Viene aggiunto un nuovo gioco a BoardLab.

Per supportarlo è necessario modificare il coordinatore generico `Match` perché
contiene assunzioni su come funziona il tabellone del gioco precedente.

Questa necessità è un segnale di allarme.

Aggiungere un nuovo gioco concreto dovrebbe richiedere principalmente
l'implementazione del contratto del gioco, non insegnare nuove regole al
coordinatore generico.

## Modello mentale completo della Sessione 01

Il modello della Sessione 01 può ora essere riassunto così:

`Game`
→ definisce il comportamento specifico del gioco

`GameState`
→ rappresenta una situazione completa

`Move`
→ rappresenta un'azione candidata

`Player`
→ identifica un partecipante

`Strategy`
→ seleziona tra le scelte legali

`Match`
→ coordina l'avanzamento della partita

Insieme supportano il flusso concettuale:

stato corrente
→ mosse legali
→ selezione della strategia
→ transizione di stato
→ controllo terminale
→ turno successivo o risultato

Questa è la base concettuale sulla quale potranno essere costruiti la futura
implementazione BoardLab e gli algoritmi di ricerca.

## Takeaway

Un `Match` generico coordina un gioco senza possederne le regole o gli algoritmi
decisionali.

Confini chiari permettono a BoardLab di aggiungere nuovi giochi e nuove
strategie indipendentemente.

Il valore dell'architettura deriva non soltanto dai singoli concetti, ma anche
dal mantenimento della corretta direzione delle loro dipendenze.

## Esercizio per il learner

Usando il tuo gioco originale e le strategie inventate negli esercizi
precedenti:

1. descrivi un turno completo dallo stato corrente allo stato successore;
2. identifica quale responsabilità appartiene a `Game`;
3. identifica quale responsabilità appartiene a `GameState`;
4. identifica quale responsabilità appartiene a `Move`;
5. identifica quale responsabilità appartiene a `Player`;
6. identifica quale responsabilità appartiene a `Strategy`;
7. identifica quale responsabilità appartiene a `Match`;
8. fornisci un esempio di responsabilità che renderebbe `Match` troppo specifico
   rispetto a un gioco.

Non progettare ancora le interfacce Python definitive.
