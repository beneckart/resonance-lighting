#!/usr/bin/env python3
"""Local web dashboard for a USB-tethered net_bench serial bridge.

The dashboard owns the serial port (COM7 in the travel setup), parses the master's
`nb-*` lines, serves a localhost web UI, and can write safe serial commands back to
the bridge. It optionally rebroadcasts `nb-*` lines to UDP :54321 so the existing
net_bench loggers/monitors can keep working.

Examples:
  python ops/bench/net_bench_dashboard.py --port COM7
  python ops/bench/net_bench_dashboard.py --port /dev/ttyACM0 --http-port 8765
"""
from __future__ import annotations

import argparse
import json
import re
import socket
import threading
import time
import urllib.parse
from collections import deque
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

import serial  # pyserial


RX_MASTER = re.compile(
    r"nb-master id=(\w+) ch=(\d+) frames=(\d+) sendok=(\d+) sendfail=(\d+) up=(\d+) bv=([\d.]+)"
)
RX_PEER = re.compile(
    r"nb-peer id=(\w+) seq=(\d+) rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) bv=([\d.-]+) "
    r"ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=(\d+) mode=(\d+) dlpdr=([\d.]+) dlrssi=(-?\d+) up=(\d+) age=(\d+)"
    r"(?: sv=([\d.-]+) sma=(-?\d+) sgood=(\d+))?"
    r"(?: lux=([\w.\-]+) ch0=(\d+) ch1=(\d+) ptc=([\w.\-]+) prh=(-?\d+) btc=([\w.\-]+))?"
    r"(?: ipv=(-?\d+) ipa=(-?\d+) ibv=(-?\d+) iba=(-?\d+))?"
)
RX_SCANAP = re.compile(
    r"nb-scanap from=(\w+) scan=(\d+) idx=(\d+) count=(\d+) bssid=([0-9a-fA-F:]+) "
    r"ap_rssi=(-?\d+) ch=(\d+) enc=(\d+) linkrssi=(-?\d+) ssid=(.*)"
)


def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def maybe_float(text: str | None) -> float | None:
    if text is None:
        return None
    try:
        value = float(text)
    except ValueError:
        return None
    return None if value != value else value


def maybe_ina(text: str | None) -> int | None:
    if text is None:
        return None
    value = int(text)
    return None if value == -32768 else value


def watts(volts: float | None, milliamps: int | None) -> float | None:
    if volts is None or milliamps is None:
        return None
    return round(volts * milliamps / 1000.0, 4)


class DashboardState:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.master: dict[str, Any] | None = None
        self.peers: dict[str, dict[str, Any]] = {}
        self.scans: deque[dict[str, Any]] = deque(maxlen=80)
        self.raw: deque[dict[str, Any]] = deque(maxlen=160)
        self.events: deque[dict[str, Any]] = deque(maxlen=400)
        self.serial_status: dict[str, Any] = {
            "connected": False,
            "port": None,
            "error": None,
            "lines": 0,
            "started_ts": now_iso(),
        }
        self.last_command: dict[str, Any] | None = None
        self.serial_handle: serial.Serial | None = None

    def add_event(self, kind: str, payload: dict[str, Any]) -> None:
        with self.lock:
            seq = self.events[-1]["seq"] + 1 if self.events else 1
            event = {"seq": seq, "kind": kind, "ts_utc": now_iso(), **payload}
            self.events.append(event)

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            return {
                "ts_utc": now_iso(),
                "serial": dict(self.serial_status),
                "master": dict(self.master) if self.master else None,
                "peers": {pid: dict(peer) for pid, peer in self.peers.items()},
                "scans": list(self.scans),
                "raw": list(self.raw),
                "last_command": dict(self.last_command) if self.last_command else None,
            }

    def mark_serial(self, **kwargs: Any) -> None:
        with self.lock:
            self.serial_status.update(kwargs)

    def remember_command(self, cmd: str, label: str) -> None:
        with self.lock:
            self.last_command = {"cmd": cmd, "label": label, "ts_utc": now_iso()}


class SerialWorker(threading.Thread):
    def __init__(
        self,
        state: DashboardState,
        port: str,
        baud: int,
        udp_host: str | None,
        udp_port: int,
    ) -> None:
        super().__init__(daemon=True)
        self.state = state
        self.port = port
        self.baud = baud
        self.udp_host = udp_host
        self.udp_port = udp_port
        self.stop_event = threading.Event()
        self.udp_sock: socket.socket | None = None
        if udp_host:
            self.udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    def run(self) -> None:
        self.state.mark_serial(port=self.port)
        while not self.stop_event.is_set():
            try:
                ser = serial.Serial(self.port, self.baud, timeout=1.0)
            except (serial.SerialException, OSError) as exc:
                self.state.mark_serial(connected=False, error=str(exc))
                time.sleep(1.5)
                continue
            self.state.serial_handle = ser
            self.state.mark_serial(connected=True, error=None)
            self.state.add_event("status", {"message": f"opened {self.port}"})
            try:
                while not self.stop_event.is_set():
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", "replace").strip()
                    if not line:
                        continue
                    self.handle_line(line)
                    if self.udp_sock and line.startswith("nb-"):
                        self.udp_sock.sendto((line + "\n").encode("utf-8"), (self.udp_host, self.udp_port))
            except (serial.SerialException, OSError) as exc:
                self.state.mark_serial(connected=False, error=str(exc))
                self.state.add_event("status", {"message": f"serial dropped: {exc}"})
            finally:
                try:
                    ser.close()
                except Exception:
                    pass
                if self.state.serial_handle is ser:
                    self.state.serial_handle = None
            time.sleep(0.8)

    def handle_line(self, line: str) -> None:
        ts = now_iso()
        with self.state.lock:
            self.state.serial_status["lines"] = int(self.state.serial_status.get("lines", 0)) + 1
            self.state.raw.append({"ts_utc": ts, "line": line})

        m = RX_MASTER.search(line)
        if m:
            pid, ch, frames, send_ok, send_fail, up, bv = m.groups()
            row = {
                "id": pid,
                "channel": int(ch),
                "frames": int(frames),
                "send_ok": int(send_ok),
                "send_fail": int(send_fail),
                "uptime_ms": int(up),
                "battery_v": float(bv),
                "ts_utc": ts,
            }
            with self.state.lock:
                self.state.master = row
            self.state.add_event("master", row)
            return

        m = RX_PEER.search(line)
        if m:
            (
                pid,
                seq,
                rx,
                gaps,
                pdr,
                rssi,
                bv,
                ima,
                soc,
                rr,
                ca,
                mode,
                dlpdr,
                dlrssi,
                up,
                age,
                sv,
                sma,
                sgood,
                lux,
                ch0,
                ch1,
                ptc,
                prh,
                btc,
                ipv,
                ipa,
                ibv,
                iba,
            ) = m.groups()
            supply_v = maybe_float(sv)
            supply_ma = int(sma) if sma is not None else None
            battery_v = float(bv)
            battery_ma = int(ima)
            ina_panel_mv = maybe_ina(ipv)
            ina_panel_ma = maybe_ina(ipa)
            row = {
                "id": pid,
                "seq": int(seq),
                "rx": int(rx),
                "gaps": int(gaps),
                "pdr": float(pdr),
                "rssi_dbm": int(rssi),
                "battery_v": battery_v,
                "battery_ma": battery_ma,
                "battery_w": watts(battery_v, battery_ma),
                "soc_pct": int(soc),
                "reset_reason": rr,
                "ca_state": int(ca),
                "peer_mode": int(mode),
                "dl_pdr": float(dlpdr),
                "dl_rssi_dbm": int(dlrssi),
                "uptime_ms": int(up),
                "age_ms": int(age),
                "supply_v": supply_v,
                "supply_ma": supply_ma,
                "supply_good": bool(int(sgood)) if sgood is not None else None,
                "supply_w": watts(supply_v, supply_ma),
                "lux": maybe_float(lux),
                "light_sat": lux == "sat",
                "light_ch0": int(ch0) if ch0 is not None else None,
                "light_ch1": int(ch1) if ch1 is not None else None,
                "panel_temp_c": maybe_float(ptc),
                "panel_rh_pct": None if prh is None or int(prh) < 0 else int(prh),
                "batt_temp_c": maybe_float(btc),
                "ina_panel_mv": ina_panel_mv,
                "ina_panel_ma": ina_panel_ma,
                "ina_panel_w": round(ina_panel_mv * ina_panel_ma / 1e6, 4)
                if ina_panel_mv is not None and ina_panel_ma is not None
                else None,
                "ina_batt_mv": maybe_ina(ibv),
                "ina_batt_ma": maybe_ina(iba),
                "ts_utc": ts,
            }
            if row["supply_w"] is not None and row["battery_w"] is not None:
                row["load_w"] = round(row["supply_w"] - row["battery_w"], 4)
            with self.state.lock:
                self.state.peers[pid] = row
            self.state.add_event("peer", row)
            return

        m = RX_SCANAP.search(line)
        if m:
            frm, scan, idx, count, bssid, ap_rssi, ch, enc, linkrssi, ssid = m.groups()
            row = {
                "from": frm,
                "scan_id": int(scan),
                "idx": int(idx),
                "count": int(count),
                "bssid": bssid,
                "ap_rssi_dbm": int(ap_rssi),
                "channel": int(ch),
                "enc": int(enc),
                "link_rssi_dbm": int(linkrssi),
                "ssid": ssid.rstrip(),
                "ts_utc": ts,
            }
            with self.state.lock:
                self.state.scans.append(row)
            self.state.add_event("scanap", row)

    def send_command(self, cmd: str, label: str) -> None:
        handle = self.state.serial_handle
        if handle is None or not handle.is_open:
            raise RuntimeError("serial port is not open")
        handle.write(cmd.encode("ascii"))
        handle.flush()
        self.state.remember_command(cmd, label)
        self.state.add_event("command", {"cmd": cmd, "label": label})


HTML = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>net_bench dashboard</title>
<style>
:root {
  color-scheme: light;
  --bg: #f5f7f6;
  --panel: #ffffff;
  --ink: #17201c;
  --muted: #65716b;
  --line: #d7ded9;
  --green: #14853f;
  --red: #bd3030;
  --amber: #b46b00;
  --blue: #1769aa;
  --soft-green: #e8f5ec;
  --soft-red: #fae9e8;
  --soft-amber: #fff3dc;
  --soft-blue: #e8f1fa;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--ink); }
main { max-width: 1320px; margin: 0 auto; padding: 18px; }
header { display: flex; align-items: end; justify-content: space-between; gap: 14px; margin-bottom: 16px; }
h1 { font-size: 22px; margin: 0; font-weight: 720; letter-spacing: 0; }
.sub { color: var(--muted); font-size: 13px; margin-top: 3px; }
.status { display: flex; gap: 8px; flex-wrap: wrap; justify-content: end; }
.pill { border: 1px solid var(--line); background: var(--panel); border-radius: 999px; padding: 7px 10px; font-size: 13px; line-height: 1; }
.ok { color: var(--green); background: var(--soft-green); border-color: #bbe2c6; }
.bad { color: var(--red); background: var(--soft-red); border-color: #efc2bd; }
.warn { color: var(--amber); background: var(--soft-amber); border-color: #f0d198; }
.grid { display: grid; grid-template-columns: repeat(12, 1fr); gap: 12px; }
.panel { background: var(--panel); border: 1px solid var(--line); border-radius: 8px; padding: 14px; min-width: 0; }
.span-3 { grid-column: span 3; }
.span-4 { grid-column: span 4; }
.span-5 { grid-column: span 5; }
.span-7 { grid-column: span 7; }
.span-8 { grid-column: span 8; }
.span-12 { grid-column: span 12; }
.metric-label { color: var(--muted); font-size: 12px; text-transform: uppercase; letter-spacing: .04em; }
.metric-value { font-size: clamp(24px, 3.6vw, 42px); line-height: 1.04; font-weight: 760; letter-spacing: 0; margin-top: 7px; font-variant-numeric: tabular-nums; }
.metric-unit { color: var(--muted); font-size: 15px; font-weight: 560; margin-left: 4px; }
.metric-foot { color: var(--muted); margin-top: 9px; font-size: 13px; min-height: 18px; font-variant-numeric: tabular-nums; }
.metric-value.good { color: var(--green); }
.metric-value.bad { color: var(--red); }
.metric-value.warn { color: var(--amber); }
.section-title { font-size: 14px; color: var(--muted); margin: 0 0 11px; font-weight: 700; text-transform: uppercase; letter-spacing: .04em; }
.env-grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 10px; }
.env-cell { border: 1px solid var(--line); border-radius: 7px; padding: 10px; min-height: 78px; }
.env-label { color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .04em; }
.env-value { margin-top: 8px; font-size: 23px; font-weight: 760; font-variant-numeric: tabular-nums; }
.section-head { display: flex; align-items: center; justify-content: space-between; gap: 8px; margin: 0 0 11px; }
.section-head .section-title { margin: 0; }
.unit-toggle { min-height: 30px; padding: 0 10px; font-size: 12px; font-weight: 760; }
.table-wrap { overflow: auto; }
table { width: 100%; border-collapse: collapse; font-size: 13px; }
th, td { padding: 8px 7px; border-bottom: 1px solid var(--line); text-align: right; white-space: nowrap; font-variant-numeric: tabular-nums; }
th:first-child, td:first-child { text-align: left; }
th { color: var(--muted); font-weight: 700; font-size: 12px; }
.controls { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 8px; }
button, input { font: inherit; border-radius: 7px; border: 1px solid var(--line); background: #fff; color: var(--ink); min-height: 38px; }
button { cursor: pointer; font-weight: 700; }
button:hover { border-color: #9aaba1; background: #f9fbfa; }
button.primary { background: var(--soft-blue); border-color: #b8d5ec; color: var(--blue); }
button.warn { background: var(--soft-amber); border-color: #f0d198; color: var(--amber); }
input { padding: 0 10px; width: 100%; font-variant-numeric: tabular-nums; }
.maintain { display: grid; grid-template-columns: minmax(0, 1fr) 76px; gap: 8px; margin-top: 8px; }
.history { height: 168px; overflow: auto; background: #111814; color: #dbe8df; border-radius: 7px; padding: 10px; font-family: ui-monospace, SFMono-Regular, Consolas, monospace; font-size: 12px; line-height: 1.45; }
.history div { white-space: pre-wrap; overflow-wrap: anywhere; }
.signal { width: 100%; height: 8px; background: #edf1ef; border-radius: 999px; overflow: hidden; }
.signal > span { display: block; height: 100%; background: linear-gradient(90deg, #bd3030, #b46b00, #14853f); }
.spark { height: 76px; width: 100%; display: block; }
.empty { color: var(--muted); padding: 18px 0; }
@media (max-width: 920px) {
  main { padding: 12px; }
  header { align-items: start; flex-direction: column; }
  .status { justify-content: start; }
  .span-3, .span-4, .span-5, .span-7, .span-8 { grid-column: span 12; }
  .controls { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
</style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>net_bench solar bridge</h1>
      <div class="sub" id="subtitle">Waiting for telemetry</div>
    </div>
    <div class="status">
      <span class="pill" id="serialPill">serial</span>
      <span class="pill" id="peerPill">peer</span>
      <span class="pill" id="chargePill">supply</span>
    </div>
  </header>

  <section class="grid">
    <div class="panel span-3">
      <div class="metric-label">Panel INA</div>
      <div class="metric-value" id="panelW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="panelSpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="panelFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-label">Charger Supply</div>
      <div class="metric-value" id="supplyW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="supplySpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="supplyFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-label">Battery</div>
      <div class="metric-value" id="batteryW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="batterySpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="batteryFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-label">Light</div>
      <div class="metric-value" id="lux">--<span class="metric-unit">lux</span></div>
      <canvas class="spark" id="luxSpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="luxFoot">--</div>
    </div>

    <div class="panel span-4">
      <div class="section-head">
        <p class="section-title">Environment</p>
        <button class="unit-toggle" id="tempToggle" type="button">F</button>
      </div>
      <div class="env-grid">
        <div class="env-cell">
          <div class="env-label">Panel temp</div>
          <div class="env-value" id="panelTemp">--</div>
        </div>
        <div class="env-cell">
          <div class="env-label">Humidity</div>
          <div class="env-value" id="panelRh">--</div>
        </div>
        <div class="env-cell">
          <div class="env-label">Batt temp</div>
          <div class="env-value" id="battTemp">--</div>
        </div>
      </div>
      <div class="metric-foot" id="envFoot">SHT31 / battery-temp fields when present</div>
    </div>

    <div class="panel span-8">
      <p class="section-title">Peers</p>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>id</th><th>age</th><th>RSSI</th><th>PDR</th><th>SOC</th><th>VBAT</th>
              <th>IBAT</th><th>SV</th><th>SMa</th><th>INA panel</th><th>Panel C</th>
              <th>RH</th><th>Batt C</th><th>good</th>
            </tr>
          </thead>
          <tbody id="peerRows"><tr><td colspan="14" class="empty">Waiting for peer heartbeat</td></tr></tbody>
        </table>
      </div>
    </div>

    <div class="panel span-4">
      <p class="section-title">Controls</p>
      <div class="controls">
        <button data-cmd="r">Refresh</button>
        <button class="primary" data-cmd="m46">4.6 V</button>
        <button class="primary" data-cmd="m52">5.2 V</button>
        <button class="primary" data-cmd="m71">7.1 V</button>
        <button class="warn" data-cmd="U">Peer maint</button>
        <button data-cmd="c">Resume</button>
        <button data-cmd="I">Identify all</button>
        <button data-cmd="i">Identify next</button>
      </div>
      <div class="maintain">
        <input id="maintainInput" inputmode="decimal" placeholder="MPP volts, e.g. 6.8">
        <button id="maintainBtn">Set</button>
      </div>
      <div class="metric-foot" id="commandStatus">No command sent</div>
    </div>

    <div class="panel span-5">
      <p class="section-title">Master</p>
      <div class="table-wrap">
        <table><tbody id="masterRows"><tr><td class="empty">Waiting for master line</td></tr></tbody></table>
      </div>
    </div>

    <div class="panel span-7">
      <p class="section-title">Recent serial</p>
      <div class="history" id="rawLog"></div>
    </div>
  </section>
</main>

<script>
const history = {
  panel: [], supply: [], battery: [], lux: []
};
let state = null;
let tempUnit = localStorage.getItem("netBenchTempUnit") || "F";

function fmt(v, digits = 2) {
  if (v === null || v === undefined || Number.isNaN(Number(v))) return "--";
  return Number(v).toFixed(digits);
}
function esc(s) {
  return String(s).replace(/[&<>"']/g, ch => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[ch]));
}
function age(ts) {
  if (!ts) return "--";
  const s = Math.max(0, (Date.now() - Date.parse(ts)) / 1000);
  return s < 10 ? `${s.toFixed(1)}s` : `${Math.round(s)}s`;
}
function msAge(ms) {
  if (ms === null || ms === undefined) return "--";
  const s = Math.max(0, Number(ms) / 1000);
  return s < 10 ? `${s.toFixed(1)}s` : `${Math.round(s)}s`;
}
function freshPeer(peer) {
  return peer && Number(peer.age_ms) < 5000;
}
function clsForGood(ok) {
  return ok ? "pill ok" : "pill bad";
}
function bestPeer(s) {
  const peers = Object.values(s.peers || {});
  if (!peers.length) return null;
  peers.sort((a, b) => Date.parse(b.ts_utc) - Date.parse(a.ts_utc));
  return peers[0];
}
function pushHist(name, value) {
  if (value === null || value === undefined || Number.isNaN(Number(value))) return;
  history[name].push(Number(value));
  if (history[name].length > 120) history[name].shift();
}
function drawSpark(id, values, color) {
  const canvas = document.getElementById(id);
  const ctx = canvas.getContext("2d");
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = "#d7ded9";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, h - 10);
  ctx.lineTo(w, h - 10);
  ctx.stroke();
  if (values.length < 2) return;
  const min = Math.min(...values), max = Math.max(...values);
  const span = Math.max(0.001, max - min);
  ctx.strokeStyle = color;
  ctx.lineWidth = 2.5;
  ctx.beginPath();
  values.forEach((v, i) => {
    const x = i / (values.length - 1) * w;
    const y = h - 12 - ((v - min) / span) * (h - 22);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
}
function setMetric(id, value, unit, klass = "") {
  document.getElementById(id).innerHTML = `${value}<span class="metric-unit">${unit}</span>`;
  document.getElementById(id).className = `metric-value ${klass}`.trim();
}
function setText(id, text) {
  document.getElementById(id).textContent = text;
}
function tempValue(v) {
  if (v === null || v === undefined || Number.isNaN(Number(v))) return null;
  return tempUnit === "F" ? Number(v) * 9 / 5 + 32 : Number(v);
}
function fmtTemp(v) {
  const t = tempValue(v);
  return t === null ? "--" : `${fmt(t, 1)} ${tempUnit}`;
}
function fmtRh(v) {
  return v === null || v === undefined || Number.isNaN(Number(v)) ? "--" : `${fmt(v, 0)}%`;
}
function render(s) {
  state = s;
  const peer = bestPeer(s);
  document.getElementById("tempToggle").textContent = tempUnit;
  const serialPill = document.getElementById("serialPill");
  serialPill.textContent = s.serial.connected ? `${s.serial.port} open` : `${s.serial.port || "serial"} closed`;
  serialPill.className = s.serial.connected ? "pill ok" : "pill bad";
  const isFresh = freshPeer(peer);
  document.getElementById("subtitle").textContent = peer
    ? `Peer ${peer.id} data age ${msAge(peer.age_ms)}, ${s.serial.lines} serial lines`
    : `Listening on ${s.serial.port || "serial"}, ${s.serial.lines} serial lines`;
  document.getElementById("peerPill").textContent = peer ? (isFresh ? `peer ${peer.id}` : `stale ${msAge(peer.age_ms)}`) : "no peer";
  document.getElementById("peerPill").className = peer ? (isFresh ? "pill ok" : "pill bad") : "pill warn";
  document.getElementById("chargePill").textContent = !peer ? "no supply" :
    (!isFresh ? "supply stale" : (peer.supply_good ? "charger good" : "charger not good"));
  document.getElementById("chargePill").className = peer && isFresh && peer.supply_good ? "pill ok" : "pill warn";

  if (peer) {
    const panelW = peer.ina_panel_w ?? null;
    const panelHarvestW = panelW === null ? null : Math.abs(panelW);
    const supplyW = peer.supply_w ?? null;
    const batteryW = peer.battery_w ?? null;
    const lux = peer.light_sat ? null : peer.lux;
    pushHist("panel", panelHarvestW);
    pushHist("supply", supplyW);
    pushHist("battery", batteryW);
    pushHist("lux", lux);

    setMetric("panelW", fmt(panelHarvestW, 3), "W", panelHarvestW > 0.05 ? "good" : "");
    document.getElementById("panelFoot").textContent =
      `${fmt((peer.ina_panel_mv ?? 0) / 1000, 3)} V, ${peer.ina_panel_ma ?? "--"} mA`;
    setMetric("supplyW", fmt(supplyW, 3), "W", peer.supply_good ? "good" : "warn");
    document.getElementById("supplyFoot").textContent =
      `${fmt(peer.supply_v, 3)} V, ${peer.supply_ma ?? "--"} mA, good=${peer.supply_good ? 1 : 0}`;
    setMetric("batteryW", fmt(batteryW, 3), "W", batteryW > 0 ? "good" : "bad");
    document.getElementById("batteryFoot").textContent =
      `${fmt(peer.battery_v, 3)} V, ${peer.battery_ma} mA, SOC ${peer.soc_pct}%`;
    setMetric("lux", peer.light_sat ? "sat" : fmt(lux, 1), "lux");
    document.getElementById("luxFoot").textContent =
      `ch0 ${peer.light_ch0 ?? "--"}, ch1 ${peer.light_ch1 ?? "--"}`;
    setText("panelTemp", fmtTemp(peer.panel_temp_c));
    setText("panelRh", fmtRh(peer.panel_rh_pct));
    setText("battTemp", fmtTemp(peer.batt_temp_c));
    setText("envFoot", peer.panel_temp_c === null && peer.panel_rh_pct === null && peer.batt_temp_c === null
      ? "No temp/RH sensor data in latest heartbeat"
      : `Updated ${msAge(peer.age_ms)} ago`);
  }
  drawSpark("panelSpark", history.panel, "#14853f");
  drawSpark("supplySpark", history.supply, "#1769aa");
  drawSpark("batterySpark", history.battery, "#bd3030");
  drawSpark("luxSpark", history.lux, "#b46b00");

  const rows = Object.values(s.peers || {}).sort((a, b) => a.id.localeCompare(b.id));
  document.getElementById("peerRows").innerHTML = rows.length ? rows.map(p => {
    const pct = Math.max(0, Math.min(100, (p.rssi_dbm + 90) / 65 * 100));
    return `<tr>
      <td>${esc(p.id)}</td><td>${msAge(p.age_ms)}</td>
      <td><div>${p.rssi_dbm} dBm</div><div class="signal"><span style="width:${pct}%"></span></div></td>
      <td>${fmt(p.pdr * 100, 1)}%</td><td>${p.soc_pct}%</td><td>${fmt(p.battery_v, 3)}</td>
      <td>${p.battery_ma}</td><td>${fmt(p.supply_v, 3)}</td><td>${p.supply_ma ?? "--"}</td>
      <td>${fmt((p.ina_panel_mv ?? 0) / 1000, 3)} V / ${p.ina_panel_ma ?? "--"} mA</td>
      <td>${fmtTemp(p.panel_temp_c)}</td><td>${fmtRh(p.panel_rh_pct)}</td><td>${fmtTemp(p.batt_temp_c)}</td>
      <td>${freshPeer(p) ? (p.supply_good ? "yes" : "no") : "stale"}</td>
    </tr>`;
  }).join("") : `<tr><td colspan="14" class="empty">Waiting for peer heartbeat</td></tr>`;

  const m = s.master;
  document.getElementById("masterRows").innerHTML = m ? `
    <tr><th>id</th><td>${esc(m.id)}</td></tr>
    <tr><th>channel</th><td>${m.channel}</td></tr>
    <tr><th>uptime</th><td>${Math.round(m.uptime_ms / 1000)} s</td></tr>
    <tr><th>battery</th><td>${fmt(m.battery_v, 3)} V</td></tr>
    <tr><th>frames</th><td>${m.frames}</td></tr>
    <tr><th>send fail</th><td>${m.send_fail}</td></tr>` : `<tr><td class="empty">Waiting for master line</td></tr>`;

  document.getElementById("rawLog").innerHTML = (s.raw || []).slice(-22).map(r => `<div>${esc(r.line)}</div>`).join("");
}
function sendCommand(cmd, label) {
  fetch("/api/cmd", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({cmd, label: label || cmd})
  }).then(async res => {
    const data = await res.json();
    document.getElementById("commandStatus").textContent =
      data.ok ? `Sent ${data.cmd}` : `Command failed: ${data.error}`;
  }).catch(err => {
    document.getElementById("commandStatus").textContent = `Command failed: ${err}`;
  });
}
document.querySelectorAll("button[data-cmd]").forEach(btn => {
  btn.addEventListener("click", () => sendCommand(btn.dataset.cmd, btn.textContent.trim()));
});
document.getElementById("maintainBtn").addEventListener("click", () => {
  const raw = document.getElementById("maintainInput").value.trim();
  const v = Number(raw);
  if (!Number.isFinite(v) || v < 4.0 || v > 16.8) {
    document.getElementById("commandStatus").textContent = "Enter 4.0 to 16.8 V";
    return;
  }
  sendCommand(`m${Math.round(v * 10)}`, `Set ${v.toFixed(1)} V`);
});
document.getElementById("tempToggle").addEventListener("click", () => {
  tempUnit = tempUnit === "F" ? "C" : "F";
  localStorage.setItem("netBenchTempUnit", tempUnit);
  if (state) render(state);
});
const es = new EventSource("/events");
es.addEventListener("snapshot", ev => render(JSON.parse(ev.data)));
es.onerror = () => {
  document.getElementById("serialPill").textContent = "dashboard reconnecting";
  document.getElementById("serialPill").className = "pill warn";
};
fetch("/api/state").then(r => r.json()).then(render);
</script>
</body>
</html>
"""


def parse_body(handler: BaseHTTPRequestHandler) -> dict[str, Any]:
    length = int(handler.headers.get("Content-Length", "0"))
    if length <= 0:
        return {}
    raw = handler.rfile.read(length).decode("utf-8", "replace")
    return json.loads(raw)


def valid_command(cmd: str) -> bool:
    if cmd in {"r", "U", "c", "I", "i", "+", "-"}:
        return True
    m = re.fullmatch(r"m(\d{2,3})", cmd)
    if not m:
        return False
    value = int(m.group(1))
    return 40 <= value <= 168


def make_handler(state: DashboardState, worker: SerialWorker):
    class Handler(BaseHTTPRequestHandler):
        server_version = "NetBenchDashboard/1.0"

        def log_message(self, fmt: str, *args: Any) -> None:
            return

        def send_json(self, status: int, payload: dict[str, Any]) -> None:
            data = json.dumps(payload, separators=(",", ":")).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self) -> None:
            path = urllib.parse.urlparse(self.path).path
            if path == "/":
                data = HTML.encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(data)))
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(data)
                return
            if path == "/api/state":
                self.send_json(200, state.snapshot())
                return
            if path == "/events":
                self.send_response(200)
                self.send_header("Content-Type", "text/event-stream")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Connection", "keep-alive")
                self.end_headers()
                try:
                    while True:
                        payload = json.dumps(state.snapshot(), separators=(",", ":"))
                        self.wfile.write(f"event: snapshot\ndata: {payload}\n\n".encode("utf-8"))
                        self.wfile.flush()
                        time.sleep(1.0)
                except (BrokenPipeError, ConnectionResetError, TimeoutError):
                    return
            self.send_error(404)

        def do_POST(self) -> None:
            path = urllib.parse.urlparse(self.path).path
            if path != "/api/cmd":
                self.send_error(404)
                return
            try:
                body = parse_body(self)
                cmd = str(body.get("cmd", ""))
                label = str(body.get("label", cmd))
                if not valid_command(cmd):
                    self.send_json(400, {"ok": False, "error": "unsupported command"})
                    return
                worker.send_command(cmd, label)
                self.send_json(200, {"ok": True, "cmd": cmd})
            except Exception as exc:
                self.send_json(500, {"ok": False, "error": str(exc)})

    return Handler


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM7", help="serial-bridge master USB port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--bind", default="127.0.0.1")
    ap.add_argument("--http-port", type=int, default=8765)
    ap.add_argument("--udp-host", default="255.255.255.255", help="set empty to disable UDP forwarding")
    ap.add_argument("--udp-port", type=int, default=54321)
    args = ap.parse_args()

    state = DashboardState()
    udp_host = args.udp_host or None
    worker = SerialWorker(state, args.port, args.baud, udp_host, args.udp_port)
    worker.start()

    server = ThreadingHTTPServer((args.bind, args.http_port), make_handler(state, worker))
    url = f"http://{args.bind}:{args.http_port}/"
    print(f"net_bench_dashboard: {args.port}@{args.baud} -> {url}", flush=True)
    if udp_host:
        print(f"udp forwarding: {udp_host}:{args.udp_port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nbye", flush=True)
    finally:
        worker.stop_event.set()
        server.server_close()


if __name__ == "__main__":
    main()
