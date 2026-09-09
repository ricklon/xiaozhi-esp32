#!/usr/bin/env python3
"""Convert a Coglet state/export JSON file into reviewable serial commands.
Prints only: never connects, writes NVS, engages or flashes hardware.
Eyemech calibration intentionally cannot be imported into different mechanics.
"""
import json
import math
import sys
from pathlib import Path

roles = ('base', 'tilt', 'lid_left', 'lid_right', 'mouth', 'ears')
data = json.loads(Path(sys.argv[1]).read_text())
if data.get('calibration_version') != 1:
    raise SystemExit('Expected Coglet calibration_version 1; no Eyemech import')
axes = data['axes']
if len(axes) != 6 or {a['role'] for a in axes} != set(roles):
    raise SystemExit('Expected all six Coglet roles exactly once')
commands = ['!coglet release']
for role in roles:
    commands.append(f'!coglet configure {role} -1 90 90 1000 2000 0 0')
used = set()
for a in axes:
    values = [a[k] for k in ('channel', 'low', 'high', 'min_us', 'max_us', 'trim_us')]
    if not all(type(v) in (int, float) and math.isfinite(v) for v in values):
        raise SystemExit('Non-finite or non-numeric calibration')
    channel, low, high, min_us, max_us, trim = values
    if any(type(v) is not int for v in (channel, min_us, max_us, trim)):
        raise SystemExit('Channel and pulse fields must be integers')
    if not (-1 <= channel <= 5 and 0 <= low <= 180 and 0 <= high <= 180
            and 300 <= min_us < max_us <= 3000
            and 300 <= min_us + trim < max_us + trim <= 3000):
        raise SystemExit('Calibration bounds invalid')
    if channel >= 0:
        if channel in used:
            raise SystemExit('Duplicate channel')
        used.add(channel)
    confirmed = a['confirmed']
    if type(confirmed) is not bool or (confirmed and (channel < 0 or low == high)):
        raise SystemExit('Invalid confirmation')
    commands.append('!coglet configure '+a['role']+' '+' '.join(map(str, values))+f' {int(confirmed)}')
trim, coeff = data['lid_trim'], data['upper_coeff']
if not all(type(v) in (int, float) and math.isfinite(v) and 0 <= v <= 1 for v in (trim, coeff)):
    raise SystemExit('Invalid lid coupling')
commands += [f'!coglet lids {trim} {coeff}', '!coglet save', '!coglet export']
print('\n'.join(commands))
