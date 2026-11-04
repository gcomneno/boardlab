# ChronoOops! — Un laboratorio deterministico C11 sui viaggi nel tempo

[English](README.md) | [Italiano](README.it.md)

ChronoOops! è un gioco sperimentale cooperativo a turni di
GiadaWare BoardLab.

Due personaggi vivono in anni differenti della stessa linea temporale modificabile:

- **A (1985)** indaga sulle decisioni storiche e le modifica.
- **B (2025)** ne osserva le conseguenze e interagisce con il
  mondo risultante.

Gli eventi immaginari del gioco possono essere assurdi. Le sue regole
computazionali devono rimanere esplicite, deterministiche e verificabili.

Questa directory contiene una campagna CLI giocabile di cinque missioni,
basata su motori deterministici C11. Le sezioni didattiche seguenti
approfondiscono in particolare il motore della Missione 001.
Non è presente un'interfaccia grafica.

## Campagna CLI — Missioni 001–005

Le cinque missioni sono disponibili nella campagna da terminale.
Le specifiche progettuali sono consultabili qui:

- [Missione 001 — Il ponte che non c'era. Poi c'era. Poi era un'anatra.](../../docs/games/chrono-oops/mission-001.it.md)
- [Missione 002 — Il prestito di domani](../../docs/games/chrono-oops/mission-002.it.md)
- [Missione 003 — Il messaggio che non doveva arrivare](../../docs/games/chrono-oops/mission-003.it.md)
- [Missione 004 — Due eventi, una sola causa](../../docs/games/chrono-oops/mission-004.it.md)
- [Missione 005 — Il minuto che non è mai esistito](../../docs/games/chrono-oops/mission-005.it.md)

Le sezioni numerate di questo README restano dedicate soprattutto
all'analisi didattica della Missione 001.

## 1. Obiettivi didattici

Studiando questo esempio puoi imparare a:

1. Rappresentare un mondo di gioco tramite strutture ed enumerazioni C.
2. Modellare le azioni come transizioni di stato validate.
3. Imporre l'ordine dei turni, i punti azione e i prerequisiti.
4. Separare gli effetti locali immediati da quelli storici differiti.
5. Ricostruire lo stato futuro dalle decisioni storiche approvate.
6. Osservare le transizioni causali tramite istantanee prima/dopo.
7. Verificare il comportamento deterministico e le operazioni rifiutate.
8. Distinguere un test riuscito da una prova universale di correttezza.

Non sono necessarie librerie esterne né un runtime Python.

## 2. File del progetto

| File | Responsabilità |
| --- | --- |
| `chrono.h` | Tipi pubblici, strutture di stato e dichiarazioni delle API |
| `chrono.c` | Validazione delle azioni, transizioni di stato e propagazione causale |
| `test_chrono.c` | Scenari eseguibili, test di regressione e invarianti |

La specifica progettuale autorevole è:

`../../docs/games/chrono-oops/mission-001.md`

Questo percorso è relativo alla directory che contiene il README.

La specifica descrive le regole di gioco previste. I test descrivono
il comportamento attualmente verificato dell'implementazione.

## 3. Compilazione ed esecuzione

Dalla directory principale del repository BoardLab:

```sh
cd src/chrono_oops

gcc -std=c11 -O0 -g \
    -Wall -Wextra -Wpedantic -Werror \
    chrono.c test_chrono.c -o test_chrono

./test_chrono
```

L'eseguibile è un artefatto della compilazione locale. Non è necessario
per comprendere o modificare i file sorgenti.

Per evitare di collocare binari nella directory dei sorgenti, puoi
compilare invece in una directory temporanea a tua scelta.

Opzioni di compilazione:

- `-std=c11`: utilizza lo standard del linguaggio ISO C11.
- `-O0`: disabilita le ottimizzazioni durante il debugging introduttivo.
- `-g`: include le informazioni di debug.
- `-Wall -Wextra -Wpedantic`: richiede diagnostiche aggiuntive.
- `-Werror`: rifiuta la compilazione in presenza di warning.

I test utilizzano `assert()`. Non compilarli con `-DNDEBUG`,
che ne disabilita le asserzioni.

## 4. Il modello delle transizioni di stato

Il motore espone quattro funzioni principali:

```c
void chrono_init(ChronoState *state);

ChronoResult chrono_apply(
    ChronoState *state,
    ChronoAction action
);

ChronoResult chrono_end_turn(
    ChronoState *state,
    ChronoTrace *trace
);

bool chrono_is_victory(const ChronoState *state);
```

Una simulazione tipica segue questa sequenza:

```text
INITIAL STATE
     |
     v
VALIDATE ACTION
     |
     +---- invalid ----> REJECT WITHOUT STATE CHANGE
     |
     v
APPLY LOCAL OR HISTORICAL DECISION
     |
     v
END TURN
     |
     v
PROPAGATE HISTORICAL CONSEQUENCES
     |
     v
CHECK VICTORY / DEADLINE
     |
     v
NEXT PLAYER (A <-> B)
```

Un turno offre due punti azione. I giocatori si alternano A, B, A, B.
La Missione 001 prevede otto turni complessivi, non otto per giocatore.

Ogni azione elencata costa un punto azione. La comunicazione tramite
Cronofono e la consultazione della Lente del Tempo sono operazioni
gratuite a livello progettuale; un'implementazione generale di queste
interfacce non fa parte del prototipo C11.

## 5. Perché le azioni rifiutate sono atomiche

`chrono_apply()` valida un'azione lavorando su una copia locale
dello stato di gioco.

Soltanto un'azione riuscita sostituisce lo stato originale.

Di conseguenza, un'azione non valida non deve consumare punti azione,
spostare un personaggio, modificare oggetti o alterare il progresso
della missione.

Il comportamento ricorda una piccola transazione:

```text
COPY -> VALIDATE -> MODIFY COPY -> COMMIT
                       |
                       +-- failure -> DISCARD COPY
```

Leggi `test_invalid_action_atomicity()` per alcuni esempi eseguibili.

## 6. Due tipi di conseguenze

### Azioni locali nel 2025

B può esaminare o preparare la macchina Papera, attraversare il fiume,
entrare nella torre o recuperare la capsula.

Queste azioni aggiornano direttamente lo stato di gioco pertinente.

### Decisioni storiche nel 1985

A può approvare un progetto corretto per il ponte oppure autorizzare
la costruzione della macchina Papera.

L'approvazione di un progetto non ricostruisce immediatamente il mondo
futuro. La modifica diventa visibile quando `chrono_end_turn()`
propaga la decisione storica.

La taratura della Papera segue lo stesso principio:

1. A legge il manuale storico.
2. A registra una decisione di taratura.
3. `calibration_pending` diventa true.
4. Il turno corrente termina.
5. La macchina nel futuro risulta tarata.
6. Il flag di attesa viene azzerato.

La taratura deve conservare la preparazione già completata da B
nel 2025, come richiesto dalla Missione 001.

## 7. Osservare la causalità

`ChronoTrace` registra:

- Lo stato del mondo prima della propagazione.
- Lo stato del mondo dopo la propagazione.
- Un'azione causale principale.
- Il turno in cui è avvenuta la propagazione.
- Se i campi registrati del mondo sono cambiati.

Per esempio:

```text
TURN 3
Cause: APPROVE_BRIDGE
Before: bridge_exists = false
After:  bridge_exists = true
Changed: true
```

Questa è una rappresentazione illustrativa dei valori disponibili
tramite la struttura delle tracce, non un formato di logging integrato.

La traccia attuale è volutamente limitata. Non rappresenta
un grafo completo delle dipendenze e non registra separatamente
ogni azione locale eseguita durante il turno.

Le modifiche al mondo vengono confrontate campo per campo, anziché
utilizzare `memcmp()` sulle strutture C. Il padding delle strutture
non costituisce dato di gioco e non deve influenzare l'uguaglianza
semantica.

## 8. Le due strategie della Missione 001

### Strategia A — Ponte riparato

A recupera i progetti storici, B identifica il difetto del ponte
e A approva la correzione.

Il ponte riparato diventa disponibile dopo il turno 3.

B raggiunge la torre e recupera la capsula al turno 6.

Risultato previsto:

- Vittoria della missione al turno 6.
- Due turni rimanenti.
- Nessun Registro delle Maree.

### Strategia B — Papera a Pendolo

A approva un'improbabile macchina per attraversare il fiume.

B la prepara, A corregge la sua taratura storica e B
attraversa successivamente il fiume.

Il registratore di bordo acquisisce automaticamente il Registro delle Maree.

Risultato previsto:

- Vittoria della missione al turno 8.
- Registro delle Maree acquisito.
- Nessun turno rimanente.

Nessuna strategia è universalmente migliore: una privilegia
la rapidità, mentre l'altra completa l'obiettivo facoltativo.

## 9. Leggere i test

Ordine di studio consigliato:

1. `run_route()` — primi tre turni di entrambe le strategie.
2. `test_complete_bridge_mission()` — scenario completo del ponte.
3. `test_complete_duck_mission()` — scenario completo della Papera.
4. `test_historical_trace()` — evidenza causale prima/dopo.
5. `test_invalid_action_atomicity()` — transizioni rifiutate.
6. `test_deterministic_replay()` — input uguali, risultati uguali.
7. `test_mission_deadline()` — scadenza degli otto turni.

La suite ha dimostrato questi comportamenti rappresentativi.
Non ha esplorato esaustivamente ogni possibile sequenza di azioni
né dimostrato formalmente la correttezza di tutti i possibili
stati di gioco.

## 10. Esperimenti per lo studente

Prova questi esercizi in una copia di lavoro o in un branch separato:

**Esercizio A — Contabilità dei punti azione**

Tenta una terza azione nello stesso turno. Prevedi il risultato
restituito e verifica che lo stato rimanga invariato.

**Esercizio B — Informazione storica mancante**

Prova a selezionare il ponte riparato prima che B identifichi
il difetto. Spiega quale prerequisito impedisce l'azione.

**Esercizio C — Propagazione differita**

Osserva `bridge_exists` immediatamente prima e immediatamente
dopo la conclusione del turno 3. Spiega perché i valori differiscono.

**Esercizio D — Riproduzione deterministica**

Esegui sequenze identiche a partire da due stati inizializzati
indipendentemente. Confronta i loro campi significativi
e le rispettive tracce causali.

**Esercizio E — Estensione della Lente del Tempo**

Progetta un'ulteriore rappresentazione delle tracce che registri
le azioni locali senza inventare cause né alterare il risultato.

Conserva i test esistenti durante gli esperimenti.

## 11. Limiti attuali

Implementato e verificato:

- Transizioni di stato deterministiche in C11.
- Alternanza dei turni e limiti dei punti azione.
- Validazione e rifiuto delle azioni.
- Creazione storica del ponte e della Papera.
- Taratura storica differita della Papera.
- Recupero della capsula e acquisizione del Registro delle Maree.
- Vittoria e scadenza della missione.
- Test rappresentativi delle tracce causali e della riproduzione.

Non implementato in questo prototipo:

- Interfaccia grafica o sistemi RPG completi.
- Multiplayer di rete.
- Compagno cooperativo controllato dall'IA.
- Inventario generale o griglia geografica.
- Meccaniche di debito delle azioni future.
- Pianificazione o ricerca automatica.
- Grafo completo delle dipendenze causali.
- Verifica esaustiva dello spazio degli stati.

## 12. Principio ingegneristico

L'implementazione deve essere comprensibile senza richiedere
spiegazioni da parte dell'autore originale.

Preferisci regole esplicite, nomi precisi, funzioni piccole,
test mirati e commenti che spieghino le decisioni progettuali.

Un gioco può essere assurdo. La sua implementazione deve essere spiegabile.
