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
    substitutesIn: [],
    substitutions: 0
  };
}

export function substitute(
  roster,
  random = Math.random,
  { allowSubstituteOut = false } = {},
) {
  if (roster.substitutions >= 5 ||
      roster.active.length === 0 ||
      roster.bench.length === 0) {
    return null;
  }

  const eligibleOutgoing = allowSubstituteOut
    ? roster.active
    : roster.active.filter(
        number => !roster.substitutesIn.includes(number)
      );

  if (eligibleOutgoing.length === 0) return null;

  const out = eligibleOutgoing[pickIndex(eligibleOutgoing.length, random)];
  const outgoingPosition = roster.identities?.[out]?.position;
  const eligibleIncoming = outgoingPosition === "Goalkeeper"
    ? roster.bench.filter(
        number => roster.identities?.[number]?.position === "Goalkeeper"
      )
    : roster.bench;

  if (eligibleIncoming.length === 0) return null;

  const into = eligibleIncoming[pickIndex(eligibleIncoming.length, random)];
  const incoming = roster.bench.indexOf(into);
  const outgoing = roster.active.indexOf(out);

  roster.active.splice(outgoing, 1, into);
  roster.bench.splice(incoming, 1);
  roster.departed.push(out);
  roster.substitutesIn.push(into);
  roster.substitutions += 1;

  return { out, in: into };
}

/**
 * Pick a goalscorer from players currently on the pitch.
 * The selected shirt number is stored with the goal event.
 */
export function pickScorer(roster, random = Math.random, minute = 0) {
  if (!roster || !Array.isArray(roster.active) ||
      roster.active.length === 0) {
    return null;
  }

  const identities = roster.identities;
  if (!identities) {
    return roster.active[pickIndex(roster.active.length, random)];
  }

  const goalkeepers = roster.active.filter(
    number => identities[number]?.position === "Goalkeeper"
  );
  const outfield = roster.active.filter(
    number => identities[number]?.position !== "Goalkeeper"
  );

  if (outfield.length === 0) {
    return roster.active[pickIndex(roster.active.length, random)];
  }

  if (minute >= 85 && goalkeepers.length > 0 && random() < 0.01) {
    return goalkeepers[pickIndex(goalkeepers.length, random)];
  }

  return outfield[pickIndex(outfield.length, random)];
}

export function sendOff(roster, random = Math.random) {
  if (!roster.active.length) return null;

  const index = pickIndex(roster.active.length, random);
  const number = roster.active.splice(index, 1)[0];
  roster.sentOff.push(number);

  return number;
}
