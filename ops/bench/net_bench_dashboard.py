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
    r"(?: fw=(\S+))?"
)
RX_PEER = re.compile(
    r"nb-peer id=(\w+) seq=(\d+) rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) bv=([\d.-]+) "
    r"ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=(\d+) mode=(\d+) dlpdr=([\d.]+) dlrssi=(-?\d+) up=(\d+) age=(\d+)"
    r"(?: sv=([\d.-]+) sma=(-?\d+) sgood=(\d+))?"
    r"(?: lux=([\w.\-]+) ch0=(\d+) ch1=(\d+) ptc=([\w.\-]+) prh=(-?\d+) btc=([\w.\-]+))?"
    r"(?: ipv=(-?\d+) ipa=(-?\d+) ibv=(-?\d+) iba=(-?\d+))?"
    r"(?: cap=(\d+) chg=(\d+))?"
    r"(?: dd=([\d.]+) ddb=(\d+) dda=(\d+))?"
    r"(?: fw=(\S+))?"
    r"(?: mt=(\d+))?"
    r"(?: fc=(\d+) fcr=(\d+) fcc=(\d+) fce=(\d+) fcchg=(\d+) fcdis=(\d+) fcmin=(\d+) fcmax=(\d+))?"
    r"(?: bqv=(\d+) bqichg=(\d+) bqvreg=(\d+) bq16=([0-9A-Fa-f]{2}) bq18=([0-9A-Fa-f]{2})"
    r" bq1d=([0-9A-Fa-f]{2}) bq1e=([0-9A-Fa-f]{2}) bq1f=([0-9A-Fa-f]{2})"
    r" bq20=([0-9A-Fa-f]{2}) bq21=([0-9A-Fa-f]{2}) bq22=([0-9A-Fa-f]{2}) bq38=([0-9A-Fa-f]{2}))?"
    r"(?: fcwhc=(\d+) fcwhd=(\d+) fcpw=(\d+) fcbw=(\d+) fcdw=(\d+) fclow=(\d+)"
    r" fcmchg=(\d+) fcmwait=(\d+) fcmdraw=(\d+) fcmprot=(\d+))?"
    r"(?: mppts=(\d+) mpptr=(\d+) mpptn=(\d+) mpptv=(\d+) mpptbest=(\d+) mpptlast=(\d+)"
    r" mppt46=(\d+) mppt48=(\d+) mppt50=(\d+))?"
    r"(?: fcdim=(\d+) fclat=(\d+))?"
    r"(?: prof=(\d+) life=(\d+) ptier=(\d+) prog=(\d+) nmin=(\d+))?"
    r"(?: cls=(\d+) ledrail=(\d+) ledr=(\d+) ledg=(\d+) ledb=(\d+) ledw=(\d+) ledn=(\d+))?"
)
RX_SCANAP = re.compile(
    r"nb-scanap from=(\w+) scan=(\d+) idx=(\d+) count=(\d+) bssid=([0-9a-fA-F:]+) "
    r"ap_rssi=(-?\d+) ch=(\d+) enc=(\d+) linkrssi=(-?\d+) ssid=(.*)"
)
RX_BOOT = re.compile(r"=== Resonance (?:net-bench|fixture) (\S+) ===")


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


def maybe_u16(text: str | None) -> int | None:
    if text is None:
        return None
    value = int(text)
    return None if value == 65535 else value


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
        self.bridge_boot_fw: str | None = None

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
            pid, ch, frames, send_ok, send_fail, up, bv, fw = m.groups()
            row = {
                "id": pid,
                "channel": int(ch),
                "frames": int(frames),
                "send_ok": int(send_ok),
                "send_fail": int(send_fail),
                "uptime_ms": int(up),
                "battery_v": float(bv),
                "firmware_rev": fw or self.state.bridge_boot_fw,
                "ts_utc": ts,
            }
            with self.state.lock:
                self.state.master = row
            self.state.add_event("master", row)
            return

        m = RX_BOOT.search(line)
        if m:
            fw = m.group(1)
            with self.state.lock:
                self.state.bridge_boot_fw = fw
                if self.state.master:
                    self.state.master["firmware_rev"] = fw
            self.state.add_event("status", {"message": f"bridge firmware {fw}"})
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
                fw,
                mt,
                fc,
                fcr,
                fcc,
                fce,
                fcchg,
                fcdis,
                fcmin,
                fcmax,
                bqv,
                bqichg,
                bqvreg,
                bq16,
                bq18,
                bq1d,
                bq1e,
                bq1f,
                bq20,
                bq21,
                bq22,
                bq38,
                fcwhc,
                fcwhd,
                fcpw,
                fcbw,
                fcdw,
                fclow,
                fcmchg,
                fcmwait,
                fcmdraw,
                fcmprot,
                mppts,
                mpptr,
                mpptn,
                mpptv,
                mpptbest,
                mpptlast,
                mppt46,
                mppt48,
                mppt50,
                fcdim,
                fclat,
                profile,
                life,
                power_tier,
                active_program,
                night_min,
                fixture_class,
                led_rail_on,
                led_r,
                led_g,
                led_b,
                led_w,
                led_lit_pixels,
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
                "firmware_rev": fw,
                "maint_status": int(mt) if mt is not None else None,
                "field_phase": int(fc) if fc is not None else None,
                "field_reason": int(fcr) if fcr is not None else None,
                "field_cycle": int(fcc) if fcc is not None else None,
                "field_elapsed_s": int(fce) if fce is not None else None,
                "field_charge_mah": int(fcchg) if fcchg is not None else None,
                "field_discharge_mah": int(fcdis) if fcdis is not None else None,
                "field_min_mv": int(fcmin) if fcmin is not None else None,
                "field_max_mv": int(fcmax) if fcmax is not None else None,
                "profile": int(profile) if profile is not None else None,
                "life_state": int(life) if life is not None else None,
                "power_tier": int(power_tier) if power_tier is not None else None,
                "active_program": int(active_program) if active_program is not None else None,
                "night_min": int(night_min) if night_min is not None else None,
                "fixture_class": int(fixture_class) if fixture_class is not None else None,
                "led_rail_on": bool(int(led_rail_on)) if led_rail_on is not None else None,
                "led_r": int(led_r) if led_r is not None else None,
                "led_g": int(led_g) if led_g is not None else None,
                "led_b": int(led_b) if led_b is not None else None,
                "led_w": int(led_w) if led_w is not None else None,
                "led_lit_pixels": int(led_lit_pixels) if led_lit_pixels is not None else None,
                "ts_utc": ts,
            }
            if bq16 is not None:
                r16 = int(bq16, 16)
                r18 = int(bq18, 16)
                s1 = int(bq1e, 16)
                row.update(
                    bq_vindpm_mv=maybe_u16(bqv),
                    bq_ichg_ma=maybe_u16(bqichg),
                    bq_vreg_mv=maybe_u16(bqvreg),
                    bq_reg16=r16,
                    bq_reg18=r18,
                    bq_stat0=int(bq1d, 16),
                    bq_stat1=s1,
                    bq_fault0=int(bq1f, 16),
                    bq_flag0=int(bq20, 16),
                    bq_flag1=int(bq21, 16),
                    bq_fault_flag0=int(bq22, 16),
                    bq_part=int(bq38, 16),
                    bq_chg_en=bool(r16 & (1 << 5)),
                    bq_en_hiz=bool(r16 & (1 << 4)),
                    bq_batfet_ctrl=r18 & 0x03,
                    bq_vbus_stat=s1 & 0x07,
                    bq_chg_stat=(s1 >> 3) & 0x03,
                )
            if fcwhc is not None:
                row.update(
                    field_charge_wh=round(int(fcwhc) / 10.0, 1),
                    field_discharge_wh=round(int(fcwhd) / 10.0, 1),
                    field_peak_panel_w=round(int(fcpw) / 100.0, 2),
                    field_peak_charge_w=round(int(fcbw) / 100.0, 2),
                    field_peak_draw_w=round(int(fcdw) / 100.0, 2),
                    field_low_s=int(fclow),
                    field_charge_min=int(fcmchg),
                    field_wait_min=int(fcmwait),
                    field_draw_min=int(fcmdraw),
                    field_protect_min=int(fcmprot),
                )
            if mppts is not None:
                row.update(
                    mppt_status=int(mppts),
                    mppt_reason=int(mpptr),
                    mppt_runs=int(mpptn),
                    mppt_active_v=round(int(mpptv) / 10.0, 1),
                    mppt_best_v=round(int(mpptbest) / 10.0, 1),
                    mppt_last_v=round(int(mpptlast) / 10.0, 1),
                    mppt_p46_w=round(int(mppt46) / 100.0, 2),
                    mppt_p48_w=round(int(mppt48) / 100.0, 2),
                    mppt_p50_w=round(int(mppt50) / 100.0, 2),
                )
            if fcdim is not None:
                row.update(
                    field_load_dimmed=bool(int(fcdim)),
                    field_protect_latched=bool(int(fclat)),
                )
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
  color-scheme: dark;
  --bg: #0c1311;
  --panel: #14201c;
  --panel-raised: #192822;
  --ink: #eef5f1;
  --muted: #91a49b;
  --line: #2c4037;
  --green: #61d492;
  --red: #ff6b68;
  --amber: #f0bd62;
  --blue: #70b7f0;
  --soft-green: #183b2a;
  --soft-red: #422321;
  --soft-amber: #3b301d;
  --soft-blue: #1b3446;
  --cell-empty: #26362f;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--ink); }
main { max-width: 1440px; margin: 0 auto; padding: 22px; }
header { display: flex; align-items: end; justify-content: space-between; gap: 14px; margin-bottom: 18px; }
h1 { font-size: 24px; margin: 0; font-weight: 720; letter-spacing: -.02em; }
.sub { color: var(--muted); font-size: 13px; margin-top: 3px; }
.status { display: flex; gap: 8px; flex-wrap: wrap; justify-content: end; }
.pill { border: 1px solid var(--line); background: var(--panel); border-radius: 999px; padding: 7px 10px; font-size: 13px; line-height: 1; }
.ok { color: var(--green); background: var(--soft-green); border-color: #35684b; }
.bad { color: var(--red); background: var(--soft-red); border-color: #70403c; }
.warn { color: var(--amber); background: var(--soft-amber); border-color: #69552e; }
.fleet-overview { margin-bottom: 18px; }
.fleet-head { display: flex; align-items: end; justify-content: space-between; gap: 16px; margin-bottom: 12px; }
.fleet-title { margin: 0; font-size: 14px; color: var(--muted); font-weight: 700; text-transform: uppercase; letter-spacing: .08em; }
.fleet-counts { display: flex; gap: 14px; flex-wrap: wrap; color: var(--muted); font-size: 13px; font-variant-numeric: tabular-nums; }
.fleet-counts strong { color: var(--ink); font-weight: 720; }
.fleet-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(66px, 1fr)); gap: 8px; }
.fixture-tile { position: relative; min-width: 0; height: 82px; padding: 7px 5px 6px; border: 1px solid var(--line); border-radius: 10px; background: var(--panel); color: var(--ink); display: flex; flex-direction: column; align-items: center; justify-content: space-between; overflow: hidden; transition: border-color .14s ease, background .14s ease, opacity .14s ease, transform .14s ease; }
.fixture-tile:hover { background: var(--panel-raised); border-color: #4d6c5e; transform: translateY(-1px); }
.fixture-tile.selected { border-color: var(--blue); box-shadow: 0 0 0 1px var(--blue); }
.fixture-tile.attention { border-color: #85672f; }
.fixture-tile.critical { border-color: #99504a; background: #251918; }
.fixture-tile.late { opacity: .54; }
.fixture-tile.silent { opacity: .28; filter: grayscale(.75); }
.fixture-tile.panel-suspect::after { content: ""; position: absolute; inset: 0; border-radius: 9px; box-shadow: inset 0 0 0 1px var(--red); pointer-events: none; }
.fixture-glyph { position: relative; width: 44px; height: 55px; display: flex; justify-content: center; align-items: center; }
.battery-orb { width: 36px; height: 36px; border-radius: 50%; border: 2px solid color-mix(in srgb, var(--battery-color) 78%, var(--ink)); background: linear-gradient(to top, var(--battery-color) 0 var(--battery-fill), var(--cell-empty) var(--battery-fill) 100%); box-shadow: inset 0 0 0 3px rgba(0,0,0,.18); }
.battery-orb::after { content: ""; position: absolute; width: 10px; height: 3px; border-radius: 3px; background: color-mix(in srgb, var(--battery-color) 72%, var(--ink)); left: 17px; top: 6px; }
.power-source { position: absolute; right: -1px; top: 0; width: 18px; height: 18px; display: grid; place-items: center; z-index: 2; }
.power-source svg { width: 16px; height: 16px; stroke-width: 2; fill: none; stroke: currentColor; }
.power-source.solar { color: #ffd56e; filter: drop-shadow(0 0 4px rgba(255,213,110,.38)); }
.power-source.usb { color: var(--blue); }
.power-source.panel-missing { color: var(--red); }
.light-foot { position: absolute; left: 6px; right: 6px; bottom: 0; height: 7px; border-radius: 50% 50% 5px 5px; background: var(--light-color); box-shadow: 0 -3px 12px var(--light-color); opacity: var(--light-on); }
.fixture-id { font: 720 13px/1 ui-monospace, SFMono-Regular, Consolas, monospace; letter-spacing: .04em; }
.fleet-empty { color: var(--muted); padding: 34px 0; grid-column: 1 / -1; text-align: center; }
.selected-summary { margin-top: 10px; min-height: 42px; border-top: 1px solid var(--line); padding-top: 10px; display: flex; align-items: center; gap: 14px; flex-wrap: wrap; color: var(--muted); font-size: 13px; font-variant-numeric: tabular-nums; }
.selected-summary strong { color: var(--ink); }
.selected-summary .summary-light { width: 12px; height: 12px; border-radius: 50%; background: var(--summary-light, var(--cell-empty)); box-shadow: 0 0 9px var(--summary-light, transparent); }
.fleet-legend { display: flex; gap: 13px; flex-wrap: wrap; align-items: center; margin-top: 11px; color: var(--muted); font-size: 12px; }
.legend-item { display: inline-flex; align-items: center; gap: 6px; white-space: nowrap; }
.legend-battery { width: 13px; height: 13px; border-radius: 50%; display: inline-block; border: 1px solid currentColor; }
.legend-battery.good { color: var(--green); background: var(--green); }
.legend-battery.watch { color: var(--amber); background: linear-gradient(to top, var(--amber) 0 55%, var(--cell-empty) 55%); }
.legend-battery.critical { color: var(--red); background: linear-gradient(to top, var(--red) 0 25%, var(--cell-empty) 25%); }
.legend-source { width: 13px; height: 13px; border-radius: 50%; display: inline-block; background: #ffd56e; box-shadow: 0 0 5px rgba(255,213,110,.35); }
.legend-light { width: 16px; height: 5px; border-radius: 5px; display: inline-block; background: linear-gradient(90deg, #e25757, #6cc98c, #6ba9e7); box-shadow: 0 0 5px #6ba9e7; }
.legend-fade { width: 16px; height: 13px; border: 1px solid var(--line); border-radius: 4px; background: var(--panel); opacity: .4; }
.diagnostics { border-top: 1px solid var(--line); padding-top: 4px; }
.diagnostics > summary { cursor: pointer; color: var(--muted); font-size: 13px; font-weight: 700; padding: 12px 2px; list-style-position: outside; }
.diagnostics[open] > summary { color: var(--ink); }
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
.peer-chip.active { background: var(--soft-blue); border-color: #477596; color: var(--blue); }
.peer-chip.bad { background: var(--soft-red); border-color: #70403c; color: var(--red); }
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
tr.peer-row:hover { background: var(--panel-raised); }
tr.peer-row.active-row { background: var(--soft-blue); }
.row-main { font-weight: 760; }
.row-sub { color: var(--muted); font-size: 12px; margin-top: 2px; }
.state-list { display: flex; gap: 5px; flex-wrap: wrap; justify-content: flex-end; }
.state-tag { border: 1px solid var(--line); border-radius: 999px; padding: 3px 7px; font-size: 12px; color: var(--muted); }
.state-tag.good { color: var(--green); border-color: #35684b; background: var(--soft-green); }
.state-tag.bad { color: var(--red); border-color: #70403c; background: var(--soft-red); }
.state-tag.warn { color: var(--amber); border-color: #69552e; background: var(--soft-amber); }
.controls { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 8px; }
button, input { font: inherit; border-radius: 7px; border: 1px solid var(--line); background: var(--panel-raised); color: var(--ink); min-height: 38px; }
button { cursor: pointer; font-weight: 700; }
button:hover { border-color: #4d6c5e; background: #203129; }
button:disabled { cursor: not-allowed; opacity: .45; background: #111a17; }
button.primary { background: var(--soft-blue); border-color: #477596; color: var(--blue); }
button.warn { background: var(--soft-amber); border-color: #69552e; color: var(--amber); }
button.danger { background: var(--soft-red); border-color: #70403c; color: var(--red); }
input { padding: 0 10px; width: 100%; font-variant-numeric: tabular-nums; }
.maintain { display: grid; grid-template-columns: minmax(0, 1fr) 92px; gap: 8px; margin-top: 8px; }
.command-status { border: 1px solid var(--line); border-radius: 7px; padding: 9px 10px; margin: 0 0 10px; min-height: 38px; font-size: 13px; }
.history { height: 168px; overflow: auto; background: #08100d; color: #dbe8df; border-radius: 7px; padding: 10px; font-family: ui-monospace, SFMono-Regular, Consolas, monospace; font-size: 12px; line-height: 1.45; }
.history div { white-space: pre-wrap; overflow-wrap: anywhere; }
.signal { width: 100%; height: 8px; background: var(--cell-empty); border-radius: 999px; overflow: hidden; }
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
@media (max-width: 520px) {
  .fleet-head { align-items: start; flex-direction: column; gap: 6px; }
  .fleet-grid { grid-template-columns: repeat(auto-fill, minmax(60px, 1fr)); gap: 6px; }
  .fixture-tile { height: 78px; }
  .fleet-counts { gap: 9px; }
  .selected-summary { gap: 8px 12px; }
}
</style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>Resonance fleet</h1>
      <div class="sub" id="subtitle">Waiting for the first fixture</div>
    </div>
    <div class="status">
      <span class="pill" id="serialPill">serial</span>
      <span class="pill" id="peerPill">peer</span>
      <span class="pill" id="chargePill">supply</span>
    </div>
  </header>

  <section class="fleet-overview" aria-labelledby="fleetHeading">
    <div class="fleet-head">
      <h2 class="fleet-title" id="fleetHeading">Light health</h2>
      <div class="fleet-counts" id="fleetCounts" aria-live="polite">
        <span><strong>0</strong> seen</span>
      </div>
    </div>
    <div class="fleet-grid" id="fleetGrid">
      <div class="fleet-empty">Listening for ESP-NOW heartbeats...</div>
    </div>
    <div class="selected-summary" id="selectedSummary">
      Select a light to see its exact voltage, link age, power source, and rendered color.
    </div>
    <div class="fleet-legend" aria-label="Fleet glyph legend">
      <span class="legend-item"><i class="legend-battery good"></i> battery healthy</span>
      <span class="legend-item"><i class="legend-battery watch"></i> battery watch</span>
      <span class="legend-item"><i class="legend-battery critical"></i> battery critical</span>
      <span class="legend-item"><i class="legend-source"></i> charging input</span>
      <span class="legend-item"><i class="legend-light"></i> rendered light color</span>
      <span class="legend-item"><i class="legend-fade"></i> late heartbeat</span>
    </div>
  </section>

  <details class="diagnostics">
  <summary>Bench details and controls</summary>
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
      <div class="command-status" id="commandStatus" aria-live="polite">Select a peer for targeted controls</div>
      <div class="controls">
        <button data-cmd="r">Refresh</button>
        <button class="primary" data-cmd="m46">4.6 V</button>
        <button class="primary" data-cmd="m52">5.2 V</button>
        <button class="primary" data-cmd="m71">7.1 V</button>
        <button class="warn" id="peerMaintBtn">Peer maint</button>
        <button class="warn" data-cmd="S">Sleep 6h</button>
        <button data-cmd="c">Resume</button>
        <button data-cmd="I">Identify all</button>
        <button data-cmd="i">Identify next</button>
      </div>
      <div class="maintain">
        <input id="maintainInput" type="number" min="4.6" max="16.8" step="0.1" inputmode="decimal" placeholder="MPP volts, 4.6-16.8">
        <button id="maintainBtn">Set</button>
      </div>
      <div class="maintain">
        <input id="capacityInput" inputmode="numeric" placeholder="Selected peer capacity mAh">
        <button id="capacityBtn">Cap peer</button>
      </div>
      <div class="maintain">
        <input id="chargeInput" inputmode="numeric" placeholder="Selected peer charge mA">
        <button id="chargeBtn">Charge peer</button>
      </div>
      <div class="maintain">
        <input id="solenoidPulseInput" type="number" min="5" max="300" step="1" value="40" aria-label="Solenoid pulse milliseconds">
        <button class="danger" id="solenoidBtn">Strike D7</button>
      </div>
      <div class="controls">
        <button class="warn" data-cmd="R1">Radio 1 Hz</button>
        <button data-cmd="R2">2 Hz</button>
        <button data-cmd="R5">5 Hz</button>
        <button data-cmd="R10">10 Hz</button>
      </div>
      <div class="maintain">
        <input id="rateInput" inputmode="numeric" placeholder="Heartbeat Hz, e.g. 1">
        <button id="rateBtn">Hz</button>
      </div>
      <div class="maintain">
        <input id="napInput" inputmode="numeric" placeholder="Nap selected seconds, e.g. 3600">
        <button id="napBtn">Nap</button>
      </div>
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
  </details>
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
const FIXTURE_CLASS = {0: "unknown", 1: "downlight", 2: "perimeter", 3: "trunk", 4: "chandelier"};
const LIFE_STATE = {0: "boot", 1: "day charge", 2: "day active", 3: "night show", 4: "commission"};
const POWER_TIER = {0: "full", 1: "dim", 2: "LEDs off", 3: "protect"};
const PROGRAM = {0: "idle", 1: "CA", 2: "bridge", 3: "direct", 4: "commission fallback"};

function compactPeerIds(peers) {
  const groups = new Map();
  peers.forEach(p => {
    const suffix = p.id.slice(-2);
    if (!groups.has(suffix)) groups.set(suffix, []);
    groups.get(suffix).push(p.id);
  });
  const labels = new Map();
  groups.forEach((ids, suffix) => {
    ids.sort().forEach((id, index) => {
      labels.set(id, ids.length === 1 ? suffix : `${suffix}-${index + 1}`);
    });
  });
  return labels;
}

function compensatedBatteryV(peer) {
  if (!peer || !finite(peer.battery_v)) return null;
  const drawA = finite(peer.battery_ma) ? Math.max(0, -Number(peer.battery_ma) / 1000) : 0;
  return Number(peer.battery_v) + 0.15 * drawA;
}

function batteryVisual(peer) {
  const v = compensatedBatteryV(peer);
  if (!finite(v) || Number(v) < 0.5) return {name: "unknown", color: "#66786f", fill: 12, v: null};
  if (v >= 3.10) return {name: "healthy", color: "#61d492", fill: 100, v};
  if (v >= 3.00) return {name: "watch", color: "#f0bd62", fill: 55 + (v - 3.00) * 450, v};
  if (v >= 2.95) return {name: "low", color: "#ed8d55", fill: 30 + (v - 2.95) * 500, v};
  return {name: "critical", color: "#ff6b68", fill: Math.max(8, Math.min(30, (v - 2.70) * 88)), v};
}

function expectedHeartbeatMs(peer) {
  if (Number(peer.profile) === 0) return 1000;
  if (Number(peer.profile) === 1) {
    if (Number(peer.power_tier) === 3) return 900000;
    if (Number(peer.life_state) === 1) return 315000;
    return 5000;
  }
  return 1000;
}

function heartbeatState(peer) {
  const ageMs = Number(peer.age_ms || 0);
  const expected = expectedHeartbeatMs(peer);
  if (ageMs <= Math.max(2500, expected * 2.4)) return "live";
  if (ageMs <= Math.max(9000, expected * 3.1)) return "late";
  return "silent";
}

function panelExpected(peer) {
  return [1, 2, 3].includes(Number(peer.fixture_class));
}

function daylightConsensus(peers) {
  const solar = peers.filter(p => panelExpected(p) && heartbeatState(p) !== "silent" && finite(p.supply_v));
  if (!solar.length) return false;
  const powered = solar.filter(p => p.supply_good === true && Number(p.supply_v) >= 4.0).length;
  return powered >= Math.max(1, Math.ceil(solar.length * .35));
}

function displayedLight(peer) {
  if (peer.led_rail_on !== true || !finite(peer.led_lit_pixels) || Number(peer.led_lit_pixels) < 1)
    return {on: false, r: 0, g: 0, b: 0};
  const w = finite(peer.led_w) ? Number(peer.led_w) : 0;
  return {
    on: true,
    r: Math.min(255, Number(peer.led_r || 0) + w),
    g: Math.min(255, Number(peer.led_g || 0) + w),
    b: Math.min(255, Number(peer.led_b || 0) + w)
  };
}

function sunIcon(missing = false) {
  return `<svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="3"></circle><path d="M12 1v3M12 20v3M4.2 4.2l2.1 2.1M17.7 17.7l2.1 2.1M1 12h3M20 12h3M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1"></path>${missing ? '<path d="M4 4l16 16"></path>' : ''}</svg>`;
}

function plugIcon() {
  return `<svg viewBox="0 0 24 24" aria-hidden="true"><path d="M8 3v6M16 3v6M6 9h12v2a6 6 0 0 1-6 6v4M9 21h6"></path></svg>`;
}

function powerSource(peer, daylight) {
  const panelMissing = daylight && panelExpected(peer) && peer.supply_good !== true;
  if (panelMissing) return {kind: "panel-missing", label: "panel input missing", icon: sunIcon(true), suspect: true};
  if (peer.supply_good !== true || !finite(peer.supply_v) || Number(peer.supply_v) < 4.0)
    return {kind: "none", label: "no active input", icon: "", suspect: false};
  if (panelExpected(peer)) return {kind: "solar", label: "panel-class input active", icon: sunIcon(false), suspect: false};
  return {kind: "usb", label: "external / USB input active", icon: plugIcon(), suspect: false};
}

function fleetHealth(peer, daylight) {
  const battery = batteryVisual(peer);
  const heartbeat = heartbeatState(peer);
  const source = powerSource(peer, daylight);
  const critical = battery.name === "critical" || Number(peer.power_tier) >= 2 ||
    Number(peer.maint_status) === 3 || Number(peer.bq_fault0 || 0) !== 0;
  const attention = critical || battery.name === "low" || battery.name === "watch" ||
    source.suspect || Number(peer.maint_status) === 2;
  return {battery, heartbeat, source, critical, attention};
}

function renderFleet(peers, selectedId) {
  const labels = compactPeerIds(peers);
  const daylight = daylightConsensus(peers);
  const health = new Map(peers.map(p => [p.id, fleetHealth(p, daylight)]));
  let healthy = 0, attention = 0, silent = 0, powered = 0;
  peers.forEach(p => {
    const h = health.get(p.id);
    if (h.heartbeat === "silent") silent++;
    else if (h.attention) attention++;
    else healthy++;
    if (p.supply_good === true) powered++;
  });
  document.getElementById("fleetCounts").innerHTML = peers.length
    ? `<span><strong>${peers.length}</strong> seen</span><span><strong>${healthy}</strong> healthy</span>` +
      `<span><strong>${attention}</strong> attention</span><span><strong>${silent}</strong> silent</span>` +
      `<span><strong>${powered}</strong> powered</span>`
    : `<span><strong>0</strong> seen</span>`;
  document.getElementById("fleetGrid").innerHTML = peers.length ? peers.map(peer => {
    const h = health.get(peer.id);
    const light = displayedLight(peer);
    const classes = ["fixture-tile"];
    if (peer.id === selectedId) classes.push("selected");
    if (h.critical) classes.push("critical");
    else if (h.attention) classes.push("attention");
    if (h.heartbeat === "late") classes.push("late");
    if (h.heartbeat === "silent") classes.push("silent");
    if (h.source.suspect) classes.push("panel-suspect");
    const lightColor = light.on ? `rgb(${light.r},${light.g},${light.b})` : "transparent";
    const ageText = msAge(peer.age_ms);
    const voltageText = finite(peer.battery_v) ? `${fmt(peer.battery_v, 3)} V` : "battery unknown";
    const label = `${peer.id}, ${voltageText}, ${h.battery.name}, ${h.heartbeat}, ${h.source.label}, ` +
      `${light.on ? `light ${light.r} ${light.g} ${light.b}` : "light off"}`;
    const sourceMarkup = h.source.icon
      ? `<span class="power-source ${h.source.kind}">${h.source.icon}</span>` : "";
    return `<button type="button" class="${classes.join(" ")}" data-fleet-id="${esc(peer.id)}" aria-label="${esc(label)}" title="${esc(label)}; last heard ${ageText}">` +
      `<span class="fixture-glyph" style="--battery-color:${h.battery.color};--battery-fill:${h.battery.fill}%;--light-color:${lightColor};--light-on:${light.on ? 1 : 0}">` +
      `${sourceMarkup}<span class="battery-orb"></span><span class="light-foot"></span></span>` +
      `<span class="fixture-id">${esc(labels.get(peer.id))}</span></button>`;
  }).join("") : `<div class="fleet-empty">Listening for ESP-NOW heartbeats...</div>`;
  document.querySelectorAll("[data-fleet-id]").forEach(tile => {
    tile.addEventListener("click", () => setFocus(tile.dataset.fleetId));
  });

  const selected = peers.find(p => p.id === selectedId);
  if (!selected) {
    document.getElementById("selectedSummary").innerHTML = peers.length
      ? `Select a light for exact voltage, link age, source, and output. Colliding two-digit MAC suffixes gain -1, -2, and so on.`
      : `Select a light to see its exact voltage, link age, power source, and rendered color.`;
    return;
  }
  const h = health.get(selected.id);
  const light = displayedLight(selected);
  const lightColor = light.on ? `rgb(${light.r},${light.g},${light.b})` : "#26362f";
  const comp = compensatedBatteryV(selected);
  const cls = FIXTURE_CLASS[selected.fixture_class] || "unknown";
  const life = LIFE_STATE[selected.life_state] || "unknown state";
  const tier = POWER_TIER[selected.power_tier] || "unknown tier";
  const program = PROGRAM[selected.active_program] || "unknown program";
  document.getElementById("selectedSummary").innerHTML =
    `<span class="summary-light" style="--summary-light:${lightColor}"></span>` +
    `<strong>${esc(selected.id)}</strong><span>${esc(cls)}</span>` +
    `<span>${fmt(selected.battery_v, 3)} V${finite(comp) ? ` (${fmt(comp, 3)} V load-comp)` : ""}</span>` +
    `<span>${selected.battery_ma ?? "--"} mA</span><span>${esc(h.source.label)}</span>` +
    `<span>heard ${msAge(selected.age_ms)} ago</span><span>${esc(life)} / ${esc(tier)} / ${esc(program)}</span>` +
    `<span>${light.on ? `RGB ${light.r},${light.g},${light.b} - ${selected.led_lit_pixels} px` : "light off"}</span>`;
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
function commandTargetPeer() {
  if (!state) return null;
  const peers = sortedPeers(state);
  if (focusedPeerId === "all") {
    const fresh = peers.filter(freshPeer);
    return fresh.length === 1 ? fresh[0] : null;
  }
  return peers.find(p => p.id === focusedPeerId) || null;
}
function setCommandStatus(text, klass = "") {
  const el = document.getElementById("commandStatus");
  el.textContent = text;
  el.className = `command-status ${klass}`.trim();
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
    const suffix = fieldPhase(peer) || (peer.drawdown_active ? "draw" : (hasPanel(peer) ? "panel" : "node"));
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
const FIELD_PHASE = {
  0: "off",
  1: "boot",
  2: "charge",
  3: "wait-dark",
  4: "draw",
  5: "protect"
};
function fieldPhase(peer) {
  return FIELD_PHASE[peer.field_phase] || null;
}
function tagsForPeer(peer) {
  const tags = [];
  if (!freshPeer(peer)) tags.push(["stale", "warn"]);
  if (freshPeer(peer)) tags.push(["fresh", "good"]);
  const phase = fieldPhase(peer);
  if (phase) tags.push([phase, phase === "protect" ? "bad" : (phase === "draw" ? "warn" : "good")]);
  if (hasPanel(peer)) tags.push(["panel", "good"]);
  if (peer.drawdown_active) tags.push(["drawdown", "warn"]);
  if (peer.maint_status === 2) tags.push(["OTA power warn", "warn"]);
  else if (peer.maint_status === 3) tags.push(["OTA start failed", "bad"]);
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
  renderFleet(rows, selected ? selected.id : null);
  const commandPeer = commandTargetPeer();
  ["peerMaintBtn", "capacityBtn", "chargeBtn", "solenoidBtn", "napBtn"].forEach(id => {
    const el = document.getElementById(id);
    el.disabled = !commandPeer;
    el.title = commandPeer
      ? `Targets ${commandPeer.id}`
      : "Select exactly one fresh peer";
  });

  document.getElementById("tempToggle").textContent = tempUnit;
  const serialPill = document.getElementById("serialPill");
  serialPill.textContent = s.serial.connected ? `${s.serial.port} open` : `${s.serial.port || "serial"} closed`;
  serialPill.className = s.serial.connected ? "pill ok" : "pill bad";

  if (rows.length) {
    const panelCount = rows.filter(hasPanel).length;
    const supplyGood = rows.filter(p => freshPeer(p) && p.supply_good).length;
    const liveCount = rows.filter(p => heartbeatState(p) === "live").length;
    const silentCount = rows.filter(p => heartbeatState(p) === "silent").length;
    document.getElementById("subtitle").textContent =
      `${rows.length} lights seen on channel ${s.master?.channel ?? "--"}; ${s.serial.lines} bridge lines`;
    document.getElementById("peerPill").textContent = `${liveCount}/${rows.length} on time`;
    document.getElementById("peerPill").className = silentCount ? "pill bad" : (liveCount === rows.length ? "pill ok" : "pill warn");
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
    const fwLine = p.firmware_rev
      ? `<div class="row-sub">fw ${esc(p.firmware_rev)}</div>`
      : `<div class="row-sub">fw ?</div>`;
    const ddCell = p.drawdown_mah !== null && p.drawdown_mah !== undefined
      ? `<div class="row-sub">dd ${fmt(p.drawdown_mah, 1)}/${p.drawdown_budget_mah ?? "--"} mAh</div>`
      : "";
    const fcCell = p.field_phase !== null && p.field_phase !== undefined
      ? `<div class="row-sub">cycle ${p.field_cycle} ${esc(fieldPhase(p) || "?")} ${p.field_elapsed_s}s ` +
        `+${p.field_charge_mah} / -${p.field_discharge_mah} mAh</div>` +
        (p.field_charge_wh !== null && p.field_charge_wh !== undefined
          ? `<div class="row-sub">${fmt(p.field_charge_wh, 1)}Wh in / ${fmt(p.field_discharge_wh, 1)}Wh out ` +
            `peak ${fmt(p.field_peak_panel_w, 2)}W panel low ${p.field_low_s}s</div>`
          : "")
      : "";
    const latchCell = p.field_load_dimmed !== null && p.field_load_dimmed !== undefined
      ? `<div class="row-sub">dim ${p.field_load_dimmed ? 1 : 0} latched ${p.field_protect_latched ? 1 : 0}</div>`
      : "";
    const mpptCell = p.mppt_status !== null && p.mppt_status !== undefined
      ? `<div class="row-sub">mppt best ${fmt(p.mppt_best_v, 1)}V active ${fmt(p.mppt_active_v, 1)}V ` +
        `p46/p48/p50 ${fmt(p.mppt_p46_w, 2)}/${fmt(p.mppt_p48_w, 2)}/${fmt(p.mppt_p50_w, 2)}W ` +
        `r${p.mppt_reason} n${p.mppt_runs}</div>`
      : "";
    const active = p.id === effectiveFocus ? " active-row" : "";
    return `<tr class="peer-row${active}" data-peer-id="${esc(p.id)}">
      <td><div class="row-main">${esc(p.id)}</div>${fwLine}${cfgLine}${ddCell}${fcCell}${latchCell}${mpptCell}</td>
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
    <tr><th>firmware</th><td>${esc(m.firmware_rev || "?")}</td></tr>
    <tr><th>channel</th><td>${m.channel}</td></tr>
    <tr><th>uptime</th><td>${Math.round(m.uptime_ms / 1000)} s</td></tr>
    <tr><th>battery</th><td>${fmt(m.battery_v, 3)} V</td></tr>
    <tr><th>frames</th><td>${m.frames}</td></tr>
    <tr><th>send fail</th><td>${m.send_fail}</td></tr>` : `<tr><td class="empty">Waiting for master line</td></tr>`;

  document.getElementById("rawLog").innerHTML = (s.raw || []).slice(-22).map(r => `<div>${esc(r.line)}</div>`).join("");
}
async function sendCommand(cmd, label) {
  setCommandStatus(`Sending ${cmd}...`, "warn");
  try {
    const res = await fetch("/api/cmd", {
      method: "POST",
      headers: {"Content-Type": "application/json"},
      body: JSON.stringify({cmd, label: label || cmd})
    });
    const data = await res.json();
    if (!data.ok) throw new Error(data.error || "command rejected");
    setCommandStatus(`Sent ${data.cmd}`, "ok");
    return data;
  } catch (err) {
    setCommandStatus(`Command failed: ${err}`, "bad");
    return null;
  }
}
async function sendMaintainCommand(v10, label) {
  const sent = await sendCommand(`m${v10}`, label);
  if (!sent) return;
  setCommandStatus(`Sent m${v10}; awaiting charger telemetry...`, "warn");
  const expectedMv = v10 * 100;
  const deadline = Date.now() + 6000;
  while (Date.now() < deadline) {
    try {
      const res = await fetch("/api/state", {cache: "no-store"});
      const snapshot = await res.json();
      const peers = sortedPeers(snapshot).filter(freshPeer);
      if (peers.length && peers.every(p => p.bq_vindpm_mv === expectedMv)) {
        setCommandStatus(`Verified ${(v10 / 10).toFixed(1)} V on ${peers.length} peer${peers.length === 1 ? "" : "s"}`, "ok");
        return;
      }
    } catch (_) {}
    await new Promise(resolve => setTimeout(resolve, 400));
  }
  setCommandStatus(`Command sent, but peer telemetry did not verify ${(v10 / 10).toFixed(1)} V`, "bad");
}
document.querySelectorAll("button[data-cmd]").forEach(btn => {
  btn.addEventListener("click", () => {
    const cmd = btn.dataset.cmd;
    if (/^m\d+$/.test(cmd))
      sendMaintainCommand(Number(cmd.slice(1)), btn.textContent.trim());
    else
      sendCommand(cmd, btn.textContent.trim());
  });
});
document.getElementById("peerMaintBtn").addEventListener("click", () => {
  const peer = commandTargetPeer();
  if (!peer) {
    setCommandStatus("Select exactly one fresh peer for maintenance", "bad");
    return;
  }
  sendCommand(`U${peer.id}`, `Target ${peer.id} maintenance`);
});
document.getElementById("maintainBtn").addEventListener("click", () => {
  const raw = document.getElementById("maintainInput").value.trim();
  const v = Number(raw);
  if (!Number.isFinite(v) || v < 4.6 || v > 16.8) {
    setCommandStatus("PowerFeather SDK range is 4.6 to 16.8 V", "bad");
    return;
  }
  sendMaintainCommand(Math.round(v * 10), `Set ${v.toFixed(1)} V`);
});
document.getElementById("capacityBtn").addEventListener("click", () => {
  const peer = commandTargetPeer();
  if (!peer) {
    setCommandStatus("Select exactly one fresh peer for capacity", "bad");
    return;
  }
  const raw = document.getElementById("capacityInput").value.trim();
  const mah = Number(raw);
  if (!Number.isInteger(mah) || mah < 100 || mah > 30000) {
    setCommandStatus("Enter 100 to 30000 mAh", "bad");
    return;
  }
  sendCommand(`C${peer.id}:${mah}`, `Set ${peer.id} capacity ${mah} mAh`);
});
document.getElementById("chargeBtn").addEventListener("click", () => {
  const peer = commandTargetPeer();
  if (!peer) {
    setCommandStatus("Select exactly one fresh peer for charge", "bad");
    return;
  }
  const raw = document.getElementById("chargeInput").value.trim();
  const ma = Number(raw);
  if (!Number.isInteger(ma) || ma < 40 || ma > 2000) {
    setCommandStatus("Enter 40 to 2000 mA", "bad");
    return;
  }
  sendCommand(`G${peer.id}:${ma}`, `Set ${peer.id} charge ${ma} mA`);
});
document.getElementById("solenoidBtn").addEventListener("click", () => {
  const peer = commandTargetPeer();
  if (!peer) {
    setCommandStatus("Select exactly one fresh solenoid peer", "bad");
    return;
  }
  const raw = document.getElementById("solenoidPulseInput").value.trim();
  const ms = Number(raw);
  if (!Number.isInteger(ms) || ms < 5 || ms > 300) {
    setCommandStatus("Enter a 5 to 300 ms pulse", "bad");
    return;
  }
  sendCommand(`K${peer.id}:${ms}`, `Strike ${peer.id} D7 for ${ms} ms`);
});
document.getElementById("rateBtn").addEventListener("click", () => {
  const raw = document.getElementById("rateInput").value.trim();
  const hz = Number(raw);
  if (!Number.isInteger(hz) || hz < 1 || hz > 100) {
    setCommandStatus("Enter 1 to 100 Hz", "bad");
    return;
  }
  sendCommand(`R${hz}`, `Set radio ${hz} Hz`);
});
document.getElementById("napBtn").addEventListener("click", () => {
  const peer = commandTargetPeer();
  if (!peer) {
    setCommandStatus("Select exactly one fresh peer to nap", "bad");
    return;
  }
  const raw = document.getElementById("napInput").value.trim();
  const seconds = raw ? Number(raw) : 3600;
  if (!Number.isInteger(seconds) || seconds < 1 || seconds > 65535) {
    setCommandStatus("Enter 1 to 65535 seconds", "bad");
    return;
  }
  sendCommand(`P${peer.id}:${seconds}`, `Nap ${peer.id} ${seconds}s`);
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
    m = re.fullmatch(r"i[0-9A-Fa-f]{6}(?::(\d{1,3}))?", cmd)
    if m:
        if m.group(1) is None:
            return True
        value = int(m.group(1))
        return 1 <= value <= 255
    if re.fullmatch(r"U[0-9A-Fa-f]{6}", cmd):
        return True
    m = re.fullmatch(r"S(\d{1,5})", cmd)
    if m:
        value = int(m.group(1))
        return 1 <= value <= 65535
    m = re.fullmatch(r"m(\d{2,3})", cmd)
    if m:
        value = int(m.group(1))
        return 46 <= value <= 168
    m = re.fullmatch(r"C(?:[0-9A-Fa-f]{6}:)?(\d{3,5})", cmd)
    if m:
        value = int(m.group(1))
        return 100 <= value <= 30000
    m = re.fullmatch(r"G(?:[0-9A-Fa-f]{6}:)?(\d{2,4})", cmd)
    if m:
        value = int(m.group(1))
        return 40 <= value <= 2000
    m = re.fullmatch(r"K[0-9A-Fa-f]{6}:(\d{1,3})", cmd)
    if m:
        value = int(m.group(1))
        return 5 <= value <= 300
    m = re.fullmatch(r"R(\d{1,3})", cmd)
    if m:
        value = int(m.group(1))
        return 1 <= value <= 100
    m = re.fullmatch(r"P([0-9A-Fa-f]{6})(?::(\d{1,5}))?", cmd)
    if m:
        if m.group(2) is None:
            return True
        value = int(m.group(2))
        return 1 <= value <= 65535
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
