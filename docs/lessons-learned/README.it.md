# BoardLab — LeLe: lezioni apprese

[English](README.md) | [Italiano](README.it.md)

## Scopo

Questa cartella conserva le lezioni ingegneristiche dimostrate durante lo
sviluppo del motore Python, della parità Kotlin, dell'app Android e della CI.

Sono retrospettive di progetto, non esercizi didattici già completati.

## Lezioni

1. **Evidenze prima dell'implementazione.** Esaminare il repository,
   individuare i vincoli e definire gate PASS/FAIL osservabili prima
   di modificare il codice.
2. **Separare dominio e presentazione.** Regole e transizioni devono restare
   indipendenti da CLI, interfaccia Android e ciclo di vita della piattaforma.
3. **Usare contratti di parità eseguibili.** Fixture condivise e transizioni
   attese sono più affidabili del confronto visivo tra implementazioni.
4. **Mantenere intercambiabili le strategie.** Baseline deterministiche,
   budget espliciti e benchmark riproducibili consentono confronti AI utili.
5. **Validare progressivamente.** Analisi statica, test unitari, parità,
   build, artefatti CI e prove su dispositivo verificano proprietà diverse.
6. **Trattare gli artefatti CI come evidenze.** Verificare integrità,
   metadata, firma e provenienza, non soltanto il colore verde del job.
7. **Distinguere debug e release.** Un APK debug funzionante non dimostra
   la prontezza produttiva. Lint e packaging release richiedono gate autonomi.
8. **Non aggirare i controlli per ottenere una build verde.** Una dipendenza
   assente nella cache offline non dimostra un difetto del sorgente.
   Diagnosticare la risoluzione prima di disabilitare i controlli.
9. **Rispettare i confini del progetto.** Toolchain, output di build,
   fonti private e documentazione pubblicabile hanno destinazioni distinte.
10. **Distinguere dimostrato e pianificato.** Una partita riuscita su
    dispositivo reale dimostra funzionalità di base, non production readiness,
    accessibilità, firma release o copertura del ciclo di vita.

## Milestone verificate

- Motore Python e validazione delle strategie.
- Parità del dominio Kotlin e controller indipendente dalla piattaforma.
- Applicazione Android Compose e partita completa su dispositivo reale.
- APK debug e release unsigned prodotti da GitHub Actions.
- Lint release e verifica dell'APK non firmato.

## Gate produttivi ancora aperti

Test Android automatizzati, firma release e gestione delle chiavi,
verifiche del ciclo di vita e accessibilità, policy di distribuzione
e preparazione alla pubblicazione restano attività future distinte.
