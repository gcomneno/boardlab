# Guida allo sviluppo

Questa guida permette di preparare e verificare BoardLab partendo da un clone
pulito del repository.

## Requisiti

- Git;
- Python 3.12 o successivo;
- `uv`.

Non sono richieste installazioni globali di Ruff, mypy o pytest.

## Preparazione iniziale

Dalla directory in cui si vogliono conservare i progetti:

```bash
git clone https://github.com/gcomneno/boardlab.git
cd boardlab
uv sync --group dev
```

`uv sync`:

1. crea `.venv` quando necessario;
2. installa BoardLab come package modificabile;
3. installa gli strumenti di sviluppo;
4. utilizza le versioni registrate in `uv.lock`.

## Verifica completa

Eseguire dalla radice del repository:

```bash
uv run ruff format --check .
uv run ruff check .
uv run mypy
uv run pytest
```

Tutti i comandi devono terminare con successo prima di creare un commit.

## Formattazione automatica

Quando Ruff segnala file non formattati:

```bash
uv run ruff format .
```

Dopo la formattazione, ripetere la verifica completa.

## Aggiornamento delle dipendenze

Per aggiornare intenzionalmente il lock file:

```bash
uv lock --upgrade
uv sync --group dev
```

Il diff di `uv.lock` deve essere revisionato e committato insieme alla modifica
che richiede l'aggiornamento.

## Aggiunta di una dipendenza di runtime

```bash
uv add nome-pacchetto
```

Le dipendenze di runtime devono essere necessarie al funzionamento di BoardLab,
non soltanto allo sviluppo.

## Aggiunta di una dipendenza di sviluppo

```bash
uv add --dev nome-pacchetto
```

Gli strumenti di test, linting, typing e manutenzione appartengono al gruppo di
sviluppo.

## Controllo prima del commit

```bash
git diff --check
git status -sb
uv run ruff format --check .
uv run ruff check .
uv run mypy
uv run pytest
```

`git diff` non mostra i file non tracciati. Usare sempre anche `git status`
prima di considerare completo il contenuto di un commit.
