# Repository hygiene

[English](REPO-HYGIENE.md) | [Italiano](REPO-HYGIENE.it.md)

## Purpose

BoardLab is a public software and learning repository.

Its Git history must contain only material that is appropriate to publish,
reviewable, and compatible with the rights of game designers, publishers,
artists, authors, and other source owners.

## Public content policy

The repository may contain:

- original source code;
- original study notes and explanations;
- original diagrams and examples;
- original games and game states created for BoardLab;
- original exercises, quizzes, and answer keys;
- original analyses and comparisons;
- bibliographic and source references;
- small quotations when genuinely necessary for commentary and attribution.

The repository must not become a substitute for a commercial game, rulebook,
book, magazine, or other protected source.

## Board-game source boundaries

Do not commit material such as:

- commercial rulebook PDFs;
- scans or photographs of manuals;
- scans or photographs of protected game components;
- card, board, or packaging artwork;
- substantial collections of card text;
- integral or substantial copied rules;
- substantial translations of protected rulebooks;
- source transcripts;
- purchased or privately supplied editorial material.

References to commercial games should identify the relevant source or concept
without reproducing enough protected material to replace the original.

## Original images and diagrams

Image files are not forbidden by extension alone.

Original BoardLab diagrams, charts, screenshots of BoardLab itself, and other
publication-safe images may be tracked when their origin and publication rights
are clear.

Protected artwork, scans, photographs, or extracted assets from commercial
games must not be published merely because their file format is allowed.

## Private source material

Private study material must remain outside the public Git history.

The preferred local location is:

`sources/private/`

This path is ignored by Git and must never become tracked.

## Bilingual learning documentation

New structured learning material should normally use English as the canonical
language with an Italian `.it.md` counterpart.

Commands, paths, identifiers, APIs, filenames, code, and technical meaning must
remain aligned across each bilingual pair.

Existing technical documentation does not need to be translated retroactively
unless deliberately included in a migration scope.

## Preparation versus study

Repository preparation and active study are separate states.

A session may be marked **Prepared** only after its learning path, assessments,
navigation, publication checks, and repository-wide validation are complete.

Prepared does not mean Studied.

Learner exercises, quiz attempts, reviewed answers, game analyses, and similar
activities must only be marked complete after they have actually happened.

## Automated checks

Repository validation should eventually cover both software quality and public
learning-material quality.

Software checks include:

- Ruff formatting;
- Ruff linting;
- mypy strict type checking;
- pytest.

Repository and learning-material checks include:

- bilingual document classification and synchronization;
- private-source detection;
- unsafe public-content detection;
- quiz and answer-key separation;
- expected learning-path inventory;
- navigation and link consistency;
- whitespace consistency.

Automated controls reduce accidental publication risk but do not replace human
review of copyright, privacy, licensing, or attribution.

## Before committing

Before a preparation commit:

1. run the complete repository validation;
2. inspect tracked, untracked, and staged files;
3. verify that private or protected source material is absent;
4. run `git diff --check`;
5. review the staged diff before committing.
