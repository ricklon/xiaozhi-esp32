class SerialConsole {
  constructor() {
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.keepReading = false;
    this.readLoopPromise = null;
    this.disconnecting = false;
    this.manualDisconnect = false;
    this.savedPortInfo = null;
    this.reconnecting = false;
    this.reconnectTimer = null;
    this.reconnectDeadline = 0;
    this.reconnectAttempts = 0;

    this.connectBtn = document.getElementById("btn-connect");
    this.disconnectBtn = document.getElementById("btn-disconnect");
    this.clearBtn = document.getElementById("btn-clear");
    this.sendBtn = document.getElementById("btn-send");
    this.input = document.getElementById("serial-input");
    this.output = document.getElementById("serial-output");
    this.statusDot = document.getElementById("status-dot");
    this.statusText = document.getElementById("status-text");
    this.quickBtns = [...document.querySelectorAll(".quick-btn")];

    this.init();
  }

  init() {
    if (!("serial" in navigator)) {
      this.setStatus("error", "Use Chrome or Edge");
      this.connectBtn.disabled = true;
      this.input.disabled = true;
      this.sendBtn.disabled = true;
      return;
    }

    this.connectBtn.addEventListener("click", () => {
      this.connect().catch((error) => this.showError("Connect failed", error));
    });
    this.disconnectBtn.addEventListener("click", () => {
      this.disconnect().catch((error) => this.showError("Disconnect failed", error));
    });
    this.clearBtn.addEventListener("click", () => this.clear());
    this.sendBtn.addEventListener("click", () => {
      this.sendInput().catch((error) => this.showError("Send failed", error));
    });
    this.input.addEventListener("keydown", (event) => {
      if (event.key === "Enter") {
        event.preventDefault();
        this.sendInput().catch((error) => this.showError("Send failed", error));
      }
    });

    for (const button of this.quickBtns) {
      button.addEventListener("click", () => {
        this.send(button.dataset.cmd).catch((error) => this.showError("Send failed", error));
      });
    }

    navigator.serial.addEventListener("connect", (event) => {
      if (this.reconnecting && event.target) {
        this.tryReconnect(event.target);
      }
    });

    window.addEventListener("beforeunload", () => {
      if (this.port) {
        this.disconnect();
      }
    });

    this.updateUI();
  }

  async connect(port = null, options = {}) {
    if (this.port || this.disconnecting) {
      return;
    }

    if (!options.isReconnect) {
      this.clearReconnect();
    }

    this.setStatus("connecting", "Opening...");
    this.updateUI();

    try {
      this.manualDisconnect = false;
      this.port = port || await navigator.serial.requestPort();
      this.savedPortInfo = this.port.getInfo ? this.port.getInfo() : null;
      await this.port.open({
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: "none",
        flowControl: "none",
      });

      this.writer = this.port.writable.getWriter();
      this.keepReading = true;
      this.readLoopPromise = this.readLoop();

      this.port.addEventListener("disconnect", () => {
        this.handleConnectionLost("Device disconnected");
      });

      this.clearReconnect(false);
      this.setStatus("connected", "Connected");
      this.print(options.isReconnect ? "[Reconnected]\n" : "[Connected]\n", "system");
      this.updateUI();
    } catch (error) {
      await this.closePort();
      this.setStatus("error", "Failed");
      this.updateUI();
      throw error;
    }
  }

  async disconnect() {
    this.manualDisconnect = true;
    this.disconnecting = true;
    this.keepReading = false;
    this.clearReconnect();

    if (this.reader) {
      await this.reader.cancel().catch(() => {});
    }
    if (this.readLoopPromise) {
      await this.readLoopPromise.catch(() => {});
    }

    await this.closePort();
    this.disconnecting = false;

    this.setStatus("", "Disconnected");
    this.print("[Disconnected]\n", "system");
    this.updateUI();
  }

  async closePort() {
    if (this.writer) {
      this.writer.releaseLock();
      this.writer = null;
    }

    if (this.port) {
      await this.port.close().catch(() => {});
      this.port = null;
    }

    this.reader = null;
    this.readLoopPromise = null;
  }

  markDisconnected(statusText = "Disconnected") {
    this.keepReading = false;
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.readLoopPromise = null;
    this.disconnecting = false;
    this.setStatus("", statusText);
    this.updateUI();
  }

  async handleConnectionLost(reason) {
    if (this.manualDisconnect || this.disconnecting || this.reconnecting) {
      return;
    }

    this.print(`\n[${reason}]\n`, "system");
    await this.closePort();
    this.startReconnect();
  }

  startReconnect() {
    if (!this.savedPortInfo) {
      this.markDisconnected("Disconnected");
      this.print("[The board reset and Chrome dropped the USB serial port. Click Connect again after it boots.]\n", "system");
      return;
    }

    this.reconnecting = true;
    this.reconnectAttempts = 0;
    this.reconnectDeadline = Date.now() + 45000;
    this.setStatus("connecting", "Reconnecting...");
    this.updateUI();
    this.print("[Waiting for the device to return...]\n", "system");
    this.scheduleReconnect(250);
  }

  clearReconnect(updateStatus = true) {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.reconnecting = false;
    this.reconnectDeadline = 0;
    this.reconnectAttempts = 0;
    if (updateStatus && !this.port) {
      this.setStatus("", "Disconnected");
      this.updateUI();
    }
  }

  scheduleReconnect(delayMs) {
    if (!this.reconnecting || this.manualDisconnect) {
      return;
    }

    if (Date.now() > this.reconnectDeadline) {
      this.clearReconnect();
      this.print("[Device did not return. Click Connect to select the serial port again.]\n", "system");
      return;
    }

    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
    }

    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.tryReconnect().catch((error) => {
        console.debug("Reconnect attempt failed", error);
        this.scheduleReconnect(Math.min(2000, 300 + this.reconnectAttempts * 250));
      });
    }, delayMs);
  }

  async tryReconnect(candidatePort = null) {
    if (!this.reconnecting || this.port || this.disconnecting || this.manualDisconnect) {
      return;
    }

    this.reconnectAttempts += 1;
    const port = candidatePort || await this.findReconnectPort();
    if (!port) {
      this.scheduleReconnect(Math.min(2000, 300 + this.reconnectAttempts * 250));
      return;
    }

    if (!this.matchesSavedPort(port)) {
      this.scheduleReconnect(Math.min(2000, 300 + this.reconnectAttempts * 250));
      return;
    }

    this.print("[Device returned, reopening serial port...]\n", "system");
    await this.connect(port, { isReconnect: true });
  }

  async findReconnectPort() {
    const ports = await navigator.serial.getPorts();
    return ports.find((port) => this.matchesSavedPort(port)) || null;
  }

  matchesSavedPort(port) {
    if (!this.savedPortInfo || !port?.getInfo) {
      return false;
    }

    const info = port.getInfo();
    if (this.savedPortInfo.usbVendorId && info.usbVendorId !== this.savedPortInfo.usbVendorId) {
      return false;
    }
    if (this.savedPortInfo.usbProductId && info.usbProductId !== this.savedPortInfo.usbProductId) {
      return false;
    }
    return true;
  }

  async readLoop() {
    const decoder = new TextDecoder();

    try {
      while (this.port?.readable && this.keepReading) {
        this.reader = this.port.readable.getReader();

        try {
          while (this.keepReading) {
            const { value, done } = await this.reader.read();
            if (done) {
              break;
            }
            if (value) {
              this.print(decoder.decode(value, { stream: true }));
            }
          }
        } finally {
          this.reader.releaseLock();
          this.reader = null;
        }
      }
    } catch (error) {
      if (this.keepReading && !this.disconnecting) {
        await this.handleConnectionLost(`Connection lost: ${error.message}`);
      }
    } finally {
      const tail = decoder.decode();
      if (tail) {
        this.print(tail);
      }
    }
  }

  async sendInput() {
    const command = this.input.value.trim();
    if (!command) {
      return;
    }

    await this.send(command);
    this.input.value = "";
  }

  async send(command) {
    if (!this.writer) {
      throw new Error("Not connected");
    }

    await this.writer.write(new TextEncoder().encode(`${command}\r\n`));
    this.print(`> ${command}\n`, "sent");
  }

  print(text, type) {
    const placeholder = this.output.querySelector(".placeholder");
    if (placeholder) {
      placeholder.remove();
    }

    const span = document.createElement("span");
    if (type) {
      span.className = `cmd-${type}`;
    }
    span.textContent = text;
    this.output.appendChild(span);
    this.output.scrollTop = this.output.scrollHeight;
  }

  clear() {
    this.output.innerHTML = '<span class="placeholder">Click Connect to open serial port...</span>';
  }

  showError(message, error) {
    console.error(message, error);
    this.print(`\n[${message}: ${error.message}]\n`, "error");
  }

  setStatus(state, text) {
    this.statusDot.className = `status-indicator${state ? ` ${state}` : ""}`;
    this.statusText.className = `status-text${state ? ` ${state}` : ""}`;
    this.statusText.textContent = text;
  }

  updateUI() {
    const connected = !!this.port;
    const busy = this.statusDot.classList.contains("connecting") || this.disconnecting || this.reconnecting;

    this.connectBtn.disabled = connected || busy;
    this.disconnectBtn.disabled = !connected || this.disconnecting;
    this.clearBtn.disabled = false;
    this.sendBtn.disabled = !connected;
    this.input.disabled = !connected;

    for (const button of this.quickBtns) {
      button.disabled = !connected;
    }
  }
}

window.addEventListener("DOMContentLoaded", () => {
  new SerialConsole();
});
