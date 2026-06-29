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
    r"(?: cap=(\d+) chg=(\d+))?"
    r"(?: dd=([\d.]+) ddb=(\d+) dda=(\d+))?"
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
                cap,
                chg,
                dd,
                ddb,
                dda,
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
                "config_capacity_mah": int(cap) if cap is not None else None,
                "config_charge_ma": int(chg) if chg is not None else None,
                "drawdown_mah": float(dd) if dd is not None else None,
                "drawdown_budget_mah": int(ddb) if ddb is not None else None,
                "drawdown_active": bool(int(dda)) if dda is not None else None,
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
.metric-top { display: flex; align-items: start; justify-content: space-between; gap: 8px; }
.metric-source { color: var(--muted); font-size: 12px; font-weight: 760; font-variant-numeric: tabular-nums; white-space: nowrap; }
.metric-source.good { color: var(--green); }
.metric-source.bad { color: var(--red); }
.metric-source.warn { color: var(--amber); }
.metric-source.good, .metric-source.bad, .metric-source.warn { background: transparent; border-color: transparent; }
.metric-value { font-size: clamp(24px, 3.6vw, 42px); line-height: 1.04; font-weight: 760; letter-spacing: 0; margin-top: 7px; font-variant-numeric: tabular-nums; }
.metric-unit { color: var(--muted); font-size: 15px; font-weight: 560; margin-left: 4px; }
.metric-foot { color: var(--muted); margin-top: 9px; font-size: 13px; min-height: 18px; font-variant-numeric: tabular-nums; }
.metric-value.good { color: var(--green); }
.metric-value.bad { color: var(--red); }
.metric-value.warn { color: var(--amber); }
.metric-value.muted { color: var(--muted); }
.metric-value.good, .metric-value.bad, .metric-value.warn, .metric-value.muted { background: transparent; border-color: transparent; }
.section-title { font-size: 14px; color: var(--muted); margin: 0 0 11px; font-weight: 700; text-transform: uppercase; letter-spacing: .04em; }
.peer-selector { display: flex; flex-wrap: wrap; gap: 8px; margin: -4px 0 14px; }
.peer-chip { min-height: 34px; padding: 0 11px; border-radius: 999px; background: var(--panel); color: var(--ink); font-size: 13px; display: inline-flex; align-items: center; gap: 7px; }
.peer-chip.active { background: var(--soft-blue); border-color: #9cc7e8; color: var(--blue); }
.peer-chip.bad { background: var(--soft-red); border-color: #efc2bd; color: var(--red); }
.peer-chip .chip-sub { color: var(--muted); font-weight: 600; }
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
tr.peer-row { cursor: pointer; }
tr.peer-row:hover { background: #f7faf8; }
tr.peer-row.active-row { background: #edf6fc; }
.row-main { font-weight: 760; }
.row-sub { color: var(--muted); font-size: 12px; margin-top: 2px; }
.state-list { display: flex; gap: 5px; flex-wrap: wrap; justify-content: flex-end; }
.state-tag { border: 1px solid var(--line); border-radius: 999px; padding: 3px 7px; font-size: 12px; color: var(--muted); }
.state-tag.good { color: var(--green); border-color: #bbe2c6; background: var(--soft-green); }
.state-tag.bad { color: var(--red); border-color: #efc2bd; background: var(--soft-red); }
.state-tag.warn { color: var(--amber); border-color: #f0d198; background: var(--soft-amber); }
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

  <nav class="peer-selector" id="peerSelector" aria-label="Peer focus"></nav>

  <section class="grid">
    <div class="panel span-3">
      <div class="metric-top">
        <div class="metric-label">Panel INA</div>
        <div class="metric-source" id="panelSource">--</div>
      </div>
      <div class="metric-value" id="panelW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="panelSpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="panelFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-top">
        <div class="metric-label">Charger Supply</div>
        <div class="metric-source" id="supplySource">--</div>
      </div>
      <div class="metric-value" id="supplyW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="supplySpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="supplyFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-top">
        <div class="metric-label" id="batteryLabel">Battery</div>
        <div class="metric-source" id="batterySource">--</div>
      </div>
      <div class="metric-value" id="batteryW">--<span class="metric-unit">W</span></div>
      <canvas class="spark" id="batterySpark" width="420" height="76"></canvas>
      <div class="metric-foot" id="batteryFoot">--</div>
    </div>
    <div class="panel span-3">
      <div class="metric-top">
        <div class="metric-label">Light</div>
        <div class="metric-source" id="luxSource">--</div>
      </div>
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
              <th>id</th><th>age</th><th>link</th><th>battery</th><th>supply</th><th>panel</th><th>state</th>
            </tr>
          </thead>
          <tbody id="peerRows"><tr><td colspan="7" class="empty">Waiting for peer heartbeat</td></tr></tbody>
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
        <button class="warn" data-cmd="S">Sleep 6h</button>
        <button data-cmd="c">Resume</button>
        <button data-cmd="I">Identify all</button>
        <button data-cmd="i">Identify next</button>
      </div>
      <div class="maintain">
        <input id="maintainInput" inputmode="decimal" placeholder="MPP volts, e.g. 6.8">
        <button id="maintainBtn">Set</button>
      </div>
      <div class="maintain">
        <input id="capacityInput" inputmode="numeric" placeholder="Capacity mAh, e.g. 6000">
        <button id="capacityBtn">Cap</button>
      </div>
      <div class="maintain">
        <input id="chargeInput" inputmode="numeric" placeholder="Charge mA, e.g. 1500">
        <button id="chargeBtn">Charge</button>
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
let focusedPeerId = localStorage.getItem("netBenchPeerFocus") || "all";
let activeHistoryKey = "";

function fmt(v, digits = 2) {
  if (v === null || v === undefined || Number.isNaN(Number(v))) return "--";
  return Number(v).toFixed(digits);
}
function finite(v) {
  return v !== null && v !== undefined && Number.isFinite(Number(v));
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
function sortedPeers(s) {
  return Object.values(s.peers || {}).sort((a, b) => a.id.localeCompare(b.id));
}
function hasPanel(peer) {
  return peer && finite(peer.ina_panel_mv) && finite(peer.ina_panel_ma);
}
function hasSupply(peer) {
  return peer && finite(peer.supply_v) && finite(peer.supply_ma) && finite(peer.supply_w);
}
function hasLight(peer) {
  return peer && (peer.light_sat || finite(peer.lux) ||
    (finite(peer.light_ch0) && finite(peer.light_ch1) && (Number(peer.light_ch0) > 0 || Number(peer.light_ch1) > 0)));
}
function hasEnv(peer) {
  return peer && (finite(peer.panel_temp_c) || finite(peer.panel_rh_pct) || finite(peer.batt_temp_c));
}
function panelHarvestW(peer) {
  if (!hasPanel(peer)) return null;
  if (finite(peer.ina_panel_w)) return Math.abs(Number(peer.ina_panel_w));
  return Math.abs(Number(peer.ina_panel_mv) * Number(peer.ina_panel_ma) / 1e6);
}
function peerPool(peers) {
  const fresh = peers.filter(freshPeer);
  return fresh.length ? fresh : peers;
}
function pickPeer(peers, predicate, scoreFn) {
  const candidates = peerPool(peers).filter(predicate);
  if (!candidates.length) return null;
  candidates.sort((a, b) => {
    const scoreDelta = (scoreFn ? scoreFn(b) : 0) - (scoreFn ? scoreFn(a) : 0);
    if (scoreDelta !== 0) return scoreDelta;
    return Date.parse(b.ts_utc) - Date.parse(a.ts_utc);
  });
  return candidates[0];
}
function aggregateBattery(peers) {
  const candidates = peerPool(peers).filter(p => finite(p.battery_w));
  if (!candidates.length) return null;
  const total = candidates.reduce((sum, p) => sum + Number(p.battery_w), 0);
  const charge = candidates.reduce((sum, p) => sum + Math.max(0, Number(p.battery_w)), 0);
  const draw = candidates.reduce((sum, p) => sum + Math.max(0, -Number(p.battery_w)), 0);
  return {total, charge, draw, count: candidates.length};
}
function clearHistoryIfNeeded(key) {
  if (key === activeHistoryKey) return;
  Object.keys(history).forEach(name => history[name] = []);
  activeHistoryKey = key;
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
function setSource(id, text, klass = "") {
  const el = document.getElementById(id);
  el.textContent = text;
  el.className = `metric-source ${klass}`.trim();
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
function setFocus(peerId) {
  focusedPeerId = peerId || "all";
  localStorage.setItem("netBenchPeerFocus", focusedPeerId);
  if (state) render(state);
}
function renderPeerSelector(peers, effectiveFocus) {
  const freshCount = peers.filter(freshPeer).length;
  const allClass = effectiveFocus === "all" ? "peer-chip active" : "peer-chip";
  const buttons = [
    `<button class="${allClass}" type="button" data-peer-focus="all">All <span class="chip-sub">${freshCount}/${peers.length}</span></button>`
  ];
  peers.forEach(peer => {
    const active = effectiveFocus === peer.id ? " active" : "";
    const stale = freshPeer(peer) ? "" : " bad";
    const suffix = peer.drawdown_active ? "draw" : (hasPanel(peer) ? "panel" : "node");
    buttons.push(`<button class="peer-chip${active}${stale}" type="button" data-peer-focus="${esc(peer.id)}">${esc(peer.id)} <span class="chip-sub">${suffix}</span></button>`);
  });
  document.getElementById("peerSelector").innerHTML = buttons.join("");
  document.querySelectorAll("[data-peer-focus]").forEach(btn => {
    btn.addEventListener("click", () => setFocus(btn.dataset.peerFocus));
  });
}
function peerSource(peer) {
  return peer ? peer.id : "none";
}
function metricClassForPower(value) {
  if (!finite(value)) return "muted";
  if (Number(value) > 0.02) return "good";
  if (Number(value) < -0.02) return "bad";
  return "";
}
function tagsForPeer(peer) {
  const tags = [];
  if (!freshPeer(peer)) tags.push(["stale", "warn"]);
  if (freshPeer(peer)) tags.push(["fresh", "good"]);
  if (hasPanel(peer)) tags.push(["panel", "good"]);
  if (peer.drawdown_active) tags.push(["drawdown", "warn"]);
  if (peer.supply_good === true) tags.push(["charge", "good"]);
  else if (hasSupply(peer) && !peer.drawdown_active) tags.push(["no charge", "warn"]);
  return `<div class="state-list">${tags.map(([label, klass]) => `<span class="state-tag ${klass}">${label}</span>`).join("")}</div>`;
}
function envSummary(peer) {
  const rows = [];
  if (finite(peer.panel_temp_c)) rows.push(`panel ${fmtTemp(peer.panel_temp_c)}`);
  if (finite(peer.panel_rh_pct)) rows.push(`${fmtRh(peer.panel_rh_pct)} RH`);
  if (finite(peer.batt_temp_c)) rows.push(`batt ${fmtTemp(peer.batt_temp_c)}`);
  return rows.length ? rows.map(row => `<div>${row}</div>`).join("") : "--";
}
function render(s) {
  state = s;
  const rows = sortedPeers(s);
  const selected = focusedPeerId !== "all" ? rows.find(p => p.id === focusedPeerId) : null;
  const effectiveFocus = selected ? selected.id : "all";
  const visiblePeers = selected ? [selected] : rows;
  const freshVisible = visiblePeers.filter(freshPeer);
  const panelPeer = pickPeer(visiblePeers, hasPanel, p => panelHarvestW(p) || 0);
  const supplyPeer = pickPeer(visiblePeers, hasSupply, p => (p.supply_good ? 10 : 0) + Math.abs(Number(p.supply_w || 0)));
  const envPeer = pickPeer(visiblePeers, hasEnv, () => 1);
  const lightPeer = pickPeer(visiblePeers, hasLight, p => p.light_sat ? 10 : Number(p.lux || 0));
  const batteryAgg = selected ? null : aggregateBattery(visiblePeers);
  const historyKey = effectiveFocus;
  clearHistoryIfNeeded(historyKey);
  renderPeerSelector(rows, effectiveFocus);

  document.getElementById("tempToggle").textContent = tempUnit;
  const serialPill = document.getElementById("serialPill");
  serialPill.textContent = s.serial.connected ? `${s.serial.port} open` : `${s.serial.port || "serial"} closed`;
  serialPill.className = s.serial.connected ? "pill ok" : "pill bad";

  if (rows.length) {
    const panelCount = rows.filter(hasPanel).length;
    const supplyGood = rows.filter(p => freshPeer(p) && p.supply_good).length;
    document.getElementById("subtitle").textContent = selected
      ? `Peer ${selected.id} data age ${msAge(selected.age_ms)}, ${s.serial.lines} serial lines`
      : `${rows.length} peers, ${rows.filter(freshPeer).length} fresh, ${panelCount} panel source, ${s.serial.lines} serial lines`;
    document.getElementById("peerPill").textContent = selected
      ? (freshPeer(selected) ? `peer ${selected.id}` : `stale ${msAge(selected.age_ms)}`)
      : `${freshVisible.length}/${visiblePeers.length} peers fresh`;
    document.getElementById("peerPill").className = (selected ? freshPeer(selected) : freshVisible.length > 0) ? "pill ok" : "pill bad";
    document.getElementById("chargePill").textContent = supplyGood ? `${supplyGood} charger good` : "no charger good";
    document.getElementById("chargePill").className = supplyGood ? "pill ok" : "pill warn";
  } else {
    document.getElementById("subtitle").textContent = `Listening on ${s.serial.port || "serial"}, ${s.serial.lines} serial lines`;
    document.getElementById("peerPill").textContent = "no peer";
    document.getElementById("peerPill").className = "pill warn";
    document.getElementById("chargePill").textContent = "no supply";
    document.getElementById("chargePill").className = "pill warn";
  }

  if (panelPeer) {
    const w = panelHarvestW(panelPeer);
    pushHist("panel", w);
    setSource("panelSource", peerSource(panelPeer), freshPeer(panelPeer) ? "good" : "warn");
    setMetric("panelW", fmt(w, 3), "W", w > 0.05 ? "good" : "");
    document.getElementById("panelFoot").textContent =
      `${fmt(Number(panelPeer.ina_panel_mv) / 1000, 3)} V, ${panelPeer.ina_panel_ma} mA`;
  } else {
    setSource("panelSource", "none", "warn");
    setMetric("panelW", "--", "W", "muted");
    document.getElementById("panelFoot").textContent = selected ? "No panel telemetry on selected peer" : "No panel telemetry";
  }

  if (supplyPeer) {
    const supplyW = Number(supplyPeer.supply_w);
    pushHist("supply", supplyW);
    setSource("supplySource", peerSource(supplyPeer), supplyPeer.supply_good ? "good" : "warn");
    setMetric("supplyW", fmt(supplyW, 3), "W", supplyPeer.supply_good ? "good" : "warn");
    document.getElementById("supplyFoot").textContent =
      `${fmt(supplyPeer.supply_v, 3)} V, ${supplyPeer.supply_ma} mA, good=${supplyPeer.supply_good ? 1 : 0}`;
  } else {
    setSource("supplySource", "none", "warn");
    setMetric("supplyW", "--", "W", "muted");
    document.getElementById("supplyFoot").textContent = "No charger supply telemetry";
  }

  if (selected) {
    const batteryW = finite(selected.battery_w) ? Number(selected.battery_w) : null;
    pushHist("battery", batteryW);
    setText("batteryLabel", "Battery");
    setSource("batterySource", selected.id, freshPeer(selected) ? "" : "warn");
    setMetric("batteryW", fmt(batteryW, 3), "W", metricClassForPower(batteryW));
    document.getElementById("batteryFoot").textContent =
      `${fmt(selected.battery_v, 3)} V, ${selected.battery_ma} mA, SOC ${selected.soc_pct}%`;
  } else if (batteryAgg) {
    pushHist("battery", batteryAgg.total);
    setText("batteryLabel", "Net Battery");
    setSource("batterySource", `${batteryAgg.count} peers`);
    setMetric("batteryW", fmt(batteryAgg.total, 3), "W", metricClassForPower(batteryAgg.total));
    document.getElementById("batteryFoot").textContent =
      `charge ${fmt(batteryAgg.charge, 3)} W, draw ${fmt(batteryAgg.draw, 3)} W`;
  } else {
    setText("batteryLabel", "Battery");
    setSource("batterySource", "none", "warn");
    setMetric("batteryW", "--", "W", "muted");
    document.getElementById("batteryFoot").textContent = "No battery telemetry";
  }

  if (lightPeer) {
    const lux = lightPeer.light_sat ? null : lightPeer.lux;
    pushHist("lux", lux);
    setSource("luxSource", peerSource(lightPeer), lightPeer.light_sat ? "warn" : "");
    setMetric("lux", lightPeer.light_sat ? "sat" : fmt(lux, 1), "lux");
    document.getElementById("luxFoot").textContent =
      `ch0 ${lightPeer.light_ch0 ?? "--"}, ch1 ${lightPeer.light_ch1 ?? "--"}`;
  } else {
    setSource("luxSource", "none", "warn");
    setMetric("lux", "--", "lux", "muted");
    document.getElementById("luxFoot").textContent = "No light telemetry";
  }

  if (envPeer) {
    setText("panelTemp", fmtTemp(envPeer.panel_temp_c));
    setText("panelRh", fmtRh(envPeer.panel_rh_pct));
    setText("battTemp", fmtTemp(envPeer.batt_temp_c));
    setText("envFoot", `Source ${envPeer.id}, updated ${msAge(envPeer.age_ms)} ago`);
  } else {
    setText("panelTemp", "--");
    setText("panelRh", "--");
    setText("battTemp", "--");
    setText("envFoot", selected ? "No temp/RH data on selected peer" : "No temp/RH sensor data");
  }

  drawSpark("panelSpark", history.panel, "#14853f");
  drawSpark("supplySpark", history.supply, "#1769aa");
  drawSpark("batterySpark", history.battery, "#bd3030");
  drawSpark("luxSpark", history.lux, "#b46b00");

  document.getElementById("peerRows").innerHTML = rows.length ? rows.map(p => {
    const pct = Math.max(0, Math.min(100, (p.rssi_dbm + 90) / 65 * 100));
    const panelW = panelHarvestW(p);
    const panelCell = hasPanel(p)
      ? `<div>${fmt(panelW, 3)} W</div><div class="row-sub">${fmt(Number(p.ina_panel_mv) / 1000, 3)} V / ${p.ina_panel_ma} mA</div>`
      : "--";
    const supplyCell = hasSupply(p)
      ? `<div>${fmt(p.supply_w, 3)} W</div><div class="row-sub">${fmt(p.supply_v, 3)} V / ${p.supply_ma} mA</div>`
      : "--";
    const cfgLine = p.config_capacity_mah
      ? `<div class="row-sub">${p.config_capacity_mah} mAh / ${p.config_charge_ma} mA</div>`
      : "";
    const ddCell = p.drawdown_mah !== null && p.drawdown_mah !== undefined
      ? `<div class="row-sub">dd ${fmt(p.drawdown_mah, 1)}/${p.drawdown_budget_mah ?? "--"} mAh</div>`
      : "";
    const active = p.id === effectiveFocus ? " active-row" : "";
    return `<tr class="peer-row${active}" data-peer-id="${esc(p.id)}">
      <td><div class="row-main">${esc(p.id)}</div>${cfgLine}${ddCell}</td>
      <td>${msAge(p.age_ms)}</td>
      <td><div>${p.rssi_dbm} dBm</div><div class="signal"><span style="width:${pct}%"></span></div><div class="row-sub">${fmt(p.pdr * 100, 1)}% PDR</div></td>
      <td><div>${fmt(p.battery_w, 3)} W</div><div class="row-sub">${fmt(p.battery_v, 3)} V / ${p.battery_ma} mA / ${p.soc_pct}%</div></td>
      <td>${supplyCell}</td>
      <td>${panelCell}</td>
      <td>${tagsForPeer(p)}</td>
    </tr>`;
  }).join("") : `<tr><td colspan="7" class="empty">Waiting for peer heartbeat</td></tr>`;
  document.querySelectorAll("[data-peer-id]").forEach(row => {
    row.addEventListener("click", () => setFocus(row.dataset.peerId));
  });

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
document.getElementById("capacityBtn").addEventListener("click", () => {
  const raw = document.getElementById("capacityInput").value.trim();
  const mah = Number(raw);
  if (!Number.isInteger(mah) || mah < 100 || mah > 30000) {
    document.getElementById("commandStatus").textContent = "Enter 100 to 30000 mAh";
    return;
  }
  sendCommand(`C${mah}`, `Set capacity ${mah} mAh`);
});
document.getElementById("chargeBtn").addEventListener("click", () => {
  const raw = document.getElementById("chargeInput").value.trim();
  const ma = Number(raw);
  if (!Number.isInteger(ma) || ma < 40 || ma > 2000) {
    document.getElementById("commandStatus").textContent = "Enter 40 to 2000 mA";
    return;
  }
  sendCommand(`G${ma}`, `Set charge ${ma} mA`);
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
    if cmd in {"r", "U", "S", "c", "I", "i", "+", "-"}:
        return True
    m = re.fullmatch(r"S(\d{1,5})", cmd)
    if m:
        value = int(m.group(1))
        return 1 <= value <= 65535
    m = re.fullmatch(r"m(\d{2,3})", cmd)
    if m:
        value = int(m.group(1))
        return 40 <= value <= 168
    m = re.fullmatch(r"C(\d{3,5})", cmd)
    if m:
        value = int(m.group(1))
        return 100 <= value <= 30000
    m = re.fullmatch(r"G(\d{2,4})", cmd)
    if m:
        value = int(m.group(1))
        return 40 <= value <= 2000
    m = re.fullmatch(r"D(?:[0-9A-Fa-f]{6})?(?::\d{1,5})?", cmd)
    if m:
        if cmd == "D":
            return True
        if ":" in cmd:
            value = int(cmd.split(":", 1)[1])
            return 0 <= value <= 30000
        return True
    return False


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
