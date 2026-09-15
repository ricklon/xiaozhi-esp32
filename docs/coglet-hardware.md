# Coglet hardware reference (S3 and C3)

Wiring, parts and power for both Coglet profiles, derived from
`main/boards/coglet/config.h` and `main/boards/coglet-c3/config.h`. The pin
numbers here are the authority the firmware compiles against; the silkscreen
pad names differ between the two XIAO boards.

## Block diagram (both profiles)

```
                 +---------------------------+
   USB  ---------+  XIAO ESP32-S3 / C3       |
 (console,       |                           |
  5V in)         |  I2S: BCLK, WS shared     |
                 |    DOUT --> amp           +---> MAX98357A ---> 4-8 ohm speaker
                 |    DIN  <-- mic           |     (amp Vin 5V)
                 |                           +<--- INMP441 mic (3V3)
                 |                           |
                 |  I2C: SDA, SCL  ----------+---> PCA9685 (0x40, 50 Hz)
                 |  GPIO: /OE     -----------+---> PCA9685 /OE (active low)
                 |                           |         |
                 |  (S3 only) DVP camera     |         +--> ch0 base     servo
                 +---------------------------+         +--> ch1 tilt     servo
                                                       +--> ch2 lids
     servo supply (switched, 4.8-6 V) --> PCA V+ ------+--> ch3 mouth
     common ground with the XIAO                       +--> ch4 ears
```

Logic and servo power are separate. PCA9685 logic VCC is 3.3 V from the XIAO;
servo V+ comes from its own switched supply. Grounds are common. Never take
servo current from the XIAO regulator or the USB 3.3 V rail.

## Parts

| Part | Role | Notes |
|---|---|---|
| Seeed XIAO ESP32-S3 Sense | S3 Coglet MCU | PSRAM, camera connector |
| Seeed XIAO ESP32-C3 | C3 Coglet MCU | no PSRAM, no camera, no user LED |
| INMP441 | I2S microphone | 3.3 V, L/R pin selects the I2S slot |
| MAX98357A | I2S class-D amp | 5 V, SD pin sets gain and shutdown |
| PCA9685 breakout | 16-channel servo PWM | address 0x40, 100 kHz I2C, ~50 Hz output |
| Hobby servos x5 | base, tilt, lids (one servo, both top lids), mouth, ears | model not yet recorded — see Unknowns |
| Servo supply | switched V+ for PCA | sized for the fitted servos; physical switch is the emergency stop |
| Camera (S3 only) | OV2640 / OV3660 / OV5640 | select in menuconfig; disabled until confirmed |

## Pin map — XIAO ESP32-S3 Sense (`coglet`)

| Function | Pad | GPIO |
|---|---|---|
| Amp DIN (speaker data) | D0 | 1 |
| I2S BCLK (shared) | D1 | 2 |
| I2S WS/LRC (shared) | D2 | 3 |
| Mic SD (data in) | D3 | 4 |
| PCA9685 SDA (I2C port 1) | D4 | 5 |
| PCA9685 SCL (I2C port 1) | D5 | 6 |
| PCA9685 /OE | D10 | 9 |
| User LED (active low) | — | 21 |
| Boot button | — | 0 |
| Camera SCCB SDA / SCL | — | 40 / 39 |
| Camera D0..D7 | — | 15, 17, 18, 16, 14, 12, 11, 48 |
| Camera XCLK / PCLK / VSYNC / HREF | — | 10 / 13 / 38 / 47 |

Servo I2C is controller **1** so the camera keeps controller 0. /OE on GPIO9 is
not the camera's XCLK GPIO10.

## Pin map — XIAO ESP32-C3 (`coglet-c3`)

| Function | Pad | GPIO |
|---|---|---|
| Amp DIN (speaker data) | D0 | 2 |
| I2S BCLK (shared) | D1 | 3 |
| Mic SD (data in) | **D2** | 4 |
| I2S WS/LRC (shared) | **D3** | 5 |
| PCA9685 **SCL** | **D4** | 6 |
| PCA9685 **SDA** | **D5** | 7 |
| PCA9685 /OE | D10 | 10 |
| Boot button | — | 9 |

Two pairs run opposite to the S3 and to the XIAO silkscreen default, both
confirmed against hardware:

- **D2 is mic data, D3 is WS.** Swapped, the amp runs with no LRC (steady hiss)
  and capture samples a clock line, reading flat zero.
- **SCL on D4, SDA on D5.** Swapped, no address on the bus answers at all.

GPIO8 and GPIO9 are strapping pins (GPIO9 is the boot button), so /OE uses
GPIO10. The C3 has no user-controllable LED; see issue #8.

## Power and safety

- Servo V+ is a separate switched supply; that switch is the real emergency stop.
- PCA9685 logic VCC 3.3 V, I2C pull-ups to 3.3 V, grounds common.
- /OE is active low. The firmware drives it high before configuring the PCA, so
  outputs stay off across a reset **only if /OE is actually wired** and pulled up
  to the PCA logic rail. Many breakouts ship with /OE pulled down; then release
  depends on I2C alone and cannot stop PWM if that bus fails.
- Every boot starts released with no pose commanded, whatever is stored in NVS.

## Bench checks

```bash
!status                  # firmware, IP, heap
!coglet scan             # expect 0x40 on the configured pins
!coglet state            # released, pca_initialized, per-role mapping
!mic                     # peak meter; zero means wiring, not code
!speaker test            # chime through the amp
```

`!coglet scan <sda> <scl>` rebuilds the bus on other pins, which distinguishes a
swapped pair (silent bus) from a wrong address or an unpowered board. It tears
the bus down, so reboot afterwards.

## Unknowns to confirm at the bench

1. **Servo model** and its true pulse range. The firmware's 1000-2000 us default
   is a placeholder, which is why a 70-110 degree window moves so little.
2. **Servo supply** voltage and current rating.
3. **Channel assignment** for lids, mouth and ears. Only base (ch0) and tilt
   (ch1) are mapped today.
4. **Is /OE wired** on this breakout, and is it pulled up or down?
5. **Ears**: one shared servo or two independent? The role model assumes one.
6. **Camera module marking** on the S3 unit (OV2640 / OV3660 / OV5640).
