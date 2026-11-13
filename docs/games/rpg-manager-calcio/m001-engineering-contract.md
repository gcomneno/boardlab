# RPG Manager Calcio — M001 Engineering Contract v1.0

**Progetto:** GiadaWare BoardLab / RPG Manager Calcio  
**Milestone:** M001 — First Playable Season  
**Data:** 13 novembre 2026  
**Baseline funzionale:** Design Freeze 1.0, CONCEPT 001–010  
**Stato:** Specificazione completa candidata all'approvazione  
**Implementazione:** ISO C11, CLI testuale, target iniziale Linux  
**Regola di modifica:** ogni cambiamento normativo richiede una revisione esplicita del contratto.

---

# 1. Scopo e delimitazione

## 1.1 Obiettivo

M001 deve produrre un gioco CLI con cui sia possibile:

1. Generare un campionato procedurale.
2. Scegliere una fra tre squadre proposte.
3. Gestire una rosa e la relativa preparazione.
4. Selezionare formazione e tattica.
5. Disputare partite simulate per possessi e fasi.
6. Intervenire attraverso decisioni situazionali RPG.
7. Affrontare allenatori automatici dotati di personalità.
8. Sviluppare calciatori mediante esperienza, allenamenti e talenti.
9. Completare una stagione da 14 giornate.
10. Salvare e riprendere la carriera.
11. Consultare classifica, statistiche e rapporto finale.

Le partite delle squadre automatiche devono utilizzare il medesimo motore di quelle del giocatore.

## 1.2 Ambito M001

| Componente | Requisito |
|---|---|
| Squadre | 8 |
| Calciatori | 18 per squadra, 144 complessivi |
| Allenatori | 8 |
| Scelta iniziale | 3 proposte procedurali |
| Calendario | Doppio girone, 14 giornate |
| Partite | 56 |
| Moduli | 4-4-2, 4-3-3, 3-5-2 |
| Campo | 15 zone tattiche |
| Giocatori di movimento | 16 attributi |
| Portieri | 4 attributi specialistici aggiuntivi |
| Progressione RPG | Livelli 1–20 |
| Talenti iniziali | 8 |
| Controllo | Automatico e situazionale |
| Salvataggi | Ai confini gestionali |
| Fine carriera M001 | Rapporto conclusivo della prima stagione |

## 1.3 Esclusioni normative

Non fanno parte di M001:

- Mercato e contratti.
- Finanze e infrastrutture societarie.
- Carriera pluriennale.
- Campionati multipli e coppe.
- Supplementari, rigori decisivi e VAR.
- Interfacce grafiche.
- Multiplayer.
- Machine learning.
- Salvataggio durante un possesso.
- Simulazione fisica continua dei movimenti.

Nessuna esclusione può impedire di completare una stagione giocabile.

---

# 2. Principi architetturali

## 2.1 Fonte di verità

Lo stato autorevole appartiene esclusivamente al core di gioco.

CLI, cronaca, statistiche derivate e rapporti non possono modificare direttamente lo stato.

Ogni operazione avviene attraverso un comando validato.

## 2.2 Componenti logici

| Componente | Responsabilità |
|---|---|
| `world` | Generazione di squadre, giocatori e allenatori |
| `roster` | Attributi, ruoli e formazioni |
| `match` | Stati, possessi, azioni e regole calcistiche |
| `decision` | Validazione delle istruzioni |
| `ai` | Scelta automatica delle istruzioni |
| `rpg` | Allenamento, esperienza, talenti e condizione |
| `season` | Calendario, avanzamento e classifica |
| `persistence` | Salvataggio, caricamento e validazione |
| `event` | Registro strutturato degli eventi |
| `cli` | Presentazione e raccolta degli input |

I nomi identificano responsabilità, non impongono file o directory.

## 2.3 Regola delle dipendenze

`world`, `roster` e i tipi fondamentali costituiscono il livello dati.

`match` utilizza dati e decisioni validate. Non dipende dalla CLI, dall'implementazione dell'IA o dalla serializzazione.

`ai` riceve uno stato osservabile e restituisce decisioni candidate.

`rpg` applica la crescita ai dati persistenti nei confini gestionali.

`season` orchestra le partite e aggiorna una sola volta i risultati ufficiali.

`cli` e `persistence` operano mediante interfacce esplicite, senza duplicare regole sportive.

Sono vietate dipendenze circolari tra componenti.

---

# 3. Identità e dati fondamentali

## 3.1 Identificativi

Ogni entità persistente possiede un identificativo numerico stabile.

Tipi logici:

- `TeamId`
- `PlayerId`
- `CoachId`
- `MatchId`
- `EventId`
- `SeasonId`

Il valore zero è riservato a `INVALID_ID`.

Gli identificativi non vengono riutilizzati entro una carriera.

## 3.2 Calciatore

Ogni calciatore possiede:

- Identità e squadra.
- Età, ruolo principale e compatibilità secondarie.
- Sedici attributi generali da 1 a 100.
- Quattro attributi specialistici da portiere, se applicabili.
- Livello RPG da 1 a 20 ed esperienza accumulata.
- Potenziale individuale nascosto.
- Progressi di allenamento.
- Talenti consolidati e progressi verso talenti candidati.
- Energia, fatica, morale e disponibilità.
- Disciplina, leadership, determinazione e collaborazione.
- Relazioni necessarie al modello semplificato di intesa.
- Statistiche stagionali.

Gli attributi di carattere sono inclusi nei sedici attributi generali: non devono essere duplicati in un secondo profilo numerico.

Il ruolo determina le compatibilità e i pesi delle capacità, non modifica direttamente gli attributi.

## 3.3 Energia, fatica e morale

Per evitare stati ridondanti:

- `energy`: 0–100, con 100 = completamente recuperato.
- `morale`: 0–100, con 50 = neutro.
- `fitness`: disponibilità fisica e penalità temporanee.
- La fatica visualizzata è derivata da `100 - energy`.

Non deve esistere un secondo valore di fatica aggiornato indipendentemente dall'energia.

## 3.4 Portieri — C01

Il portiere mantiene gli attributi generali pertinenti e aggiunge:

1. Riflessi.
2. Presa.
3. Uscite.
4. Posizionamento da portiere.

Questi attributi partecipano soltanto alle risoluzioni in cui sono pertinenti.

Un giocatore di movimento non può essere trattato come portiere ordinario, salvo una situazione eccezionale di partita gestita esplicitamente dal motore.

---

# 4. Generazione procedurale

## 4.1 Seed e versionamento

Una carriera è identificata da:

`WorldSeed + GeneratorVersion + RulesVersion`

Il seed è un valore a 64 bit senza segno.

L'interfaccia può accettare un seed testuale; la conversione canonica in valore numerico deve essere deterministica e versionata.

I nomi dei giocatori non contribuiscono al calcolo delle statistiche attraverso hash non documentati.

## 4.2 Generazione dell'universo

L'ordine logico obbligatorio è:

1. Identità delle otto squadre.
2. Identità degli otto allenatori.
3. Archetipi delle rose.
4. Distribuzione di età e ruoli.
5. Attributi e potenziale.
6. Compatibilità dei ruoli.
7. Talenti e relazioni iniziali.
8. Validazione.
9. Calendario.
10. Scelta delle tre candidate.

Una generazione non valida non deve essere presentata all'utente.

## 4.3 Composizione delle rose

Ogni squadra contiene esattamente:

| Reparto | Numero |
|---|---:|
| Portieri | 2 |
| Difensori | 5 |
| Centrocampisti | 7 |
| Attaccanti | 4 |

Il generatore deve garantire la presenza di undici titolari distinti in ciascuno dei tre moduli iniziali.

La validazione usa gli slot effettivi dei moduli, non soltanto il reparto.

Una compatibilità secondaria può coprire uno slot, con penalità esplicita di efficacia.

Devono essere disponibili un portiere di riserva e almeno due alternative di movimento compatibili con la formazione iniziale proposta.

## 4.4 Formazioni — C02

Gli slot obbligatori sono:

**4-4-2:** POR, TD, DC, DC, TS, ED, CC, CC, ES, ATT, ATT.

**4-3-3:** POR, TD, DC, DC, TS, CC, MED, CC, AD, ATT, AS.

**3-5-2:** POR, DC, DC, DC, ED, CC, MED, CC, ES, ATT, ATT.

Le abbreviazioni identificano slot funzionali, non professioni esclusive dei calciatori.

Un calciatore può occupare uno slot soltanto se possiede una compatibilità principale o secondaria dichiarata, salvo sostituzioni d'emergenza esplicitamente consentite dalle regole.

La ricerca della formazione valida è un problema di assegnazione uno-a-uno: nessun giocatore può coprire due slot.

## 4.5 Diversità delle proposte

Le tre squadre proposte devono essere selezionate tra le otto effettivamente generate.

I loro profili devono differire almeno su due delle seguenti dimensioni:

- Fascia di forza iniziale.
- Fascia di età media.
- Profilo tattico prevalente.
- Potenziale aggregato stimato.
- Affiatamento iniziale.

Una differenza di sola denominazione non è sufficiente.

Il sistema deve poter utilizzare una selezione deterministica di ripiego se non trova tre profili sufficientemente diversi; non deve entrare in un ciclo di rigenerazione illimitato.

## 4.6 Forza delle squadre

La forza iniziale deriva dall'efficacia dei migliori undici disponibili in una formazione legale, considerando attributi pertinenti, compatibilità, energia iniziale e organizzazione tattica.

Non coincide con la media aritmetica dei sedici attributi.

Per M001 l'algoritmo deve creare squadre entro una fascia di forza iniziale controllata, senza pretendere che abbiano probabilità identiche di vittoria.

Il rapporto tra le tre proposte deve essere verificato sulle formazioni valide, non su un valore nominale astratto.

---

# 5. Stato e regole della partita — B01

## 5.1 Modello gerarchico

Lo stato della partita contiene tre dimensioni distinte.

**Periodo:**

- Non iniziata.
- Primo tempo.
- Intervallo.
- Secondo tempo.
- Conclusa.

**Modalità di gioco:**

- Pallone in gioco.
- Ripresa da eseguire.
- Finestra gestionale.
- Periodo terminato.

**Fase del possesso:**

- Costruzione.
- Progressione.
- Sviluppo offensivo.
- Occasione.
- Transizione.

La risoluzione del tiro è un evento, non uno stato persistente obbligatorio.

Un'azione può terminare in una qualsiasi fase.

## 5.2 Campo tattico

Il campo è suddiviso in quindici zone:

- Cinque settori longitudinali.
- Tre corsie: sinistra, centro, destra.

Le zone vengono interpretate rispetto alla direzione d'attacco della squadra in possesso.

Al cambio di possesso, la posizione fisica astratta viene trasformata nel sistema di riferimento della nuova squadra.

Le squadre cambiano direzione all'intervallo.

Il motore non deve invertire automaticamente sinistra e destra rispetto al riferimento tattico della squadra.

## 5.3 Possesso

Un possesso possiede:

- Squadra in possesso.
- Zona corrente.
- Fase.
- Iniziatore.
- Eventuale calciatore attualmente in controllo del pallone.
- Numero di azioni.
- Tempo trascorso.
- Contesto tattico.
- Stato della sequenza offensiva.

Termina per:

- Recupero avversario.
- Pallone uscito.
- Interruzione regolamentare.
- Gol.
- Conclusione del periodo.

Una respinta non determina automaticamente un cambio di possesso: il motore deve assegnare o risolvere il pallone conteso.

## 5.4 Azioni fondamentali

Sono supportate:

1. Passaggio.
2. Conduzione.
3. Dribbling.
4. Contrasto.
5. Cross.
6. Tiro.
7. Movimento tattico.
8. Ripresa da palla inattiva.

Ogni azione possiede:

- Precondizioni.
- Partecipanti validi.
- Parametri contestuali.
- Esiti enumerati.
- Variazione temporale.
- Eventi emessi.
- Stato successivo.

Un'azione illegale non può essere risolta attraverso una penalità casuale: deve essere respinta prima dell'esecuzione.

## 5.5 Risoluzione

La risoluzione segue obbligatoriamente questa sequenza:

`State -> CandidateAction -> Validation -> ProbabilityModel -> RandomDraw -> Outcome -> Events -> Transition`

L'esito viene applicato una sola volta.

Le probabilità devono essere derivate da fattori pertinenti:

- Attributi degli attori.
- Posizione tattica.
- Opposizione.
- Istruzioni.
- Energia.
- Morale.
- Talenti.
- Contesto dell'azione.

Non sono ammessi bonus dipendenti dall'identità della squadra controllata dal giocatore.

## 5.6 Modello probabilistico iniziale

Per i duelli ordinari, M001 utilizza punteggi interi normalizzati e una funzione logistica tabulata.

La differenza di efficacia è:

`D = A - O + C`

Dove:

- `A` è l'efficacia contestuale dell'attaccante, 0–100.
- `O` è l'opposizione contestuale, 0–100.
- `C` è il modificatore tattico e situazionale, compreso inizialmente fra -20 e +20.

`D` viene limitato all'intervallo -100…100.

La probabilità base è:

`P = 10000 / (1 + exp(-D / 16))`

Questa formula costituisce la specifica matematica di riferimento. Nell'esecuzione C11 deve essere utilizzata una tabella intera canonica di 201 valori, generata e congelata con una procedura di arrotondamento definita nel repository; non si deve utilizzare `exp()` a runtime per decidere gli esiti.

L'estrazione è uniforme su 0…9999. L'azione riesce se il valore estratto è strettamente inferiore a `P`.

Il valore 0 o 10000 è ammesso soltanto per esiti deterministici, non per normali duelli probabilistici.

Per ogni famiglia di azioni, gli esiti secondari devono essere selezionati con probabilità condizionate la cui somma sia esattamente 10000 unità.

Questa formula non si applica indiscriminatamente a ogni evento: conclusioni, falli e situazioni speciali possiedono distribuzioni dedicate.

## 5.7 Tiri e gol

La risoluzione avviene in tre passaggi:

1. Costruzione della qualità dell'occasione.
2. Determinazione della traiettoria della conclusione.
3. Risoluzione dell'opposizione difensiva e del portiere.

Gli esiti finali possibili sono:

- Gol.
- Parata con possesso del portiere.
- Parata con ribattuta.
- Tiro respinto.
- Tiro fuori.
- Tiro su palo o traversa, con successiva risoluzione della palla.

La qualità dell'occasione viene registrata prima dell'esito finale.

Un gol non può essere assegnato senza una conclusione valida o un evento di autorete esplicitamente rappresentato.

## 5.8 Regole temporali

Sono previsti due periodi da 45 minuti regolamentari.

Il tempo interno utilizza secondi interi.

Il primo tempo termina non prima di 2700 secondi; il secondo non prima di 5400 secondi complessivi.

Il recupero è calcolato dal motore in base alle interruzioni registrate, secondo una politica deterministica versionata.

Un evento non può avanzare oltre il limite finale del periodo senza che il motore gestisca esplicitamente la conclusione della sequenza.

Il tempo non può diminuire.

Sono vietati possessi senza avanzamento temporale illimitati: ogni sequenza ha un limite di azioni e una regola di terminazione.

## 5.9 Regole sportive M001

- Undici giocatori iniziali per squadra.
- Massimo cinque sostituzioni per squadra.
- Massimo tre finestre ordinarie di sostituzione per squadra, escluso l'intervallo.
- Le espulsioni riducono il numero di calciatori schierabili.
- La seconda ammonizione determina l'espulsione.
- Nessun calciatore espulso può rientrare.
- I calciatori sostituiti non possono rientrare.
- Rimesse laterali, rinvii, calci d'angolo, punizioni e rigori durante la partita sono supportati.
- Il fuorigioco usa una valutazione astratta di posizione e momento del passaggio, compatibile con il modello a zone.
- Nessun VAR, supplementare o rigore decisivo.

L'astrazione del fuorigioco non pretende fedeltà geometrica alle regole reali.

In caso di riduzione sotto il numero minimo regolamentare di giocatori, la partita viene interrotta e marcata come sospesa per insufficienza di giocatori; il motore non inventa automaticamente un risultato finale.

Per M001 la disciplina stagionale si limita alle espulsioni e agli eventi della partita. Le squalifiche cumulative fra giornate non sono previste.

## 5.10 Invarianti della partita

Sono sempre vietati:

- Possesso assegnato a una squadra inesistente.
- Calciatore inesistente o non schierabile coinvolto in un'azione.
- Due portatori contemporanei dello stesso pallone.
- Gol attribuito due volte.
- Tempo negativo o decrescente.
- Sostituzioni illegali.
- Calciatore espulso ancora impiegato.
- Eventi di gioco dopo la conclusione.
- Partita conclusa senza stato terminale esplicito.

---

# 6. Decisioni e interventi — B02

## 6.1 Contratto unico

Ogni istruzione viene rappresentata come `Decision`.

Campi obbligatori:

- Identificativo della partita.
- Squadra destinataria.
- Tipo di istruzione.
- Parametri.
- Stato osservabile di riferimento.
- Finestra di applicazione.
- Identificativo progressivo della decisione.

La provenienza umana o automatica è metadato di audit e non modifica le regole di validazione.

## 6.2 Decisioni consentite

M001 supporta:

- Cambio di modulo.
- Cambio di mentalità.
- Regolazione del pressing.
- Regolazione del ritmo.
- Regolazione dell'ampiezza.
- Sostituzione.
- Istruzione contestuale di protezione.
- Istruzione contestuale di pressione.
- Istruzione contestuale di transizione.
- Mantenimento delle istruzioni esistenti.

Le istruzioni contestuali modificano il comportamento futuro e non selezionano direttamente passaggi, dribbling o tiri.

## 6.3 Finestre decisionali

Tre categorie:

**Prepartita:** impostazione completa.

**Intervallo:** modifiche complete, incluse sostituzioni.

**Interruzione utile:** sostituzioni, adattamenti e istruzioni situazionali consentite.

Durante il pallone in gioco non vengono applicati cambi di modulo o sostituzioni.

Le istruzioni possono essere accodate per la prima finestra valida.

## 6.4 Validazione

Ogni decisione viene sottoposta a:

1. Verifica di identità e proprietà.
2. Verifica del periodo e della finestra.
3. Verifica degli attori.
4. Verifica delle risorse e delle sostituzioni disponibili.
5. Verifica di coerenza tattica.
6. Applicazione o rifiuto atomico.

Una decisione respinta non consuma sostituzioni, finestre o estrazioni casuali.

La CLI non può aggirare il validatore.

## 6.5 Interventi situazionali

Un'opportunità situazionale viene creata solo se un osservatore tattico rileva condizioni sufficienti.

Ogni opportunità contiene:

- Evidenza osservabile.
- Tipologia.
- Opzioni valide.
- Durata di disponibilità.
- Effetti previsti.
- Momento di applicazione.

Il giocatore può scegliere un'opzione oppure non intervenire.

Il motore deve evitare opportunità duplicate per una stessa condizione invariata.

La frequenza desiderata è inizialmente 3–6 opportunità rilevanti per partita, come obiettivo di esperienza utente e non come quota obbligatoria.

---

# 7. Allenatori e IA — C04

## 7.1 Personalità

Ogni allenatore possiede quattro attributi 0–100:

- Aggressività.
- Prudenza.
- Adattabilità.
- Gestione umana.

La filosofia tattica della squadra è distinta dalla personalità del suo allenatore.

La prima indica la predisposizione della rosa; la seconda orienta le decisioni.

## 7.2 Procedura decisionale

La stessa procedura viene utilizzata da tutti gli allenatori automatici:

`Observe -> GenerateCandidates -> Validate -> Score -> Select`

Le osservazioni sono limitate alle informazioni effettivamente disponibili all'allenatore.

L'IA non può leggere:

- Estrazioni future.
- Probabilità nascoste del motore.
- Potenziali segreti avversari.
- Decisioni umane non ancora applicate.

## 7.3 Scoring

Le decisioni vengono confrontate mediante cinque fattori normalizzati su -100…100:

- Beneficio offensivo.
- Protezione difensiva.
- Costo energetico.
- Stabilità tattica.
- Beneficio di gestione della rosa.

Il punteggio di utilità è una somma pesata intera.

I pesi derivano dalla personalità e dal contesto della partita; sono normalizzati a somma 100.

Un cambiamento tattico è ammesso soltanto quando supera l'utilità del mantenimento dell'assetto di almeno 5 punti, salvo situazione obbligatoria o indisponibilità di un calciatore.

Dopo un cambio discrezionale è previsto un periodo di stabilizzazione di almeno 300 secondi di tempo di gioco, salvo eventi straordinari.

## 7.4 Parità di trattamento

IA e giocatore utilizzano lo stesso:

- Validatore.
- Catalogo delle istruzioni.
- Modello della partita.
- Sistema di sostituzioni.
- Calendario.
- Sistema di crescita.

Le personalità non applicano bonus diretti al punteggio.

## 7.5 Modalità automatica

Nelle partite senza intervento umano, un controller automatico produce le decisioni.

La modalità automatica deve essere disponibile anche per la squadra normalmente controllata dal giocatore.

Il motore delle partite non deve conoscere la differenza tra sessione interattiva e simulazione massiva.

---

# 8. Progressione RPG — C03

## 8.1 Separazione delle progressioni

Sono definiti tre sistemi.

**Esperienza RPG:** maturazione del personaggio.

**Attributi calcistici:** capacità effettive.

**Talenti:** specializzazioni condizionali.

I sistemi sono collegati ma non equivalenti.

## 8.2 Livelli

Livelli ammessi: 1–20.

La soglia cumulativa XP per raggiungere il livello `L`, per `L >= 1`, è:

`XP_required(L) = 50 × (L - 1) × L`

Quindi il livello 1 inizia a 0 XP, il livello 2 a 100 XP e il livello 20 a 19000 XP.

L'XP non diminuisce e non può superare il limite numerico del tipo persistente.

Raggiungere un livello non assegna automaticamente incrementi agli attributi.

## 8.3 Esperienza delle partite

Per ogni calciatore:

`XP_match = XP_minutes + XP_contribution + XP_learning`

Dove:

- I minuti giocati producono al massimo 60 XP.
- Il contributo sportivo normalizzato per ruolo produce da 0 a 40 XP.
- Gli obiettivi di apprendimento producono da 0 a 20 XP.

Massimo per partita: 120 XP.

Una prestazione individuale non può generare XP illimitata attraverso il numero di passaggi o altre azioni ripetitive.

I contributi vengono valutati rispetto al ruolo e agli eventi registrati.

## 8.4 Allenamento settimanale

La settimana gestionale prevede cinque slot ordinari di preparazione.

Ogni slot può essere assegnato a:

- Allenamento tattico.
- Sviluppo individuale.
- Recupero.

Gli stimoli vengono applicati una volta per slot, non a ogni consultazione della schermata.

Allenamento tattico e individuale consumano energia.

Il recupero ripristina energia, entro il limite 100.

Il calendario gestionale impedisce di eseguire due volte la stessa sessione.

## 8.5 Crescita degli attributi

Ogni attributo possiede un contatore di progresso da allenamento.

L'incremento di un punto richiede:

- Soglia di progresso raggiunta.
- Disponibilità fisica.
- Compatibilità con il percorso.
- Margine rispetto al potenziale.
- Limiti stagionali.

Per M001 ogni attributo può crescere al massimo di 4 punti nella stagione; l'insieme degli attributi generali e specialistici di un calciatore può crescere complessivamente al massimo di 12 punti.

Il potenziale rappresenta una capacità aggregata di crescita, non un limite individuale rigido identico per tutti gli attributi.

L'età modifica il rendimento dell'allenamento, non produce automaticamente decadimento durante M001.

## 8.6 Talenti guidati

Catalogo iniziale di otto talenti:

1. Passaggio illuminante.
2. Intercettatore.
3. Sangue freddo.
4. Primo passo.
5. Regista naturale.
6. Secondo fiato.
7. Capitano.
8. Mentore.

Ogni talento deve possedere:

- Requisiti di attributi.
- Ruoli compatibili.
- Condizioni di maturazione.
- Condizioni di attivazione.
- Effetto numerico limitato.
- Eventuale incompatibilità con altri talenti.

Il manager sceglie soltanto fra talenti maturati.

Ogni calciatore può consolidare al massimo due talenti durante M001.

Il numero totale di talenti consolidati per calciatore è limitato a tre, inclusi quelli iniziali.

Un talento ordinario non può aumentare di più del 10% relativo la probabilità dell'esito che modifica, salvo effetto deterministico esplicitamente previsto.

L'effetto dei talenti è applicato prima dell'estrazione casuale e registrato nel contesto della risoluzione.

## 8.7 Morale e relazioni

Per M001 il morale individuale varia da 0 a 100.

L'intesa collettiva deriva da familiarità tattica, continuità d'impiego e compatibilità fra calciatori.

Non viene implementato un grafo sociale completo.

Le relazioni iniziali e le interazioni di spogliatoio producono un indicatore semplificato di affiatamento per reparto e per coppie rilevanti.

I modificatori derivanti dal morale e dall'intesa sono limitati, aggregati e non possono superare complessivamente il 10% relativo della probabilità di una normale azione.

---

# 9. Campionato e carriera — C06, C07, C08

## 9.1 Calendario

Otto squadre, quattordici giornate, quattro partite per giornata.

Ogni coppia di squadre deve incontrarsi due volte, una in casa e una in trasferta.

Nessuna squadra può disputare più di una partita per giornata.

La generazione del calendario deve essere deterministica.

## 9.2 Stati della giornata

La giornata si trova in uno dei seguenti stati:

- `PREPARATION`
- `LINEUP_READY`
- `MATCHDAY_RUNNING`
- `RESULTS_COMMITTED`
- `WEEK_COMPLETE`

Solo le transizioni previste sono valide.

Una giornata completata non può tornare a uno stato precedente.

## 9.3 Avanzamento

L'ordine gestionale è:

1. Consultazione.
2. Programmazione degli allenamenti.
3. Applicazione delle sessioni.
4. Formazione e tattica.
5. Simulazione delle quattro partite.
6. Registrazione transazionale dei risultati.
7. Progressione RPG e rapporto.
8. Chiusura della settimana.
9. Disponibilità del salvataggio.
10. Passaggio alla giornata successiva.

La preparazione può utilizzare impostazioni persistenti.

Il giocatore non deve essere obbligato a riconfigurare manualmente ogni settimana.

## 9.4 Classifica

Punteggio:

- Vittoria: 3.
- Pareggio: 1.
- Sconfitta: 0.

Ordinamento:

1. Punti.
2. Differenza reti.
3. Gol segnati.
4. Vittorie.
5. Identificativo stabile della squadra.

L'ultimo criterio garantisce un ordinamento deterministico, ma non attribuisce meriti sportivi aggiuntivi.

Le statistiche della classifica derivano dai risultati ufficialmente registrati.

Un risultato può essere registrato una sola volta.

## 9.5 Partite non concluse regolarmente

Una partita sospesa per insufficienza di giocatori o errore regolamentare non può essere convertita silenziosamente in risultato valido.

Il campionato si arresta con un errore diagnostico recuperabile e uno stato integro.

La gestione dei risultati a tavolino e delle decisioni disciplinari esterne è esclusa da M001.

Questa limitazione deve essere rappresentata esplicitamente anche nei test di robustezza.

## 9.6 Fine stagione

La stagione termina quando:

- Tutte le 56 partite risultano completate e registrate.
- Non esistono partite sospese irrisolte.
- Tutti i risultati sono inclusi una sola volta.
- La classifica è consistente.
- I rapporti finali sono stati generati.

L'ultimo stato della carriera M001 è `SEASON_COMPLETE`.

Non è prevista una nuova stagione nella stessa carriera.

---

# 10. Eventi e statistiche

## 10.1 Event log

Gli eventi della partita formano un registro append-only durante la simulazione.

Tipi principali:

- Inizio periodo.
- Inizio possesso.
- Azione.
- Recupero.
- Fallo.
- Ammonizione.
- Espulsione.
- Fuorigioco.
- Ripresa.
- Tiro.
- Gol.
- Sostituzione.
- Decisione tattica applicata.
- Fine periodo.
- Fine partita.

Ogni evento possiede:

- ID progressivo.
- Partita.
- Tempo.
- Tipo.
- Squadra.
- Attori pertinenti.
- Dati dell'esito.
- Riferimenti causali, quando necessari.

## 10.2 Statistiche

Tiri, tiri nello specchio, gol, ammonizioni, espulsioni e sostituzioni derivano dal registro degli eventi.

Il possesso viene stimato dal tempo di controllo registrato nelle sequenze e non da una semplice conta dei possessi.

Le statistiche individuali devono riflettere l'effettiva partecipazione alle azioni.

## 10.3 Cronaca

La cronaca non è fonte di verità.

Una frase viene prodotta soltanto da un evento registrato.

Non possono essere generati gol, assist, falli o risultati narrativi indipendenti dal motore.

---

# 11. Determinismo — B03

## 11.1 PRNG

Si adotta `PCG32` con algoritmo e parametri di inizializzazione fissati nella specifica tecnica del progetto.

Sono vietati `rand()` e generatori casuali impliciti nel percorso normativo del motore.

Le estrazioni sono su interi senza segno e usano procedure prive di bias per ottenere intervalli limitati.

## 11.2 Flussi separati

Sono definiti flussi indipendenti:

- Generazione dell'universo.
- Attributi e predisposizioni.
- Calendario.
- Simulazione della singola partita.
- Decisioni automatiche.
- Progressione RPG.

Ogni partita deriva il proprio stato PRNG da seed, ID della partita e versione dell'algoritmo.

La simulazione di una partita non deve dipendere dal numero di estrazioni consumate da una partita precedente.

## 11.3 Replay

Un replay è identificato da:

`RulesVersion + WorldSnapshot + MatchId + MatchSeed + DecisionSequence`

La riproduzione deve fornire:

- Stato finale identico.
- Eventi strutturati identici.
- Statistiche identiche.
- Hash canonico del risultato identico.

La visualizzazione testuale non è inclusa nell'hash normativo del replay.

## 11.4 Aritmetica

Le decisioni probabilistiche devono utilizzare aritmetica intera con intervalli e arrotondamenti espliciti.

I confronti che determinano l'esito delle azioni non possono dipendere da floating-point non controllato.

Le operazioni aritmetiche devono prevenire overflow e conversioni implicite pericolose.

---

# 12. Salvataggi — B05

## 12.1 Ambito

Sono permessi salvataggi:

- Dopo la generazione e la scelta della squadra.
- Durante la preparazione gestionale, prima dell'avvio della giornata.
- Dopo una giornata completamente registrata.
- Alla fine della stagione.

Non sono permessi salvataggi durante un possesso o mentre una giornata sta registrando risultati.

## 12.2 Formato

Si utilizza un formato testuale canonico, versionato e con schema rigido, senza dipendenza obbligatoria da librerie esterne.

Ogni record include tipo, identificativo e campi espliciti.

I record hanno ordine canonico deterministico.

Le stringhe utilizzano codifica UTF-8 valida e una rappresentazione non ambigua.

Il salvataggio contiene:

- Versioni.
- Seed.
- Squadre e allenatori.
- Rose e stati individuali.
- Talenti e progressi.
- Calendario e risultati.
- Stato della settimana.
- Programmi di allenamento.
- Istruzioni persistenti.
- Statistiche necessarie.
- Stato dei flussi casuali persistenti.
- Identificativi e contatori progressivi.

I salvataggi di versione non supportata vengono rifiutati senza modifiche allo stato corrente.

## 12.3 Integrità

Il salvataggio deve essere validato prima di essere accettato.

Sono verificati:

- Schema.
- Versione.
- Integrità dei record.
- Identificativi.
- Vincoli numerici.
- Coerenza delle relazioni.
- Coerenza del calendario.
- Coerenza dei risultati.

La scrittura utilizza un file temporaneo nella stessa directory del salvataggio e sostituzione controllata.

Sul target Linux, la procedura deve includere controllo degli errori di scrittura, sincronizzazione del file e sostituzione atomica mediante rename, con sincronizzazione della directory quando richiesta dalla garanzia di durabilità.

## 12.4 Ripresa

La ripresa non deve:

- Ripetere un allenamento già applicato.
- Registrare nuovamente una partita.
- Assegnare due volte XP.
- Modificare il calendario.
- Cambiare decisioni persistenti.
- Alterare il futuro casuale della simulazione.

---

# 13. CLI

## 13.1 Funzioni obbligatorie

La CLI deve offrire:

- Nuova carriera.
- Inserimento o generazione del seed.
- Scelta fra tre squadre.
- Visualizzazione rosa.
- Visualizzazione dettagli del calciatore.
- Scelta formazione e modulo.
- Impostazione tattica.
- Programmazione degli allenamenti.
- Avvio giornata.
- Partita interattiva.
- Simulazione automatica.
- Classifica.
- Calendario.
- Rapporto post-partita.
- Salvataggio.
- Caricamento.
- Rapporto di fine stagione.

## 13.2 Ergonomia

Sono obbligatori:

- Valori predefiniti validi.
- Conferma delle operazioni irreversibili.
- Diagnostica comprensibile.
- Possibilità di non intervenire nelle finestre RPG.
- Delegabilità delle decisioni ordinarie.
- Nessuna richiesta di input per le partite interamente automatiche.

Input non valido non deve terminare la sessione, corrompere lo stato o consumare una decisione.

## 13.3 Modalità non interattiva

Per la qualificazione devono essere disponibili comandi non interattivi equivalenti a:

- Generare e validare un universo.
- Simulare una partita.
- Simulare una stagione.
- Riprodurre una partita da replay.
- Validare un salvataggio.
- Eseguire una campagna statistica.

I nomi esatti delle opzioni CLI saranno fissati nella specifica dell'interfaccia, preservando tali capacità.

---

# 14. Qualificazione — B06

## 14.1 Livello 1: invarianti

Requisito: zero violazioni.

Verifiche obbligatorie:

- Validità delle rose.
- Legalità delle formazioni.
- Legalità delle sostituzioni.
- Coerenza temporale.
- Coerenza dei possessi.
- Coerenza delle riprese.
- Integrità degli eventi.
- Integrità della classifica.
- Correttezza dell'aggiornamento stagionale.
- Integrità dei salvataggi.

Qualsiasi violazione costituisce un fallimento bloccante.

## 14.2 Livello 2: determinismo

Eseguire almeno:

- 100 universi con generazione ripetuta.
- 1.000 replay di partite con decisioni equivalenti.
- 100 stagioni ripetute.
- 100 confronti tra carriera continua e carriera salvata/ripresa.

Esito richiesto: identità canonica nel 100% dei confronti.

La verifica multipiattaforma diventa obbligatoria solo sulle piattaforme dichiarate ufficialmente supportate.

## 14.3 Livello 3: proprietà metamorfiche

Sono richieste prove che controllino, su campioni di seed e situazioni equivalenti:

- Incrementare l'efficacia del tiratore non riduca sistematicamente la probabilità di una conclusione valida.
- Incrementare l'opposizione difensiva non migliori sistematicamente l'attaccante.
- Aumentare la fatica non migliori sistematicamente la prestazione atletica.
- I talenti agiscano soltanto nei contesti consentiti.
- L'IA non selezioni istruzioni illegali.
- Le modifiche tattiche producano effetti coerenti con il rispettivo contratto.

Quando una proprietà deve essere monotona per definizione, il test deve confrontare direttamente la funzione di probabilità, non soltanto gli esiti casuali.

## 14.4 Livello 4: plausibilità statistica

La qualificazione prevede almeno 10.000 partite automatiche, con seed documentati e rose appartenenti a più profili.

Soglie iniziali proposte, da congelare prima della campagna finale:

| Metrica | Intervallo di qualificazione |
|---|---|
| Gol totali medi per partita | 1,8–4,0 |
| Tiri totali medi per partita | 12–36 |
| Percentuale di incontri con almeno un gol | 65–95% |
| Pareggi | 15–40% |
| Cartellini rossi medi per partita | 0–0,6 |
| Tempo di possesso contabilizzato | 100% del tempo di controllo valido |

Gli intervalli sono volutamente ampi. Servono a identificare anomalie macroscopiche, non certificano da soli il realismo calcistico.

Devono inoltre essere verificati:

- Assenza di risultati estremi sistematici.
- Distribuzioni non degeneri.
- Differenze misurabili tra profili tattici.
- Assenza di una tattica universalmente dominante nel campione di scenari previsto.

La campagna di calibrazione deve essere separata dalla campagna finale di accettazione.

## 14.5 Livello 5: percorso giocabile

Deve essere eseguito almeno un percorso completo dalla nuova carriera al rapporto finale.

Il percorso deve includere:

- Scelta della squadra.
- Modifica della formazione.
- Programmazione dell'allenamento.
- Partita con intervento RPG.
- Partita automatica.
- Progressione di almeno un calciatore.
- Consolidamento di un talento, tramite scenario valido.
- Salvataggio e ripresa.
- Completamento della quattordicesima giornata.

Il test non deve falsificare i risultati sportivi per soddisfare la condizione di progressione: può utilizzare uno scenario iniziale di qualificazione con prerequisiti noti.

---

# 15. Sequenza M001 e closure gate

## M001-A — World Generation

**Deliverable:** universo deterministico, rose valide, tre proposte e calendario.

**Gate:**
- `WORLD_VALIDITY=PASS`
- `ROSTER_COVERAGE=PASS`
- `CANDIDATE_DIVERSITY=PASS`
- `GENERATION_REPLAY=PASS`
- `CALENDAR_VALIDITY=PASS`

## M001-B — Match Engine

**Deliverable:** simulatore autonomo basato su possessi, eventi e regole.

**Gate:**
- `MATCH_STATE=PASS`
- `MATCH_RULES=PASS`
- `MATCH_TIME=PASS`
- `EVENT_INTEGRITY=PASS`
- `MATCH_REPLAY=PASS`

## M001-C — Decisions & AI

**Deliverable:** decisioni validate, interventi situazionali, IA con personalità.

**Gate:**
- `DECISION_VALIDATION=PASS`
- `HUMAN_AI_PARITY=PASS`
- `AI_INFORMATION_FAIRNESS=PASS`
- `AI_STABILITY=PASS`
- `TACTICAL_EFFECT=PASS`

## M001-D — RPG Progression

**Deliverable:** crescita, allenamenti, morale, esperienza e talenti guidati.

**Gate:**
- `XP_INTEGRITY=PASS`
- `ATTRIBUTE_LIMITS=PASS`
- `TALENT_VALIDATION=PASS`
- `TRAINING_ONCE=PASS`
- `RPG_PERSISTENCE=PASS`

## M001-E — Season & CLI

**Deliverable:** campionato completo giocabile e persistente.

**Gate:**
- `SEASON_SCHEDULE=PASS`
- `RESULT_COMMIT=PASS`
- `STANDINGS=PASS`
- `SAVE_RESUME=PASS`
- `CLI_PLAYABILITY=PASS`

## M001-F — Integrated Qualification

**Deliverable:** dossier di qualificazione con risultati riproducibili.

**Gate:**
- `INVARIANTS=PASS`
- `DETERMINISM=PASS`
- `METAMORPHIC=PASS`
- `STATISTICAL=PASS`
- `END_TO_END=PASS`

Ogni fase conserva la propria evidenza di chiusura.

Un gate fallito impedisce la dichiarazione di completamento della relativa fase.

---

# 16. Matrice di tracciabilità dell'audit

| ID | Problema | Risoluzione |
|---|---|---|
| B01 | Regole incomplete | §5 |
| B02 | Confine simulazione/decisioni | §2 e §6 |
| B03 | Determinismo | §4 e §11 |
| B04 | Generazione insufficiente | §3 e §4 |
| B05 | Persistenza | §9 e §12 |
| B06 | Qualificazione | §14 e §15 |
| C01 | Portieri | §3.4 |
| C02 | Copertura ruoli | §4.3–4.4 |
| C03 | Progressioni sovrapposte | §8 |
| C04 | Allenatore/stile squadra | §7 |
| C05 | Troppe interruzioni | §6.5 e §13 |
| C06 | Settimana senza stato | §9.2–9.3 |
| C07 | Classifica divergente | §9.4 e §10 |
| C08 | Salvataggio a giornata incompleta | §9.2 e §12 |

Tutte le criticità individuate nell'audit sono assegnate a una sezione normativa.

---

# 17. Politica di evoluzione

La baseline funzionale 1.0 rimane congelata.

Sono consentite durante M001:

- Correzioni di errori.
- Miglioramenti delle prestazioni.
- Calibrazioni numeriche documentate.
- Rifiniture della CLI.
- Rafforzamenti dei test.
- Chiarimenti che non alterano il contratto.

Richiedono una revisione esplicita:

- Nuove meccaniche fondamentali.
- Nuovi sistemi di progressione.
- Cambiamenti incompatibili dei salvataggi.
- Modifiche ai criteri di determinismo.
- Alterazioni dei gate di accettazione.
- Estensione del perimetro M001.

Una correzione a un requisito tecnicamente impossibile o contraddittorio non può essere applicata silenziosamente.

---

# 18. Criterio finale di accettazione

**M001 è completata soltanto quando:**

1. Tutte le fasi A–F hanno superato i rispettivi closure gate.
2. La prima stagione è giocabile dall'inizio alla fine tramite CLI.
3. La modalità automatica completa lo stesso campionato.
4. Il salvataggio e la ripresa preservano lo stato e la riproducibilità.
5. I sistemi RPG influiscono effettivamente sulle prestazioni senza sostituire la logica calcistica.
6. Le decisioni dell'allenatore e dell'IA rispettano lo stesso contratto.
7. Le evidenze di qualificazione sono riproducibili.
8. Non restano violazioni note delle invarianti normative.

**Condizione conclusiva:** RPG Manager Calcio M001 deve essere un gioco funzionante, non soltanto un insieme di componenti che superano test isolati.

---

**Fine — M001 Engineering Contract v1.0**
