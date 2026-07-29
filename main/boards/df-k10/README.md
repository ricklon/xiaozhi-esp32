# DFRobot UNIHIKER K10

## Agent Hub home screen

The K10 defaults to English and uses the conversation-style UI. Transcribed user
speech and Agent Hub responses appear as wrapped message bubbles. The footer
keeps the physical controls visible:

* A: pause/resume all listening, including local wake-word detection; hold for
  one second to lower volume.
* B: start/stop continuous transcription while listening is enabled;
  double-click to capture a photo in the transcript; hold for one second to
  raise volume. Agent Hub saves each VAD-segmented transcript without invoking
  the LLM or speaker.

The header contains a compact network/emotion indicator, leaving the main area
exclusively for the scrolling conversation. A photo appears in that conversation
immediately after capture, then uploads through Agent Hub's authenticated image
endpoint.

Transcript snapshots add a `purpose=transcript` field to the existing multipart
image upload. Agent Hub should save these uploads as chronological
`[image:PATH]` history entries for the device and return an immediate accepted
response without running vision inference. Uploads without this field retain
the normal camera-explanation behavior.

When idle, say `Computer` to use the normal spoken assistant. When listening is
paused, the wake word cannot reactivate the device.

## USB serial console

The native USB Serial/JTAG console accepts the shared diagnostic and setup
commands, including `!help`, `!status`, `!wifi`, `!server`, `!camera`, `!mic`,
`!speaker`, `!stop`, and `!reboot`.

Provision Wi-Fi with `!wifi SSID PASSWORD`. The command stores the credential
in device NVS; do not add hotspot names or passwords to tracked board headers.

When Agent Hub advertises heartbeat support during authenticated check-in, the
K10 completes registration with an immediate health report and repeats it at
the server-requested interval (60 seconds by default). The dashboard therefore
shows an idle or paused device as healthy without requiring an open voice
WebSocket. A missing heartbeat is classified by Agent Hub as offline after its
configured timeout (180 seconds by default).

## Build

The board defaults select the ESP32-S3 target, 16 MB partition layout, octal
PSRAM, GC2145 camera, native USB Serial/JTAG console, and the Agent Hub public
Funnel endpoint automatically. `switch-board.sh` reads the two Agent Hub values
from `../agent-hub/.env` when that file exists:

```bash
./switch-board.sh df-k10 build
```

For another layout, set `AGENT_HUB_ENV_FILE`. Explicit environment values take
precedence over the dotenv file:

```bash
AGENT_HUB_PUBLIC_HOST=agent-hub.panthera-hamlet.ts.net \
AGENT_HUB_SERVER_ENROLLMENT_TOKEN='YOUR_ENROLLMENT_TOKEN' \
./switch-board.sh df-k10 build
```

Agent Hub must use matching values:

```dotenv
AGENT_HUB_PUBLIC_HOST=agent-hub.panthera-hamlet.ts.net
AGENT_HUB_SERVER_ENROLLMENT_TOKEN=YOUR_ENROLLMENT_TOKEN
```

The device checks in at standard HTTPS port 443 using:

```text
https://agent-hub.panthera-hamlet.ts.net/xiaozhi/ota/?enrollment_token=YOUR_ENROLLMENT_TOKEN
```

The Agent Hub sidecar must be running and Tailscale Funnel enabled before the
device can enroll. The token is redacted from `!server`, `!status`, server
history, and `switch-board.sh status` output.

The equivalent manual configuration is described below.

## Manual configuration

Set the ESP32-S3 build target:

```bash
idf.py set-target esp32s3
```

Open `menuconfig`:

```bash
idf.py menuconfig
```

Select the board:

```
Xiaozhi Assistant -> Board Type -> DFRobot UNIHIKER K10
```

Select octal PSRAM:

```
Component config -> ESP PSRAM -> SPI RAM config -> Mode (QUAD/OCT) -> Octal Mode PSRAM
```

Enable camera-buffer byte swapping:

```
Xiaozhi Assistant -> Camera Configuration -> Enable software camera buffer endianness swapping
```

Configure the GC2145 camera:

```
Component config -> Espressif Camera Sensors Configurations
  -> Camera Sensor Configuration
  -> Select and Set Camera Sensor
  -> GC2145
  -> Auto detect GC2145
```

```
Component config -> Espressif Camera Sensors Configurations
  -> Camera Sensor Configuration
  -> Select and Set Camera Sensor
  -> GC2145
  -> Select default output format for DVP interface
  -> RGB565 800x600 20fps, DVP 8-bit, 20M input
```

Build the firmware:

```bash
idf.py build
```
