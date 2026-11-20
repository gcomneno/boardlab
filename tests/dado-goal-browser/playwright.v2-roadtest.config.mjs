// Run with the installed Node 24 and Playwright, for example:
// /path/to/node24 node_modules/@playwright/test/cli.js test -c playwright.v2-roadtest.config.mjs
import { defineConfig } from "@playwright/test";
import { tmpdir } from "node:os";
import { join } from "node:path";

if (Number(process.versions.node.split(".")[0]) !== 24) {
  throw new Error("RT04 requires Node 24: invoke the installed Playwright CLI with Node 24 explicitly");
}

// The spec forks this entry point with Node 24 and --roadtest-v2. Using the
// RT03 handle gives the test local SQLite observability without an HTTP debug
// endpoint. Port 0 always creates a new exclusive 127.0.0.1 listener; there is
// no readiness probe that could accidentally adopt another running server.
if (process.argv.includes("--roadtest-v2")) {
  const args = process.argv.slice(2);
  const version = args.find(arg => /^--rules-version=[12]$/.test(arg));
  if (!process.send || args.length !== 2 || !version) {
    throw new Error("RT04 server entry requires IPC, --roadtest-v2 and --rules-version=1|2");
  }
  const { startRoadTestServer } = await import("./support/v2-roadtest-server.mjs");
  let handle;
  let stopping;
  const stop = () => stopping ??= (async () => {
    try { await handle?.stop(); }
    catch (error) { console.error(error); process.exitCode = 1; }
    finally { if (process.connected) process.disconnect(); }
  })();
  process.on("message", message => { if (message === "stop") void stop(); });
  process.on("SIGTERM", () => { void stop(); });
  process.on("SIGINT", () => { void stop(); });
  process.on("disconnect", () => { void stop(); });
  handle = await startRoadTestServer({ port: 0, rulesVersion: Number(version.at(-1)) });
  if (stopping) await handle.stop();
  else process.send({ origin: handle.origin, address: handle.address,
    databasePath: handle.databasePath, tempDir: handle.tempDir });
}

// Inherit one artifact directory in workers and child servers as well.
process.env.DADO_GOAL_RT04_OUTPUT_DIR ??= join(tmpdir(), `dado-goal-rt04-results-${process.pid}`);

export default defineConfig({
  testDir: ".",
  // Deliberately does not match the default configuration's *.spec.mjs.
  testMatch: ["v2-roadtest.roadtest.mjs"],
  fullyParallel: false,
  workers: 1,
  retries: 0,
  timeout: 90_000,
  expect: { timeout: 8_000 },
  outputDir: process.env.DADO_GOAL_RT04_OUTPUT_DIR,
  reporter: [["line"]],
  use: {
    browserName: "chromium",
    headless: true,
    launchOptions: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH
      ? { executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH } : {},
    trace: "retain-on-failure",
    screenshot: "only-on-failure",
  },
});
