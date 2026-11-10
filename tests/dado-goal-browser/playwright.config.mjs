import { defineConfig } from "@playwright/test";

const root = new URL("../../", import.meta.url).pathname;

export default defineConfig({
  testDir: ".",
  testMatch: ["*.spec.mjs"],
  fullyParallel: false,
  workers: 1,
  retries: 0,
  timeout: 30_000,
  expect: {
    timeout: 5_000,
  },
  reporter: [["line"]],
  use: {
    headless: true,
    launchOptions: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH
      ? { executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH }
      : {},
    trace: "retain-on-failure",
    screenshot: "only-on-failure",
  },
  webServer: [
    {
      command: "python3 -m http.server 4173 --bind 127.0.0.1",
      cwd: root,
      url: "http://127.0.0.1:4173/web/dado-goal/",
      reuseExistingServer: !process.env.CI,
      timeout: 15_000,
    },
  ],
});
