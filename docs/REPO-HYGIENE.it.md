# Igiene del repository

[English](REPO-HYGIENE.md) | [Italiano](REPO-HYGIENE.it.md)

## Scopo

BoardLab è un repository pubblico di software e apprendimento.

La sua cronologia Git deve contenere soltanto materiale appropriato alla
pubblicazione, revisionabile e compatibile con i diritti di game designer,
editori, artisti, autori e altri proprietari delle fonti.

## Politica dei contenuti pubblici

Il repository può contenere:

- codice sorgente originale;
- appunti ed elaborazioni didattiche originali;
- diagrammi ed esempi originali;
- giochi e stati di gioco originali creati per BoardLab;
- esercizi, quiz e answer key originali;
- analisi e comparazioni originali;
- riferimenti bibliografici e alle fonti;
- brevi citazioni quando realmente necessarie per commento e attribuzione.

Il repository non deve diventare un sostituto di un gioco commerciale, di un
regolamento, di un libro, di una rivista o di un'altra fonte protetta.

## Confini delle fonti relative ai giochi da tavolo

Non committare materiale come:

- PDF di regolamenti commerciali;
- scansioni o fotografie di manuali;
- scansioni o fotografie di componenti di gioco protetti;
- artwork di carte, tabelloni o confezioni;
- raccolte sostanziali di testi delle carte;
- regolamenti copiati integralmente o in parte sostanziale;
- traduzioni sostanziali di regolamenti protetti;
- trascrizioni delle fonti;
- materiale editoriale acquistato o fornito privatamente.

I riferimenti a giochi commerciali devono identificare la fonte o il concetto
rilevante senza riprodurre abbastanza materiale protetto da sostituire
l'originale.

## Immagini e diagrammi originali

I file immagine non sono vietati soltanto in base alla loro estensione.

Diagrammi originali BoardLab, grafici, screenshot di BoardLab stesso e altre
immagini pubblicabili possono essere tracciati quando origine e diritti di
pubblicazione sono chiari.

Artwork protetto, scansioni, fotografie o asset estratti da giochi commerciali
non devono essere pubblicati soltanto perché il loro formato è consentito.

## Materiale sorgente privato

Il materiale di studio privato deve restare fuori dalla cronologia Git
pubblica.

La posizione locale preferita è:

`sources/private/`

Questo percorso è ignorato da Git e non deve mai diventare tracciato.

## Documentazione didattica bilingue

Il nuovo materiale didattico strutturato dovrebbe normalmente usare l'inglese
come lingua canonica con una controparte italiana `.it.md`.

Comandi, percorsi, identificatori, API, nomi dei file, codice e significato
tecnico devono rimanere allineati all'interno di ogni coppia bilingue.

La documentazione tecnica esistente non deve essere tradotta retroattivamente,
salvo che venga inclusa deliberatamente in uno specifico ambito di migrazione.

## Preparazione e studio

La preparazione del repository e lo studio attivo sono stati separati.

Una sessione può essere marcata **Prepared** soltanto dopo che percorso
didattico, assessment, navigazione, controlli di pubblicazione e validazione
repository-wide sono completi.

Prepared non significa Studied.

Esercizi del learner, tentativi dei quiz, risposte revisionate, analisi di
partite e attività analoghe devono essere marcati completati soltanto dopo che
sono realmente avvenuti.

## Controlli automatici

La validazione del repository dovrà coprire sia la qualità del software sia
quella del materiale didattico pubblico.

I controlli software includono:

- formattazione Ruff;
- linting Ruff;
- controllo tipi mypy strict;
- pytest.

I controlli del repository e del materiale didattico includono:

- classificazione e sincronizzazione della documentazione bilingue;
- rilevamento di fonti private;
- rilevamento di contenuti pubblici non sicuri;
- separazione tra quiz e answer key;
- inventario atteso del percorso didattico;
- coerenza della navigazione e dei link;
- coerenza del whitespace.

I controlli automatici riducono il rischio di pubblicazioni accidentali ma non
sostituiscono la revisione umana di copyright, privacy, licenze o attribuzione.

## Prima del commit

Prima di un commit di preparazione:

1. eseguire la validazione completa del repository;
2. ispezionare file tracciati, untracked e staged;
3. verificare l'assenza di fonti private o protette;
4. eseguire `git diff --check`;
5. revisionare il diff staged prima del commit.
