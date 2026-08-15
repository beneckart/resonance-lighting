#!/usr/bin/env python3
"""Serial->file tee for the ina_monitor stream (KB2040/Metro).

Owns the monitor's serial port (only ONE process can) and appends every line,
host-timestamped, to a shared file. Consumers tail that file instead of the port --
which is how TWO rigs share one 4-channel monitor: each afk_discharge.py instance
runs with --ina-file <same file> and filters its own --batt-ch/--led-ch addresses.
The raw line is kept intact after the timestamp so every existing ina-line regex
still matches, and the file doubles as the post-hoc reconcile/re-integration input.

  ./ina_logger.py --port /dev/ttyACM2 --out data/ca/2026-07-07-ina.log

Stdlib + pyserial.
"""
import argparse, os
import serial
from datetime import datetime, timezone

ap = argparse.ArgumentParser()
ap.add_argument("--port", default="/dev/ttyACM2")
ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--out", required=True)
ap.add_argument("--quiet", action="store_true", help="no stdout heartbeat")
a = ap.parse_args()

os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
ser = serial.Serial(a.port, a.baud, timeout=1.0)
n = 0
with open(a.out, "a") as fh:
    print(f"ina_logger: {a.port} -> {a.out} (Ctrl-C to stop)")
    try:
        while True:
            line = ser.readline().decode("utf-8", "replace").strip()
            if not line:
                continue
            fh.write(datetime.now(timezone.utc).isoformat() + " " + line + "\n")
            fh.flush()
            n += 1
            if not a.quiet and n % 200 == 0:  # heartbeat, not a firehose
                print(f"  {n} lines ({line})")
    except KeyboardInterrupt:
        print(f"\nstopped after {n} lines -> {a.out}")
