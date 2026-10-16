#!/usr/bin/env python3
"""Decompression tone lab: classify, tune, order and sequence lanterns over USB.

Local web app for USB-connected decompression-canary lanterns. Tabs: Lanterns
(chime type, per-lantern solenoid pulse, notes), Tone order (ranking, play
runs, microphone latency sync) and Patterns (step sequencer). Strikes are
exact-target `K<id>:<ms>` commands sent only after a fresh telemetry identity
check on that port. State is saved on every change to
ops/bench/data/<site>/decompression-tone-map.json. The browser UI lives in
ops/bench/tone_lab_ui/.

    python ops/bench/decompression_tone_lab.py --site "Brian bench" --port 8765

Requires pyserial; numpy and sounddevice enable the microphone features.
"""
import argparse
import csv
import glob
import json
import math
import struct
import zlib
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
UI_DIR = Path(__file__).resolve().parent / "tone_lab_ui"
UI_TYPES = {".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8",
            ".css": "text/css; charset=utf-8"}
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
STATE.setdefault("patterns", {})
STATE.setdefault("tone_links", [])  # lanterns marked "same tone as the one above" in the order
STATE.setdefault("interactions", [])  # sensor -> pattern-variable rules and Atom button rules
STATE.setdefault("atoms", {})         # Atom radio id -> friendly name
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
        "decompression": bool(d.get("decompression")), "tmf": bool(d.get("tmf8820_present")),
        "vl": bool(d.get("vl53l5cx_present")), "seen_utc": utc(),
        # ADR 0084 firmware pattern mode (absent on older images)
        "fw_pattern": "pattern_id" in d, "pattern_id": d.get("pattern_id"), "pattern_crc": d.get("pattern_crc"),
        "pattern_playing": d.get("pattern_playing"), "pattern_role": d.get("pattern_role"),
        "pattern_locked": d.get("pattern_locked"), "pattern_conductor": d.get("pattern_conductor"),
        "pattern_left_s": d.get("pattern_left_s"), "pattern_played": d.get("pattern_played"),
        # Atom presses heard over radio (fx-261016+): count, last source, relay mode
        "radio_plays": d.get("radio_plays"), "radio_last_src": d.get("radio_last_src"),
        "radio_last_boot": d.get("radio_last_boot"), "radio_last_seq": d.get("radio_last_seq"),
        "radio_relay": d.get("radio_relay"),
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


def play(order, gap_ms, pulses, comp=False, measure=False, slots=None):
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
        # slots: beat index per lantern (same-tone groups share a slot and strike together)
        slot_of = {fid: (slots or {}).get(fid, i) for i, fid in enumerate(ready)}
        if slots:  # renumber so the shared slots stay consecutive after offline lanterns drop out
            order_slots = sorted(set(slot_of.values()))
            slot_of = {f: order_slots.index(v) for f, v in slot_of.items()}
        schedule = sorted(((slot_of[fid] * gap_ms + lead - lat.get(fid, lead)) / 1000.0, slot_of[fid], fid)
                          for fid in ready)
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
            rows = sorted((slot, fid) for _, slot, fid in schedule)
            expected = [sent_stream[f] + (cal[f] if cal[f] is not None else fallback) for _, f in rows]
            hits, err = measure_hits(rec, expected, gap_ms)
            found = [(slot, f, h) for (slot, f), h in zip(rows, hits or []) if h is not None]
            if err:
                timing = {"error": err}
            elif len(found) < 2:
                timing = {"error": f"heard {len(found)} of {len(rows)} strikes -- quieter room, mic closer, or let chimes settle between runs"}
            else:
                # Compare each heard hit with an ideal grid at the requested gap, best-fit offset removed.
                # Lanterns sharing a slot strike together, so they are measured against the same beat.
                off = float(np.mean([h - slot * gap_ms for slot, _, h in found]))
                errs = {f: round(h - slot * gap_ms - off, 1) for slot, f, h in found}
                vals = list(errs.values())
                timing = {"errors_ms": [{"fixture_id": f, "error_ms": errs.get(f)} for _, f in rows],
                          "heard": len(found), "of": len(rows),
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


# ---------------------------------------------------------------------------
# Proximity ding (host-side, USB only). The decompression firmware does not yet
# chime on presence by itself, so while this mode is on the app polls each
# lantern's TMF8820 depth and sends one exact-target strike when a hand comes
# within range. Detection mirrors the firmware gate's confidence floor
# (PRESENCE_MIN_CONFIDENCE 20 in firmware/fixture/src/core/presence_wave.h).
# ---------------------------------------------------------------------------
PROX_MIN_CONFIDENCE = 40   # a hand must be a confident return
PROX_LEARN_CONFIDENCE = 20 # but anything at the firmware floor counts as background (fixtures, walls)
PROX_WEAK_CONFIDENCE = 22  # a weak return counts as a hand only when seen twice in a row (far hands, sun)
PROX_DELTA_MM = 300        # a hand must be this much nearer than the learned background (firmware PRESENCE_DELTA_MM)
PROX = {"on": False, "range_mm": 600, "cooldown_s": 1.5, "silent": True, "active": set()}  # watch-only until ding is chosen
VL_CONF = 100  # VL53L5CX zones arrive pre-filtered (status 5/9 only), so a nonzero zone is a confident return


def has_sensor(l):
    """A distance sensor the proximity view can read: downlight TMF8820 or perimeter VL53L5CX."""
    return bool(l.get("tmf") or l.get("vl"))


def prox_range_for(fid):
    with state_lock:
        r = STATE.get("lanterns", {}).get(fid, {}).get("prox_range_mm")
    return max(100, min(4000, int(r))) if isinstance(r, (int, float)) and r else PROX["range_mm"]
prox_relearn = set()    # fixture ids whose detector should relearn its background


class ProxDetector:
    """Background-relative hand detector for one lantern's single-depth readings.

    Learn: the first ~1.5 s of readings set the background from every return
    at the firmware floor (confidence >= 20), so a fixture or wall whose
    confidence wobbles around the hand threshold is background, not a hand.
    Trigger: one confident reading within range and PROX_DELTA_MM nearer than
    the background (a passing hand is often seen for a single report). Re-arm:
    5 consecutive non-hand readings (~0.6 s), so one dropout cannot re-trigger.
    The background resets after 3 s of consistently farther readings.
    A background nearer than the range itself (e.g. something 18 cm away)
    leaves only the space in front of it."""

    LEARN_S, HIT_N, CLEAR_N = 1.5, 1, 5
    FORGET_S = 12.0  # a learned background is only forgotten after this long unseen
    STILL_S = 3.0                   # continuously present at one distance this long = fixture
    DUTY_S, DUTY_MIN = 20.0, 0.40   # or present >40% of 20 s at one distance (flickering fixture)

    def __init__(self, now):
        self.t0, self.learn, self.background = now, [], None
        self.learnTotal = 0
        self.hits = self.clears = self.farther = self.weak = 0
        self.bgSeenAt = now
        self.window = []  # (time, depth or 0) over the last DUTY_S, to spot a static intruder
        self.still_mm, self.still_since = 0, now
        self.armed = True
        self.last_ding = 0.0

    def hand(self, depth, conf, range_mm, min_conf=PROX_MIN_CONFIDENCE):
        if conf < min_conf or not (20 <= depth <= range_mm):
            return False
        return self.background is None or depth <= self.background - PROX_DELTA_MM

    def step(self, now, depth, conf, range_mm, cooldown_s):
        """Returns (state, ding) where state is learning/blocked/ready/hand."""
        if now - self.t0 < self.LEARN_S:
            self.learnTotal += 1
            if conf >= PROX_LEARN_CONFIDENCE and depth > 0:
                self.learn.append(depth)
            return "learning", False
        if self.learn is not None:
            # Background = the near quartile of returns seen in at least a fifth of
            # learning samples (something that flickers in and out still counts).
            enough = len(self.learn) >= max(2, self.learnTotal // 5)
            self.background = sorted(self.learn)[len(self.learn) // 4] if enough else None
            self.learn = None
        # Background recovery. Something that flickers in and out (a cable, a
        # neighbouring lantern at the edge of the cone) must stay background, so a
        # learned background is only forgotten after FORGET_S without any return
        # near it -- long enough to rule out flicker, short enough to recover
        # from a hand that was present during learning.
        if self.background is not None:
            if conf >= PROX_LEARN_CONFIDENCE and depth > 0 and depth < self.background + PROX_DELTA_MM:
                self.bgSeenAt = now
            elif now - self.bgSeenAt >= self.FORGET_S:
                self.background = depth if (conf >= PROX_LEARN_CONFIDENCE and depth > 0) else None
                self.bgSeenAt = now
        # A fixture that was absent during learning but now sits in the cone a
        # large share of the time (flickering cable, neighbouring lantern) is
        # absorbed into the background; a waving hand is present only briefly.
        seen_now = depth if conf >= PROX_LEARN_CONFIDENCE and depth > 0 else 0
        # Continuous stillness: the same distance (within 15 mm) on every report.
        if seen_now and self.still_mm and abs(seen_now - self.still_mm) <= 15:
            if now - self.still_since >= self.STILL_S and (self.background is None or seen_now < self.background):
                self.background, self.bgSeenAt, self.window = seen_now, now, []
                self.still_mm = 0
        else:
            self.still_mm, self.still_since = seen_now, now
        self.window.append((now, seen_now))
        self.window = [(t, d) for t, d in self.window if now - t <= self.DUTY_S]
        if now - self.window[0][0] >= self.DUTY_S * 0.8:
            seen = sorted(d for _, d in self.window if d)
            if seen:
                cand = seen[len(seen) // 2]
                near = sum(1 for _, d in self.window if d and abs(d - cand) <= 15)  # fixtures read within ~10 mm
                if near / len(self.window) >= self.DUTY_MIN and (self.background is None or cand < self.background):
                    self.background, self.bgSeenAt, self.window = cand, now, []
        # A confident return counts at once; a weak one (a far hand, or sun
        # lowering confidence) only on the second consecutive report.
        strong = self.hand(depth, conf, range_mm)
        weak = not strong and self.hand(depth, conf, range_mm, PROX_WEAK_CONFIDENCE)
        self.weak = self.weak + 1 if weak else 0
        if strong or self.weak >= 2:
            self.hits, self.clears = self.hits + 1, 0
        elif weak:
            self.clears = 0  # first weak sighting: wait for confirmation, do not re-arm
        else:
            self.clears, self.hits = self.clears + 1, 0
            if self.clears >= self.CLEAR_N:
                self.armed = True
        ding = False
        if self.hits >= self.HIT_N and self.armed and now - self.last_ding >= cooldown_s:
            ding, self.armed, self.last_ding = True, False, now
        # The firmware ignores returns under 80 mm, so a background closer than
        # 80 + 300 mm leaves no room for a hand in front of it.
        if self.background is not None and self.background - PROX_DELTA_MM < 80:
            return "blocked", ding
        return ("hand" if self.hits else "ready"), ding
prox_live = {}          # fid -> {"depth_mm", "confidence", "present", "status"}
prox_trace = {}         # fid -> recent raw readings [(t, depth, conf, state)] for diagnosis
prox_events = []        # [{"id", "utc", "fixture_id", "depth_mm", "pulse_ms"}]
prox_threads = {}
prox_parked = {}
prox_pause_count = 0
prox_cv = threading.Condition()


class ProxPaused:
    """Park the proximity workers (closing their ports) around another serial operation."""

    def __enter__(self):
        global prox_pause_count
        with prox_cv:
            prox_pause_count += 1
            prox_cv.wait_for(lambda: all(prox_parked.get(f, True) for f, t in prox_threads.items() if t.is_alive()),
                             timeout=3.0)

    def __exit__(self, *a):
        global prox_pause_count
        with prox_cv:
            prox_pause_count -= 1
            prox_cv.notify_all()


class ZoneDetector:
    """Per-zone detection for the TMF8820's 3x3 zones (telemetry tof_zone_mm/conf).

    Each zone has its own ProxDetector, so a zone that permanently sees part of
    the lantern (Tangela and Ness: a steady ~20 cm return) is blocked on its own
    while the other zones still see a hand at normal distances. The lantern dings
    when any zone sees a hand, with one lantern-level re-arm and cooldown."""

    def __init__(self, now, n=9):
        self.zones = [ProxDetector(now) for _ in range(n)]
        self.armed, self.clears, self.last_ding = True, 0, 0.0
        self.states = ["learning"] * n

    def step(self, now, mm, conf, range_mm, cooldown_s):
        # Zone-level dings are disabled (infinite cooldown); the lantern decides.
        self.states = [z.step(now, mm[i], conf[i], range_mm, 1e12)[0] for i, z in enumerate(self.zones)]
        hand = "hand" in self.states
        if hand:
            self.clears = 0
        else:
            self.clears += 1
            if self.clears >= ProxDetector.CLEAR_N:
                self.armed = True
        ding = hand and self.armed and now - self.last_ding >= cooldown_s
        if ding:
            self.armed, self.last_ding = False, now
        if "learning" in self.states:
            return "learning", False
        if hand:
            return "hand", ding
        return ("blocked" if all(st == "blocked" for st in self.states) else "ready"), ding

    def open_zones(self):
        return [i for i, st in enumerate(self.states) if st != "blocked"]

    def background(self):
        """Nearest background among usable zones (for the chart), or None."""
        bgs = [self.zones[i].background for i in self.open_zones() if self.zones[i].background]
        return min(bgs) if bgs else None


def prox_worker(fid):
    conn, det = None, None
    while PROX["on"] and fid in PROX["active"]:
        with prox_cv:
            if prox_pause_count:
                if conn:
                    conn.close()
                    conn = None
                prox_parked[fid] = True
                prox_cv.notify_all()
                prox_cv.wait(timeout=0.2)
                continue
            prox_parked[fid] = False
        info = lanterns.get(fid)
        if not info or info.get("offline"):
            time.sleep(1.0)
            continue
        try:
            with lock_for(info["port"]):
                if conn is None:
                    conn = serial.Serial(info["port"], 115200, timeout=0.03)
                    time.sleep(0.1)
                d = telemetry(conn, wait=0.5)
                if not d or d.get("fixture_id") != fid:
                    raise RuntimeError(f"identity mismatch: {d and d.get('fixture_id')}")
                now = time.time()
                if fid in prox_relearn:
                    det = None
                    prox_relearn.discard(fid)
                rng = prox_range_for(fid)
                ok = bool(d.get("tmf_read_ok"))
                zmm, zconf = d.get("tof_zone_mm"), d.get("tof_zone_conf")
                vmm = d.get("vl_zone_nearest_mm")
                if d.get("vl53l5cx_present") and isinstance(vmm, list) and len(vmm) == 16:
                    # Perimeter VL53L5CX: 4x4 zones of the nearest valid return (0 = none).
                    ok, zmm = bool(d.get("vl_read_ok")), [int(v or 0) for v in vmm]
                    zconf = [VL_CONF if v else 0 for v in zmm]
                if ok and isinstance(zmm, list) and len(zmm) in (9, 16) and isinstance(zconf, list) and len(zconf) == len(zmm):
                    if not isinstance(det, ZoneDetector) or len(det.zones) != len(zmm):
                        det = ZoneDetector(now, len(zmm))
                    state, ding = det.step(now, zmm, zconf, rng, PROX["cooldown_s"])
                    usable = [i for i in det.open_zones() if zmm[i] and zconf[i] >= PROX_LEARN_CONFIDENCE]
                    near = min(usable, key=lambda i: zmm[i]) if usable else None
                    depth, conf = (zmm[near], zconf[near]) if near is not None else (0, 0)
                    background = det.background()
                    zones = [{"mm": zmm[i], "conf": zconf[i], "bg": det.zones[i].background, "state": det.states[i]}
                             for i in range(len(zmm))]
                else:  # firmware without per-zone telemetry: single closest return
                    if not isinstance(det, ProxDetector):
                        det = ProxDetector(now)
                    depth = (d.get("tof_depth_mm") or 0) if ok else 0
                    conf = (d.get("tof_confidence") or 0) if ok else 0
                    state, ding = det.step(now, depth, conf, rng, PROX["cooldown_s"])
                    background, zones = det.background, None
                prox_live[fid] = {"depth_mm": depth, "confidence": conf, "status": state,
                                  "background_mm": background, "zones": zones, "range_mm": rng,
                                  "sensor": "vl53l5cx" if zones and len(zones) == 16 else "tmf8820"}
                tr = prox_trace.setdefault(fid, [])
                tr.append((round(now, 3), depth, conf, state, d.get("tmf_reads")))
                del tr[:-400]
                if ding:
                    with state_lock:
                        ms = STATE["lanterns"].get(fid, {}).get("pulse_ms") or STATE["pulse_ms"]
                        perimeter = STATE["lanterns"].get(fid, {}).get("role") == "perimeter"
                    silent = PROX["silent"] or perimeter  # perimeter lights have no chime
                    if not silent:  # watch-only mode detects and records but never strikes
                        conn.write(f"K{fid}:{ms}\n".encode())
                    ev = {"id": len(prox_events) + 1, "utc": utc(), "t": round(now, 3), "fixture_id": fid,
                          "depth_mm": depth, "confidence": conf, "background_mm": background,
                          "pulse_ms": ms, "silent": silent}
                    prox_events.append(ev)
                    del prox_events[:-200]
                    with open(STRIKE_LOG, "a") as f:
                        f.write(json.dumps({"kind": "proximity", **ev}) + "\n")
        except Exception as ex:
            prox_live[fid] = {"status": f"error: {ex}"}
            if conn:
                try: conn.close()
                except Exception: pass
            conn, det = None, None
            time.sleep(1.0)
            continue
        time.sleep(0.08)
    if conn:
        conn.close()
    with prox_cv:
        prox_parked[fid] = True
        prox_cv.notify_all()


def start_prox_worker(fid):
    t = prox_threads.get(fid)
    if t and t.is_alive():
        return
    t = threading.Thread(target=prox_worker, args=(fid,), daemon=True)
    prox_threads[fid] = t
    t.start()


def set_prox_sensor(fid, active):
    """Turn one sensor's proximity watch on or off without touching the others."""
    fid = str(fid).upper()
    if active:
        if not has_sensor(lanterns.get(fid, {})):
            return {**prox_status(), "error": f"{fid} has no distance sensor reporting"}
        PROX["active"].add(fid)
        PROX["on"] = True
        start_prox_worker(fid)
    else:
        PROX["active"].discard(fid)
        t = prox_threads.pop(fid, None)
        if t:
            t.join(timeout=2.0)
        prox_live.pop(fid, None)
        if not PROX["active"]:
            PROX["on"] = False
    return prox_status()


def set_proximity(on, range_mm=None, silent=None, relearn=None):
    if range_mm is not None:
        PROX["range_mm"] = max(100, min(2000, int(range_mm)))
    if silent is not None:
        PROX["silent"] = bool(silent)
    if relearn:
        prox_relearn.update(list(prox_threads) if relearn == "all" else [str(f).upper() for f in relearn])
    if on and not PROX["on"]:
        PROX["on"] = True
        with state_lock:
            sensed = [f for f, l in lanterns.items() if has_sensor(l) and not l.get("offline")]
        PROX["active"] = set(sensed)
        for fid in sensed:
            start_prox_worker(fid)
    elif not on and PROX["on"]:
        PROX["on"] = False
        PROX["active"] = set()
        for t in list(prox_threads.values()):
            t.join(timeout=2.0)
        prox_threads.clear()
        prox_live.clear()
        prox_relearn.clear()
    return prox_status()


def prox_status(since=0):
    with state_lock:
        no_sensor = [f for f, l in lanterns.items() if not has_sensor(l)]
    return {"on": PROX["on"], "silent": PROX["silent"], "range_mm": PROX["range_mm"], "live": prox_live,
            "active": sorted(PROX["active"]),
            "events": [e for e in prox_events if e["id"] > since], "no_sensor": no_sensor,
            "delta_mm": PROX_DELTA_MM, "min_confidence": PROX_MIN_CONFIDENCE,
            "learn_confidence": PROX_LEARN_CONFIDENCE, "server_t": round(time.time(), 3)}


# ---------------------------------------------------------------------------
# Patterns (step sequencer). A pattern is {bpm, steps, steps_per_beat,
# hits: {fixture_id: [step, ...]}}. Its "lanes" -- loop length plus each
# lantern's hit offsets in ms -- are the same data a future firmware
# pattern mode would store per lantern (see the proposed ADR 0084).
# ---------------------------------------------------------------------------
MIN_HIT_SPACING_MS = 250      # tightest gap the locally timed signature already uses
MAX_PATTERN_PLAY_S = 600      # a USB pattern run stops itself after 10 minutes
PLAYER = {"running": False, "stop": threading.Event(), "status": {}}


def pattern_lanes(p):
    """Validate a pattern; return (loop_ms, step_ms, {fid: [offset_ms]}) or raise ValueError."""
    bpm, steps, spb = float(p.get("bpm", 120)), int(p.get("steps", 16)), int(p.get("steps_per_beat", 4))
    if not (20 <= bpm <= 300 and 1 <= steps <= 256 and 1 <= spb <= 8):
        raise ValueError("bpm 20..300, steps 1..256, steps per beat 1..8")
    step_ms = 60000.0 / (bpm * spb)
    loop_ms = steps * step_ms
    lanes, close = {}, []
    for fid, idx in (p.get("hits") or {}).items():
        idx = sorted({int(i) for i in idx if 0 <= int(i) < steps})
        if not idx:
            continue
        gaps = [(b - a) * step_ms for a, b in zip(idx, idx[1:])] + [(steps - idx[-1] + idx[0]) * step_ms]
        if min(gaps) < MIN_HIT_SPACING_MS:
            close.append(fid)
        lanes[fid.upper()] = [round(i * step_ms, 3) for i in idx]
    if close:
        names = ", ".join(lanterns.get(f, {}).get("callsign") or f for f in close)
        raise ValueError(f"hits closer than {MIN_HIT_SPACING_MS} ms on {names} (counting the loop wrap); "
                         f"slow the tempo or space them out")
    return loop_ms, step_ms, lanes


def pattern_player(lanes, loop_ms, loops, comp, pulses, start_epoch_ms):
    st = PLAYER["status"]
    fids = [f for f in lanes if f in lanterns and not lanterns[f].get("offline")]
    st["skipped"] = [f for f in lanes if f not in fids]
    PLAYER["fids"] = set(fids)
    PLAYER["phase"] = "check"
    with ProxPaused():
        locks = [lock_for(lanterns[f]["port"]) for f in fids]
        for lk in locks: lk.acquire()
        conns, before = {}, {}
        try:
            for f in fids:  # identity check before any strike
                s = serial.Serial(lanterns[f]["port"], 115200, timeout=0.05)
                time.sleep(0.05)
                d = telemetry(s, wait=1.5)
                if not d or d.get("fixture_id") != f:
                    s.close()
                    st.setdefault("errors", []).append(f"{f}: identity mismatch")
                    continue
                conns[f], before[f] = s, d.get("solenoid_strikes")
            lat = {}
            if comp:
                with state_lock:
                    lat = {f: STATE["lanterns"].get(f, {}).get("latency_ms") for f in conns}
                known = [v for v in lat.values() if v is not None]
                fill = sorted(known)[len(known) // 2] if known else 0.0
                lat = {f: (v if v is not None else fill) for f, v in lat.items()}
            lead = max(lat.values()) if lat else 0.0
            st["lead_ms"] = lead
            PLAYER["phase"] = "play"  # write-only from here until the strike-count check
            # Sound for each hit lands at start + loop*loop_ms + offset (+ lead when compensating).
            t0 = time.perf_counter() + (start_epoch_ms / 1000.0 - time.time())
            sent = {f: 0 for f in conns}
            k = 0
            while not PLAYER["stop"].is_set() and (loops == 0 or k < loops):
                if (k + 1) * loop_ms / 1000.0 > MAX_PATTERN_PLAY_S:
                    st["note"] = f"stopped after {MAX_PATTERN_PLAY_S // 60} minutes"
                    break
                events = sorted((k * loop_ms + off + lead - lat.get(f, lead), f) for f in conns for off in lanes[f])
                for rel_ms, f in events:
                    due = t0 + rel_ms / 1000.0
                    while not PLAYER["stop"].is_set():
                        left = due - time.perf_counter()
                        if left <= 0:
                            break
                        time.sleep(left - 0.002 if left > 0.004 else 0)
                    if PLAYER["stop"].is_set():
                        break
                    conns[f].write(f"K{f}:{pulses[f]}\n".encode())
                    sent[f] += 1
                k += 1
                st["loops_done"] = k
            PLAYER["phase"] = "confirm"
            side_end = time.time() + 1.5  # let a listener read that started during play finish
            while PLAYER.get("side_reads") and time.time() < side_end:
                time.sleep(0.02)
            time.sleep(0.4)
            short = []
            for f, s in conns.items():
                d = telemetry(s, wait=1.5)
                got = (d.get("solenoid_strikes") - before[f]) if d and before[f] is not None else None
                if got != sent[f]:
                    short.append(f"{lanterns[f].get('callsign') or f} {got}/{sent[f]}")
            st["confirmed"] = not short
            st["unconfirmed"] = short
            st["sent"] = sum(sent.values())
        except Exception as ex:
            st.setdefault("errors", []).append(str(ex))
        finally:
            for s in conns.values():
                try: s.close()
                except Exception: pass
            for lk in locks: lk.release()
    with open(STRIKE_LOG, "a") as fh:
        fh.write(json.dumps({"kind": "pattern", "utc": utc(), "loop_ms": round(loop_ms, 1), "loops": st.get("loops_done"),
                             "compensated": bool(comp), "status": {k: v for k, v in st.items() if k != "name"},
                             "name": st.get("name")}) + "\n")
    PLAYER["running"] = False
    PLAYER["fids"] = set()
    PLAYER["phase"] = None


def start_pattern(p, loops, comp):
    if PLAYER["running"]:
        return {"ok": False, "error": "a pattern is already playing"}
    try:
        loop_ms, step_ms, lanes = pattern_lanes(p)
    except ValueError as ex:
        return {"ok": False, "error": str(ex)}
    if not lanes:
        return {"ok": False, "error": "pattern has no hits"}
    with state_lock:
        pulses = {f: STATE["lanterns"].get(f, {}).get("pulse_ms") or STATE["pulse_ms"] for f in lanes}
    start_epoch_ms = time.time() * 1000 + 1500  # identity checks happen before this
    PLAYER["stop"].clear()
    PLAYER["running"] = True
    PLAYER["status"] = {"name": p.get("name"), "loops_done": 0, "loop_ms": loop_ms, "step_ms": step_ms,
                        "start_epoch_ms": start_epoch_ms, "loops": loops}
    threading.Thread(target=pattern_player, args=(lanes, loop_ms, loops, comp, pulses, start_epoch_ms),
                     daemon=True).start()
    return {"ok": True, **PLAYER["status"]}


# ---------------------------------------------------------------------------
# Standalone (firmware) patterns, ADR 0084: load each lantern's part over USB,
# verify it by CRC, then start/stop playback that continues without the laptop.
# ---------------------------------------------------------------------------
def part_crc(pid, loop_us, pulse, lead, offsets):
    """Same CRC32 as PatternPart::crc() in firmware/fixture/src/core/pattern_part.cpp."""
    words = [pid, loop_us, pulse, lead, len(offsets), *offsets]
    return zlib.crc32(b"".join(struct.pack("<I", w) for w in words)) & 0xFFFFFFFF


def pattern_parts(p, use_lead):
    """Per-lantern parts for every online lantern (no hits = silent timekeeper)."""
    loop_ms, step_ms, lanes = pattern_lanes(p)
    loop_us = int(round(loop_ms * 1000))
    canon = json.dumps({"loop_us": loop_us, "lanes": {f: [int(round(o * 1000)) for o in v] for f, v in sorted(lanes.items())}},
                       sort_keys=True).encode()
    pid = (zlib.crc32(canon) & 0xFFFFFFFF) or 1
    with state_lock:
        meta = {f: dict(STATE["lanterns"].get(f, {})) for f in lanterns}
        online = [f for f, l in lanterns.items() if not l.get("offline") and l.get("fw_pattern")]
    lat = {f: meta[f].get("latency_ms") for f in online if meta[f].get("latency_ms") is not None}
    base = min(lat.values()) if lat else 0.0
    parts = {}
    for f in online:
        offs = [int(round(o * 1000)) for o in lanes.get(f, [])]
        pulse = meta[f].get("pulse_ms") or STATE["pulse_ms"]
        lead = int(round((lat.get(f, base) - base) * 1000)) if use_lead else 0
        lead = max(0, min(lead, 200000))
        parts[f] = {"pattern_id": pid, "loop_us": loop_us, "pulse_ms": pulse, "lead_us": lead, "offsets_us": offs,
                    "crc": part_crc(pid, loop_us, pulse, lead, offs)}
    return pid, parts


def pattern_command(fid, line, wait_s=4.0):
    """Identity-checked USB send of one '!P' line; returns the firmware's 'pattern:' reply or an error."""
    info = lanterns.get(fid)
    if not info or info.get("offline"):
        return {"ok": False, "fixture_id": fid, "error": "not connected"}
    with lock_for(info["port"]):
        try:
            with serial.Serial(info["port"], 115200, timeout=0.05) as s:
                time.sleep(0.1)
                d = telemetry(s, wait=1.5)
                if not d or d.get("fixture_id") != fid:
                    return {"ok": False, "fixture_id": fid, "error": f"identity mismatch: {d and d.get('fixture_id')}"}
                if "pattern_id" not in d:
                    return {"ok": False, "fixture_id": fid, "error": "firmware has no pattern mode (needs the ADR 0084 build)"}
                s.reset_input_buffer()
                s.write((line + "\n").encode())
                end, buf = time.time() + wait_s, b""
                while time.time() < end:
                    buf += s.read(4096)
                    for ln in buf.decode(errors="replace").splitlines():
                        if ln.startswith("pattern:"):
                            return {"ok": "refused" not in ln, "fixture_id": fid, "reply": ln.strip()}
        except Exception as ex:
            return {"ok": False, "fixture_id": fid, "error": str(ex)}
    return {"ok": False, "fixture_id": fid, "error": "no reply"}


def parallel(fn, items):
    out = {}
    def run(k, *a):
        out[k] = fn(k, *a)
    th = [threading.Thread(target=run, args=(k, *a)) for k, *a in items]
    for t in th: t.start()
    for t in th: t.join()
    return out


def upload_pattern(p, use_lead):
    try:
        pid, parts = pattern_parts(p, use_lead)
    except ValueError as ex:
        return {"ok": False, "error": str(ex)}
    if not parts:
        return {"ok": False, "error": "no connected lanterns have pattern-mode firmware (ADR 0084)"}
    skipped = [f for f, l in lanterns.items() if not l.get("offline") and not l.get("fw_pattern")]
    items = []
    for f, pt in parts.items():
        offs = ",".join(str(o) for o in pt["offsets_us"]) or "-"
        items.append((f, f"!P{f} SET {pid:08x} {pt['loop_us']} {pt['pulse_ms']} {pt['lead_us']} {offs}"))
    res = parallel(pattern_command, items)
    for f, r in res.items():
        if r.get("error") == "no reply":  # NVS writes can outlast the wait; verify from telemetry instead
            d = query_port(lanterns[f]["port"])
            if d and d.get("pattern_crc") == f"{parts[f]['crc']:08x}":
                r.update(ok=True, reply=f"pattern: verified from telemetry crc={parts[f]['crc']:08x}")
                r.pop("error", None)
        if r.get("ok"):
            want = f"crc={parts[f]['crc']:08x}"
            if want not in r.get("reply", ""):
                r.update(ok=False, error=f"CRC mismatch (expected {want}): {r.get('reply')}")
        r["hits"] = len(parts[f]["offsets_us"])
        r["lead_us"] = parts[f]["lead_us"]
    log_entry = {"kind": "pattern_upload", "utc": utc(), "pattern_id": f"{pid:08x}", "name": p.get("name"),
                 "use_lead": use_lead, "results": list(res.values())}
    with open(STRIKE_LOG, "a") as fh:
        fh.write(json.dumps(log_entry) + "\n")
    return {"ok": all(r.get("ok") for r in res.values()), "pattern_id": f"{pid:08x}", "results": list(res.values()),
            "skipped_old_firmware": skipped}


def standalone(action, pattern_id=None, minutes=None):
    online = [f for f, l in lanterns.items() if not l.get("offline") and l.get("fw_pattern")]
    if action == "play":
        line = lambda f: f"!P{f} PLAY {pattern_id} {minutes}"
    else:
        line = lambda f: f"!P{f} STOP"
    res = parallel(pattern_command, [(f, line(f)) for f in online])
    with open(STRIKE_LOG, "a") as fh:
        fh.write(json.dumps({"kind": f"pattern_standalone_{action}", "utc": utc(), "pattern_id": pattern_id,
                             "minutes": minutes, "results": list(res.values())}) + "\n")
    return {"ok": all(r.get("ok") for r in res.values()), "results": list(res.values())}


# ---------------------------------------------------------------------------
# Wake / keep awake. A sleeping lantern is off USB and radio, but it wakes on
# its own timer (about every 2 minutes). The waker watches the USB ports and,
# for lanterns asked to wake (or all of them in keep-awake mode), opens a
# 60-minute RAM session the moment each one appears; keep-awake also renews
# sessions before they lapse. It never waits on a busy port.
# ---------------------------------------------------------------------------
WAKE = {"targets": {}, "keep": False}
WAKE_EVENTS = []
KEEP_RENEW_BELOW_S = 15 * 60
SESSION_MINUTES = 60


def wake_event(fid, msg, ok=True):
    WAKE_EVENTS.append({"id": len(WAKE_EVENTS) + 1, "utc": utc(), "fixture_id": fid, "msg": msg, "ok": ok})
    del WAKE_EVENTS[:-300]


def wake_port(port, want_fids, renew_below_s):
    """Identify the lantern on `port`; open/renew its session if wanted. Non-blocking on busy ports."""
    lk = lock_for(port)
    if not lk.acquire(blocking=False):
        return None
    try:
        with serial.Serial(port, 115200, timeout=0.05) as s:
            time.sleep(0.1)
            d = telemetry(s, wait=1.5)
            if not d or not d.get("fixture_id"):
                return None
            fid = d["fixture_id"]
            left = d.get("decompression_left_s") or 0
            if (fid in want_fids or WAKE["keep"]) and d.get("decompression") and left < renew_below_s:
                s.reset_input_buffer()
                s.write(f"!D{fid}:{SESSION_MINUTES}\n".encode())
                # Firmware reads serial one byte per main-loop pass, so a command
                # line can take most of a second to land; confirm patiently.
                d2, new_left, end = d, 0, time.time() + 4.0
                while time.time() < end and new_left < 600:
                    time.sleep(0.4)
                    d2 = telemetry(s, wait=1.0) or d2
                    new_left = d2.get("decompression_left_s") or 0
                if new_left >= 600:
                    wake_event(fid, f"session {round(new_left / 60)} min" + (" (renewed)" if left else ""))
                    d = d2
                else:
                    wake_event(fid, "session not confirmed; will retry", ok=False)
            info = summarize(port, d)
            with state_lock:
                lanterns[fid] = info
            if fid in WAKE["targets"] and (info.get("session_left_s") or 0) >= 600:
                WAKE["targets"].pop(fid, None)
            return info
    except Exception:
        return None
    finally:
        lk.release()


def waker_loop():
    next_check = {}  # port -> monotonic time of the next routine check
    while True:
        time.sleep(1.0)
        try:
            ports = set(glob.glob("/dev/cu.usbmodem*"))
            with state_lock:
                for fid, l in lanterns.items():
                    if l.get("port") not in ports and not l.get("offline"):
                        l["offline"] = True
                by_port = {l["port"]: (fid, l) for fid, l in lanterns.items()}
            for port in list(next_check):
                if port not in ports:
                    next_check.pop(port)  # asleep: check immediately when it reappears
            if PLAYER["running"] or not (WAKE["targets"] or WAKE["keep"]):
                continue
            now = time.monotonic()
            for port in ports:
                fid, l = by_port.get(port, (None, None))
                fresh = l is None or l.get("offline")          # just woke or never seen
                wanted = fid in WAKE["targets"] or WAKE["keep"] or fid is None
                if not wanted or (not fresh and now < next_check.get(port, 0)):
                    continue
                info = wake_port(port, set(WAKE["targets"]), KEEP_RENEW_BELOW_S if WAKE["keep"] else 600)
                next_check[port] = now + (20 if info else 3)
        except Exception as ex:
            wake_event("", f"waker error: {ex}", ok=False)


def wake_status(since=0):
    return {"keep": WAKE["keep"], "targets": sorted(WAKE["targets"]),
            "events": [e for e in WAKE_EVENTS if e["id"] > since]}


# ---------------------------------------------------------------------------
# Interactions: a sensor drives a pattern variable live (host-side, USB).
# Rule: {id, name, source, far_mm, near_mm, pattern_id, bpm_from, bpm_to,
#        curve: linear|ease_in|ease_out, smooth_s, idle: stop|base, idle_s}
# Someone at far_mm plays the pattern at bpm_from; at near_mm (or closer) at
# bpm_to. Distance comes from the proximity detector's per-zone "hand" reading
# for the source lantern (proximity runs in watch-only if it was off). The
# tempo is capped at the pattern's fastest safe BPM (250 ms per lantern).
# ---------------------------------------------------------------------------
INTERACT = {"armed": False, "stop": threading.Event(), "sim": None, "sim_on": False, "status": {}}


def find_pattern(pattern_id):
    with state_lock:
        for p in STATE.get("patterns", {}).values():
            if p.get("id") == pattern_id:
                return dict(p)
    return None


def pattern_bpm_max(p):
    """Fastest BPM at which no lantern's hits are closer than MIN_HIT_SPACING_MS (wrap included)."""
    steps, spb, best = int(p.get("steps", 16)), int(p.get("steps_per_beat", 4)), 300.0
    for idx in (p.get("hits") or {}).values():
        idx = sorted({int(i) for i in idx if 0 <= int(i) < steps})
        if len(idx) < 2:
            continue
        k = min([b - a for a, b in zip(idx, idx[1:])] + [steps - idx[-1] + idx[0]])
        best = min(best, k * 60000.0 / (spb * MIN_HIT_SPACING_MS))
    return best


def shape(x, curve):
    x = max(0.0, min(1.0, x))
    return x * x if curve == "ease_in" else 1 - (1 - x) * (1 - x) if curve == "ease_out" else x


def source_members(src):
    """Lanterns a sensor rule watches: one fixture id, or a whole group
    ("group:perimeter" / "group:lantern", from each lantern's role on the Lanterns tab)."""
    if not str(src).startswith("group:"):
        return [str(src).upper()]
    want_perim = src == "group:perimeter"
    with state_lock:
        order, meta = list(STATE.get("order", [])), STATE.get("lanterns", {})
        return [f for f in order if has_sensor(lanterns.get(f, {}))
                and (meta.get(f, {}).get("role") == "perimeter") == want_perim]


def source_reading(members):
    """Nearest person across the watched sensors: (distance_mm or None, status, fixture)."""
    best, status, who = None, None, None
    for f in members:
        v = prox_live.get(f) or {}
        status = status or v.get("status")
        if v.get("status") == "hand" and v.get("depth_mm") and (best is None or v["depth_mm"] < best):
            best, status, who = v["depth_mm"], "hand", f
    return best, status, who


def interaction_runner(rule, p):
    st = INTERACT["status"]
    steps, spb = int(p["steps"]), int(p["steps_per_beat"])
    by_step = {}
    for fid, idx in (p.get("hits") or {}).items():
        for i in idx:
            if 0 <= int(i) < steps:
                by_step.setdefault(int(i), []).append(fid.upper())
    with state_lock:
        pulses = {f: STATE["lanterns"].get(f, {}).get("pulse_ms") or STATE["pulse_ms"] for f in {x for v in by_step.values() for x in v}}
    src = rule["source"] if str(rule["source"]).startswith("group:") else rule["source"].upper()
    members, members_t = source_members(src), time.perf_counter()
    started_prox = False
    conns = {}
    try:
        if not PROX["on"]:
            set_proximity(True, silent=True)
            started_prox = True
        for f in pulses:  # identity check once; strikes then go out on a write-only connection
            info = lanterns.get(f)
            if not info or info.get("offline"):
                st.setdefault("skipped_lanterns", []).append(f)
                continue
            with lock_for(info["port"]):
                s = serial.Serial(info["port"], 115200, timeout=0.05)
                time.sleep(0.05)
                d = telemetry(s, wait=1.5)
            if not d or d.get("fixture_id") != f:
                s.close()
                st.setdefault("skipped_lanterns", []).append(f)
                continue
            conns[f] = s
        bpm_max = pattern_bpm_max(p)
        lo, hi = float(rule["bpm_from"]), min(float(rule["bpm_to"]), bpm_max)
        far, near = float(rule["far_mm"]), float(rule["near_mm"])
        bpm, last_seen, last_t = lo, -1e9, time.perf_counter()
        next_step, step_i = None, 0
        last_fire = {}
        st.update(bpm_max=round(bpm_max, 1), hits=0, skipped=0)
        while not INTERACT["stop"].is_set():
            now = time.perf_counter()
            dt, last_t = now - last_t, now
            if now - members_t > 2.0:  # roles can change on the Lanterns tab while a rule runs
                members, members_t = source_members(src), now
            near_who = None
            if INTERACT["sim_on"]:
                dist = INTERACT["sim"]
                src_status = "simulated"
            else:
                dist, src_status, near_who = source_reading(members)
            if dist is not None:
                last_seen = now
                x = shape((far - float(dist)) / max(1.0, far - near), rule.get("curve", "linear"))
                target, idle = lo + (hi - lo) * x, False
            else:
                x, target = 0.0, lo
                idle = now - last_seen > float(rule.get("idle_s", 3))
            tau = max(0.05, float(rule.get("smooth_s", 0.5)))
            bpm += (target - bpm) * (1 - math.exp(-dt / tau))
            bpm = max(float(RULE_BPM_MIN), min(bpm, bpm_max))
            playing = not (idle and rule.get("idle", "stop") == "stop")
            if not playing:
                next_step = None
            else:
                if next_step is None:
                    next_step, step_i = now + 0.05, 0
                if now >= next_step:
                    for f in by_step.get(step_i, []):
                        if f not in conns:
                            continue
                        if now - last_fire.get(f, -1e9) < MIN_HIT_SPACING_MS / 1000.0:
                            st["skipped"] += 1  # tempo rose mid-loop: never strike one lantern faster than 250 ms
                            continue
                        conns[f].write(f"K{f}:{pulses[f]}\n".encode())
                        last_fire[f] = now
                        st["hits"] += 1
                    step_i = (step_i + 1) % steps
                    step_s = 60.0 / (bpm * spb)
                    next_step = next_step + step_s if now - next_step < step_s else now + step_s  # resync if far behind
            st.update(distance_mm=dist, x=round(x, 3), target_bpm=round(target, 1), bpm=round(bpm, 1),
                      playing=playing, idle=idle, step=step_i, src_status=src_status, nearest=near_who)
            wait = (next_step - time.perf_counter()) if next_step else 0.02
            time.sleep(max(0.0005, min(0.02, wait - 0.001)))
    except Exception as ex:
        st["error"] = str(ex)
    finally:
        for s in conns.values():
            try: s.close()
            except Exception: pass
        if started_prox:
            set_proximity(False)
        INTERACT["armed"] = False
        with open(STRIKE_LOG, "a") as fh:
            fh.write(json.dumps({"kind": "interaction", "utc": utc(), "rule": rule.get("name"), "pattern": p.get("name"),
                                 "hits": st.get("hits"), "skipped": st.get("skipped"), "error": st.get("error")}) + "\n")


RULE_BPM_MIN = 5  # slow is fine for the hardware; a sensor rule may start almost still and build up


def arm_interaction(rule):
    if INTERACT["armed"]:
        return {"ok": False, "error": "an interaction is already armed"}
    if PLAYER["running"]:
        return {"ok": False, "error": "stop the USB pattern first"}
    try:
        rule = {**rule, "far_mm": float(rule["far_mm"]), "near_mm": float(rule["near_mm"]),
                "bpm_from": float(rule["bpm_from"]), "bpm_to": float(rule["bpm_to"])}
    except (KeyError, TypeError, ValueError):
        return {"ok": False, "error": "rule needs far/near distances and BPM range"}
    if not (rule["near_mm"] < rule["far_mm"] <= 5000 and rule["near_mm"] >= 50):
        return {"ok": False, "error": "near must be at least 5 cm and closer than far (max 500 cm)"}
    if not (RULE_BPM_MIN <= rule["bpm_from"] <= 300 and RULE_BPM_MIN <= rule["bpm_to"] <= 300):
        return {"ok": False, "error": f"BPM must be {RULE_BPM_MIN}..300 (far {rule['bpm_from']:g}, near {rule['bpm_to']:g})"}
    src = str(rule.get("source", ""))
    if src.startswith("group:"):
        if not source_members(src):
            kind = "perimeter lights" if src == "group:perimeter" else "lanterns"
            return {"ok": False, "error": f"no {kind} with a distance sensor are connected (set roles on the Lanterns tab)"}
    elif not has_sensor(lanterns.get(src.upper(), {})):
        return {"ok": False, "error": "pick a source lantern that has a distance sensor"}
    p = find_pattern(rule.get("pattern_id"))
    if not p:
        return {"ok": False, "error": "pattern not found"}
    try:
        pattern_lanes({**p, "bpm": min(rule["bpm_from"], rule["bpm_to"])})  # validate the slowest tempo
    except ValueError as ex:
        return {"ok": False, "error": str(ex)}
    INTERACT["stop"].clear()
    INTERACT["armed"] = True
    INTERACT["rule"] = dict(rule)
    INTERACT["status"] = {"rule": rule.get("name"), "rule_id": rule.get("id"), "pattern": p.get("name"),
                          "bpm_max": round(pattern_bpm_max(p), 1)}
    threading.Thread(target=interaction_runner, args=(rule, p), daemon=True).start()
    return {"ok": True, **INTERACT["status"]}


# ---------------------------------------------------------------------------
# Atom buttons (ADR 0083 controllers) as tone lab triggers. The laptop has no
# radio, so a lantern on the hub relays: its telemetry counts admitted Atom PLAY
# presses with the source Atom and sequence. One press reaches all nine with the
# same (source, boot, sequence), so presses are deduplicated across lanterns.
# Relay mode (`!R<id>:1`, RAM-only) stops lanterns playing their own signature so
# a press only does what the button rule says; it is re-applied after resets.
# Button rule: {kind: "button", id, name, trigger: {atom: "<id>"|"any"},
#               action: {type: play_pattern|stop|next_pattern|arm_interaction|toggle_proximity,
#                        pattern_id, pattern_ids, interaction_id}}
# ---------------------------------------------------------------------------
ATOM = {"relay": False, "learn": False, "learned": None, "listener": None, "events": [], "cycle": {}}


def atom_event(kind, **kw):
    ATOM["events"].append({"id": len(ATOM["events"]) + 1, "utc": utc(), "kind": kind, **kw})
    del ATOM["events"][:-300]


def poll_lantern(fid):
    """Non-blocking telemetry read (skips a busy port). Updates the lantern table."""
    info = lanterns.get(fid)
    if not info or info.get("offline"):
        return None
    lk = lock_for(info["port"])
    locked = lk.acquire(blocking=False)
    # A USB pattern holds its lanterns' ports but only writes while playing, so the
    # listener may read alongside it (presses must still switch patterns mid-song).
    # Not during its identity and strike-count reads, though: two readers on one
    # port steal each other's bytes and the player aborts. Presses made then are
    # still counted by the lantern and picked up on the next poll.
    if not locked and not (PLAYER["running"] and PLAYER.get("phase") == "play" and fid in PLAYER.get("fids", ())):
        return None
    if not locked:
        PLAYER["side_reads"] = PLAYER.get("side_reads", 0) + 1
    try:
        with serial.Serial(info["port"], 115200, timeout=0.05) as s:
            time.sleep(0.05)
            d = telemetry(s, wait=1.0)
            if locked and d and d.get("fixture_id") == fid and "radio_relay" in d and ATOM["relay"] != bool(d.get("radio_relay")):
                s.write(f"!R{fid}:{1 if ATOM['relay'] else 0}\n".encode())  # RAM-only: re-apply after any reset
    except Exception:
        return None
    finally:
        if locked:
            lk.release()
        else:
            PLAYER["side_reads"] -= 1
    if not d or d.get("fixture_id") != fid:
        return None
    with state_lock:
        lanterns[fid] = summarize(info["port"], d)
    return d


DOUBLE_CLICK_S = 0.6   # presses closer than this belong to one multi-click (double/triple)


def stop_all():
    INTERACT["stop"].set()
    PLAYER["stop"].set()
    for _ in range(60):  # let the current pattern/interaction release its ports
        if not PLAYER["running"] and not INTERACT["armed"]:
            break
        time.sleep(0.05)


def active_patterns():
    """Patterns ticked Active on the Patterns tab, in name order (the rotation pool)."""
    with state_lock:
        pats = [dict(p) for p in STATE.get("patterns", {}).values() if p.get("active")]
    return sorted(pats, key=lambda p: (p.get("name") or "").lower())


def active_triggers():
    """Sensor rules ticked "in rotation" on the Interactions tab, in list order."""
    with state_lock:
        return [dict(x) for x in STATE.get("interactions", []) if x.get("kind") != "button" and x.get("active_trigger")]


def run_button_action(rule):
    a = rule.get("action") or {}
    kind = a.get("type")

    def play(pid, loops=0):
        p = find_pattern(pid)
        if not p:
            return "pattern not found"
        stop_all()
        r = start_pattern(p, loops, False)
        return f"playing {p.get('name')}" + (f" x{loops}" if loops else "") if r.get("ok") else r.get("error")

    if kind == "stop":
        stop_all()
        return "stopped"
    if kind == "play_pattern":
        return play(a.get("pattern_id"))
    if kind == "performance":
        # Play a pattern for N loops, then return to the sensor rule that was running.
        resume = INTERACT.get("rule") if INTERACT["armed"] else ATOM.get("resume")
        ATOM["perf_gen"] = gen = ATOM.get("perf_gen", 0) + 1
        ATOM["resume"] = resume
        loops = max(1, min(32, int(a.get("loops") or 2)))
        result = play(a.get("pattern_id"), loops)

        def resume_after():
            time.sleep(0.5)
            while PLAYER["running"] and ATOM.get("perf_gen") == gen:
                time.sleep(0.1)
            if ATOM.get("perf_gen") != gen or not resume or INTERACT["armed"] or PLAYER["running"]:
                return  # a newer press took over, or something else is already playing
            with state_lock:
                fresh = next((x for x in STATE.get("interactions", []) if x.get("id") == resume.get("id")), resume)
            ATOM["resume"] = None
            res = arm_interaction(fresh)
            atom_event("resume", result=(f"back to {fresh.get('name')}" if res.get("ok") else res.get("error")))
        threading.Thread(target=resume_after, daemon=True).start()
        return result + (f", then back to {resume.get('name')}" if resume else "")
    if kind == "next_active_pattern":
        # Switch a sensor rule to the next Active pattern (a fresh performance).
        pool = active_patterns()
        if not pool:
            return "no Active patterns (tick some on the Patterns tab)"
        armed_id = (INTERACT.get("rule") or {}).get("id") if INTERACT["armed"] else None
        # Target: the chosen rule, else the running one, else the one a performance
        # returns to, else the last trigger picked by a triple click, else the first
        # rule in the trigger rotation, else the first sensor rule.
        rot = active_triggers()
        target_id = (a.get("interaction_id") or armed_id or (ATOM.get("resume") or {}).get("id")
                     or ATOM.get("last_trigger") or (rot[0]["id"] if rot else None))
        with state_lock:
            rules = [x for x in STATE.get("interactions", []) if x.get("kind") != "button"]
            r = next((x for x in rules if x.get("id") == target_id), rules[0] if rules else None)
            if not r:
                return "no sensor rule to update"
            ids = [p["id"] for p in pool]
            cur = ids.index(r.get("pattern_id")) if r.get("pattern_id") in ids else -1
            nxt = pool[(cur + 1) % len(pool)]
            r["pattern_id"] = nxt["id"]
            save_state(STATE)
            rule_copy = dict(r)
        atom_event("rule_updated", interaction_id=rule_copy["id"], pattern_id=nxt["id"],
                   result=f"{rule_copy.get('name')} now plays {nxt.get('name')}")
        if armed_id == rule_copy["id"] or (ATOM.get("resume") or {}).get("id") == rule_copy["id"]:
            if INTERACT["armed"]:
                stop_all()
                arm_interaction(rule_copy)
            else:
                ATOM["resume"] = rule_copy  # a performance is playing; resume with the new pattern
        return f"{rule_copy.get('name')} now plays {nxt.get('name')}"
    if kind == "next_active_trigger":
        # Switch to the next sensor rule in the trigger rotation, keeping the current
        # pattern (double click picks the pattern, triple click picks the trigger).
        pool = active_triggers()
        if not pool:
            return "no sensor rules in the trigger rotation (tick \"in rotation\" on the Interactions tab)"
        running = INTERACT.get("rule") if INTERACT["armed"] else None
        cur = running or ATOM.get("resume")
        cur_id = (cur or {}).get("id") or ATOM.get("last_trigger")
        ids = [x["id"] for x in pool]
        nxt = pool[(ids.index(cur_id) + 1) % len(pool) if cur_id in ids else 0]
        with state_lock:
            if cur:
                fresh_cur = next((x for x in STATE.get("interactions", []) if x.get("id") == cur.get("id")), cur)
                carry = fresh_cur.get("pattern_id")
            else:
                carry = None
            saved = next((x for x in STATE.get("interactions", []) if x.get("id") == nxt["id"]), None)
            if carry and saved is not None and saved.get("pattern_id") != carry:
                saved["pattern_id"] = carry
                save_state(STATE)
            nxt = dict(saved or nxt)
        ATOM["last_trigger"] = nxt["id"]
        if carry:
            atom_event("rule_updated", interaction_id=nxt["id"], pattern_id=nxt.get("pattern_id"),
                       result=f"{nxt.get('name')} keeps the current pattern")
        if PLAYER["running"] and ATOM.get("resume") and not running:
            ATOM["resume"] = nxt  # a performance is playing; come back to the new trigger
            return f"after the performance: {nxt.get('name')}"
        stop_all()
        res = arm_interaction(nxt)
        return f"trigger: {nxt.get('name')}" if res.get("ok") else f"{nxt.get('name')}: {res.get('error')}"
    if kind == "next_pattern":
        ids = [x for x in (a.get("pattern_ids") or []) if find_pattern(x)]
        if not ids:
            return "no patterns in the list"
        i = (ATOM["cycle"].get(rule.get("id"), -1) + 1) % len(ids)
        ATOM["cycle"][rule.get("id")] = i
        return play(ids[i])
    if kind == "arm_interaction":
        with state_lock:
            r = next((x for x in STATE.get("interactions", []) if x.get("id") == a.get("interaction_id")), None)
        if not r:
            return "interaction not found"
        stop_all()
        res = arm_interaction(r)
        return f"armed {r.get('name')}" if res.get("ok") else res.get("error")
    if kind == "toggle_proximity":
        st = set_proximity(not PROX["on"], silent=False)
        return "proximity ding " + ("on" if st["on"] else "off")
    return "no action set"


def dispatch_click(src, clicks, via):
    """Run every enabled button rule matching this Atom and click type."""
    click = "triple" if clicks >= 3 else "double" if clicks == 2 else "single"
    if ATOM["learn"]:
        ATOM["learn"], ATOM["learned"] = False, src
        atom_event("learned", atom=src, via=via)
        return
    with state_lock:
        rules = [r for r in STATE.get("interactions", []) if r.get("kind") == "button" and r.get("enabled", True)
                 and (r.get("trigger") or {}).get("atom") in (src, "any")
                 and (r.get("trigger") or {}).get("click", "single") in (click, "any")]
    if not rules:
        atom_event("press", atom=src, via=via, click=click, result=f"no rule for a {click} click")
        return
    for r in rules:
        result = run_button_action(r)
        atom_event("press", atom=src, via=via, click=click, rule=r.get("name"), result=result)
        with open(STRIKE_LOG, "a") as fh:
            fh.write(json.dumps({"kind": "atom_press", "utc": utc(), "atom": src, "click": click,
                                 "rule": r.get("name"), "result": result}) + "\n")


def atom_loop():
    """Watch the primary listener lantern's Atom press count and classify clicks.

    Only the primary listener's count drives presses (a press reaches every lantern,
    so counting several would double-count); other lanterns are polled in rotation
    only to keep relay mode applied. Press times come from the poll time minus the
    lantern's reported press age, so two presses within DOUBLE_CLICK_S make a double
    click even when they land in different polls."""
    last_count, last_listener = None, None
    last_read = 0.0  # time of the latest successful listener read
    pending = {}   # src -> {"n": presses, "t": time of latest press}
    tick = 0
    while True:
        time.sleep(0.15)
        try:
            now = time.time()
            for src, pc in list(pending.items()):  # one or two presses settle once the window passes
                # ...and only after a read taken past the window, so a press that landed
                # inside it (but wasn't read yet because polls are slow) still counts.
                if last_read - pc["t"] > DOUBLE_CLICK_S:
                    pending.pop(src)
                    dispatch_click(src, pc["n"], ATOM["listener"])
            with state_lock:
                online = [f for f in STATE.get("order", []) if f in lanterns and not lanterns[f].get("offline")]
            if not online:
                continue
            listener = ATOM["listener"] if ATOM["listener"] in online else online[0]
            ATOM["listener"] = listener
            if listener != last_listener:
                last_count, last_listener = None, listener  # new baseline for a new listener
            tick += 1
            if tick % 6 == 0:
                others = [f for f in online if f != listener]
                if others:
                    poll_lantern(others[(tick // 6) % len(others)])  # relay upkeep only
            d = poll_lantern(listener)
            if not d or "radio_plays" not in d:
                continue
            last_read = time.time()
            count = d.get("radio_plays") or 0
            if last_count is None or count < last_count:
                last_count = count
                continue
            n, last_count = count - last_count, count
            if n <= 0:
                continue
            src = d.get("radio_last_src")
            t_press = time.time() - (d.get("radio_last_age_ms") or 0) / 1000.0
            pc = pending.get(src)
            if pc and t_press - pc["t"] <= DOUBLE_CLICK_S:
                pc["n"] += n
            else:
                pc = pending[src] = {"n": n, "t": t_press}
            pc["t"] = t_press
            if pc["n"] >= 3:  # nothing longer than a triple, so fire right away
                pending.pop(src, None)
                dispatch_click(src, 3, listener)
        except Exception as ex:
            atom_event("error", result=str(ex))


def set_relay(on):
    ATOM["relay"] = bool(on)
    with state_lock:
        online = [f for f, l in lanterns.items() if not l.get("offline") and l.get("decompression")]
    parallel(lambda f: send_exact(f, f"!R{f}:{1 if on else 0}"), [(f,) for f in online])
    # Firmware reads serial slowly, so confirmation comes from telemetry: the
    # listener loop re-applies relay to any lantern that does not report it.
    atom_event("relay", result=("Atom presses now drive the tone lab (lanterns won't play the signature)" if on
                                else "Atom presses play the signature on the lanterns again"))
    return atom_status()


def atom_status(since=0):
    with state_lock:
        atoms = dict(STATE.get("atoms", {}))
    return {"relay": ATOM["relay"], "learn": ATOM["learn"], "learned": ATOM["learned"], "listener": ATOM["listener"],
            "atoms": atoms, "events": [e for e in ATOM["events"] if e["id"] > since]}


def interaction_status():
    return {"armed": INTERACT["armed"], "sim_on": INTERACT["sim_on"], "sim": INTERACT["sim"], **INTERACT["status"]}


def snapshot():
    with state_lock:
        return {"lanterns": sorted(lanterns.values(), key=lambda l: l["hub"]), "state": STATE,
                "state_file": str(STATE_FILE.relative_to(REPO)), "pulse_min": PULSE_MIN_MS, "pulse_max": PULSE_MAX_MS, "audio": sd is not None,
                "min_hit_spacing_ms": MIN_HIT_SPACING_MS}


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
        if self.path.startswith("/api/wake"):
            since = int(self.path.split("since=")[1]) if "since=" in self.path else 0
            return self.reply(wake_status(since))
        if self.path.startswith("/api/proxtrace"):
            since = float(self.path.split("since=")[1]) if "since=" in self.path else 0.0
            return self.reply({"server_t": round(time.time(), 3),
                               "trace": {f: [x for x in v if x[0] > since] for f, v in prox_trace.items()}})
        if self.path.startswith("/api/prox"):
            since = int(self.path.split("since=")[1]) if "since=" in self.path else 0
            return self.reply(prox_status(since))
        if self.path.startswith("/api/atom"):
            since = int(self.path.split("since=")[1]) if "since=" in self.path else 0
            return self.reply(atom_status(since))
        if self.path == "/api/interaction/status":
            return self.reply(interaction_status())
        if self.path == "/api/pattern/status":
            return self.reply({"running": PLAYER["running"], **PLAYER["status"]})
        name = "index.html" if self.path == "/" else self.path.lstrip("/")
        f = (UI_DIR / name).resolve()
        if f.parent == UI_DIR and f.suffix in UI_TYPES and f.exists():
            return self.reply(f.read_bytes(), ctype=UI_TYPES[f.suffix])
        if self.path == "/api/snapshot":
            return self.reply(snapshot())
        self.reply({"error": "not found"}, 404)

    def do_POST(self):
        b = self.body()
        if self.path == "/api/proximity":
            if "sensor" in b:  # per-sensor on/off (and optionally the watch/ding setting)
                if "silent" in b:
                    PROX["silent"] = bool(b["silent"])
                return self.reply(set_prox_sensor(b["sensor"], bool(b.get("active"))))
            return self.reply(set_proximity(bool(b.get("on")), b.get("range_mm"), b.get("silent"), b.get("relearn")))
        if self.path == "/api/wake":
            if "keep" in b:
                WAKE["keep"] = bool(b["keep"])
                wake_event("", "keep awake " + ("on" if WAKE["keep"] else "off"))
            ids = b.get("ids")
            if ids == "all":
                with state_lock:
                    ids = sorted(set(lanterns) | set(STATE.get("order", [])))
            for fid in ids or []:
                fid = str(fid).upper()
                if len(fid) == 6:
                    WAKE["targets"][fid] = utc()
            return self.reply(wake_status())
        if self.path == "/api/pattern/play":
            loops = int(b.get("loops", 0))
            if not 0 <= loops <= 1000:
                return self.reply({"ok": False, "error": "loops 0..1000 (0 = until stopped)"}, 400)
            return self.reply(start_pattern(b.get("pattern") or {}, loops, bool(b.get("comp"))))
        if self.path == "/api/pattern/stop":
            PLAYER["stop"].set()
            return self.reply({"ok": True})
        if self.path in ("/api/pattern/upload", "/api/pattern/standalone"):
            if PLAYER["running"]:
                return self.reply({"ok": False, "error": "stop the USB pattern first"}, 409)
            with ProxPaused():
                if self.path == "/api/pattern/upload":
                    return self.reply(upload_pattern(b.get("pattern") or {}, bool(b.get("lead"))))
                action = b.get("action")
                if action == "play":
                    pid, minutes = str(b.get("pattern_id", "")), int(b.get("minutes", 15))
                    if not (len(pid) == 8 and all(c in "0123456789abcdef" for c in pid) and 1 <= minutes <= 120):
                        return self.reply({"ok": False, "error": "pattern id (8 hex) and minutes 1..120 required"}, 400)
                    return self.reply(standalone("play", pid, minutes))
                if action == "stop":
                    return self.reply(standalone("stop"))
                return self.reply({"ok": False, "error": "action must be play or stop"}, 400)
        if self.path == "/api/pattern/lanes":
            try:
                loop_ms, step_ms, lanes = pattern_lanes(b.get("pattern") or {})
                return self.reply({"ok": True, "loop_ms": loop_ms, "step_ms": step_ms, "lanes": lanes})
            except ValueError as ex:
                return self.reply({"ok": False, "error": str(ex)})
        if self.path == "/api/atom":
            if "relay" in b:
                with ProxPaused():
                    return self.reply(set_relay(bool(b["relay"])))
            if b.get("learn"):
                ATOM["learn"], ATOM["learned"] = True, None
            if isinstance(b.get("name"), dict):  # {"atom": id, "name": text}
                aid, nm = str(b["name"].get("atom", "")).upper(), str(b["name"].get("name", "")).strip()[:40]
                if len(aid) == 6:
                    with state_lock:
                        if nm:
                            STATE["atoms"][aid] = nm
                        else:
                            STATE["atoms"].pop(aid, None)
                        save_state(STATE)
            return self.reply(atom_status())
        if self.path == "/api/interaction/arm":
            return self.reply(arm_interaction(b.get("rule") or {}))
        if self.path == "/api/interaction/disarm":
            INTERACT["stop"].set()
            return self.reply({"ok": True})
        if self.path == "/api/interaction/sim":
            INTERACT["sim_on"] = bool(b.get("on"))
            mm = b.get("mm")
            INTERACT["sim"] = float(mm) if isinstance(mm, (int, float)) else None
            return self.reply(interaction_status())
        if INTERACT["armed"] and self.path in ("/api/strike", "/api/play", "/api/session", "/api/calibrate",
                                               "/api/pattern/play", "/api/pattern/upload", "/api/pattern/standalone"):
            return self.reply({"ok": False, "error": "disarm the interaction first"}, 409)
        if PLAYER["running"] and self.path in ("/api/scan", "/api/strike", "/api/play", "/api/session", "/api/calibrate"):
            return self.reply({"ok": False, "error": "stop the pattern first"}, 409)
        if self.path in ("/api/scan", "/api/strike", "/api/play", "/api/session", "/api/calibrate"):
            with ProxPaused():
                return self.post_serial(b)
        return self.post_serial(b)

    def post_serial(self, b):
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
            raw_slots = b.get("slots")
            slots = {fid: int(v) for fid, v in zip(order, raw_slots)} if isinstance(raw_slots, list) and len(raw_slots) == len(order) else None
            return self.reply(play(order, gap, pulses, comp=bool(b.get("comp")), measure=bool(b.get("measure")), slots=slots))
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
            # Only real short fixture ids (6 hex) are kept, so a stray client value
            # can never add phantom lanterns to the tone map.
            real = lambda f: isinstance(f, str) and len(f) == 6 and all(c in "0123456789ABCDEF" for c in f)
            with state_lock:
                STATE["lanterns"] = {k: v for k, v in b.get("lanterns", STATE["lanterns"]).items() if real(k)}
                STATE["order"] = [f for f in b.get("order", STATE["order"]) if real(f)]
                STATE["presets"] = b.get("presets", STATE["presets"])
                STATE["patterns"] = b.get("patterns", STATE["patterns"])
                STATE["tone_links"] = [f for f in b.get("tone_links", STATE["tone_links"]) if real(f)]
                if isinstance(b.get("interactions"), list):
                    STATE["interactions"] = [r for r in b["interactions"] if isinstance(r, dict)]
                if pulse_ok(b.get("pulse_ms")):
                    STATE["pulse_ms"] = b["pulse_ms"]
                save_state(STATE)
            return self.reply({"ok": True, "updated_utc": STATE["updated_utc"]})
        self.reply({"error": "not found"}, 404)


if __name__ == "__main__":
    print(f"tone lab on http://localhost:{ARGS.port}  ui -> {UI_DIR}  state -> {STATE_FILE}", flush=True)
    threading.Thread(target=waker_loop, daemon=True).start()
    threading.Thread(target=atom_loop, daemon=True).start()
    ThreadingHTTPServer(("127.0.0.1", ARGS.port), H).serve_forever()
