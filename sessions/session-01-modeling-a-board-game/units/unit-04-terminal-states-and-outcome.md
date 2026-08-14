# Unit 04 — Terminal states and outcome

[English](unit-04-terminal-states-and-outcome.md) | [Italiano](unit-04-terminal-states-and-outcome.it.md)

## Learning objectives

After studying this unit, the learner should be able to:

- distinguish an ongoing state from a terminal state;
- explain why terminal conditions belong to the game rules;
- distinguish game termination from strategic evaluation;
- reason about an outcome from a player's point of view;
- recognize inconsistent termination logic as a domain-model defect.

## Source references

Primary BoardLab reference:

- `docs/architecture/overview.md`

Relevant architectural statements:

- the conceptual game contract must allow terminal-state detection;
- a state must be evaluable from the point of view of a player;
- `GameState` represents a complete and valid game configuration;
- generic strategies must depend on the public game contract rather than on
  concrete game rules.

Supporting reference:

- `README.md`

BoardLab keeps game rules separate from generic strategy algorithms.

The explanations and examples below are original BoardLab teaching material.

## Mental model

Not every state permits another move.

Some states mean the game has finished.

Those are terminal states.

A useful distinction is:

Terminality
→ asks whether play has ended

Outcome
→ describes what the finished state means for the participants

Strategic evaluation
→ may estimate how desirable a state is, including states that are not terminal

These concepts are related but should not be collapsed into one idea.

## Technical explanation

A terminal condition is part of the game's rules.

Examples of generic terminal patterns include:

- a victory condition has been reached;
- a defeat condition has been reached;
- a draw condition has been reached;
- no further play is permitted;
- a game-specific stopping condition has occurred.

The generic engine should be able to ask whether the current state is terminal
without knowing the concrete rule that makes it terminal.

Similarly, a generic strategy should not hard-code those concrete stopping
rules.

The game domain is authoritative.

## Original example

Continue with the invented **Three Stones** game.

Suppose its original teaching rule is:

The game ends when all three spaces are occupied.

Consider this state:

- space 1: North;
- space 2: North;
- space 3: South;
- next player: South.

All spaces are occupied.

Under this invented rule, the state is terminal.

No additional placement move is legal.

Now suppose the outcome rule says:

The player owning more spaces wins.

From North's point of view, the outcome is a win.

From South's point of view, the same terminal state is a loss.

The state is identical.

The interpretation depends on the player perspective.

## Terminality is not evaluation

A future search algorithm may need to assign useful values to states.

That does not mean every evaluated state is terminal.

For example, an ongoing state could be considered strategically favorable to
North even though the game has not ended.

Therefore:

terminal-state detection
and
state evaluation

must not be treated as synonyms.

Terminality answers a rules question.

Evaluation answers a decision-support question.

This distinction becomes especially important before introducing algorithms
such as Minimax.

## Player perspective

An outcome often needs a player perspective.

The same terminal state can mean:

- win for one player;
- loss for another;
- draw for both.

BoardLab's architecture already states that evaluation must be possible from a
player's point of view.

This avoids hiding an arbitrary universal perspective inside the domain model.

## Common modeling mistakes

### Encoding terminal rules inside a strategy

A strategy checks directly whether a concrete game has been won.

Consequence:

the strategy becomes coupled to that game.

### Treating no legal moves as universally equivalent to game over

Some games may define that condition as terminal, while others may define a
pass, skip, or another rule.

Consequence:

a generic assumption replaces game-specific semantics.

### Treating evaluation as terminal detection

A state receives a high or low score, so the program assumes the game has
ended.

Consequence:

strategic desirability is confused with rule-defined completion.

### Omitting player perspective

A result is stored as though the same label had identical meaning for every
participant.

Consequence:

win and loss semantics become ambiguous.

## Problem scenario

A generic strategy contains this rule:

"If there are no legal moves, the game is over."

A future game added to BoardLab permits a player with no legal move to pass the
turn.

The generic strategy now terminates matches incorrectly.

The defect exists because a game-specific terminal assumption leaked into a
generic algorithm.

## Takeaway

Terminality is a rule-defined property of a `GameState`.

Outcome explains the meaning of a completed game, often from a particular
`Player` perspective.

Strategic evaluation is related but distinct and may also apply to non-terminal
states.

Keeping these concepts separate prepares BoardLab for later generic search
algorithms.

## Learner exercise

Using your original game:

1. define at least one terminal condition;
2. give one example of an ongoing state;
3. give one example of a terminal state;
4. describe the terminal outcome from each player's point of view;
5. describe one non-terminal state that might still look strategically better
   for one player than for the other.

Do not assign numeric evaluation scores yet.
