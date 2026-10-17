#!/usr/bin/env python3
"""Load a Tone Lab show file onto a T-Deck running the Bridge OS Tone Lab app.

Export the show first (Tone Lab > Interactions > Export for T-Deck, which writes
ops/bench/data/<site>/tonelab-show.tls), then:

  python3 ops/bench/tdeck_show_send.py "ops/bench/data/Brian bench/tonelab-show.tls"
  python3 ops/bench/tdeck_show_send.py <show.tls> --port /dev/cu.usbmodem144401

Without --port, the T-Deck is found by its USB serial MAC (known bridges below,
or --id). Opening the port can restart the T-Deck on macOS, so this waits for
"Bridge OS up". The payload is paced in 64-byte chunks: the T-Deck's USB RX
buffer is small and drops overflow. The T-Deck verifies the CRC before storing.
"""
import argparse, sys, time
import serial
from serial.tools import list_ports

KNOWN_TDECKS = {"979604", "8EB508"}

ap = argparse.ArgumentParser()
ap.add_argument("show")
ap.add_argument("--port")
ap.add_argument("--id", help="T-Deck short MAC (default: any known bridge)")
a = ap.parse_args()
data = open(a.show, "rb").read()
port = a.port
if not port:
    want = {a.id.upper()} if a.id else KNOWN_TDECKS
    hits = [p.device for p in list_ports.comports()
            if (p.serial_number or "").replace(":", "").upper()[-6:] in want]
    if len(hits) != 1:
        sys.exit(f"found {len(hits)} T-Decks ({hits}); pass --port or --id")
    port = hits[0]
s = serial.Serial(); s.port = port; s.baudrate = 115200; s.timeout = 0.2
s.dtr = False; s.rts = False; s.open()
end, buf = time.time() + 12, b""
while time.time() < end and b"Bridge OS up" not in buf:
    buf += s.read(4096)
time.sleep(0.5); s.reset_input_buffer()
s.write(f"tl-show {len(data)}\n".encode()); time.sleep(0.05)
for i in range(0, len(data), 64):
    s.write(data[i:i + 64]); s.flush(); time.sleep(0.012)
end, buf = time.time() + 6, b""
while time.time() < end and b"tl-show ok" not in buf and b"tl-show error" not in buf:
    buf += s.read(4096)
for line in buf.decode(errors="replace").splitlines():
    if line.startswith("tl-show"):
        print(line)
        sys.exit(0 if line.startswith("tl-show ok") else 1)
sys.exit("no reply from the T-Deck (is it running the Tone Lab build?)")
