# Dado Goal — Original rules

[English](dado-goal.md) | [Italiano](dado-goal.it.md)

## Origin and authorship

Dado Goal is an original game invented by Giancarlo Cicellyn Comneno
during his childhood. The author has preserved the original handmade dice,
documented in photographs.

This reconstruction records rules confirmed directly by the author.
The original-dice photograph has not yet been acquired as a project asset;
no photograph is included by this change.

## Identity and scope

- Dado Goal Original: faithful reconstruction of the childhood game.
- Dado Goal Digital: software adaptation of the original rules.
- Dado Goal Evolution: explicitly identified new mechanics and variants.

Later innovations must not be presented as historical rules without the
author's confirmation. This document records rules, not a playable implementation.

## Components

Two players use four six-sided dice. Each player has a team die in their
chosen team colours. These two dice have identical face distributions.
Both players share the pink development die and the red/blue result die.

## Team dice

Each team die has the following six faces:

| Face | Count | Effect |
|---|---:|---|
| Tiro in porta! | 2 | Roll the pink die |
| Passaggio intercettato! | 2 | End the turn |
| Cross! | 1 | Roll the pink die |
| Passaggio | 1 | Same player rerolls their team die |

Face labels are preserved in Italian in both document versions.
Tiro in porta! and Cross! both require the pink die; neither skips it.

## Pink development die

Each listed result occupies one face.

| Face | Effect |
|---|---|
| Palla sul fondo | End the turn |
| Mezza rovesciata | Roll the result die |
| Palla alta sulla traversa | End the turn |
| Palla intercettata | End the turn |
| Rovesciata | Roll the result die |
| Colpo di testa | Roll the result die |

## Red/blue result die

Each listed result occupies one face.

| Face | Effect |
|---|---|
| Respinto! | End the turn without scoring |
| GOAL! | Add one goal for the active player and end the turn |
| FUORI! | End the turn without scoring |
| PALO/TRAVERSA | End the turn without scoring |
| RIGORE | Same player rerolls this result die |
| Parato! | End the turn without scoring |

Repeated RIGORE results cause repeated rolls of the result die.
Only GOAL! increases the score. There are no additional rebound or penalty
procedures in these confirmed rules.

## Starting player and alternation

Historically, the starting player was selected using the Neapolitan
"tocco": players showed a number of fingers, summed them, and counted
to select who started. The exact counting convention is not specified here.

Whenever a turn ends, play passes to the opponent, including after a goal.
Every new turn begins with that player's team die.
Passaggio and RIGORE rerolls remain within the same turn.

## Match length and result

Players agree on a positive number N of complete turns per team.
A turn counts even when it ends before the final die.
Individual rolls and rerolls are not additional turns.

After both players have completed N turns, the player with more goals wins.
Equal scores produce a draw, without a tie-break.

## Current championship scoring

The currently approved standings system awards three points for a win,
one for a draw, and zero for a loss. These standings points are separate
from goals scored during a match.

This is a current configuration choice; its historical use in the childhood
game has not been confirmed. Scheduling and standings tie-break criteria
are not defined by this document.

## Digital development boundaries

The intended browser location is alongside Tre Sigilli, at
`web/dado-goal`. This document does not create that implementation.

The original rules contain no strategic decisions during a turn.
Random simulation must not be presented as strategic AI.
BoardLab strategies must not be incorporated automatically.
Tre Sigilli and other games remain outside this change.

Future development must provide reproducible tests, preserve the confirmed
face multiplicities and transitions, and distinguish mathematical modelling
assumptions from historical facts. In particular, fair and independent rolls
would be a digital modelling assumption, not a measured property of the
handmade dice.

## Remaining reconstruction details

- Acquire the original-dice photograph as a visual reference.
- Confirm the exact tocco counting convention if reproducing it digitally.
- Do not infer the historical use of the current championship points system.
- Preserve any newly recalled rules as author-confirmed additions.

### Browser release smoke

Dado Goal has a small browser-level release smoke suite covering the
release-critical single-player and authoritative multiplayer surfaces.

Run it from the repository root with:

```bash
tests/dado-goal-browser/run-smoke.sh
```

The suite exercises the single-player page and a local multiplayer Worker +
D1 environment. It checks release-critical browser wiring such as match
startup, pitch/team crests, authoritative host/join state, roll authority,
state synchronization, statistics, and the stable multiplayer entry point.
Deterministic lower-level tests retain detailed coverage for authoritative
event identity and scored-penalty de-duplication.

The browser smoke suite is an automated release guard only. It does **not**
replace the mandatory manual/operational Dado Goal road test, which remains a
required gate before every publication or release.

### Public distribution validation

After publishing or changing a Dado Goal release, itch.io build, multiplayer
endpoint, or profile/showcase reference, run:

```bash
scripts/check-dado-goal-distribution.py
```

The read-only validator uses
`docs/games/dado-goal/distribution-contract.json` as the canonical distribution
contract and checks repository references, GitHub release state and targets,
the itch.io browser build, the English and Italian public showcase, and the
stable multiplayer Worker entry point and required client assets.

For repository-local checks without external requests:

```bash
scripts/check-dado-goal-distribution.py --offline
```

A full PASS answers that the maintained public Dado Goal entry points are
consistent with the current distribution contract. This post-publication
validation complements, and does not replace, the mandatory manual/operational
road test required before publication or release.
