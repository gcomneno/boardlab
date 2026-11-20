/*
 * M034-V29 — isolated Cloudflare V2 rules adapter.
 *
 * Not connected to Worker routes, D1 or the V1 authority.
 * All outcome decisions are server-side and returned as
 * replayable canonical V2 events.
 */

import {
  applyV2Event,
  rollV2,
} from "../../web/dado-goal/engine-v2.mjs";
import {
  normalizeV2Event,
} from "../../web/dado-goal/roll-event-v2.mjs";

function cryptoByte() {
  const cryptoApi = globalThis.crypto;

  if (!cryptoApi ||
      typeof cryptoApi.getRandomValues !== "function") {
    throw new Error("Secure random source unavailable");
  }

  const bytes = new Uint8Array(1);
  cryptoApi.getRandomValues(bytes);
  return bytes[0];
}

function fairDraw(sides, byteSource) {
  if (!Number.isInteger(sides) ||
      sides < 1 || sides > 256 ||
      typeof byteSource !== "function") {
    throw new RangeError("Invalid uniform draw configuration");
  }

  const limit = Math.floor(256 / sides) * sides;

  for (;;) {
    const byte = byteSource();

    if (!Number.isInteger(byte) ||
        byte < 0 || byte > 255) {
      throw new RangeError("Invalid random byte");
    }

    if (byte < limit) {
      return byte % sides;
    }
  }
}

/*
 * A result-phase action first draws 1 of 40 award slots.
 * When no penalty is awarded, it then draws a D6 face.
 *
 * Every other phase draws only its D6 face.
 *
 * Injecting byteSource permits deterministic native tests.
 * The production default uses the server's Web Crypto API.
 */
export function rollServerV2(state, byteSource = cryptoByte) {
  let drawNumber = 0;

  const random = () => {
    const sides = state.phase === "result" &&
      drawNumber === 0 ? 40 : 6;

    drawNumber += 1;

    return fairDraw(sides, byteSource) / sides;
  };

  const { event, state: next } = rollV2(state, random);

  return Object.freeze({
    event: normalizeV2Event(event),
    state: next,
  });
}

export function replayServerV2(state, event) {
  return applyV2Event(state, normalizeV2Event(event));
}
