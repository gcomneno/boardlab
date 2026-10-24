# Tre Sigilli — SHIP-03 · Custodia della firma Android

Languages: [English](tre-sigilli-ship03-signing-custody.md) | [Italiano](tre-sigilli-ship03-signing-custody.it.md)

Status: DRAFT — nessuna chiave generata.

## Identità di firma

La chiave Android release identifica le versioni distribuibili
di Tre Sigilli. Deve rimanere stabile per gli aggiornamenti
firmati con la medesima identità.

La perdita della chiave può impedire gli aggiornamenti
delle installazioni già distribuite.

## Posizioni canoniche

Materiale di firma:
`~/.local/share/giadaware/tre-sigilli/signing/`

Configurazione riservata:
`~/.config/giadaware/tre-sigilli/`

Entrambe le posizioni sono esterne al repository BoardLab.

Non memorizzare chiavi o password in repository, documentazione
pubblica, workflow CI, cronologia shell o log.

## Permessi e isolamento

- Directory private: 0700.
- Keystore e configurazioni riservate: 0600.
- Nessun collegamento simbolico verso repository o percorsi pubblici.
- Nessuna copia del keystore sotto `android/`.
- Nessuna password passata come argomento in chiaro.
- `.gitignore` costituisce una difesa aggiuntiva, non primaria.

## Backup e recupero

Prima di distribuire il primo APK firmato:

1. Predisporre almeno una copia di recupero cifrata,
   custodita separatamente dal sistema principale.
2. Conservare separatamente le credenziali necessarie
   all'apertura della copia.
3. Verificare che sia possibile leggere il backup
   senza sostituire o alterare la chiave originale.
4. Registrare l'impronta pubblica del certificato
   senza esporre materiale privato.
5. Documentare la procedura di ripristino e verificare
   la disponibilità del supporto di recupero.

Il supporto di backup deve essere concordato prima
della generazione definitiva della chiave.

## Firma e verifica

- Usare gli strumenti Android SDK ufficiali.
- Verificare l'APK con `apksigner verify`.
- Registrare SHA-256 dell'APK e impronta del certificato.
- Verificare installazione e aggiornamento sull'AVD qualificato.
- Conservare separati APK distribuibili, chiavi e password.

## Autorizzazioni e gate

La protezione Git e la presente politica non autorizzano
la generazione della chiave.

`SIGNING_CUSTODY_POLICY=PASS` richiede revisione del documento.

`KEY_GENERATION_GATE` richiede autorizzazione esplicita.

`M2_SIGNING=PASS` richiede firma verificata, custodia protetta
e piano di recupero qualificato.

Nessun APK viene pubblicato o distribuito automaticamente.
