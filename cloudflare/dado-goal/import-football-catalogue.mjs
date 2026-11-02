import { createApiFootballAdapter } from "../../web/dado-goal/providers/api-football.mjs";
import { importFootballCatalogue } from "../../web/dado-goal/football-provider.mjs";
import { putFootballCatalogue } from "./football-catalogue-d1.mjs";

const DEFAULT_REQUEST_INTERVAL_MS = 6500;

function sleep(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

export async function buildFootballCatalogue({
  apiKey,
  competitionId,
  season,
  fetchImpl = globalThis.fetch,
  wait = sleep,
  requestIntervalMs = DEFAULT_REQUEST_INTERVAL_MS,
}) {
  let firstRequest = true;

  const pacedFetch = async (...args) => {
    if (!firstRequest) {
      await wait(requestIntervalMs);
    }
    firstRequest = false;
    return fetchImpl(...args);
  };

  const adapter = createApiFootballAdapter({
    apiKey,
    fetchImpl: pacedFetch,
  });

  return importFootballCatalogue({
    adapter,
    competitionId,
    season,
  });
}


export async function importAndPersistFootballCatalogue({
  db,
  ...options
}) {
  const catalogue = await buildFootballCatalogue(options);
  await putFootballCatalogue(db, catalogue);
  return catalogue;
}
