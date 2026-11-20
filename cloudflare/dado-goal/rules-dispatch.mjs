/*
 * M034-V30 — isolated rules-version dispatcher.
 *
 * The version must originate from the authoritative persisted
 * match record, not a client-supplied request field.
 *
 * No Worker routes, D1 schema or V1 implementation are changed.
 */
import { rollServerV2 } from "./rules-v2.mjs";

export function resolveRulesVersion(persistedVersion) {
  if (persistedVersion === null ||
      persistedVersion === undefined) {
    return 1;
  }

  if (persistedVersion === 1 || persistedVersion === 2) {
    return persistedVersion;
  }

  throw new RangeError("Unsupported persisted rules version");
}

export function dispatchRulesRoll({
  persistedRulesVersion,
  state,
  rollLegacy,
  byteSource,
}) {
  const version = resolveRulesVersion(persistedRulesVersion);

  if (!state || typeof state !== "object" ||
      Array.isArray(state)) {
    throw new RangeError("Invalid match state");
  }

  if (version === 1) {
    if (state.rulesVersion !== undefined &&
        state.rulesVersion !== 1) {
      throw new RangeError("V1 state/version mismatch");
    }

    if (typeof rollLegacy !== "function") {
      throw new TypeError("Missing legacy roll handler");
    }

    return rollLegacy(state);
  }

  if (state.rulesVersion !== 2) {
    throw new RangeError("V2 state/version mismatch");
  }

  return rollServerV2(state, byteSource);
}
