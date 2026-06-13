import { defineConfig, devices } from "@playwright/test";

/**
 * Full e2e/visual test environment. Auto-starts the preview server, runs specs in
 * tests/e2e against a real Chromium, captures screenshots + video on failure.
 */
export default defineConfig({
  testDir: "./tests/e2e",
  fullyParallel: false,
  workers: 1, // GL-heavy specs: run serially so concurrent WebGL contexts don't contend
  forbidOnly: !!process.env.CI,
  retries: 1,
  reporter: [["list"]],
  outputDir: "test-results",
  use: {
    baseURL: "http://localhost:4173",
    viewport: { width: 1280, height: 800 },
    screenshot: "only-on-failure",
    trace: "retain-on-failure",
  },
  projects: [{ name: "chromium", use: { ...devices["Desktop Chrome"] } }],
  webServer: {
    command: "npm run build && npm run preview",
    url: "http://localhost:4173",
    reuseExistingServer: !process.env.CI,
    timeout: 120000,
  },
});
