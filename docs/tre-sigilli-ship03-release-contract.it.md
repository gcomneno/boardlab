# Tre Sigilli — SHIP-03 · Android Release Readiness

Languages: [English](tre-sigilli-ship03-release-contract.md) | [Italiano](tre-sigilli-ship03-release-contract.it.md)

Status: APPROVED — scope authorized; signing key generation requires separate authorization.

## Obiettivo

Qualificare una prima release Android Free, firmata, installabile
e verificata offline, senza pubblicazione automatica sul Play Store.

SHIP-02 rimane la baseline funzionale e visiva approvata.

## M1 — Release baseline

- Verificare applicationId, manifest, versionCode e versionName.
- Verificare il job CI `assembleRelease` e il suo artefatto unsigned.
- Verificare gli strumenti Android necessari alla release.
- Accertare l'assenza di credenziali e chiavi di firma tracciate.

Gate: M1_BASELINE=PASS.

## M2 — Firma locale sicura

- Definire una posizione della chiave fuori dal repository.
- Proteggere preventivamente i percorsi sensibili con regole Git.
- Creare o selezionare un'identità di firma soltanto dopo autorizzazione.
- Evitare password in shell history, sorgenti, log e GitHub Actions.
- Firmare e verificare l'APK con strumenti Android ufficiali.
- Definire recupero e backup della chiave prima della distribuzione.

Gate: M2_SIGNING=PASS.

## M3 — Collaudo release

- Installare l'APK release firmato nell'AVD qualificato.
- Verificare avvio, partita completa, modalità offline, AI e reset.
- Verificare persistenza e ripristino della sessione.
- Mantenere la suite Android SHIP-02 come baseline di regressione.
- Smontare correttamente lo storage Samsung.

Gate: M3_RUNTIME=PASS.

## M4 — Packaging

- Preparare APK qualificato, checksum SHA-256 e manifest artefatti.
- Documentare installazione, compatibilità e limitazioni.
- Tenere separati artefatti distribuibili e materiale di firma.
- Non pubblicare artefatti senza autorizzazione esplicita.

Gate: M4_PACKAGE=PASS.

## M5 — Sigillo finale

- Verificare risultati di build, test, firma e integrità artefatti.
- Revisionare il perimetro delle modifiche.
- Registrare commit e stato della release.
- Effettuare push solo dopo autorizzazione.

Gate: SHIP_03_SEAL=PASS.

## Esclusioni vincolanti

- Nessuna modifica alla skin Free approvata in SHIP-02.
- Nessun cambiamento delle regole o dell'AI.
- Niente Premium, acquisti in-app, pubblicità o telemetria.
- Nessun account, cloud o requisito di connettività.
- Nessuna pubblicazione sul Play Store.
- Nessuna generazione di chiavi durante la fase documentale.

## Stato iniziale verificato

- applicationId: com.giadaware.tresigilli
- minSdk: 26
- targetSdk e compileSdk: 36
- versionCode: 1
- versionName: 0.1.0
- CI: job unsigned release esistente
- Test SHIP-02: 8/8 PASS
- Firma release: da predisporre
- Repository GitHub: pubblico

## Autorizzazioni

Il perimetro SHIP-03 M1–M5 è stato approvato il 24 ottobre 2026.
È autorizzata la pianificazione e l'attuazione delle milestone
nel rispetto dei rispettivi gate e delle esclusioni.
La generazione o selezione della chiave di firma richiede
un'autorizzazione separata. Pubblicazione e distribuzione
richiedono autorizzazioni esplicite.
