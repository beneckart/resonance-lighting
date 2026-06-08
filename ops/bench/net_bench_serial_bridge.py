#!/usr/bin/env python3
"""Relay a USB-tethered net_bench SERIAL BRIDGE board to UDP:54321.

The desk bridge board (built `./build.sh --role master --serial-bridge`) hears the
field fleet over ESP-NOW and prints the same `nb-master` / `nb-peer` / `nb-scanap`
lines it would otherwise UDP-broadcast -- but to USB serial, so no laptop has to be
in the field. This script reads that serial port and re-emits every `nb-*` line as a
UDP broadcast to :54321, so ALL the existing UDP tooling works unchanged:

  # terminal 1: relay the bridge board's serial to UDP
  ./net_bench_serial_bridge.py --port /dev/ttyACM0

  # terminal 2: the usual loggers/monitors, now fed from the field-over-serial source
  ./net_bench_log.py --site ca --notes "yard 2.4GHz coverage scan"
  ./net_bench_monitor.py

Raw serial is also teed to stdout (and optionally a file) so you see scan progress
and any board chatter live. Stdlib + pyserial.

Note: this is for the SERIAL bridge. A WiFi-STA master already UDP-broadcasts
directly -- you don't need this for that path.
"""
import argparse, socket, sys, time
import serial  # pyserial

ap = argparse.ArgumentParser()
ap.add_argument("--port", default="/dev/ttyACM0", help="bridge board USB serial port")
ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--udp-port", type=int, default=54321)
ap.add_argument("--udp-host", default="255.255.255.255", help="broadcast by default")
ap.add_argument("--raw", default=None, help="also tee raw serial lines to this file")
ap.add_argument("--quiet", action="store_true", help="don't echo raw serial to stdout")
a = ap.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
rawfh = open(a.raw, "a") if a.raw else None

# Reconnect loop: the native-USB-CDC port can drop on board reset; just retry.
print(f"net_bench_serial_bridge: {a.port}@{a.baud} -> udp {a.udp_host}:{a.udp_port}",
      flush=True)
fwd = 0
while True:
    try:
        ser = serial.Serial(a.port, a.baud, timeout=1.0)
    except (serial.SerialException, OSError) as e:
        print(f"  (waiting for {a.port}: {e})", flush=True)
        time.sleep(2.0)
        continue
    print(f"  opened {a.port}", flush=True)
    try:
        while True:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", "replace").rstrip("\r\n")
            if not a.quiet:
                print(line, flush=True)
            if rawfh:
                rawfh.write(line + "\n"); rawfh.flush()
            if line.startswith("nb-"):
                # send WITH a trailing newline -- the UDP loggers' regexes are
                # line-oriented and net_bench packets are one line each.
                sock.sendto((line + "\n").encode("utf-8"),
                            (a.udp_host, a.udp_port))
                fwd += 1
                if fwd % 200 == 0:
                    print(f"  [relayed {fwd} nb-* lines]", flush=True)
    except (serial.SerialException, OSError) as e:
        print(f"  (serial dropped: {e}; reopening)", flush=True)
        try:
            ser.close()
        except Exception:
            pass
        time.sleep(1.0)
    except KeyboardInterrupt:
        print(f"\n=== stopped, relayed {fwd} lines ===", flush=True)
        break
