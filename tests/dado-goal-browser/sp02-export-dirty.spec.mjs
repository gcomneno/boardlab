import { test, expect } from "@playwright/test";

const GAME_URL = "http://127.0.0.1:4173/web/dado-goal/";

test.beforeEach(async ({ page }) => {
  await page.goto(GAME_URL);
});

async function downloadGlobalSave(page) {
  const pending = page.waitForEvent("download");
  await page.locator("#game-export").evaluate(button => button.click());
  return await pending;
}

test("7C fresh reload is dirty and unchanged export becomes clean", async ({ page }) => {
  const exportButton = page.locator("#game-export");
  await expect(exportButton).toBeEnabled();

  await downloadGlobalSave(page);
  await expect(page.locator("#game-save-status")).toHaveText("Salvataggio creato.");
  await expect(exportButton).toBeDisabled();

  await page.reload();
  await expect(exportButton).toBeEnabled();

  await downloadGlobalSave(page);
  await expect(exportButton).toBeDisabled();

  await page.locator("#hands").evaluate(element => {
    element.value = element.value === "2" ? "3" : "2";
    element.dispatchEvent(new Event("input", { bubbles: true }));
  });
  await expect(exportButton).toBeEnabled();
});

test("7C mutation during download dispatch cannot clean a newer draft", async ({ page }) => {
  await page.evaluate(() => {
    const originalClick = HTMLAnchorElement.prototype.click;
    HTMLAnchorElement.prototype.click = function (...args) {
      const control = document.querySelector("#hands");
      control.value = control.value === "2" ? "3" : "2";
      control.dispatchEvent(new Event("input", { bubbles: true }));
      return originalClick.apply(this, args);
    };
  });

  await downloadGlobalSave(page);

  await expect(page.locator("#game-save-status"))
    .toHaveText("Salvataggio creato; sono presenti modifiche successive.");
  await expect(page.locator("#game-export")).toBeEnabled();
});

test("7C foreign persistent change invalidates a clean export", async ({ page, context }) => {
  await downloadGlobalSave(page);
  await expect(page.locator("#game-export")).toBeDisabled();

  const foreignPage = await context.newPage();
  try {
    await foreignPage.goto(GAME_URL);
    await foreignPage.evaluate(() => {
      localStorage.setItem(
        "dado-goal.team-management.v1",
        JSON.stringify({ schemaVersion: 1, matches: [], ratings: [] }),
      );
    });

    await expect(page.locator("#game-export")).toBeEnabled();
  } finally {
    await foreignPage.close();
  }
});

test("7C failed download preparation retains dirty state", async ({ page }) => {
  await page.evaluate(() => {
    URL.createObjectURL = () => {
      throw new Error("TEST_DOWNLOAD_PREPARATION_FAILED");
    };
  });

  page.on("dialog", async dialog => {
    await dialog.dismiss();
  });

  await page.locator("#game-export").evaluate(button => button.click());

  await expect(page.locator("#game-save-status"))
    .toHaveText("Esportazione non completata.");
  await expect(page.locator("#game-export")).toBeEnabled();
});
