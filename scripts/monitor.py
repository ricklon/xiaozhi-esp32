#!/usr/bin/env python3
"""Minimal serial console reader.

idf.py monitor requires an interactive TTY, which isn't always available
(e.g. when driven from just/CI). This reads lines from the device and prints
them. Used by the `just monitor` recipe.

Usage:
    python3 scripts/monitor.py [PORT] [SECONDS]

PORT defaults to /dev/ttyACM0. SECONDS=0 (default) reads until Ctrl-C;
pass a positive number to stop automatically after that many seconds.
"""
import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
limit = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0

s = serial.Serial(port, 115200, timeout=1)
start = time.time()
try:
    while limit == 0 or time.time() - start < limit:
        line = s.readline().decode("utf-8", "replace").rstrip()
        if line:
            print(line, flush=True)
except KeyboardInterrupt:
    pass
finally:
    s.close()
