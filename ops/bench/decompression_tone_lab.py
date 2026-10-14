#!/usr/bin/env python3
"""Decompression tone lab: classify lanterns by chime type and tone order.

Local web app for USB-connected decompression-canary lanterns. Strikes are
single exact-target `K<id>:<ms>` commands sent only after a fresh telemetry
identity check on that port. Classification state is saved on every change to
ops/bench/data/<site>/decompression-tone-map.json.

    python ops/bench/decompression_tone_lab.py --site "Brian bench" --port 8765

Requires pyserial.
"""
import argparse
import csv
import glob
import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import serial

try:  # optional: microphone latency sync and timing measurement
    import numpy as np
    import sounddevice as sd
except ImportError:
    np = sd = None

REPO = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser()
ap.add_argument("--site", default="Brian bench")
ap.add_argument("--port", type=int, default=8765)
ap.add_argument("--pulse-ms", type=int, default=40, help="default pulse when the state file has none")
ARGS = ap.parse_args()

DATA_DIR = REPO / "ops" / "bench" / "data" / ARGS.site
STATE_FILE = DATA_DIR / "decompression-tone-map.json"
STRIKE_LOG = DATA_DIR / "decompression-tone-strikes.jsonl"
CAL_LOG = DATA_DIR / "decompression-latency-cal.jsonl"
CALLSIGNS = {r["fixture_id"]: r["callsign"] for r in csv.DictReader(open(REPO / "ops/fleet/callsigns.csv"))}

lanterns = {}           # fixture_id -> live info (port, telemetry summary)
port_locks = {}         # port -> lock
state_lock = threading.Lock()


def utc():
    return time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime()) + f".{int(time.time() * 1000) % 1000:03d}Z"


def load_state():
    if STATE_FILE.exists():
        return json.loads(STATE_FILE.read_text())
    return {"schema": 1, "site": ARGS.site, "order_direction": "high_to_low", "lanterns": {}, "order": [], "updated_utc": None}


def save_state(st):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    st["updated_utc"] = utc()
    tmp = STATE_FILE.with_suffix(".json.tmp")
    tmp.write_text(json.dumps(st, indent=2) + "\n")
    tmp.replace(STATE_FILE)


STATE = load_state()
STATE.setdefault("order_direction", "high_to_low")
STATE.setdefault("pulse_ms", ARGS.pulse_ms)
STATE.setdefault("presets", {})
# The app caps pulses well below the firmware's 300 ms limit while widths are being explored.
PULSE_MIN_MS, PULSE_MAX_MS = 5, 100


def pulse_ok(ms):
    return isinstance(ms, int) and PULSE_MIN_MS <= ms <= PULSE_MAX_MS


def lock_for(port):
    with state_lock:
        return port_locks.setdefault(port, threading.Lock())


def telemetry(s, wait=2.0):
    s.reset_input_buffer()
    s.write(b"t\n")
    end, buf = time.time() + wait, b""
    while time.time() < end:
        buf += s.read(4096)
        for ln in buf.split(b"\n"):
            if ln.startswith(b"{") and ln.rstrip().endswith(b"}"):
                try:
                    return json.loads(ln)
                except ValueError:
                    pass
    return None


def summarize(port, d):
    return {
        "fixture_id": d.get("fixture_id"), "callsign": CALLSIGNS.get(d.get("fixture_id")),
        "port": port, "hub": port.replace("/dev/cu.usbmodem", "")[:-1], "fw": d.get("fw"),
        "battery_v": d.get("battery_v"), "battery_ma": d.get("battery_ma"), "tier": d.get("power_tier"),
        "bq_fault": d.get("bq_fault0"), "cap_mah": d.get("battery_capacity_mah"),
        "strikes": d.get("solenoid_strikes"), "failsafes": d.get("solenoid_failsafes"),
        "blocked": d.get("solenoid_blocked"), "session_left_s": d.get("decompression_left_s"),
        "decompression": bool(d.get("decompression")), "seen_utc": utc(),
    }


def query_port(port):
    with lock_for(port):
        try:
            with serial.Serial(port, 115200, timeout=0.1) as s:
                time.sleep(0.15)
                d = telemetry(s)
        except Exception:
            return None
    if d and d.get("fixture_id"):
        info = summarize(port, d)
        with state_lock:
            lanterns[info["fixture_id"]] = info
        return info
    return None


def scan():
    ports = glob.glob("/dev/cu.usbmodem*")
    threads = [threading.Thread(target=query_port, args=(p,)) for p in ports]
    for t in threads: t.start()
    for t in threads: t.join()
    live = {l["port"] for l in lanterns.values()}
    with state_lock:
        for fid in [f for f, l in lanterns.items() if l["port"] not in ports]:
            lanterns[fid]["offline"] = True
    return snapshot()


def send_exact(fid, line, verify_strike=False, pulse_ms=None):
    """Confirm identity on the lantern's port, send one exact-target line, return result."""
    info = lanterns.get(fid)
    if not info:
        return {"ok": False, "error": f"{fid} not found; rescan"}
    port = info["port"]
    with lock_for(port):
        try:
            with serial.Serial(port, 115200, timeout=0.1) as s:
                time.sleep(0.15)
                before = telemetry(s)
                if not before or before.get("fixture_id") != fid:
                    return {"ok": False, "error": f"identity mismatch on {port}: {before and before.get('fixture_id')}"}
                sent = utc()
                s.write((line + "\n").encode())
                time.sleep(0.6)
                reply = s.read(4096).decode(errors="replace")
                after = telemetry(s)
        except Exception as ex:
            return {"ok": False, "error": str(ex)}
    if after:
        with state_lock:
            lanterns[fid] = summarize(port, after)
    res = {"ok": True, "fixture_id": fid, "cmd": line, "pulse_ms": pulse_ms, "utc": sent, "reply": reply.strip()[-200:],
           "strikes_before": before.get("solenoid_strikes"), "strikes_after": after and after.get("solenoid_strikes"),
           "blocked": after and after.get("solenoid_blocked"), "failsafes": after and after.get("solenoid_failsafes"),
           "battery_v": after and after.get("battery_v"), "session_left_s": after and after.get("decompression_left_s")}
    if verify_strike:
        res["ok"] = res["strikes_after"] is not None and res["strikes_after"] == (res["strikes_before"] or 0) + 1
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(STRIKE_LOG, "a") as f:
        f.write(json.dumps(res) + "\n")
    return res


class Recorder:
    """Microphone capture with PortAudio stream timestamps.

    Latency is measured as (sound onset ADC time) - (stream time just before the
    serial write). The microphone's own input latency is common to every lantern,
    so it cancels when lanterns are equalized against each other."""

    def __init__(self, fs=44100):
        self.fs, self.blocks = fs, []

    def _cb(self, indata, frames, t, status):
        adc = t.inputBufferAdcTime or (t.currentTime - frames / self.fs)
        self.blocks.append((adc, indata[:, 0].copy()))

    def __enter__(self):
        self.stream = sd.InputStream(samplerate=self.fs, channels=1, latency="low", callback=self._cb)
        self.stream.start()
        return self

    def __exit__(self, *a):
        self.stream.stop()
        self.stream.close()

    def now(self):
        return self.stream.time

    def envelope(self):
        """1 ms RMS of the first difference (a crude high-pass that favors the strike transient)."""
        t_start = self.blocks[0][0]
        x = np.concatenate([b for _, b in self.blocks])
        y = np.diff(x, prepend=x[:1])
        hop = self.fs // 1000
        n = len(y) // hop
        e = np.sqrt(np.mean(y[: n * hop].reshape(n, hop) ** 2, axis=1))
        frame_ms = hop * 1000.0 / self.fs  # 0.998 ms at 44.1 kHz; never assume exactly 1 ms
        return t_start, float(np.max(np.abs(x))) if len(x) else 0.0, e, frame_ms


def onset_strength(e):
    """Log-envelope rise over the trailing 5..15 ms mean."""
    le = np.log(e + 1e-7)
    cs = np.concatenate([[0.0], np.cumsum(le)])
    idx = np.arange(len(le))
    a, b = np.clip(idx - 15, 0, None), np.clip(idx - 5, 1, None)
    ref = (cs[b] - cs[a]) / np.maximum(b - a, 1)
    return le - ref


def rising_edge(o, peak, e=None):
    """Onset frame for a rise peak. With the envelope, take the first frame within
    10 ms before the peak that climbs a quarter of the way from the pre-onset level
    (robust to a previous chime still ringing); otherwise half the log rise."""
    lo = max(0, peak - 10)
    if e is not None:
        base = float(np.median(e[max(0, peak - 25):max(1, peak - 12)]))
        top = float(e[lo:peak + 3].max())
        seg = e[lo:peak + 3]
        return lo + int(np.argmax(seg - base >= 0.25 * (top - base)))
    seg = o[lo:peak + 1]
    return lo + int(np.argmax(seg >= 0.5 * o[peak]))


MIN_RISE = float(np.log(4.0)) if np is not None else 1.4  # onset must rise ~4x over the trailing level


def calibrate_one(fid, reps, pulse_ms):
    if sd is None:
        return {"ok": False, "error": "pip install numpy sounddevice to use the microphone"}
    info = lanterns.get(fid)
    if not info:
        return {"ok": False, "error": f"{fid} not found; rescan"}
    port, sends = info["port"], []
    with lock_for(port):
        try:
            with serial.Serial(port, 115200, timeout=0.05) as s:
                time.sleep(0.1)
                before = telemetry(s, wait=1.5)
                if not before or before.get("fixture_id") != fid:
                    return {"ok": False, "error": f"identity mismatch on {port}: {before and before.get('fixture_id')}"}
                with audio_lock, Recorder() as rec:
                    time.sleep(0.5)
                    for _ in range(reps):
                        sends.append(rec.now())
                        s.write(f"K{fid}:{pulse_ms}\n".encode())
                        time.sleep(1.8)
                    time.sleep(0.2)
                after = telemetry(s, wait=1.5)
        except Exception as ex:
            return {"ok": False, "error": str(ex)}
    t_start, peak_amp, e, fm = rec.envelope()
    if peak_amp < 1e-4:
        return {"ok": False, "error": "microphone is silent -- allow microphone access for this app in macOS Settings"}
    o = onset_strength(e)
    samples = []
    for ts in sends:
        exact = (ts - t_start) * 1000.0 / fm
        si = int(exact)
        win = o[si:si + int(400 / fm)]
        if len(win) and win.max() >= MIN_RISE:
            samples.append(round((rising_edge(o, si + int(np.argmax(win)), e) - exact) * fm, 1))
        else:
            samples.append(None)
    got = [v for v in samples if v is not None]
    c0, c1 = before.get("solenoid_strikes"), after and after.get("solenoid_strikes")
    res = {"fixture_id": fid, "utc": utc(), "pulse_ms": pulse_ms, "reps": reps, "samples_ms": samples,
           "strikes_before": c0, "strikes_after": c1}
    if len(got) < max(2, reps - 1):
        res.update(ok=False, error=f"heard {len(got)} of {reps} strikes -- quieter room or mic closer")
    else:
        res.update(ok=True, latency_ms=float(np.median(got)), jitter_ms=float(max(got) - min(got)))
        with state_lock:
            m = STATE["lanterns"].setdefault(fid, {"type": None, "note": ""})
            m.update(latency_ms=res["latency_ms"], latency_jitter_ms=res["jitter_ms"],
                     latency_pulse_ms=pulse_ms, latency_utc=res["utc"])
            save_state(STATE)
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(CAL_LOG, "a") as f:
        f.write(json.dumps(res) + "\n")
    return res


def attack_flux(rec, win=256):
    """Broadband onset strength ("superflux"-style), one value per ~1 ms hop.

    A mallet strike adds energy across many frequencies at once, while chimes that
    are already ringing are narrow tones whose beating moves only a few bins. Log
    magnitudes are compared with a frequency-max-filtered frame 2 hops earlier,
    and only increases are summed. Returns (t_start, frame_ms, flux) where flux[k]
    belongs to the frame centred at t_start + k*frame_ms + win/2/fs."""
    t_start = rec.blocks[0][0]
    x = np.concatenate([b for _, b in rec.blocks])
    hop = rec.fs // 1000
    n = (len(x) - win) // hop
    if n < 8:
        return t_start, hop * 1000.0 / rec.fs, np.zeros(1), float(np.max(np.abs(x))) if len(x) else 0.0
    idx = np.arange(win)[None, :] + hop * np.arange(n)[:, None]
    mag = np.abs(np.fft.rfft(x[idx] * np.hanning(win), axis=1))
    S = np.log1p(100.0 * mag)
    ref = np.maximum(np.maximum(S, np.roll(S, 1, axis=1)), np.roll(S, -1, axis=1))
    flux = np.zeros(n)
    flux[2:] = np.maximum(0.0, S[2:] - ref[:-2]).sum(axis=1)
    return t_start + (win / 2) / rec.fs, hop * 1000.0 / rec.fs, flux, float(np.max(np.abs(x)))


DEFAULT_LATENCY_MS = 65.0   # used to place the search window for an unsynced lantern
MEASURE_MIN_RISE = float(np.log(1.5)) if np is not None else 0.4  # in a dense run chimes ring over each other


def measure_hits(rec, expected_ms, gap_ms):
    """Locate each slot's chime near where it is expected (stream ms).

    Cymbals ring for seconds, so in a dense run a new chime rides on others still
    ringing. Each slot is searched only in a short window around its expected
    arrival, using the broadband attack flux, and the frame where the flux first
    reaches half its window peak is the onset. A strike whose window holds no
    clear attack (peak below 3x the window median) is reported as not heard.
    Returns one onset time (stream ms) or None per slot."""
    t0, fm, flux, peak_amp = attack_flux(rec)
    if peak_amp < 1e-4:
        return None, "microphone is silent -- allow microphone access in macOS Settings"
    before, after = min(35.0, 0.45 * gap_ms), min(80.0, 0.6 * gap_ms)
    hits = []
    for exp in expected_ms:
        lo = max(2, int((exp - before - t0 * 1000) / fm))
        hi = min(len(flux) - 1, int((exp + after - t0 * 1000) / fm))
        if hi - lo < 5:
            hits.append(None)
            continue
        w = flux[lo:hi]
        p = int(np.argmax(w))
        floor = float(np.median(w)) + 1e-9
        if w[p] < 3.0 * floor:
            hits.append(None)
            continue
        k = int(np.argmax(w[:p + 1] >= 0.5 * w[p]))
        hits.append(t0 * 1000 + (lo + k) * fm)
    return hits, None


audio_lock = threading.Lock()


def play(order, gap_ms, pulses, comp=False, measure=False):
    """Fast run: verify every identity in parallel, then fire one exact-target strike
    per lantern on an absolute schedule, then confirm all counters in parallel.
    With comp, each send is advanced by its lantern's calibrated latency so the
    sounds land on the grid. With measure, the microphone reports per-hit error."""
    targets = [(fid, lanterns[fid]["port"]) for fid in order if fid in lanterns and not lanterns[fid].get("offline")]
    if not targets:
        return {"ok": False, "error": "no online lanterns in order; rescan"}
    locks = [lock_for(p) for _, p in targets]
    for lk in locks: lk.acquire()
    conns, results = {}, {fid: {"fixture_id": fid} for fid, _ in targets}
    try:
        def open_and_check(fid, port):
            try:
                s = serial.Serial(port, 115200, timeout=0.05)
                time.sleep(0.1)
                d = telemetry(s, wait=1.5)
                if d and d.get("fixture_id") == fid:
                    conns[fid] = s
                    results[fid]["strikes_before"] = d.get("solenoid_strikes")
                else:
                    s.close()
                    results[fid]["error"] = f"identity mismatch: {d and d.get('fixture_id')}"
            except Exception as ex:
                results[fid]["error"] = str(ex)
        th = [threading.Thread(target=open_and_check, args=t) for t in targets]
        for t in th: t.start()
        for t in th: t.join()

        ready = [fid for fid, _ in targets if fid in conns]
        # Sound for slot i should land at lead + i*gap. Without compensation every
        # lantern is assumed to have the same latency (advance 0).
        lat = {}
        if comp:
            with state_lock:
                lat = {f: STATE["lanterns"].get(f, {}).get("latency_ms") for f in ready}
            known = [v for v in lat.values() if v is not None]
            fill = float(np.median(known)) if known and np is not None else 0.0
            lat = {f: (v if v is not None else fill) for f, v in lat.items()}
        lead = max(lat.values()) if lat else 0.0
        schedule = sorted(((i * gap_ms + lead - lat.get(fid, lead)) / 1000.0, i, fid) for i, fid in enumerate(ready))
        rec = Recorder() if (measure and sd is not None) else None
        if rec:
            audio_lock.acquire()
            rec.__enter__()
            time.sleep(0.4)
        try:
            t0 = time.perf_counter() + 0.25
            stream_t0 = rec.now() + (t0 - time.perf_counter()) if rec else None
            sent_stream = {}
            for rel, i, fid in schedule:
                due = t0 + rel
                while time.perf_counter() < due:
                    pass
                conns[fid].write(f"K{fid}:{pulses[fid]}\n".encode())
                if rec:
                    sent_stream[fid] = (stream_t0 + (time.perf_counter() - t0)) * 1000
                results[fid].update(slot=i, pulse_ms=pulses[fid], advance_ms=round(lead - lat.get(fid, lead), 1),
                                    sent_offset_ms=round((time.perf_counter() - t0) * 1000, 1), utc=utc())
            if rec:
                time.sleep(0.8)
        finally:
            if rec:
                rec.__exit__()
                audio_lock.release()

        timing = None
        if measure and sd is None:
            timing = {"error": "pip install numpy sounddevice to measure"}
        elif rec:
            with state_lock:
                cal = {f: STATE["lanterns"].get(f, {}).get("latency_ms") for f in ready}
            known = [v for v in cal.values() if v is not None]
            fallback = float(np.median(known)) if known else DEFAULT_LATENCY_MS
            by_slot = {i: fid for _, i, fid in schedule}
            slots = sorted(by_slot)
            expected = [sent_stream[by_slot[i]] + (cal[by_slot[i]] if cal[by_slot[i]] is not None else fallback) for i in slots]
            hits, err = measure_hits(rec, expected, gap_ms)
            found = [(i, h) for i, h in zip(slots, hits or []) if h is not None]
            if err:
                timing = {"error": err}
            elif len(found) < 2:
                timing = {"error": f"heard {len(found)} of {len(slots)} strikes -- quieter room, mic closer, or let chimes settle between runs"}
            else:
                # Compare each heard hit with an ideal grid at the requested gap, best-fit offset removed.
                off = float(np.mean([h - i * gap_ms for i, h in found]))
                errs = {i: round(h - i * gap_ms - off, 1) for i, h in found}
                vals = list(errs.values())
                timing = {"errors_ms": [{"fixture_id": by_slot[i], "error_ms": errs.get(i)} for i in slots],
                          "heard": len(found), "of": len(slots),
                          "max_abs_ms": float(max(abs(x) for x in vals)),
                          "rms_ms": round(float(np.sqrt(np.mean(np.square(vals)))), 1)}

        time.sleep(0.4)
        def confirm(fid):
            d = telemetry(conns[fid], wait=1.5)
            if d:
                results[fid].update(strikes_after=d.get("solenoid_strikes"), blocked=d.get("solenoid_blocked"),
                                    failsafes=d.get("solenoid_failsafes"), battery_v=d.get("battery_v"))
                with state_lock:
                    lanterns[fid] = summarize(conns[fid].port, d)
            b, a = results[fid].get("strikes_before"), results[fid].get("strikes_after")
            results[fid]["ok"] = a is not None and b is not None and a == b + 1
        th = [threading.Thread(target=confirm, args=(f,)) for f in ready]
        for t in th: t.start()
        for t in th: t.join()
    finally:
        for s in conns.values():
            try: s.close()
            except Exception: pass
        for lk in locks: lk.release()
    out = [results[fid] for fid, _ in targets]
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(STRIKE_LOG, "a") as f:
        f.write(json.dumps({"kind": "play", "utc": utc(), "gap_ms": gap_ms, "compensated": bool(comp),
                            "timing": timing, "results": out}) + "\n")
    return {"ok": all(r.get("ok") for r in out), "gap_ms": gap_ms, "compensated": bool(comp),
            "timing": timing, "results": out}


def snapshot():
    with state_lock:
        return {"lanterns": sorted(lanterns.values(), key=lambda l: l["hub"]), "state": STATE,
                "state_file": str(STATE_FILE.relative_to(REPO)), "pulse_min": PULSE_MIN_MS, "pulse_max": PULSE_MAX_MS, "audio": sd is not None}


class H(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def reply(self, obj, code=200, ctype="application/json"):
        body = obj if isinstance(obj, bytes) else json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def body(self):
        n = int(self.headers.get("Content-Length") or 0)
        return json.loads(self.rfile.read(n) or b"{}")

    def do_GET(self):
        if self.path == "/":
            return self.reply(PAGE.encode(), ctype="text/html; charset=utf-8")
        if self.path == "/api/snapshot":
            return self.reply(snapshot())
        self.reply({"error": "not found"}, 404)

    def do_POST(self):
        b = self.body()
        if self.path == "/api/scan":
            return self.reply(scan())
        if self.path == "/api/strike":
            fid, ms = b["id"].upper(), b.get("ms", STATE["pulse_ms"])
            if not pulse_ok(ms):
                return self.reply({"ok": False, "error": f"pulse must be {PULSE_MIN_MS}..{PULSE_MAX_MS} ms"}, 400)
            return self.reply(send_exact(fid, f"K{fid}:{ms}", verify_strike=True, pulse_ms=ms))
        if self.path == "/api/play":
            gap = int(b.get("gap_ms", 400))
            if not 100 <= gap <= 5000:
                return self.reply({"ok": False, "error": "gap must be 100..5000 ms"}, 400)
            order = [i.upper() for i in b.get("order", [])]
            given = {k.upper(): v for k, v in (b.get("pulses") or {}).items()}
            pulses = {fid: given.get(fid, STATE["pulse_ms"]) for fid in order}
            if not all(pulse_ok(v) for v in pulses.values()):
                return self.reply({"ok": False, "error": f"pulse must be {PULSE_MIN_MS}..{PULSE_MAX_MS} ms"}, 400)
            return self.reply(play(order, gap, pulses, comp=bool(b.get("comp")), measure=bool(b.get("measure"))))
        if self.path == "/api/calibrate":
            fid, reps = b["id"].upper(), int(b.get("reps", 3))
            ms = b.get("ms", STATE["pulse_ms"])
            if not (2 <= reps <= 8 and pulse_ok(ms)):
                return self.reply({"ok": False, "error": "reps 2..8 and a valid pulse required"}, 400)
            return self.reply(calibrate_one(fid, reps, ms))
        if self.path == "/api/session":
            fid, minutes = b["id"].upper(), int(b.get("minutes", 60))
            if not 0 <= minutes <= 120:
                return self.reply({"ok": False, "error": "minutes must be 0..120"}, 400)
            return self.reply(send_exact(fid, f"!D{fid}:{minutes}"))
        if self.path == "/api/state":
            with state_lock:
                STATE["lanterns"] = b.get("lanterns", STATE["lanterns"])
                STATE["order"] = b.get("order", STATE["order"])
                STATE["presets"] = b.get("presets", STATE["presets"])
                if pulse_ok(b.get("pulse_ms")):
                    STATE["pulse_ms"] = b["pulse_ms"]
                save_state(STATE)
            return self.reply({"ok": True, "updated_utc": STATE["updated_utc"]})
        self.reply({"error": "not found"}, 404)


PAGE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tone lab</title>
<style>
:root{--bg:#faf9f6;--card:#fff;--text:#1f1e1c;--muted:#6b6a65;--line:#e4e2dc;--accent:#185fa5;--accent-bg:#e6f1fb;--warn:#854f0b;--warn-bg:#faeeda;--ok:#3b6d11;--bad:#a32d2d}
@media (prefers-color-scheme:dark){:root{--bg:#1c1b19;--card:#262522;--text:#ecebe6;--muted:#a3a29c;--line:#3a3935;--accent:#85b7eb;--accent-bg:#0c447c;--warn:#fac775;--warn-bg:#633806;--ok:#97c459;--bad:#f09595}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px/1.5 system-ui,-apple-system,sans-serif}
header{position:sticky;top:0;background:var(--bg);border-bottom:1px solid var(--line);padding:10px 16px;display:flex;gap:10px;align-items:center;flex-wrap:wrap;z-index:2}
h1{font-size:17px;font-weight:600;margin:0 12px 0 0}
.bar{flex-basis:100%;display:flex;gap:8px;align-items:center;flex-wrap:wrap}.lbl{font-size:12px;color:var(--muted);width:40px}
.info{position:relative;display:inline-flex;align-items:center;justify-content:center;width:16px;height:16px;border-radius:50%;border:1px solid var(--muted);color:var(--muted);font-size:11px;font-weight:600;cursor:help;margin-left:4px;vertical-align:middle;font-style:normal}
.info:hover,.info:focus{color:var(--text);border-color:var(--text);outline:none}
.info .tip{display:none;position:absolute;top:22px;left:-8px;width:300px;padding:10px 12px;background:var(--card);color:var(--text);border:1px solid var(--line);border-radius:8px;font-size:12px;font-weight:400;line-height:1.5;z-index:10;box-shadow:0 4px 14px rgba(0,0,0,.18);cursor:default}
.info .tip.r{left:auto;right:-8px}
.info:hover .tip,.info:focus .tip,.info:focus-within .tip{display:block}
.seg{display:inline-flex}.seg button{border-radius:0;margin-left:-1px}.seg button:first-child{border-radius:8px 0 0 8px}.seg button:last-child{border-radius:0 8px 8px 0}
input[type=number]{font:inherit;font-size:13px;padding:3px 6px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--text)}
select{font:inherit;font-size:13px;padding:4px 6px;border:1px solid var(--line);border-radius:8px;background:var(--card);color:var(--text)}
button{font:inherit;font-size:13px;padding:5px 10px;border:1px solid var(--line);background:var(--card);color:var(--text);border-radius:8px;cursor:pointer}
button:hover{border-color:var(--muted)}button:disabled{opacity:.5;cursor:default}
.on-l{background:var(--accent-bg);color:var(--accent);border-color:var(--accent)}.on-s{background:var(--warn-bg);color:var(--warn);border-color:var(--warn)}
main{display:grid;grid-template-columns:minmax(0,1fr) 340px;gap:16px;padding:16px;max-width:1200px;margin:0 auto}
@media (max-width:820px){main{grid-template-columns:minmax(0,1fr)}}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px}
.card.offline{opacity:.55}.card.flash{outline:2px solid var(--accent)}
.name{font-weight:600;font-size:16px}.meta{font-size:12px;color:var(--muted)}
.row{display:flex;gap:6px;flex-wrap:wrap;margin-top:8px}
input[type=text]{width:100%;font:inherit;font-size:13px;padding:5px 8px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--text);margin-top:8px}
.panel{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px;align-self:start;position:sticky;top:180px}
.panel h2{font-size:14px;margin:0 0 8px}.ol{list-style:none;margin:0;padding:0}
.ol li{display:flex;align-items:center;gap:6px;padding:6px 4px;border-bottom:1px solid var(--line);font-size:14px;cursor:grab}
.ol li.drag{opacity:.4}.ol .k{width:20px;color:var(--muted);font-size:12px}.ol .t{flex:1}
.tag{font-size:11px;padding:1px 6px;border-radius:6px}
#log{font:12px ui-monospace,monospace;color:var(--muted);max-height:180px;overflow:auto;margin-top:10px;white-space:pre-wrap}
.status{font-size:12px;color:var(--muted)}.ok{color:var(--ok)}.bad{color:var(--bad)}
</style></head><body>
<header><h1>Tone lab</h1>
<button id="scan">Rescan lanterns</button>
<button id="sess">Extend all sessions 60 min</button>
<span class="status" id="saved"></span>
<div class="bar"><span class="lbl">Play</span>
<span class="seg" id="set"><button data-set="all" class="on-l">All</button><button data-set="large">Large</button><button data-set="small">Small</button></span>
<button data-dir="down" class="on-l">▶ High → low</button><button data-dir="up" class="on-l">▶ Low → high</button>
<label class="status">gap <input id="gapms" type="range" min="100" max="1500" step="50" value="400" style="width:110px;vertical-align:middle"> <span id="gapv">400</span> ms</label>
<button id="seq">Slow check</button></div>
<div class="bar"><span class="lbl">Timing</span>
<button id="cal">Sync latency (mic)</button><i class="info" tabindex="0" aria-label="About sync latency">i<span class="tip" role="tooltip"><b>Sync latency</b> strikes each lantern in the current set a few times and listens with the laptop microphone. It measures how long each one takes from command to sound and saves that on its card (median, plus the spread between strikes). Keep the room quiet; about 2 seconds per strike. Re-sync a lantern after changing its pulse, since pulse width changes how fast the mallet arrives.</span></i>
<label class="status">strikes each <select id="reps"><option>3</option><option selected>4</option><option>5</option></select></label>
<label class="status"><input type="checkbox" id="comp"> compensate on play</label><i class="info" tabindex="0" aria-label="About compensate on play">i<span class="tip" role="tooltip"><b>Compensate on play</b> changes when commands are sent. Each lantern has its own delay from command to chime. With this on, faster lanterns get their command sent early by however much quicker they are than the slowest one, so every chime lands exactly on the beat. Example: latencies of 30, 55 and 40 ms are sent 25, 0 and 15 ms early. Uses the latencies from Sync latency; unsynced lanterns are assumed to have the group median. Turns on automatically after a sync.</span></i>
<label class="status"><input type="checkbox" id="meas"> measure timing on play</label><i class="info" tabindex="0" aria-label="About measure timing on play">i<span class="tip r" role="tooltip"><b>Measure timing on play</b> only listens; it never changes what is sent. The microphone records the run, finds where each chime starts, and compares the hits with a perfect grid at your gap. The log shows each lantern's error (+ late, - early), the worst hit (max) and the typical error (rms). Compare a run with compensation off and on to see whether it helped. Needs a quiet room and a gap of at least 100 ms.</span></i></div>
<div class="bar"><span class="lbl"></span><span class="status">Pulse is set per lantern on each card and saves as you type (blank = firmware default 40 ms)</span></div></header>
<main><section><div class="grid" id="grid"></div></section>
<aside class="panel"><h2>Tone order (1 = highest) — drag to reorder</h2>
<div class="row" style="margin:0 0 8px"><button id="rev">Reverse</button><button id="grp">Large first, small last</button></div>
<div class="row" style="margin:0 0 8px"><select id="psel"><option value="">Saved orders…</option></select><button id="pload">Load</button><button id="psave">Save as…</button><button id="pdel">Delete</button></div>
<ul class="ol" id="order"></ul><div id="log"></div></aside></main>
<script>
let L=[],S={lanterns:{},order:[]},busy=false;
const $=id=>document.getElementById(id);
const api=(p,b)=>fetch(p,{method:b?"POST":"GET",headers:{"Content-Type":"application/json"},body:b?JSON.stringify(b):undefined}).then(r=>r.json());
function log(m,cls){const d=document.createElement("div");d.className=cls||"";d.textContent=new Date().toLocaleTimeString()+"  "+m;$("log").prepend(d)}
function meta(id){return S.lanterns[id]||(S.lanterns[id]={type:null,note:""})}
let PMIN=5,PMAX=100,curSet="all",AUDIO=false;
function save(){api("/api/state",{lanterns:S.lanterns,order:S.order,presets:S.presets,pulse_ms:S.pulse_ms}).then(r=>{$("saved").textContent=r.ok?"saved "+new Date().toLocaleTimeString():"save failed"})}
function pulseFor(id){return meta(id).pulse_ms||S.pulse_ms}
function latText(id){const m=meta(id);if(m.latency_ms==null)return "";const stale=m.latency_pulse_ms!==pulseFor(id);return ` · <span style="color:${stale?"var(--warn)":"inherit"}" title="${stale?"pulse changed since sync -- re-sync":"measured at "+m.latency_pulse_ms+" ms pulse"}">latency ${Math.round(m.latency_ms)} ms ±${Math.round(m.latency_jitter_ms/2)}${stale?" (re-sync)":""}</span>`}
function validP(v){return Number.isInteger(v)&&v>=PMIN&&v<=PMAX}
function playList(dir){let ids=S.order.filter(id=>curSet==="all"||meta(id).type===curSet);return dir==="up"?ids.slice().reverse():ids}
function renderPresets(){const v=$("psel").value;$("psel").innerHTML='<option value="">Saved orders…</option>'+Object.keys(S.presets).map(k=>`<option ${k===v?"selected":""}>${k}</option>`).join("")}
function renderHead(){document.querySelectorAll("#set button").forEach(b=>b.classList.toggle("on-l",b.dataset.set===curSet))}
function syncOrder(){const ids=L.map(l=>l.fixture_id);S.order=S.order.filter(i=>ids.includes(i));ids.forEach(i=>{if(!S.order.includes(i))S.order.push(i)})}
function name(id){const l=L.find(x=>x.fixture_id===id);return l?(l.callsign||id):id}
function tag(t){return t==="large"?'<span class="tag on-l">large</span>':t==="small"?'<span class="tag on-s">small</span>':'<span class="tag" style="color:var(--muted)">?</span>'}
function render(){
 $("grid").innerHTML=L.map(l=>{const m=meta(l.fixture_id),mins=l.session_left_s!=null?Math.round(l.session_left_s/60):"–";
 return `<div class="card ${l.offline?"offline":""}" id="c-${l.fixture_id}"><div class="name">${l.callsign||l.fixture_id}</div>
 <div class="meta">${l.fixture_id} · hub ${l.hub} · ${l.battery_v!=null?l.battery_v.toFixed(2)+" V":"?"} · session ${mins} min · strikes ${l.strikes??"?"}${latText(l.fixture_id)}</div>
 <div class="row"><button data-strike="${l.fixture_id}">Strike</button>
 <button class="${m.type==="large"?"on-l":""}" data-type="${l.fixture_id}:large">Large</button>
 <button class="${m.type==="small"?"on-s":""}" data-type="${l.fixture_id}:small">Small</button>
 <label class="status">pulse <input type="number" min="5" max="100" step="1" style="width:56px" placeholder="${S.pulse_ms}" data-pulse="${l.fixture_id}" value="${m.pulse_ms??""}"> ms</label></div>
 <input type="text" placeholder="Tone notes, e.g. bright, slight beat" data-note="${l.fixture_id}" value="${(m.note||"").replace(/"/g,"&quot;")}"></div>`}).join("");
 $("order").innerHTML=S.order.map((id,i)=>`<li draggable="true" data-i="${i}"><span class="k">${i+1}</span><span class="t">${name(id)}</span>${tag(meta(id).type)}<button data-strike="${id}" title="Strike">▶</button></li>`).join("");
}
async function strike(id){const c=$("c-"+id);c&&c.classList.add("flash");const ms=pulseFor(id);const r=await api("/api/strike",{id,ms});c&&c.classList.remove("flash");
 if(r.ok)log(`${name(id)} struck at ${ms} ms (${r.strikes_before}→${r.strikes_after}, ${r.battery_v} V)`,"ok");else log(`${name(id)}: ${r.error||"strike not confirmed"}`,"bad");
 const l=L.find(x=>x.fixture_id===id);if(l&&r.strikes_after!=null){l.strikes=r.strikes_after;l.battery_v=r.battery_v;l.session_left_s=r.session_left_s}render();return r}
document.addEventListener("click",async e=>{const b=e.target.closest("button");if(!b||busy)return;
 if(b.dataset.strike){busy=true;await strike(b.dataset.strike);busy=false}
 else if(b.dataset.type){const[id,t]=b.dataset.type.split(":");const m=meta(id);m.type=m.type===t?null:t;render();save()}});
document.addEventListener("change",e=>{const t=e.target;
 if(t.dataset.note){meta(t.dataset.note).note=t.value;save()}
 else if(t.dataset.pulse){const id=t.dataset.pulse,v=t.value===""?null:+t.value;if(v!==null&&!validP(v)){log(`${name(id)}: pulse must be ${PMIN}..${PMAX} ms`,"bad");t.value=meta(id).pulse_ms??"";return}}});
let pulseTimer=null;
document.addEventListener("input",e=>{const t=e.target;if(!t.dataset.pulse)return;const id=t.dataset.pulse,v=t.value===""?null:+t.value;
 if(v!==null&&!validP(v))return;meta(id).pulse_ms=v;clearTimeout(pulseTimer);pulseTimer=setTimeout(()=>{save();log(`${name(id)} pulse ${v===null?"default":v+" ms"} saved`)},400)});
$("set").onclick=e=>{const b=e.target.closest("button");if(!b)return;curSet=b.dataset.set;renderHead()};
$("rev").onclick=()=>{S.order.reverse();render();save()};
$("grp").onclick=()=>{const r=t=>t==="large"?0:t==="small"?2:1;S.order=S.order.map((id,i)=>[id,i]).sort((a,b)=>r(meta(a[0]).type)-r(meta(b[0]).type)||a[1]-b[1]).map(x=>x[0]);render();save()};
$("psave").onclick=()=>{const n=(prompt("Name for this order:")||"").trim();if(!n)return;S.presets[n]=S.order.slice();renderPresets();$("psel").value=n;save();log(`saved order "${n}"`)};
$("pload").onclick=()=>{const n=$("psel").value;if(!n||!S.presets[n])return;const ids=L.map(l=>l.fixture_id);S.order=S.presets[n].filter(i=>ids.includes(i));syncOrder();render();save();log(`loaded order "${n}"`)};
$("pdel").onclick=()=>{const n=$("psel").value;if(!n||!confirm(`Delete saved order "${n}"?`))return;delete S.presets[n];renderPresets();save()};
let dragI=null;
$("order").addEventListener("dragstart",e=>{const li=e.target.closest("li");dragI=+li.dataset.i;li.classList.add("drag")});
$("order").addEventListener("dragover",e=>e.preventDefault());
$("order").addEventListener("drop",e=>{e.preventDefault();const li=e.target.closest("li");if(!li||dragI===null)return;const j=+li.dataset.i;const[x]=S.order.splice(dragI,1);S.order.splice(j,0,x);dragI=null;render();save()});
$("order").addEventListener("dragend",()=>{dragI=null;render()});
$("scan").onclick=async()=>{busy=true;$("scan").disabled=true;log("scanning…");const r=await api("/api/scan",{});L=r.lanterns;syncOrder();render();log(`found ${L.filter(l=>!l.offline).length} lanterns`);$("scan").disabled=false;busy=false};
$("seq").onclick=async()=>{if(busy)return;busy=true;$("seq").disabled=true;const g=Math.max(1,+$("gap").value||3)*1000;
 for(const id of S.order){await strike(id);await new Promise(r=>setTimeout(r,g))}$("seq").disabled=false;busy=false};
$("gapms").oninput=e=>$("gapv").textContent=e.target.value;
document.querySelectorAll("[data-dir]").forEach(btn=>btn.onclick=async()=>{if(busy)return;const ids=playList(btn.dataset.dir);if(!ids.length){log(`no ${curSet} lanterns classified yet`,"bad");return}
 busy=true;btn.disabled=true;const g=+$("gapms").value;const pulses=Object.fromEntries(ids.map(id=>[id,pulseFor(id)]));
 const r=await api("/api/play",{order:ids,gap_ms:g,pulses,comp:$("comp").checked,measure:$("meas").checked});
 if(r.results){const bad=r.results.filter(x=>!x.ok);log(`played ${curSet} ${btn.dataset.dir==="up"?"low→high":"high→low"} (${r.results.length}) at ${g} ms gap`+(bad.length?` — not confirmed: ${bad.map(x=>name(x.fixture_id)+(x.error?" ("+x.error+")":"")).join(", ")}`:" — all confirmed"),bad.length?"bad":"ok");
  r.results.forEach(x=>{const l=L.find(y=>y.fixture_id===x.fixture_id);if(l&&x.strikes_after!=null){l.strikes=x.strikes_after;l.battery_v=x.battery_v}});
  if(r.timing){if(r.timing.error)log("timing: "+r.timing.error,"bad");else log(`timing ${r.compensated?"(compensated)":"(uncompensated)"}: max ${Math.round(r.timing.max_abs_ms)} ms, rms ${r.timing.rms_ms} ms — `+r.timing.errors_ms.map(x=>x.error_ms==null?`${name(x.fixture_id)} not heard`:`${name(x.fixture_id)} ${x.error_ms>0?"+":""}${Math.round(x.error_ms)}`).join(", ")+(r.timing.heard<r.timing.of?` (heard ${r.timing.heard} of ${r.timing.of})`:""),r.timing.max_abs_ms<=10?"ok":"")}
  render()}
 else log(r.error||"play failed","bad");btn.disabled=false;busy=false});
$("cal").onclick=async()=>{if(busy)return;const ids=playList("down");if(!ids.length)return;
 if(!AUDIO){log("microphone support missing: pip install numpy sounddevice","bad");return}
 busy=true;$("cal").disabled=true;log(`syncing latency for ${ids.length} lanterns — keep the room quiet`);
 for(const id of ids){const c=$("c-"+id);c&&c.classList.add("flash");const r=await api("/api/calibrate",{id,reps:+$("reps").value,ms:pulseFor(id)});c&&c.classList.remove("flash");
  if(r.ok){Object.assign(meta(id),{latency_ms:r.latency_ms,latency_jitter_ms:r.jitter_ms,latency_pulse_ms:r.pulse_ms,latency_utc:r.utc});log(`${name(id)} latency ${Math.round(r.latency_ms)} ms (spread ${Math.round(r.jitter_ms)} ms; ${r.samples_ms.map(v=>v==null?"–":v).join(", ")})`,"ok")}
  else{log(`${name(id)}: ${r.error}`,"bad");if(/silent/.test(r.error||""))break}
  render()}
 const got=ids.map(id=>meta(id).latency_ms).filter(v=>v!=null);if(got.length>1){log(`latency range ${Math.round(Math.min(...got))}–${Math.round(Math.max(...got))} ms; compensation on`);$("comp").checked=true}
 $("cal").disabled=false;busy=false};
$("sess").onclick=async()=>{if(busy)return;busy=true;for(const l of L.filter(x=>!x.offline)){const r=await api("/api/session",{id:l.fixture_id,minutes:60});log(`${name(l.fixture_id)} session: ${r.ok&&/accepted/.test(r.reply)?"60 min":(r.error||r.reply||"?")}`,r.ok?"":"bad");if(r.session_left_s!=null)l.session_left_s=r.session_left_s}render();busy=false};
api("/api/snapshot").then(r=>{S=r.state;S.lanterns=S.lanterns||{};S.order=S.order||[];S.presets=S.presets||{};PMIN=r.pulse_min;PMAX=r.pulse_max;AUDIO=r.audio;renderHead();renderPresets();$("saved").textContent="saving to "+r.state_file;$("scan").click()});
</script></body></html>"""

if __name__ == "__main__":
    print(f"tone lab on http://localhost:{ARGS.port}  state -> {STATE_FILE}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", ARGS.port), H).serve_forever()
