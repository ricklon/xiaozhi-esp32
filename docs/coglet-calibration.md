# Coglet servo calibration (one servo at a time)

The mechanism is built up one servo at a time, so every step here works on a
partly wired robot. Nothing in this procedure requires the other roles to exist.

Roles, in the order the servos are fitted (bottom of the mechanism upward):
`base`, `neck_tilt`, `neck_roll`, `eye_pan`, `eye_tilt`, `lids`, `jaw`,
`ear_left`, `ear_right`. One servo drives both top lids, so `lids` is a single
axis and there are no per-eye lid roles.
Endpoints are **semantic**, and may run numerically backwards:

| Role | `low` means | `high` means |
|---|---|---|
| base | left | right |
| neck_tilt | down | up |
| neck_roll | left | right |
| eye_pan | left | right |
| eye_tilt | down | up |
| lids | closed | open |
| jaw | closed | open |
| ear_left, ear_right | back | forward |

All calibration commands are **local only** (serial or the builder web page)
and most are **builder mode only**. Pulses never leave the hard 1000-2000 us
bound, whatever is asked for.

## Silence first

A voice agent answering room noise through the robot's own speaker makes
endpoint hunting impossible, so calibration runs with the voice side quiet:

- **`!coglet builder` pauses it automatically.** Entering builder mode aborts any
  reply in progress, pauses listening, and stops the board reconnecting.
- **Quiet does not lift by itself.** Leaving builder mode keeps the robot quiet,
  because the LLM can call `self.coglet.release` over MCP: an auto-resume let the
  robot talk its own way out of a calibration session. Say `!quiet off` when you
  are done.
- **`!quiet on` / `!quiet off`** does the same by hand, on any board, and survives
  until toggled or rebooted. `!quiet status` reports it.
- **Unplugging the speaker** is the hardware fallback and needs no firmware.
- The hub's **listen mode** silences replies without touching the robot, but it
  keeps the session open; builder mode is the better fit while calibrating.

## Procedure for one servo

Servo power on, one servo fitted, hand near the power switch.

```bash
!coglet builder              # outputs on; nothing moves; voice agent pauses
!coglet identify 0           # wiggles raw channel 0 ~1.4s; repeat per channel
                             # until the part you just fitted twitches
!coglet release              # channel mapping is edited while released
!coglet configure base 0 90 90 1000 2000 0 0
                             # assign the channel with a 90/90 window: that
                             # permits only a 90 degree horn-seating command
!coglet builder
!coglet explore base         # centres the servo at 1500 us and starts hunting
!coglet nudge -2             # step up to +-5 degrees at a time
!coglet nudge -2             # ... keep going, stopping SHORT of binding
!coglet mark low             # this position is the LEFT end
!coglet nudge 2              # ... walk out to the other end
!coglet mark high            # this position is the RIGHT end
!coglet confirm base         # a human watched both ends
!coglet save                 # persists; allowed in builder mode
```

Then check the result:

```bash
!coglet servo base 90        # inside the calibrated window now
!coglet jog base 5
!coglet state                # confirmed:true, low/high as marked
```

Notes:

- `mark` clears `confirmed` for that role, so a moved endpoint always has to be
  watched again before the robot will use it.
- `explore` deliberately leaves `commanded_degrees` null: an explored position
  is outside the calibrated window and is not a pose.
- `identify` holds the bus for about 1.4 s while it wiggles.
- Release before editing a channel mapping; `configure` refuses while engaged.

## When enough servos are fitted

`engage` needs at least one role fitted and every *assigned* role confirmed.
Roles left unassigned are treated as not fitted and are skipped.

Only three roles have choreography: `gaze` and the animations drive `eye_pan`
and `eye_tilt`, and `blink` drives `lids`. The neck, jaw and ears are
builder-only for now, so they stay still during conversation:

```bash
!coglet release
!coglet engage
!coglet gaze 0 0             # needs eye_pan and eye_tilt fitted
!coglet gaze -100 0          # eyes full left
```

`blink` and automatic blinking need the lid servo; without it, blink returns
`ESP_ERR_NOT_SUPPORTED` and auto-blink stays off. `gaze` and `animate` likewise
return `ESP_ERR_NOT_SUPPORTED` until both eye axes are fitted — expected while
working bottom-up, where the base is calibrated long before the eyes exist.

## Backing up

```bash
!coglet export > coglet-calibration.json
python3 main/boards/coglet/tools/restore_commands.py coglet-calibration.json > restore.txt
```

Review `restore.txt` before applying it over serial. It releases, restores the
mapping, saves and exports for comparison; it never engages. Only restore
calibration measured for the same unchanged mechanism.

Note `!coglet lids TRIM COEFF` (upper-lid coupling) is a different command
from the `lids` **role** used by `configure`, `explore` and `confirm`.

## Still to do

- Voice-guided calibration, where the robot walks you through this by talking.
- Neck motion during conversation: the head could follow gaze beyond the eyes'
  travel. Left out until the neck is calibrated and its load is understood.
- Jaw motion tied to speech, and any ear choreography.
