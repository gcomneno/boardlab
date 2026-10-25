import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import {
  initial, validate, legalMoves, applyMove, territoryScore,
  outcome, chooseMove, terminal, player
} from "./engine.mjs";

const fixture = JSON.parse(
  readFileSync("tests/fixtures/tre_sigilli/v1.json", "utf8")
);

assert.equal(fixture.schema_version, 1);
assert.equal(fixture.game, "tre_sigilli");
assert.deepEqual(initial(), fixture.cases[0].state);

let checks = 0;
for (const c of fixture.cases) {
  validate(c.state);
  assert.deepEqual(legalMoves(c.state), c.legal_moves, c.id + " legal");
  checks++;
  assert.equal(terminal(c.state), c.terminal, c.id + " terminal");
  checks++;
  assert.equal(c.terminal ? null : player(c.state), c.active_player, c.id + " player");
  checks++;
  assert.deepEqual(territoryScore(c.state), c.territory_scores, c.id + " territory");
  checks++;

  const expected = c.outcomes === null ? null :
    c.outcomes.north === "draw" ? "draw" :
    c.outcomes.north === "win" ? "north" : "south";
  assert.equal(outcome(c.state), expected, c.id + " outcome");
  checks++;

  if (c.ai_depth_2_move !== null) {
    assert.deepEqual(chooseMove(c.state, 2), c.ai_depth_2_move, c.id + " AI");
    checks++;
  }
  for (const t of c.transitions) {
    assert.deepEqual(applyMove(c.state, t.move), t.next_state, c.id + " transition");
    checks++;
  }
  for (const t of c.invalid_moves) {
    assert.throws(() => applyMove(c.state, t.move), /Illegal move/);
    checks++;
  }
}

console.log(`SHIP_04_FIXTURE_CASES=${fixture.cases.length}`);
console.log(`SHIP_04_PARITY_ASSERTIONS=${checks}`);
console.log("SHIP_04_ENGINE_PARITY_GATE=PASS");
