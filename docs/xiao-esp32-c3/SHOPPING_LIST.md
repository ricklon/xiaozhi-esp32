# Shopping List — XIAO ESP32-C3 Voice Assistant

All parts needed to build the standalone voice assistant described in WIRING.md.

---

## Required Components

| Component | Description | Search Term / Notes |
|---|---|---|
| **Seeed XIAO ESP32-C3** | Main MCU — Wi-Fi, USB-C, compact form factor | "XIAO ESP32-C3" |
| **MAX98357A breakout** | I2S amplifier for speaker output | "MAX98357A I2S amplifier breakout" — Adafruit #3006 or equivalent |
| **INMP441 breakout** | I2S MEMS microphone | "INMP441 I2S microphone module" |
| **Small speaker** | 4Ω or 8Ω, 1W–3W, any size that fits your enclosure | "3W 8 ohm speaker" |
| **Breadboard** | Full-size or half-size for prototyping | Standard 830-point or 400-point |
| **Jumper wires** | Male-to-male for breadboard connections | Assorted 10cm–20cm |
| **USB-C cable** | Power + programming | Any USB-C data cable |

---

## Optional / Recommended

| Component | Description | Notes |
|---|---|---|
| **5V USB power supply** | Power the device without a laptop | Phone charger works fine |
| **USB-C power bank** | Portable power | Allows fully untethered use |
| **Small enclosure/case** | 3D printed or project box | Mount board + speaker inside |
| **22 AWG solid wire** | Cleaner than jumper wires for permanent builds | Any electronics supplier |
| **3.5mm speaker terminal** | Screw terminal for speaker wires | Makes swapping speakers easy |

---

## Where to Buy

| Supplier | Good for |
|---|---|
| **Seeed Studio** (seeedstudio.com) | XIAO ESP32-C3 (official source, best price) |
| **Adafruit** (adafruit.com) | MAX98357A (#3006), quality breakouts, good docs |
| **Amazon** | INMP441 modules, speakers, jumper wires (fast shipping) |
| **AliExpress** | Cheapest option for all parts, 2–4 week shipping |
| **Mouser / DigiKey** | Best for bulk or guaranteed specs |

---

## Approximate Cost

| Part | Approx. Price |
|---|---|
| XIAO ESP32-C3 | $5–$7 |
| MAX98357A breakout | $6–$10 |
| INMP441 breakout | $3–$6 |
| Speaker (1W 8Ω) | $2–$5 |
| Breadboard + wires | $5–$10 |
| **Total** | **~$21–$38** |

---

## Notes

- The MAX98357A **SD pin must be wired to 3.3V** — if left floating the amp is in shutdown mode and you'll only hear clicks.
- The INMP441 **L/R pin must be wired to GND** to select the left channel, which matches the I2S slot configuration in firmware.
- The ESP32-C3 is **2.4 GHz only** — it will not connect to a 5 GHz Wi-Fi network.
- A speaker in the 1–3W range is ideal. The MAX98357A can deliver up to 3.2W into 4Ω at 5V.
