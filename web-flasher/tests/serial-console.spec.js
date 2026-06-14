const { test, expect } = require("@playwright/test");

test.beforeEach(async ({ page }) => {
  await page.addInitScript(() => {
    const encoder = new TextEncoder();
    let ports = [];
    const serialListeners = { connect: [], disconnect: [] };

    function makeReadablePort(chunks, options = {}) {
      let readCount = 0;
      return {
        getInfo: () => ({ usbVendorId: 12346, usbProductId: 4097 }),
        open: async () => {},
        close: async () => {},
        setSignals: async () => {
          throw new Error("setSignals should not be called");
        },
        addEventListener: () => {},
        writable: {
          getWriter: () => ({
            write: async () => {},
            releaseLock: () => {},
          }),
        },
        readable: {
          getReader: () => ({
            read: async () => {
              readCount += 1;
              if (readCount <= chunks.length) {
                return { value: encoder.encode(chunks[readCount - 1]), done: false };
              }
              if (options.throwAfterChunks) {
                throw new Error("The device has been lost.");
              }
              await new Promise((resolve) => setTimeout(resolve, 10000));
              return { done: true };
            },
            releaseLock: () => {},
            cancel: async () => {},
          }),
        },
      };
    }

    function makeFloodPort(lineCount) {
      let readCount = 0;
      return {
        getInfo: () => ({ usbVendorId: 12346, usbProductId: 4097 }),
        open: async () => {},
        close: async () => {},
        addEventListener: () => {},
        writable: { getWriter: () => ({ write: async () => {}, releaseLock: () => {} }) },
        readable: {
          getReader: () => ({
            read: async () => {
              readCount += 1;
              if (readCount <= lineCount) {
                return { value: encoder.encode(`flood line ${readCount}\n`), done: false };
              }
              if (readCount === lineCount + 1) {
                return { value: encoder.encode("LAST FLOOD LINE\n"), done: false };
              }
              await new Promise((resolve) => setTimeout(resolve, 10000));
              return { done: true };
            },
            releaseLock: () => {},
            cancel: async () => {},
          }),
        },
      };
    }

    const firstPort = makeReadablePort(["boot log\n"]);
    const disconnectingPort = makeReadablePort(["before reset\n"], { throwAfterChunks: true });
    const returnedPort = makeReadablePort(["after reset\n"]);
    const floodPort = makeFloodPort(30000);

    Object.defineProperty(navigator, "serial", {
      configurable: true,
      value: {
        requestPort: async () => firstPort,
        getPorts: async () => ports,
        addEventListener: (type, listener) => {
          serialListeners[type]?.push(listener);
        },
        __useDisconnectingPort: () => {
          ports = [];
          navigator.serial.requestPort = async () => disconnectingPort;
        },
        __returnPort: () => {
          ports = [returnedPort];
          for (const listener of serialListeners.connect) {
            listener({ target: returnedPort });
          }
        },
        __useFloodPort: () => {
          navigator.serial.requestPort = async () => floodPort;
        },
      },
    });
  });
});

test("connects, reads serial output, and keeps the port open", async ({ page }) => {
  await page.goto("/");

  await page.getByRole("button", { name: "Connect", exact: true }).click();
  await expect(page.locator("#serial-output")).toContainText("[Connected]");
  await expect(page.locator("#serial-output")).toContainText("boot log");
  await expect(page.locator("#status-text")).toHaveText("Connected");
  await expect(page.getByRole("button", { name: "Disconnect" })).toBeEnabled();
});

test("reopens the same serial device when it returns after reset", async ({ page }) => {
  await page.goto("/");
  await page.evaluate(() => navigator.serial.__useDisconnectingPort());

  await page.getByRole("button", { name: "Connect", exact: true }).click();
  await expect(page.locator("#serial-output")).toContainText("[Connected]");
  await expect(page.locator("#serial-output")).toContainText("before reset");
  await expect(page.locator("#serial-output")).toContainText("Connection lost: The device has been lost.");
  await expect(page.locator("#status-text")).toHaveText("Reconnecting...");

  await page.evaluate(() => navigator.serial.__returnPort());

  await expect(page.locator("#serial-output")).toContainText("[Device returned, reopening serial port...]");
  await expect(page.locator("#serial-output")).toContainText("[Reconnected]");
  await expect(page.locator("#serial-output")).toContainText("after reset");
  await expect(page.locator("#status-text")).toHaveText("Connected");
});

test("stays responsive and bounds retained output under a serial flood", async ({ page }) => {
  await page.goto("/");
  await page.evaluate(() => navigator.serial.__useFloodPort());

  await page.getByRole("button", { name: "Connect", exact: true }).click();

  // The newest line must render even after tens of thousands of lines, proving
  // the flood was drained rather than stalling the UI.
  await expect(page.locator("#serial-output")).toContainText("LAST FLOOD LINE", { timeout: 15000 });

  // Retained text is capped (bounded memory) and the oldest lines are trimmed.
  const stats = await page.evaluate(() => ({
    length: document.getElementById("serial-output").textContent.length,
    hasOldest: document.getElementById("serial-output").textContent.includes("flood line 1\n"),
  }));
  expect(stats.length).toBeLessThanOrEqual(220000);
  expect(stats.hasOldest).toBe(false);

  // The page is still interactive (event loop not blocked).
  await expect(page.getByRole("button", { name: "Disconnect" })).toBeEnabled();
});
