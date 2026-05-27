const { test, expect } = require("@playwright/test");

test.beforeEach(async ({ page }) => {
  await page.addInitScript(() => {
    const encoder = new TextEncoder();
    let ports = [];

    function makeReadablePort() {
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
              if (readCount === 1) {
                return { value: encoder.encode("boot log\n"), done: false };
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

    const firstPort = makeReadablePort();

    Object.defineProperty(navigator, "serial", {
      configurable: true,
      value: {
      requestPort: async () => firstPort,
      getPorts: async () => ports,
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
