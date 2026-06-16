# XIAO ESP32-S3 Eye Servo Robot

This task documents a XIAO ESP32-S3 Sense robot profile with four hobby servos:

- left eye
- right eye
- left eyelid
- right eyelid

The servos are driven by a PCA9685 16-channel I2C PWM board (Adafruit product
815), so they use only the XIAO I2C pins and leave the camera, mic, and speaker
intact. The servos must be powered from a separate 5V supply (PCA9685 V+) with a
shared ground.

## Wiring

### Base breadboard layout (reference)

![XIAO breadboard base wiring](images/xiao-breadboard-base-wiring.png)

This is the base breadboard layout used across the boards so far (XIAO + INMP441
mic + MAX98357A amp). This board keeps that audio wiring plus the Sense camera,
and adds the PCA9685 servo board on a second I2C bus. A servo-specific diagram
will be added.

### PCA9685 servo board

The XIAO talks to the PCA9685 over I2C using its default SDA/SCL pads. The bus
runs on I2C port 1 because the camera's SCCB owns port 0.

| Signal | XIAO pad | ESP32-S3 GPIO | PCA9685 pin |
| --- | --- | --- | --- |
| SDA | D4 | GPIO5 | SDA |
| SCL | D5 | GPIO6 | SCL |
| 3V3 logic | 3V3 | — | VCC |
| Ground | GND | — | GND |

Servos plug into the PCA9685 channel headers (default I2C address `0x40`):

| Servo | PCA9685 channel |
| --- | --- |
| left eye | 0 |
| right eye | 1 |
| left eyelid | 2 |
| right eyelid | 3 |

Power wiring:

```text
External 5V +  -> PCA9685 V+
External GND   -> PCA9685 GND
XIAO 3V3       -> PCA9685 VCC (logic only)
XIAO GND       -> external GND and PCA9685 GND (common ground)
PCA9685 ch0-3  -> servo signal/power/ground headers
```

Add bulk capacitance across the PCA9685 V+ rail, typically 470uF to 1000uF. A
0.1uF ceramic capacitor in parallel is also useful.

Do not power the servos from the XIAO 3.3V pin or a weak USB-derived rail. Servo
stall/startup current can brown out the ESP32.

## Firmware Profile

The firmware profile is `xiao-esp32-s3-eyes`.

It is based on the existing `xiao-esp32-s3-sense` profile and keeps:

- OV2640 camera support
- external I2S audio pinout on D0-D3
- onboard boot button
- onboard user LED
- serial Wi-Fi helper commands

The eye servos are board-specific hardware, so they are registered in this robot profile rather than in common MCP tools.

## Timer Plan

The XIAO S3 Sense camera uses LEDC timer 0/channel 0 for camera XCLK. The onboard GPIO LED uses LEDC timer 1/channel 0 through `GpioLed`.

The four servos use LEDC timer 2 at 50 Hz and channels 1-4. This avoids reconfiguring the camera or LED timers.

## MCP Tools

The robot profile exposes:

- `self.eyes.set_position` - set left/right eye servo angles.
- `self.eyes.center` - center both eyes and open eyelids.
- `self.eyes.get_position` - return current servo angles and trims.
- `self.eyelids.set_position` - set left/right eyelid servo angles.
- `self.eyelids.open` - move both eyelids to the configured open angle.
- `self.eyelids.close` - move both eyelids to the configured closed angle.
- `self.eyes.set_trim` - persist a per-servo calibration offset.

Servo ranges are intentionally conservative MCP integer ranges of 0-180 degrees. Physical linkages may need narrower safe ranges after mechanical testing.

## Build

Select the board type:

```text
CONFIG_BOARD_TYPE_XIAO_ESP32S3_EYES=y
```

The board config uses ESP32-S3 and the same camera-related sdkconfig entries as `xiao-esp32-s3-sense`.

For local `switch-board.sh` builds, those settings live in:

```text
main/boards/xiao-esp32-s3-eyes/sdkconfig.defaults
```
