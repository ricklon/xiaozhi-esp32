const { chromium } = require("@playwright/test");
const { SerialPort } = require("serialport");
const fs = require("node:fs/promises");
const path = require("node:path");

const portPath = process.env.SERIAL_PORT || "/dev/ttyACM1";
const baudRate = Number(process.env.SERIAL_BAUD || "115200");
const baseUrl = process.env.WEB_FLASHER_URL || `file://${path.resolve(__dirname, "..", "index.html")}`;
const artifactDir = path.resolve(__dirname, "..", "test-results", "serial-uat");

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function main() {
  await fs.mkdir(artifactDir, { recursive: true });

  const launchOptions = { headless: true };
  if (process.env.PLAYWRIGHT_CHROME_EXECUTABLE) {
    launchOptions.executablePath = process.env.PLAYWRIGHT_CHROME_EXECUTABLE;
  }
  const browser = await chromium.launch(launchOptions);
  const page = await browser.newPage();
  const transcript = [];

  async function closeSerialBridge() {
    if (!global.port) {
      return;
    }

    const port = global.port;
    global.port = null;
    port.removeAllListeners("data");
    if (port.isOpen) {
      await new Promise((resolve) => port.close(() => resolve()));
    }
  }

  function record(line) {
    transcript.push(line);
    process.stdout.write(line);
  }

  await page.exposeBinding("serialBridgeOpen", async () => {
    if (global.port?.isOpen) {
      return;
    }

    global.port = new SerialPort({
      path: portPath,
      baudRate,
      autoOpen: false,
    });

    global.port.on("data", (chunk) => {
      page.evaluate((text) => {
        window.__serialBridgePush(text);
      }, chunk.toString("utf8")).catch(() => {});
    });

    await new Promise((resolve, reject) => {
      global.port.open((error) => error ? reject(error) : resolve());
    });
  });

  await page.exposeBinding("serialBridgeWrite", async (_source, text) => {
    if (!global.port?.isOpen) {
      throw new Error("Serial bridge is not open");
    }
    await new Promise((resolve, reject) => {
      global.port.write(text, (error) => error ? reject(error) : resolve());
    });
  });

  await page.exposeBinding("serialBridgeClose", async () => {
    if (!global.port?.isOpen) {
      return;
    }
    await new Promise((resolve) => global.port.close(() => resolve()));
  });

  await page.addInitScript(() => {
    const encoder = new TextEncoder();
    let dataQueue = [];
    let pendingRead = null;
    let opened = false;

    window.__serialBridgePush = (text) => {
      const chunk = new TextEncoder().encode(text);
      if (pendingRead) {
        const resolve = pendingRead;
        pendingRead = null;
        resolve({ value: chunk, done: false });
      } else {
        dataQueue.push(chunk);
      }
    };

    const bridgePort = {
      getInfo: () => ({ usbVendorId: 0x303a, usbProductId: 0x1001 }),
      open: async () => {
        await window.serialBridgeOpen();
        opened = true;
      },
      close: async () => {
        opened = false;
        await window.serialBridgeClose();
      },
      addEventListener: () => {},
      writable: {
        getWriter: () => ({
          write: async (chunk) => {
            await window.serialBridgeWrite(new TextDecoder().decode(chunk));
          },
          releaseLock: () => {},
        }),
      },
      readable: {
        getReader: () => ({
          read: async () => {
            if (!opened) {
              return { done: true };
            }
            if (dataQueue.length) {
              return { value: dataQueue.shift(), done: false };
            }
            return await new Promise((resolve) => {
              pendingRead = resolve;
            });
          },
          cancel: async () => {
            if (pendingRead) {
              const resolve = pendingRead;
              pendingRead = null;
              resolve({ done: true });
            }
          },
          releaseLock: () => {},
        }),
      },
    };

    Object.defineProperty(navigator, "serial", {
      configurable: true,
      value: {
        requestPort: async () => bridgePort,
        getPorts: async () => [bridgePort],
        addEventListener: () => {},
      },
    });

    window.__serialBridgePort = bridgePort;
  });

  await page.goto(baseUrl);
  await page.getByRole("button", { name: "Connect", exact: true }).click();
  await page.waitForSelector("#status-text.connected");
  record("[uat] connected\n");

  const output = page.locator("#serial-output");

  async function snapshot(label) {
    const text = await output.textContent();
    record(`\n===== ${label} =====\n${text}\n`);
    return text || "";
  }

  async function send(command, waitMs = 2500) {
    record(`[uat] send ${command}\n`);
    await page.locator("#serial-input").fill(command);
    await page.getByRole("button", { name: "Send" }).click();
    await sleep(waitMs);
    return await snapshot(command);
  }

  await send("!reboot", 9000);
  const statusText = await send("!status", 2500);
  await send("!server", 1500);
  await send("!wifi list", 1500);
  const cameraText = await send("!camera", 3000);

  const fullTranscript = transcript.join("");
  await fs.writeFile(path.join(artifactDir, "serial-uat.txt"), fullTranscript, "utf8");
  await fs.writeFile(path.join(artifactDir, "serial-output.txt"), await output.textContent(), "utf8");

  await closeSerialBridge();
  await browser.close();

  if (!statusText.includes("Board    : XIAO ESP32-S3 Sense")) {
    throw new Error("Expected XIAO ESP32-S3 Sense status output");
  }
  if (!statusText.includes("Camera   : available")) {
    throw new Error("Expected camera to be available");
  }
  if (!cameraText.includes("Camera capture: OK")) {
    throw new Error("Expected camera capture to succeed");
  }
}

main().catch(async (error) => {
  console.error(error);
  if (global.port?.isOpen) {
    await new Promise((resolve) => global.port.close(() => resolve()));
  }
  process.exit(1);
});
