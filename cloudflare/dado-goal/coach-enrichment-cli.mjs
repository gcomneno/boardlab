import {
  readFile,
  rename,
  writeFile,
} from "node:fs/promises";

import { importCatalogueJson } from "../../web/dado-goal/catalogue-json.mjs";
import {
  exportCoachEnrichmentCheckpoint,
  importCoachEnrichmentCheckpoint,
} from "../../web/dado-goal/coach-enrichment-checkpoint.mjs";
import {
  runCoachEnrichmentBatch,
} from "../../web/dado-goal/coach-enrichment-runner.mjs";
import {
  createApiFootballAdapter,
} from "../../web/dado-goal/providers/api-football.mjs";

function parseSecrets(source) {
  return Object.fromEntries(
    source
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter((line) => line && !line.startsWith("#"))
      .map((line) => {
        const separator = line.indexOf("=");
        return [
          line.slice(0, separator).trim(),
          line.slice(separator + 1).trim(),
        ];
      }),
  );
}

export async function runCoachEnrichmentCli({
  catalogueFile,
  checkpointFile,
  requestBudget,
  secretsFile = ".env.secrets",
  fetchImpl = globalThis.fetch,
  requestIntervalMs = 6500,
  wait,
}) {
  if (!Number.isInteger(requestBudget) || requestBudget < 1) {
    throw new TypeError("requestBudget must be a positive integer");
  }

  const catalogue = importCatalogueJson(
    await readFile(catalogueFile, "utf8"),
  );

  let checkpoint = null;

  try {
    checkpoint = importCoachEnrichmentCheckpoint(
      await readFile(checkpointFile, "utf8"),
    );
  } catch (error) {
    if (error?.code !== "ENOENT") {
      throw error;
    }
  }

  const secrets = parseSecrets(await readFile(secretsFile, "utf8"));
  const apiKey = secrets.API_FOOTBALL_KEY;

  if (!apiKey) {
    throw new Error("API_FOOTBALL_KEY missing from secrets file");
  }

  const adapter = createApiFootballAdapter({
    apiKey,
    fetchImpl,
  });

  const result = await runCoachEnrichmentBatch({
    catalogue,
    adapter,
    requestBudget,
    checkpoint,
    requestIntervalMs,
    ...(wait === undefined ? {} : { wait }),
  });

  const temporaryFile = `${checkpointFile}.tmp`;

  await writeFile(
    temporaryFile,
    exportCoachEnrichmentCheckpoint(result.checkpoint),
    { flag: "w" },
  );
  await rename(temporaryFile, checkpointFile);

  return result;
}
