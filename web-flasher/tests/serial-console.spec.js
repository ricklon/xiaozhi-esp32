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

    const firstPort = makeReadablePort(["boot log\n"]);
    const disconnectingPort = makeReadablePort(["before reset\n"], { throwAfterChunks: true });
    const returnedPort = makeReadablePort(["after reset\n"]);

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
