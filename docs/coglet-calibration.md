# Coglet servo calibration (one servo at a time)

The mechanism is built up one servo at a time, so every step here works on a
partly wired robot. Nothing in this procedure requires the other roles to exist.

Roles: `base`, `tilt`, `lid_left`, `lid_right`, `mouth`, `ears`.
Endpoints are **semantic**, and may run numerically backwards:

| Role | `low` means | `high` means |
|---|---|---|
| base | left | right |
| tilt | down | up |
| lids | closed | open |
| mouth, ears | mechanism specific | mechanism specific |

All calibration commands are **local only** (serial or the builder web page)
and most are **builder mode only**. Pulses never leave the hard 1000-2000 us
bound, whatever is asked for.

## Procedure for one servo

Servo power on, one servo fitted, hand near the power switch.

```bash
!coglet builder              # outputs on; nothing moves
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

`engage` needs **base and tilt** confirmed, and every *assigned* role confirmed.
Roles left unassigned are treated as not fitted and are skipped:

```bash
!coglet release
!coglet engage
!coglet gaze 0 0             # works with only base and tilt
!coglet gaze -100 0          # full left
```

`blink` and automatic blinking need a lid servo; without one, blink returns
`ESP_ERR_NOT_SUPPORTED` and auto-blink stays off.

## Backing up

```bash
!coglet export > coglet-calibration.json
python3 main/boards/coglet/tools/restore_commands.py coglet-calibration.json > restore.txt
```

Review `restore.txt` before applying it over serial. It releases, restores the
mapping, saves and exports for comparison; it never engages. Only restore
calibration measured for the same unchanged mechanism.

## Still to do

- The lid role model assumes two independent upper lids. The real mechanism has
  **one servo for both top lids**, so the pair collapses to a single `lids`
  role and `wink` degrades to a blink. Not yet implemented.
- Voice-guided calibration, where the robot walks you through this by talking.
