# Coglet C3 — XIAO ESP32-C3

The Coglet mechanism (PCA9685 servo controller, external I²S mic and
amp) on a Seeed XIAO ESP32-C3 instead of the XIAO ESP32-S3 Sense. Firmware is
the same Xiaozhi voice agent plus the same `CogletController` — the controller
now lives in `main/boards/common/coglet_controller.cc` and is compiled for
whichever Coglet profile is selected. See `../coglet/README.md` for the servo
role model, motion adaptation, calibration and safety notes; all of that applies
unchanged here.

**No board was flashed, identified, calibrated or physically qualified while
this profile was written.** Servo roles default to unassigned and released, the
same as the S3 profile.

## What differs from the S3 Coglet

| | Coglet (S3) | Coglet C3 |
|---|---|---|
| Chip | ESP32-S3, octal PSRAM | ESP32-C3, no PSRAM |
| Flash / partitions | 8 MB, `partitions/v2/8m.csv` | 4 MB, `partitions/v2/4m.csv` |
| Camera | Sense connector, `COGLET_CAMERA_*` Kconfig | **none** — the C3 has no LCD_CAM and no PSRAM |
| Servo I²C controller | `I2C_NUM_1` (controller 0 reserved for camera SCCB) | `I2C_NUM_0` — the C3 has only one I²C controller |
| LED | GPIO21 user LED | none exposed |

Because there is no camera there is no vision/explain path on this board. Do not
carry `CONFIG_SPIRAM_*` or `CONFIG_COGLET_CAMERA_*` settings over from the S3
build; neither applies.

## Wiring — XIAO ESP32-C3

The C3 pad→GPIO map differs from both the C6 and the S3. These are C3 values:

| Function | Pad | GPIO |
|---|---|---|
| Speaker amp (MAX98357A) DIN | D0 | GPIO2 |
| I²S shared BCLK | D1 | GPIO3 |
| Microphone (INMP441) SD | D2 | GPIO4 |
| I²S shared WS/LRC | D3 | GPIO5 |
| PCA9685 SCL | D4 | GPIO6 |
| PCA9685 SDA | D5 | GPIO7 |
| PCA9685 /OE | D10 | GPIO10 |
| Boot button (onboard) | — | GPIO9 |

BCLK and WS are shared by both devices; each has its own data line, so the
speaker occupies D0/D1/D3 and the microphone D1/D2/D3. Note that D2 is the
microphone and D3 is the word clock — the **opposite** of the S3 Coglet. Swap
those two and the amp runs with no LRC (a steady hiss) while the capture path
samples a clock line and reads a flat zero.

GPIO8 and GPIO9 are strapping pins on the ESP32-C3 and GPIO9 is the boot button,
so neither can serve as the reset-safe /OE output; /OE is on GPIO10. As on the S3
profile, /OE must actually be wired for GPIO-level servo inhibition to work — if
the breakout leaves /OE pulled down, release depends on I²C alone and cannot stop
PWM when that bus fails. The physical servo-power switch remains the independent
emergency stop.

Note the I²C pair is reversed from the XIAO silkscreen default and from the S3
Coglet: **SCL is on D4 and SDA on D5**. Swapped, the bus goes completely silent —
`!coglet scan` reports no device at any address rather than just missing `0x40`.

PCA9685 address `0x40`, 100 kHz bus, nominal 50 Hz PWM. Common grounds, PCA logic
VCC at 3.3 V, I²C pull-ups to 3.3 V, and a separately switched servo V+ supply
sized for the fitted servos. Do not feed servo current from the XIAO regulator or
the USB 3.3 V rail.

## Build and flash

```bash
./switch-board.sh coglet-c3 build
./switch-board.sh coglet-c3 flash /dev/ttyACM0
```

The C3 console is USB-Serial-JTAG, so `!` serial commands work over the native
USB port at 115200 baud once `XiaoSerialInputTask` installs the VFS driver.

## Servo provisioning

Every boot starts released with all five roles unassigned. Provision channels
and endpoints over serial (`!coglet ...`) or the MCP tools before anything moves.
Servos are fitted one at a time, so a partly built mechanism is supported: gaze
needs base and tilt, and unassigned roles are skipped. See
`docs/coglet-calibration.md` for the per-servo procedure:

- `self.coglet.gaze`, `self.coglet.blink`, `self.coglet.animate`
- `self.coglet.stop`, `self.coglet.release`, `self.coglet.state`

## Listening

There is no wake word, no on-device VAD and no AEC on the C3, so the default
listening mode is auto-stop: the device streams the mic and agent-hub's VAD
decides where each utterance ends. The mic is not sent while a reply plays, so
the boot button is the only way to interrupt one. `InitializeAutoConnect()`
reopens the session about 1 s after every drop to Idle, so the robot is
effectively always listening.

agent-hub **listen mode** needs no firmware support. Turn it on by saying
"listen mode" or with the dashboard toggle, and off with "interact again". The
hub still transcribes and logs each utterance but runs no LLM turn and no
device tool, so the servos stay still. The robot speaks only when switching
modes ("Listen mode. I'll stay quiet until you say interact again." / "Okay,
I'm back.") and is silent in between. The mode lives in hub memory per MAC, so
it survives reboots but not a hub restart. This ships in agent-hub PR #81
(`feat/listen-mode`).

The robot has no indicator for listen mode: the XIAO C3 has no user LED. An
external LED is tracked in ricklon/xiaozhi-esp32#8.

## Host tests

`main/boards/coglet/tests/run_host_tests.py` compiles the shared controller
against a fake IDF and covers both profiles; it opens no device.

## Diagnosing the servo bus

`!coglet scan` sweeps 0x08-0x77 on the configured pins and lists what answers.
`!coglet scan <sda> <scl>` re-creates the bus on other pins first, so a suspected
swap or a harness on different pads can be ruled out without reflashing — it
tears the servo bus down, so reboot afterwards.
