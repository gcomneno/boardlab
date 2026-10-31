# ChronoOops! — Missione 001

## Il ponte che non c'era. Poi c'era. Poi era un'anatra.

- **Progetto:** GiadaWare BoardLab
- **Documento:** Game Design Specification M001
- **Versione:** 1.0
- **Stato:** baseline progettuale; validazione software non eseguita
- **Titolo del gioco:** provvisorio

## 1. Identità

ChronoOops! è un RPG tattico cooperativo a turni, ambientato in una
cronologia unica e modificabile.

Due protagonisti operano in epoche differenti dello stesso mondo.
Le loro decisioni modificano eventi, ambienti e oggetti, producendo
conseguenze causali osservabili.

Il gioco integra strategia, fantascienza umoristica, paradossi logici
e didattica algoritmica.

**Principio guida:** il mondo può diventare assurdo, ma le regole
che lo governano devono essere rigorose.

## 2. Cooperazione

Configurazioni previste:

- Due giocatori umani.
- Un giocatore umano e un compagno controllato dall'IA.

Le regole del mondo sono identiche in entrambe.

- **A — 1985:** modifica decisioni e cause storiche.
- **B — 2025:** osserva conseguenze, ricava informazioni e interviene
  sul proprio presente.

I turni sono alternati: A → B → A → B.

Entrambi perseguono un obiettivo comune. Le differenti possibilità
causali sono parte della strategia cooperativa.

## 3. Leggi del tempo

**L1 — Cronologia unica.** Una sola linea temporale modificabile,
senza universi paralleli.

**L2 — Causalità esplicita.** Gli effetti dipendono da cause
individuabili.

**L3 — Conservazione delle azioni anticipate.** Le azioni prese
in prestito dal futuro generano debiti da restituire.
Questa meccanica non è attiva nella missione pilota.

**L4 — Determinismo.** Uguali condizioni iniziali e decisioni
producono uguali risultati.

**L5 — Coerenza.** Le incompatibilità causali sono trattate
secondo regole esplicite, non con effetti arbitrari.

**L6 — Propagazione immediata.** Alla fine di ogni turno il mondo
viene aggiornato prima che il personaggio successivo agisca.

**L7 — Persistenza geografica.** Il personaggio conserva le proprie
coordinate mentre l'ambiente viene ricostruito.

**L8 — Evoluzione degli oggetti.** Gli oggetti possono conservarsi,
cambiare stato o tecnologia, essere sostituiti funzionalmente
oppure scomparire. L'equivalenza funzionale non è garantita.

## 4. Scenario

**Luogo:** Sottosopra, cittadina immaginaria.

Nel 2025 una capsula scientifica è custodita nella Torre
dell'Orologio, oltre il fiume.

La torre sarà demolita alla scadenza della missione.
Il ponte è stato abbattuto a causa del deterioramento dei supporti.

B deve recuperare la capsula.
A può modificare nel 1985 il progetto del collegamento fluviale.

I protagonisti comunicano attraverso il **Cronofono**.

### Obiettivi

- **Obbligatorio:** recuperare la capsula entro il turno 8.
- **Facoltativo:** ottenere il Registro delle Maree.

## 5. Regole operative

- Ogni turno concede **2 punti azione (PA)**.
- Le azioni elencate nei percorsi costano **1 PA** ciascuna.
- Gli spostamenti tra luoghi logici collegati costano **1 PA**.
- Le interazioni richiedono la presenza nel luogo pertinente.
- La comunicazione tramite Cronofono è gratuita.
- La consultazione della Lente del Tempo è gratuita.
- La comunicazione non modifica direttamente il mondo.
- Le trasformazioni causali vengono applicate alla fine del turno.
- Il limite della missione è di **8 turni complessivi**, non 8 per giocatore.

La mappa è inizialmente astratta; non esiste ancora una griglia
geografica implementata.

## 6. Stato iniziale

**A — 1985:** presso il municipio, adiacente all'archivio.
Dispone dei progetti alternativi ma ignora la causa del cedimento.

**B — 2025:** sulla riva occidentale, presso i resti del ponte.
Può analizzarli, ma non raggiungere direttamente la torre.

**Mondo:** ponte assente; macchina alternativa assente;
capsula conservata nella torre.

B può scoprire il difetto manifestatosi nel futuro.
A può modificare il progetto nel passato.

## 7. Percorso A — Ponte corretto

| Turno | Giocatore | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Raggiunge l'archivio | Consulta il progetto |
| 2 | B | Esamina i resti | Identifica il difetto |
| 3 | A | Seleziona la correzione | Approva il progetto |
| 4 | B | Attraversa il ponte | Raggiunge la torre |
| 5 | A | Verifica il progetto | Nessuna |
| 6 | B | Accede alla torre | Recupera la capsula |

Al turno 2 B comunica gratuitamente il difetto ad A.
Al termine del turno 3 il mondo del 2025 viene aggiornato.

**Risultato nominale:** vittoria al turno 6.

**Vantaggio:** due turni di margine.

**Svantaggio:** il Registro delle Maree non viene acquisito.

**Regola di scenario:** il progetto corretto elimina il difetto
che avrebbe causato la demolizione del ponte.

## 8. Percorso B — Papera a Pendolo

Una macchina fluviale a forma di anatra impiega contrappesi
e un sistema meccanico di propulsione.

| Turno | Giocatore | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Raggiunge l'archivio | Consulta i progetti alternativi |
| 2 | B | Esamina i resti | Rileva le condizioni del fiume |
| 3 | A | Seleziona la Papera | Approva la costruzione |
| 4 | B | Esamina la macchina | Prepara il meccanismo |
| 5 | A | Consulta il manuale storico | Aggiorna la taratura |
| 6 | B | Avvia la macchina | Attraversa il fiume |
| 7 | A | Verifica l'intervento | Nessuna |
| 8 | B | Accede alla torre | Recupera la capsula |

B comunica gratuitamente i dati del fiume ad A.
Il mondo viene aggiornato dopo i turni 3 e 5.

Durante l'attraversamento il registratore di bordo acquisisce
automaticamente il **Registro delle Maree**, senza costi in PA.

**Risultato nominale:** vittoria al turno 8, con obiettivo facoltativo.

**Vantaggio:** acquisizione del Registro delle Maree.

**Svantaggio:** nessun turno residuo.

**Regola di scenario:** la nuova taratura non invalida la
preparazione del meccanismo già compiuta da B.

## 9. Confronto strategico

| Criterio | Ponte corretto | Papera a Pendolo |
|---|---|---|
| Turni | 6 | 8 |
| Capsula | Recuperata | Recuperata |
| Registro delle Maree | No | Sì |
| Turni residui | 2 | 0 |
| Cooperazione | Necessaria | Necessaria |
| Tema didattico | Grafo causale | Pianificazione multi-obiettivo |

Nessuna strategia è universalmente migliore:
il confronto dipende dagli obiettivi.

## 10. Vittoria, sconfitta e blocchi

- **Vittoria:** capsula acquisita entro il turno 8.
- **Vittoria con bonus:** capsula e Registro delle Maree acquisiti.
- **Sconfitta:** capsula non acquisita alla fine del turno 8.
- **Blocco:** un piano può rendere l'obiettivo irraggiungibile;
  il motore dovrà riconoscerlo e spiegarlo.

## 11. Lente del Tempo

La Lente non modifica direttamente lo stato di gioco.

Deve rendere osservabili:

1. Decisione o evento iniziale.
2. Dipendenze causali interessate.
3. Stato del mondo prima e dopo la modifica.
4. Cambiamenti di oggetti, luoghi e azioni disponibili.
5. Vincoli soddisfatti o violati.
6. Confronto tra strategie, quando disponibile.

Modalità:

- **Avventura:** spiegazioni facoltative.
- **Apprendista:** ragionamento guidato.
- **Ricercatore:** confronto di scenari e ipotesi.

Le spiegazioni devono derivare dalle transizioni effettivamente
calcolate dal motore, mai da giustificazioni inventate.

## 12. Modello algoritmico candidato

- **Causal Graph:** dipendenze tra eventi e stati.
- **Timeline Recalculation:** propagazione delle modifiche.
- **State Transition Engine:** turni, posizioni, oggetti e risorse.
- **Constraint Validator:** prerequisiti, PA e condizioni di vittoria.
- **Planning / Search:** ricerca di sequenze alternative.
- **Explainability Layer:** tracce verificabili.
- **Companion AI:** azioni motivate, senza conoscenze privilegiate.

Tecniche candidate: grafi orientati, ordinamento topologico
quando applicabile, propagazione incrementale, ricerca negli stati,
CSP, backtracking, A*, Dijkstra e Utility AI.

Non tutte appartengono alla prima implementazione.

## 13. Contratto di verifica futura

Il motore dovrà controllare:

- Alternanza A/B.
- Massimo 2 PA per turno.
- Informazioni disponibili solo quando acquisite o comunicate.
- Propagazione causale prima del turno successivo.
- Presenza del ponte solo dopo la modifica pertinente.
- Presenza della Papera solo dopo l'approvazione.
- Validità della preparazione dopo la taratura.
- Vittoria nominale dei due percorsi ai turni 6 e 8.
- Acquisizione del Registro alle condizioni previste.
- Riproducibilità deterministica degli stati.
- Corrispondenza fra transizioni e spiegazioni della Lente.

## 14. Esclusioni del pilota

Non sono ancora previsti:

- Combattimento tattico.
- Progressione dei personaggi e inventario completo.
- Debiti di azioni future effettivamente giocabili.
- Mappe spaziali complete.
- Multiplayer di rete.
- IA del compagno implementata.
- Generazione procedurale.

Questa missione è un **vertical slice concettuale della
cooperazione causale**, non l'intero RPG.

## 15. Qualificazione

- **DESIGN_GATE:** PASS — contratto progettuale esplicito.
- **PAPER_FEASIBILITY:** PASS — sequenze nominali di 6 e 8 turni.
- **STRATEGY_BALANCE:** PROVISIONAL.
- **ENGINE_VALIDATION:** NOT_RUN.
- **IMPLEMENTATION:** NOT_STARTED.
- **PUBLICATION:** NOT_STARTED.

La verifica software e il valore concreto del Registro delle Maree
restano attività successive.
