#!/usr/bin/env python3
"""Log the presence bench (firmware/presence_bench) to JSONL.

Polls http://<host>/api/frame (full sensor frames: MLX90640 thermal, VL53L5CX
multizone ToF, TMF8821 zones, XM125 radar) and periodically /api/state, appending
typed rows to ops/bench/data/presence/<stamp>_<label>.jsonl:

  {"type":"meta", ...}          first line: label/host/notes/firmware version
  {"type":"frame", ...}         one per poll -- the /api/frame JSON + timestamps
  {"type":"state", ...}         every --state-every frames
  {"type":"mark", ...}          press Enter (optionally type a note first) during a
                                walk-under -> ground-truth mark for latency stats
  {"type":"summary", ...}       trailing line: frames, drops, per-sensor rates

Stdlib only. Usage:
  python3 presence_logger.py --host presencebench.local --label walkunder-r1 \
      [--hz 3] [--duration 120] [--state-every 5] [--notes "..."]

Detection thresholds intentionally do NOT live here -- the dashboard (browser JS)
and offline analysis own detection; this logger records raw frames so any rule can
be re-run against the data later.
"""

import argparse
import json
import select
import sys
import time
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

DEFAULT_OUT = Path(__file__).parent / "data" / "presence"


def fetch_json(url, timeout=5.0):
    with urllib.request.urlopen(url, timeout=timeout) as r:
        return json.loads(r.read().decode())


def utcnow():
    return datetime.now(timezone.utc).isoformat()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--host", default="presencebench.local")
    ap.add_argument("--label", required=True, help="run label (goes in the filename)")
    ap.add_argument("--hz", type=float, default=3.0, help="frame poll rate (default 3)")
    ap.add_argument("--duration", type=float, default=0, help="seconds; 0 = until Ctrl-C")
    ap.add_argument("--state-every", type=int, default=5, help="merge a state row every N frames")
    ap.add_argument("--notes", default="")
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT))
    args = ap.parse_args()

    base = f"http://{args.host}"
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y-%m-%d_%H%M%S")
    path = out_dir / f"{stamp}_{args.label}.jsonl"

    try:
        state0 = fetch_json(base + "/api/state")
    except Exception as e:
        sys.exit(f"cannot reach {base}/api/state: {e}")

    meta = {
        "type": "meta",
        "label": args.label,
        "host": args.host,
        "notes": args.notes,
        "start_utc": utcnow(),
        "fw": state0.get("v"),
        "i2c_hz": state0.get("i2c_hz"),
        "xm_app": state0.get("xm_app"),
        "poll_hz": args.hz,
    }

    frames = 0
    drops = 0
    marks = 0
    period = 1.0 / max(0.1, args.hz)
    t_end = time.time() + args.duration if args.duration > 0 else None

    print(f"logging -> {path}")
    print("press Enter to drop a ground-truth mark (type a note first for a labeled mark); Ctrl-C to stop")

    with open(path, "w") as f:
        f.write(json.dumps(meta) + "\n")
        try:
            while t_end is None or time.time() < t_end:
                t0 = time.time()
                # non-blocking stdin: Enter = mark
                while select.select([sys.stdin], [], [], 0)[0]:
                    note = sys.stdin.readline().strip()
                    marks += 1
                    row = {"type": "mark", "ts_utc": utcnow(), "t_host": time.time(), "note": note}
                    f.write(json.dumps(row) + "\n")
                    f.flush()
                    print(f"  mark #{marks} {note or ''}")
                try:
                    frame = fetch_json(base + "/api/frame")
                    row = {"type": "frame", "ts_utc": utcnow(), "t_host": time.time()}
                    row.update(frame)
                    f.write(json.dumps(row) + "\n")
                    frames += 1
                    if args.state_every > 0 and frames % args.state_every == 0:
                        st = fetch_json(base + "/api/state")
                        srow = {"type": "state", "ts_utc": utcnow(), "t_host": time.time()}
                        srow.update(st)
                        f.write(json.dumps(srow) + "\n")
                except Exception as e:
                    drops += 1
                    if drops % 10 == 1:
                        print(f"  poll error ({drops} so far): {e}", file=sys.stderr)
                dt = time.time() - t0
                if dt < period:
                    time.sleep(period - dt)
        except KeyboardInterrupt:
            print("\nstopping")
        finally:
            per_sensor = {}
            try:
                st = fetch_json(base + "/api/state", timeout=2.0)
                for k in ("mlx", "vl53", "tmf", "xm"):
                    if isinstance(st.get(k), dict):
                        per_sensor[k] = {"st": st[k].get("st"), "hz": st[k].get("hz"),
                                         "err": st[k].get("err")}
            except Exception:
                pass
            summary = {"type": "summary", "end_utc": utcnow(), "frames": frames,
                       "dropped": drops, "marks": marks, "per_sensor": per_sensor}
            f.write(json.dumps(summary) + "\n")
    print(f"done: {frames} frames, {drops} drops, {marks} marks -> {path}")


if __name__ == "__main__":
    main()
