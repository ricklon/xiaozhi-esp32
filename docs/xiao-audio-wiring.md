# XIAO external I2S audio — the house standard

One wiring convention for every XIAO board in this repo that uses an external
INMP441 microphone and a MAX98357A amplifier. Wire to this; do not adapt the
firmware per harness. Confirmed on the C3 Coglet (2026-09-13).

The amp and the mic **share** the bit clock and the word clock, and each has its
own data line. That is why only four GPIOs are needed for full duplex audio.

| Signal | Pad | S3 GPIO | C3 GPIO | Goes to |
|---|---|---|---|---|
| Amp data (DOUT) | D0 | 1 | 2 | MAX98357A **DIN** |
| Bit clock (BCLK) | D1 | 2 | 3 | amp **BCLK** *and* mic **SCK** |
| Mic data (DIN) | **D2** | 3 | 4 | INMP441 **SD** |
| Word clock (WS/LRC) | **D3** | 4 | 5 | amp **LRC** *and* mic **WS** |

**D2 is the microphone and D3 is the word clock.** Reversing that pair produces
two symptoms that read as separate hardware faults, but are one swap:

- the amp has data and bit clock but no LRC, so it **buzzes or hisses**, and
- capture samples a clock line, so `MIC peak` is **flat zero** forever.

## INMP441 microphone

| Pin | Connect to |
|---|---|
| VDD | 3V3 |
| GND | GND |
| **L/R** | **GND** — selects the left slot, which the firmware reads |
| SCK | D1 (bit clock) |
| WS | D3 (word clock) |
| SD | D2 (mic data) |

L/R left floating or tied high puts the microphone in the right slot and capture
reads zero even with every other wire correct.

## MAX98357A amplifier

| Pin | Connect to |
|---|---|
| Vin | **5 V** — not the 3.3 V rail |
| GND | GND, shared with the XIAO |
| DIN | D0 |
| BCLK | D1 |
| LRC | D3 |
| SD | **not floating** — leave the module's default pull-up, or tie high for 9 dB |
| Speaker +/- | 4-8 ohm speaker, not a piezo |

Powering the amp from 3.3 V, or leaving SD floating, produces a buzz that no pin
mapping will fix. A buzz that worsens with volume is the supply, not the pins.

## Checking it

```bash
!speaker test            # built-in chime, no server needed: clean tone expected
!mic                     # then watch the MIC peak meter while making noise
```

`MIC peak` stays at zero while listening is paused (`!quiet on`) and while the
device is idle, because the input is disabled — trigger a session first.

Flat zero with correct wiring and a working module means no data on the mic data
line at all: check continuity to D2 before touching firmware.

## Firmware

Board `config.h` files follow this table:

- `main/boards/coglet-c3/config.h` — DOUT=GPIO2, BCLK=GPIO3, DIN=GPIO4, WS=GPIO5
- `main/boards/xiao-esp32-s3-sense/config.h` — DOUT=GPIO1, BCLK=GPIO2, DIN=GPIO3, WS=GPIO4

Still on the old, unverified order (WS on D2, mic on D3), to be aligned when the
hardware is next in hand: `main/boards/coglet/config.h` (S3 Coglet).
