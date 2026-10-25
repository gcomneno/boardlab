import { createStrategy } from "./strategies.mjs";
import {
  analyzeMove, explainMoveItalian
} from "./guided-analysis.mjs";

let strategy = null;

// The tutor is independent from the configured opponent.
// It has no mutable random state.
const tutor = createStrategy({
  algorithm: "alpha_beta",
  parameters: { depth: 2 }
});

self.onmessage = ({ data }) => {
  const id = data?.id;

  try {
    if (!data || !Number.isSafeInteger(id) || id < 0) {
      throw new Error("Invalid request identifier");
    }

    if (data.type === "configure") {
      strategy = createStrategy(data.config);
      self.postMessage({
        id,
        type: "configured",
        config: strategy.config
      });
      return;
    }

    if (data.type === "choose") {
      if (strategy === null) {
        throw new Error("Strategy not configured");
      }

      const move = strategy.choose(data.state);
      self.postMessage({ id, type: "move", move });
      return;
    }

    if (data.type === "suggest") {
      const move = tutor.choose(data.state);
      const analysis = analyzeMove(data.state, move);

      self.postMessage({
        id,
        type: "suggestion",
        move,
        analysis,
        explanation: explainMoveItalian(analysis)
      });
      return;
    }

    throw new Error("Unknown worker command");
  } catch (error) {
    self.postMessage({
      id: Number.isSafeInteger(id) ? id : null,
      type: "error",
      message: error instanceof Error
        ? error.message
        : "Unknown worker error"
    });
  }
};
