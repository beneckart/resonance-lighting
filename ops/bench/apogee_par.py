#!/usr/bin/env python3
"""
Apogee SQ-420 USB PAR Sensor Reader

Reads photosynthetically active radiation (PAR) in µmol/m²/s.
Protocol reverse-engineered: sends 0x55 0x21, reads back 5 bytes
(1 byte status + little-endian float), applies calibration.

Usage:
    python apogee_par.py              # single reading
    python apogee_par.py --continuous  # continuous readings (1/sec)
    python apogee_par.py --port /dev/ttyUSB1  # specify port
"""

import argparse
import glob
import struct
import sys
import time

import serial


def find_sensor_port():
    """Auto-detect the sensor's serial port."""
    candidates = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if not candidates:
        sys.exit(
            "No USB serial devices found. Is the sensor plugged in?\n"
            "You may need to add yourself to the 'dialout' group:\n"
            "  sudo usermod -aG dialout $USER  (then log out/in)"
        )
    if len(candidates) == 1:
        return candidates[0]
    print(f"Multiple serial devices found: {', '.join(candidates)}")
    print(f"Trying {candidates[0]} — override with --port if wrong.")
    return candidates[0]


def read_par(ser):
    """Send command and read PAR value from sensor."""
    ser.reset_input_buffer()
    ser.write(b"\x55\x21")
    raw = ser.read(5)
    if len(raw) < 5:
        raise IOError(f"Expected 5 bytes, got {len(raw)}")
    _status, raw_value = struct.unpack("<bf", raw)
    par = (raw_value - 0.00171) * 26010
    return par


def main():
    parser = argparse.ArgumentParser(description="Read Apogee SQ-420 PAR sensor")
    parser.add_argument("--port", help="Serial port (default: auto-detect)")
    parser.add_argument(
        "--continuous", action="store_true", help="Continuous readings, 1/sec"
    )
    parser.add_argument(
        "--interval", type=float, default=1.0, help="Seconds between readings (default: 1.0)"
    )
    parser.add_argument(
        "--csv", action="store_true", help="Output as CSV (timestamp, par)"
    )
    args = parser.parse_args()

    port = args.port or find_sensor_port()

    try:
        ser = serial.Serial(port, baudrate=115200, xonxoff=False, timeout=0.5)
    except serial.SerialException as e:
        sys.exit(f"Could not open {port}: {e}")

    print(f"Connected to {port}", file=sys.stderr)

    if args.csv:
        print("timestamp,par_umol_m2_s")

    try:
        if args.continuous:
            while True:
                par = read_par(ser)
                if args.csv:
                    print(f"{time.time():.3f},{par:.1f}")
                else:
                    print(f"\r  PAR: {par:7.1f} µmol/m²/s", end="", flush=True)
                time.sleep(args.interval)
        else:
            par = read_par(ser)
            if args.csv:
                print(f"{time.time():.3f},{par:.1f}")
            else:
                print(f"  PAR: {par:.1f} µmol/m²/s")
    except KeyboardInterrupt:
        print("\n")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
