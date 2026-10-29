"use strict";

const {app, BrowserWindow} = require("electron");
const path = require("node:path");

function createWindow() {
  const window = new BrowserWindow({
    width: 1100,
    height: 850,
    minWidth: 760,
    minHeight: 600,
    autoHideMenuBar: true,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true
    }
  });

  window.loadFile(path.join(__dirname, "game", "index.html"));

  window.webContents.setWindowOpenHandler(() => ({
    action: "deny"
  }));
}

app.whenReady().then(createWindow);

app.on("window-all-closed", () => {
  app.quit();
});
