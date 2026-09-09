# Coglet — XIAO ESP32-S3 Sense

This profile combines Xiaozhi audio, camera and native MCP with a six-channel
Coglet controller adapted from `../eyemech-esp32-xiao`. Existing Sense and Eyes
profiles are unchanged. **No flashing, board identification, calibration export
from a connected board, or physical qualification was performed during development.**

The fitted camera and channel assignment have not been supplied. The default
build therefore disables camera initialization and leaves every servo role
unmapped, unconfirmed and released. This is a buildable provisioning profile,
not a ready-to-drive calibration for an unidentified mechanism.

## Wiring

| Function | XIAO ESP32-S3 connection |
|---|---|
| Speaker MAX98357A DIN | D0 / GPIO1 |
| External I²S shared BCLK | D1 / GPIO2 |
| External I²S shared WS/LRC | D2 / GPIO3 |
| External microphone DOUT | D3 / GPIO4 |
| PCA9685 SDA | D4 / GPIO5, I²C controller **1** |
| PCA9685 SCL | D5 / GPIO6, I²C controller **1** |
| PCA9685 address | `0x40`, 100 kHz bus, nominal 50 Hz PWM |
| PCA9685 /OE | D10 / GPIO9; **must actually be connected** for GPIO disable |
| Camera SCCB | SDA GPIO40 / SCL GPIO39, I²C controller **0** |
| Camera D0…D7 | GPIO15, 17, 18, 16, 14, 12, 11, 48 |
| Camera clocks/sync | XCLK GPIO10, PCLK GPIO13, VSYNC GPIO38, HREF GPIO47 |

GPIO9/D10 is distinct from camera XCLK GPIO10. Use the external I²S microphone,
not the Sense expansion board microphone; this profile preserves GPIO1–4 audio.
The Sense microSD interface is not enabled; avoid attaching SD circuitry to the
servo I²C pins. Use common grounds, PCA logic VCC at 3.3 V, I²C pull-ups to 3.3 V,
and a separately switched servo V+ supply sized for the *fitted* servos. Do not
feed servo current from the XIAO regulator or USB 3.3 V rail.

Eyemech's operating guide says D10 is **unwired** and /OE is pulled down. In that
state software release relies on I²C, and cannot stop PWM if that bus fails.
For reset-safe hardware inhibition, wire D10 to /OE and replace the pull-down
with a suitable pull-up (e.g. 10 kΩ) to the PCA logic 3.3 V rail. Inspect the
actual breakout before changing its resistors. Verify /OE high across reset
with a scope before relying on it. The firmware drives it high before PCA setup.

`release` latches future commands out and requests PCA ALL_LED full-off. It does
**not** cut servo power, measure limpness, or report actual position. If release
fails, the latch still blocks future commands and the error is returned. The
physical servo-power switch remains the independent emergency stop. Firmware
cannot guarantee a motion-free reset with unwired /OE and retained PCA pulses.

## Coglet mapping and motion adaptation

Logical roles are `base`, `tilt`, `lid_left`, `lid_right`, `mouth`, `ears`.
Provision each with a unique PCA channel 0–5; defaults are **-1 (unassigned)**.
This six-role model assumes two independent upper eyelids and one mechanically
shared ears servo. Confirm that arrangement and the actual plug mapping before
provisioning. If the ears are independent or the lids share a servo, change the
role model before driving; do not assign duplicate channels to simulate coupling.

The old Eyemech assignment was 0=LR, 1=UD, 2=TL, 3=BL, 4=TR, 5=BR. **Do not copy
that assignment or its measured angles to Coglet.** Gaze maps to base/tilt, blink
and wink use only the two upper lids, and mouth/ears are available for individual
builder tests. They remain released during conversational gaze and expressions;
no lower-lid command is repurposed as a mouth or ears command.

The normalized keyframes, durations and interpolation for `look`, `roll`,
`side_eye`, `wink`, `surprise`, and `sleepy` come from Eyemech's
`components/eye_motion/eye_motion.c`. Upper-lid tracking retains its calibrated
arithmetic: open fraction = `(0.5 + 0.5*trim) * (1 - upper_coeff*(1-vertical))`,
with initial trim 0.85 and coefficient 0.8. These are tunable aesthetic defaults,
not measured Coglet values. Reversed endpoints are supported. There are no
lower lids on this role model, so Eyemech's lower-lid 0.4 coefficient is omitted.

The on-device task runs at approximately 100 Hz, with 2–7 second randomized
blink gaps, 70 ms closed and a 70 ms reopening allowance. Expressions suppress
blinking and leave a settling interval. Deliberate differences from Eyemech:

- Every boot is released, regardless of stored calibration. No neutral pose,
  random gaze, or mode-change movement is issued at startup.
- Local `engage` clears outputs before enabling /OE and commands no position.
  Automatic blinking starts after an explicit gaze or animation command.
- `stop` freezes the last commanded expression pose immediately; blinking can
  resume after settling. `release` prevents all further motion until local engage.
- Mouth and ears have no invented animation choreography.
- All I²C errors propagate. A later animation/timer error aborts motion and
  latches release; poll diagnostic state after an asynchronous action starts.
- Successful per-axis writes alone update the cache. Faults invalidate cached
  positions on release. A coordinated move is not electrically atomic: earlier
  channels may receive their commands before a later channel fails.
- There is one mutex for serial, HTTP, MCP and timer state. PCA MODE1 is checked
  even when repeated targets would otherwise skip writes. No Grove Vision UART
  subsystem or separate Eyemech Wi-Fi stack is imported.

PWM uses a nominal 25 MHz oscillator and prescaler 121. Oscillator accuracy and
servo pulse ranges require measurement; this is not a measured 50 Hz claim.

## Preserve calibration before migration

First establish the target board identity (label, USB identity, chip/flash size),
which mechanism is attached, and whether logic and servo rails are powered.
**Cut servo V+ before opening a serial port or using esptool**: connecting can
reset the MCU. Do not flash until the owner has verified the backup and power
state. No commands below authorize operating an unidentified attached board.

From the existing Eyemech firmware, capture `!status` and the browser
`GET /api/state` output to files. Record all six endpoints, per-axis min/max pulse,
angle range, trim_us, lid_trim, lid_coeff and safeboot. Its measured values reside
in NVS namespace `eyemech`, blob `servo_cal_v1`, with separate `lid_trim`,
`lid_coeff`, `safeboot` keys; compiled defaults are different.

After confirming the correct serial port and keeping servo power off, use a
read-only esptool operation to back up the **entire detected flash size** and the
partition table, hash the files, and verify they are nonempty and readable.
For a board whose live partition table confirms the checked-in Eyemech layout,
NVS is at `0x9000`, length `0x6000`; save that raw region separately as well.
Use esptool's `read-flash` syntax for the installed version; establish offsets
from the live table, not from this example. Keep backups outside build folders.
Raw flash/NVS may contain Wi-Fi and service credentials; store it privately.

The checked-in Xiaozhi 8 MB layout has NVS `0x9000..0xcfff` (0x4000 bytes), while
Eyemech's is `0x9000..0xefff` (0x6000 bytes). Xiaozhi's OTA data occupies part of
that old NVS space. **Matching NVS starting addresses do not preserve calibration.**
Do not write the old raw NVS blob into the smaller partition or treat an ordinary
application flash as a migration guarantee.

Coglet uses independent namespace `coglet`, key `cal_v1`, schema version 1.
It does not auto-import Eyemech geometry. Recalibrate the different mechanism.
For subsequent Coglet backups, save the JSON line from `!coglet export` and a raw
backup. Generate reviewable restore commands offline with:

```bash
python3 main/boards/coglet/tools/restore_commands.py coglet-calibration.json > restore.txt
```

Review before applying over serial: the generated commands release, restore
mapping/calibration, save and export for comparison; they never engage. Only
restore calibration measured for the same unchanged mechanism. The JSON export
is portable; the internal binary NVS structure is not a cross-version format.

## Build, camera confirmation and provisioning

```bash
./switch-board.sh coglet build
python3 main/boards/coglet/tests/run_host_tests.py
```

The build goes to `build-coglet/`, with its own sdkconfig. No flash is performed.
Identify the fitted camera from the module marking/BOM before selecting
`COGLET_CAMERA_OV2640`, `COGLET_CAMERA_OV3660`, or `COGLET_CAMERA_OV5640` in the
Coglet menuconfig choice. Unconfirmed is the default. The profile uses the
existing `esp32-camera` driver with JPEG QVGA, not the similarly named
`esp_cam_sensor` format menus. The selected sensor's driver support must be
compiled in (the existing driver's defaults include all three). At runtime a
missing or mismatched sensor PID leaves the camera unavailable and logs an error.
If another model is fitted, add its verified configuration before enabling it.
A build of a selectable branch alone is not confirmation of the fitted model.

Once identity, backups, camera selection and servo power-off state are verified,
a builder can separately authorize flashing the built Coglet image. Keep V+ off
for initial USB, Wi-Fi, audio and camera checks.

Serial runs at 115200 over USB-Serial-JTAG. Existing commands remain available:
`!wifi SSID PASSWORD`, `!server URL`, `!status`, `!speaker test`, `!speaker vol N`,
`!mic`, `!camera`, `!stop` (voice session stop), and `!reboot`.
Robot commands use the distinct `!coglet` prefix:

| Command | Effect |
|---|---|
| `!coglet state` / `!coglet export` | JSON diagnostic state plus full calibration |
| `!coglet release` | Latched software output release |
| `!coglet configure ROLE CH LOW HIGH MIN_US MAX_US TRIM_US CONFIRMED` | While released, set one role; confirmed is 0 or 1 |
| `!coglet lids TRIM COEFF` | While released, set 0–1 upper-lid tracking settings |
| `!coglet save` | While released, persist current configuration; reports NVS failures |
| `!coglet builder` | Clear release for isolated servo tests; no automatic movement |
| `!coglet servo ROLE DEGREES` | Builder only, within configured bounds |
| `!coglet jog ROLE DELTA` | Builder only, at most ±5°, requires known commanded angle |
| `!coglet engage` | Require all six roles confirmed; leave all outputs off until requested |
| `!coglet gaze X Y` | Normal mode, -100…100 mapped to calibrated base/tilt endpoints |
| `!coglet blink` | One timed blink after an established gaze; refused during animation |
| `!coglet animate NAME` | One named expression |
| `!coglet stop` | Stop expression; does not latch release |
| `!coglet web on` / `off` | Enable/disable browser controls for this boot only |

For calibration, initially configure a **narrow builder-approved test window**
and confirmed=0; a 90/90 window permits only a 90° horn-seating command. Pulse
mapping is linear over 0–180° with the supplied microsecond range and trim.
The 1000–2000 µs empty defaults are merely placeholders, not measured values.
Use one unloaded servo, then a single 2° direction test after any horn change.
Release before editing the window. Never infer endpoints from another servo;
LOW/HIGH are semantic endpoints, and may be numerically reversed. For lids they
mean closed/open; for base and tilt they mean left/right and down/up. Stop short
of binding, have a human confirm each endpoint, then set confirmed=1 and save.
For channel swaps, unassign the affected roles first (CH=-1, confirmed=0).

Browser: enable over serial and visit `http://DEVICE_IP:8080/`. The page offers
all builder/action commands and a release button; `POST /command` accepts the
same command text without `!coglet`. It is a local, unauthenticated builder
interface, disabled each reboot: use a trusted bench network and turn it off
when done. No CORS is enabled and mismatching browser Origins are rejected.
The page does not require a separate recovery AP; Xiaozhi owns Wi-Fi provisioning.

## Agent Hub

Configure `!server http://VERIFIED_HUB_HOST:8003/xiaozhi/ota/` using the actual
reachable server address. For a deployment requiring enrollment, use its HTTPS
check-in URL with `?enrollment_token=...`, supplied privately through provisioning.
Do not hardcode another machine's LAN address or commit credentials.

Normal Xiaozhi check-in sends device-id/client-id and board metadata; Agent Hub
registers the Xiaozhi device and returns the voice WebSocket URL/token. The
existing voice WebSocket hello advertises MCP, and Agent Hub performs
`initialize` followed by paginated `tools/list`. No `/agent/register` bridge or
additional device transport is needed. The board's firmware ID is `coglet`.
The inherited reconnect task opens the voice session after entering idle.

Conversation tools: `self.coglet.gaze`, `.blink`, `.animate`, `.stop`, `.release`,
`.state`, alongside normal Xiaozhi audio/device and configured camera tools.
Local engagement, mapping, calibration and raw servo commands are not MCP tools.
Synchronous hardware failures throw through Xiaozhi's MCP error path. For timed
actions, the response means **started**, not physically completed; query state
for `animation`, `last_hardware_error`, `release_error`, and `released`.
Agent Hub already handles the protocol and error responses; no Hub code changes
are required. An optional dashboard label, persona describing the tools, or
Coglet-specific test buttons can be added separately.

The web flasher includes a **Flash Coglet** option backed by
`web-flasher/manifest-coglet.json`. Coglet is also registered in the supported
firmware build and release matrix. To build and package its firmware, manifest
and release ZIP:

```sh
python3 scripts/build_supported_firmware.py --board coglet --build --output-root dist
```

For local serving, copy `dist/firmware/coglet/` into
`web-flasher/firmware/coglet/` and `dist/manifests/manifest-coglet.json` into
`web-flasher/`. Firmware binaries are gitignored. Publishing the site and OTA
artifacts remains a separate deployment step.

## Staged qualification (physical evidence still required)

Record date, firmware hash, board/servo/camera identities, calibration export
hash, supply voltage/current rating and observations for every stage. A compile
or mocked test is an automated check, **not** physical evidence.

1. **Power-off inspection and backup:** verify channels, common ground, rail
   polarity, actual /OE wiring/pull-up, target identity and verified backup.
   With V+ off, boot/reset and confirm released state, no automatic engagement.
   Scope /OE and PWM across warm/cold reset before servo-powered tests.
2. **Speaker alone, V+ off:** set low volume, run `!speaker test`; record audible
   chime, distortion and supply behavior. A successful I²S write is insufficient.
3. **Microphone alone, V+ off:** use `!mic` diagnostics and a voice session;
   record nonzero changing capture levels and intelligible transcription of a
   known phrase. Check mute/unmute. Software levels do not prove intelligibility.
4. **Camera, V+ off:** confirm fitted model, select/build its configuration, then
   capture with `!camera` and camera MCP. Record logged PID and an inspected
   image, resolution, orientation and successful Hub image handling.
5. **Individual servos:** one unloaded channel at a time, builder mode, known
   conservative pulse/window, hand on the rail switch. Confirm role/direction
   with one 2° step, measure endpoints with clearance, release, save and export.
   Observe mouth and ears separately. Reboot and verify released state and exact
   calibration round-trip before reconnecting all linkages.
6. **Coordinated behavior:** after all calibration is confirmed, locally engage;
   verify no motion until a command. Test center then small bounded gaze, lid
   tracking, blink, and each named expression. Check reversed lid endpoints,
   collision clearance and supply droop. Confirm stop, then release, and wait
   >10 seconds to verify auto-blink cannot reenergize anything. Use the physical
   rail switch immediately if binding or buzzing occurs.
7. **MCP discovery/calls:** on Hub's normal voice session, capture check-in and
   MCP handshake logs; verify all six Coglet tools and absence of engage/raw
   calibration tools. Call state and release first. While released, gaze/blink/
   animations must error. After local engagement test valid calls and invalid
   bounds/names. With V+ off and logic powered, disconnect PCA I²C for a missing
   device test: immediate calls must error; a timed fault must latch release
   and appear in state. Reboot with V+ off to recover a failed initialization.
8. **Combined operation:** voice input/output plus repeated camera captures and
   all motions, logging heap, I²C errors, resets, audio dropouts and frame failures
   over a recorded duration. Exercise concurrent serial/browser/MCP stop and
   release. Repeat warm/cold reset tests with properly wired /OE, observing PWM;
   verify disconnected Wi-Fi does not disable serial release. Cut the rail to
   demonstrate the independent physical stop. Lack of servo feedback means
   firmware cannot detect rail loss, sag, stalled linkages or actual angles.

Automated coverage: real controller compiled against fake IDF I²C/NVS, testing
released boot, no-motion engagement, mapping/calibration validation, reversed
lid tracking, all animation completion, builder isolation, error propagation,
failed-write cache behavior, absent-PCA detection and latched release.
Firmware build results and outstanding physical checks should be recorded with
any deployment; do not turn this procedure into a claim that it has been run.

## Development verification record — 2026-09-08

- Reviewed Eyemech checkout at `f662248` and Agent Hub at `696ae18`.
- `./switch-board.sh coglet build`: PASS with installed ESP-IDF **5.5.4**.
  The repository guide names 5.5.2; that SDK version was not exercised here.
- Default firmware: camera unconfirmed/disabled, roles unassigned, released boot.
  `build-coglet/xiaozhi.bin` SHA-256:
  `018c03fe02c0fb1d7cf814853fa93d00ad8f5ae1ca3160a5719784f4895ff6a0`.
- OV2640, OV3660 and OV5640 board initialization branches: cross-compile PASS
  with the build's toolchain/includes and temporary preprocessor selections.
  These were object compilation checks, not three full firmware builds and not
  fitted-sensor identification.
- Host controller and calibration restore checks: PASS. No serial/device access.
- Build emitted existing esp_video/lwIP `_IO`, `_IOR`, `_IOW` macro redefinition
  warnings; no build errors. Existing board profiles were not rebuilt.
- Physical stages 1–8 above: **NOT RUN**. Target identity, actual channel mapping,
  camera identity, hardware calibration backup and power state remain unverified.
