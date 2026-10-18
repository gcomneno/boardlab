# BoardLab — LeLe architetturali

[English](README.md) | [Italiano](README.it.md)

## Scopo

Questa raccolta conserva decisioni riutilizzabili di architettura e software
design dimostrate da BoardLab. È una fonte per future LeLe Manager qualificate
singolarmente, non una vault né un registro di studio completato.

## Lezioni architetturali

### 1. I contratti comportamentali collegano implementazioni indipendenti

- **Problema:** Un motore Python e un'app nativa Kotlin devono rispettare
  le stesse semantiche senza condividere codice eseguibile.
- **Trade-off:** Due implementazioni duplicano la logica di dominio, ma evitano
  Python incorporato in Android e l'accoppiamento dell'app alla CLI.
- **Decisione:** Mantenere Python canonico e definire fixture di parità
  versionate ed eseguibili per stati, azioni legali, transizioni ed esiti.
- **Evidenza:** [ADR sulla parità Android](../architecture/adr/0002-android-parity.md);
  parità del dominio Kotlin e relativi controlli CI.
- **Riutilizzo:** Migrazioni di dominio fra linguaggi o piattaforme che devono
  preservare il comportamento osservabile.

### 2. Il dominio funzionale deve restare indipendente dai canali di utilizzo

- **Problema:** Le regole devono funzionare nella CLI, nelle strategie e su
  Android senza dipendere dalla UI o dal ciclo di vita della piattaforma.
- **Trade-off:** Confini espliciti richiedono coordinamento aggiuntivo, ma
  impediscono alle esigenze della UI di contaminare gli invarianti.
- **Decisione:** Separare stato immutabile e transizioni deterministiche da
  coordinamento della partita, controller Kotlin e Jetpack Compose.
- **Evidenza:** [Contratto Tre Sigilli](../games/tre-sigilli.it.md);
  GameController Kotlin indipendente e applicazione Android.
- **Riutilizzo:** Domini condivisi da più interfacce, adapter o ambienti.

### 3. Astrazioni stabili consentono di riusare gli algoritmi su giochi nuovi

- **Problema:** Aggiungere un secondo gioco rischia di imporre la riscrittura
  del coordinamento delle partite e di ogni strategia AI.
- **Trade-off:** Un contratto generico vincola le implementazioni, ma rende
  verificabile il riuso di strategie e orchestrazione.
- **Decisione:** Implementare Tre Sigilli e Reversi dietro lo stesso contratto;
  mantenere intercambiabili Random, Minimax, Alpha-Beta e MCTS.
- **Evidenza:** [Contratto Reversi](../games/reversi.it.md);
  test di compatibilità e integrazione delle strategie generiche.
- **Riutilizzo:** Motori estensibili, sistemi di regole e algoritmi decisionali
  sostituibili.

### 4. Il determinismo è una capacità architetturale, non un trucco di test

- **Problema:** Confrontare algoritmi di ricerca e implementazioni diverse
  è difficile con casualità e ordinamenti non controllati.
- **Trade-off:** Seed, budget e spareggi espliciti vincolano l'esecuzione,
  ma rendono i risultati spiegabili e riproducibili.
- **Decisione:** Specificare ordine delle azioni legali e transizioni
  deterministiche; usare test con seed e benchmark riproducibili.
- **Evidenza:** [Regole Tre Sigilli](../games/tre-sigilli.it.md);
  [regole Reversi](../games/reversi.it.md) e documentazione dei benchmark.
- **Riutilizzo:** Debugging algoritmico, confronti fra implementazioni
  e regressioni su ambienti diversi.

### 5. I confini architetturali devono produrre evidenze eseguibili

- **Problema:** Diagrammi e intenzioni architetturali possono divergere
  dal comportamento effettivamente distribuito.
- **Trade-off:** Fixture contrattuali e gate CI separati richiedono manutenzione,
  ma intercettano le violazioni proprio ai confini interessati.
- **Decisione:** Verificare separatamente semantica Python, parità Kotlin,
  build Android debug e release unsigned; ispezionare gli artefatti senza
  confondere il successo CI con la prontezza produttiva.
- **Evidenza:** [ADR sulla parità Android](../architecture/adr/0002-android-parity.md);
  milestone CI SHIP-M2–SHIP-M7 e verifica su dispositivo reale.
- **Riutilizzo:** Architetture distribuite su runtime, piattaforme o pipeline
  di rilascio con modalità di guasto differenti.

## Milestone verificate

- Motore Python e implementazioni riutilizzabili delle strategie.
- Parità del dominio Kotlin e controller indipendente dalla piattaforma.
- Applicazione Android Compose provata in una partita completa su dispositivo.
- APK debug e release unsigned prodotti dalla CI; Lint release eseguito.

## Gate produttivi ancora aperti

Test Android automatizzati, firma release e gestione delle chiavi,
verifiche del ciclo di vita e accessibilità, policy di distribuzione
e preparazione alla pubblicazione restano attività future.
Queste lezioni non dimostrano la prontezza produttiva.
