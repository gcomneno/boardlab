/**
 * Narrative football roster.
 *
 * Shirt numbers are unique within each team. A player who leaves
 * the pitch never becomes eligible to play again.
 *
 * This module does not affect the dice engine or scoring.
 */

function pickIndex(length, random) {
  const value = random();
  if (typeof value !== "number" ||
      !Number.isFinite(value) ||
      value < 0 || value >= 1) {
    throw new RangeError("Invalid random value");
  }
  return Math.floor(value * length);
}

export function createRoster(random = Math.random) {
  const numbers = Array.from({ length: 99 }, (_, i) => i + 1);

  for (let i = numbers.length - 1; i > 0; i -= 1) {
    const j = pickIndex(i + 1, random);
    [numbers[i], numbers[j]] = [numbers[j], numbers[i]];
  }

  return {
    active: numbers.slice(0, 11),
    bench: numbers.slice(11, 20),
    departed: [],
    sentOff: [],
    substitutions: 0
  };
}

export function substitute(roster, random = Math.random) {
  if (roster.substitutions >= 5 ||
      roster.active.length === 0 ||
      roster.bench.length === 0) {
    return null;
  }

  const outgoing = pickIndex(roster.active.length, random);
  const incoming = pickIndex(roster.bench.length, random);

  const out = roster.active[outgoing];
  const into = roster.bench[incoming];

  roster.active.splice(outgoing, 1, into);
  roster.bench.splice(incoming, 1);
  roster.departed.push(out);
  roster.substitutions += 1;

  return { out, in: into };
}

export function sendOff(roster, random = Math.random) {
  if (!roster.active.length) return null;

  const index = pickIndex(roster.active.length, random);
  const number = roster.active.splice(index, 1)[0];
  roster.sentOff.push(number);

  return number;
}
