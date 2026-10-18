# CLI

Interfaccia iniziale per eseguire partite, esperimenti e benchmark.

La CLI utilizza il dominio, ma il dominio non dipende dalla CLI.

## Tre Sigilli — Human vs AI

Avvio dalla radice del repository:

```bash
uv run --no-sync python -m boardlab.cli.tre_sigilli
```

Selezionare `N` (NORTH, primo giocatore) oppure `S` (SOUTH).

L'avversario utilizza Alpha-Beta con profondità 2.

La scacchiera è 4×4, con celle numerate da `00` a `15`.

- Turni 0–5: inserire una cella libera, per esempio `6`.
- Turni 6–17: inserire origine e destinazione, per esempio `11 7`.
- Le mosse illegali non consumano il turno.
- `Ctrl+C` interrompe la partita.

Al termine vengono mostrati i punteggi territoriali e il risultato.

La CLI non salva le partite.
