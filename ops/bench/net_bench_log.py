#!/usr/bin/env python3
"""Listen for net_bench master-bridge UDP stats and log them to JSONL.

The net_bench MASTER (WiFi-STA on the bench AP) broadcasts to :54321 two line
types every ~1 s, bridging the fully-wireless ESP-NOW peer fleet to one tether:

  nb-master id=AABBCC ch=6 frames=N sendok=N sendfail=N up=N bv=F
  nb-peer   id=AABBCC seq=N rx=N gaps=M pdr=F rssi=D bv=F ima=D soc=D rr=NAME \
            ca=D mode=D dlpdr=F dlrssi=D up=N age=N

  pdr     = uplink packet-delivery-ratio (peer->master) from seq gaps
  dlpdr   = downlink PDR (master multicast as the peer sees it)
  rssi    = peer's heartbeat RSSI at the master; dlrssi = master RSSI at the peer

Writes site-partitioned JSONL to ops/bench/data/<site>/<run-id>.jsonl, schema-
compatible with the rest of the bench. Reboots flagged inline (uptime drop).
Stdlib only.

Examples:
  ./net_bench_log.py --site ca --operator ben --battery liion-4400 \\
      --topology master-multicast --tx-rate 10 --notes "tree-scale 1-6m" --duration 7200
  ./net_bench_log.py --site ca --master-ip 192.168.4.50   # filter one master
"""
import argparse, json, os, re, socket, time
from datetime import datetime, timezone

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

ap = argparse.ArgumentParser()
ap.add_argument("--site", default="ca")
ap.add_argument("--operator", default="ben")
ap.add_argument("--battery", default="liion-4400", help="ASTERISK: Li-ion now; re-verify on LFP")
ap.add_argument("--topology", default="master-multicast", help="master-multicast | peer-mesh")
ap.add_argument("--tx-rate", type=int, default=None, help="per-node send rate Hz (for the run-id/notes)")
ap.add_argument("--notes", default="")
ap.add_argument("--run-id", default=None)
ap.add_argument("--master-ip", default=None, help="only log packets from this master IP")
ap.add_argument("--port", type=int, default=54321)
ap.add_argument("--duration", type=float, default=3600)
ap.add_argument("--out", default=None, help="explicit output path (overrides site/run-id)")
a = ap.parse_args()

now0 = datetime.now(timezone.utc)
run_id = a.run_id or "-".join(
    [now0.strftime("%Y-%m-%d"), a.site, a.battery, "net", a.topology,
     (f"{a.tx_rate}hz" if a.tx_rate else "rNA"), now0.strftime("%H%M")])
out = a.out or os.path.join(DATA_DIR, a.site, run_id + ".jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)

rx_master = re.compile(
    r"nb-master id=(\w+) ch=(\d+) frames=(\d+) sendok=(\d+) sendfail=(\d+) up=(\d+) bv=([\d.]+)")
rx_peer = re.compile(
    r"nb-peer id=(\w+) seq=(\d+) rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) bv=([\d.-]+) "
    r"ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=(\d+) mode=(\d+) dlpdr=([\d.]+) dlrssi=(-?\d+) up=(\d+) age=(\d+)"
    r"(?: sv=([\d.-]+) sma=(-?\d+) sgood=(\d+))?"   # supply (panel) side; optional (pre-.7 peers omit it)
    r"(?: lux=([\w.\-]+) ch0=(\d+) ch1=(\d+) ptc=([\w.\-]+) prh=(-?\d+) btc=([\w.\-]+))?"   # env sensors (2026-06-10.1+); lux: number|sat|nan
    r"(?: ipv=(-?\d+) ipa=(-?\d+) ibv=(-?\d+) iba=(-?\d+))?"  # onboard INA meters (2026-06-11.2+); -32768 = absent
    r"(?: cap=(\d+) chg=(\d+))?"
    r"(?: dd=([\d.]+) ddb=(\d+) dda=(\d+))?"
    r"(?: fw=(\S+))?"
    r"(?: mt=(\d+))?")
# Field 2.4 GHz coverage scan (relayed over ESP-NOW by a -DNB_SCAN_REPORT peer).
# ssid is LAST because it may contain spaces.
rx_scanap = re.compile(
    r"nb-scanap from=(\w+) scan=(\d+) idx=(\d+) count=(\d+) bssid=([0-9a-fA-F:]+) "
    r"ap_rssi=(-?\d+) ch=(\d+) enc=(\d+) linkrssi=(-?\d+) ssid=(.*)")

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", a.port)); s.settimeout(1.0)

t0 = time.time(); n = 0; reb = 0
last_up = {}  # peer id -> last uptime, for reboot detection
meta = dict(run_id=run_id, site=a.site, operator=a.operator, battery=a.battery,
            topology=a.topology, tx_rate_hz=a.tx_rate, notes=a.notes)
print(f"net_bench_log -> {out}  ({a.topology}, {a.duration:.0f}s). reboots flagged inline.", flush=True)
with open(out, "w") as fh:
    while time.time() - t0 < a.duration:
        try:
            d, addr = s.recvfrom(1024)
        except socket.timeout:
            continue
        if a.master_ip and addr[0] != a.master_ip:
            continue
        text = d.decode(errors="replace")
        ts = datetime.now(timezone.utc).isoformat()
        el = round(time.time() - t0, 1)
        m = rx_peer.search(text)
        if m:
            (pid, seq, rxc, gaps, pdr, rssi, bv, ima, soc, rr, ca, mode,
             dlpdr, dlrssi, up, age, sv, sma, sgood,
             lux, ch0, ch1, ptc, prh, btc, ipv, ipa, ibv, iba,
             cap, chg, dd, ddb, dda, fw, mt) = m.groups()
            up = int(up)
            if pid in last_up and up < last_up[pid] - 2000:
                reb += 1
                print(f"+{el:6.0f}s  REBOOT #{reb} peer {pid} up {last_up[pid]}->{up} rr={rr} bv~{bv}", flush=True)
            last_up[pid] = up
            row = dict(meta, ts_utc=ts, elapsed_s=el, src="peer", master_ip=addr[0],
                       peer_id=pid, last_seq=int(seq), rx=int(rxc), gaps=int(gaps),
                       pdr=float(pdr), rssi_dbm=int(rssi), battery_v=float(bv),
                       battery_ma=int(ima), soc_pct=int(soc), reset_reason=rr,
                       ca_state=int(ca), peer_mode=int(mode), dl_pdr=float(dlpdr),
                       dl_rssi_dbm=int(dlrssi), uptime_ms=up, age_ms=int(age))
            if sv is not None:  # supply (panel) side: V, current into board, charger-good
                supply_w = round(float(sv) * int(sma) / 1000.0, 3)  # panel harvest
                # net battery power (>0 charging) and derived system load
                batt_w = round(float(bv) * int(ima) / 1000.0, 3)
                row.update(supply_v=float(sv), supply_ma=int(sma),
                           supply_good=bool(int(sgood)), supply_w=supply_w,
                           battery_w=batt_w, load_w=round(supply_w - batt_w, 3))
            if lux is not None:  # env sensors on the peer's STEMMA bus
                def numok(s):  # "nan"/"sat" -> None (absent / saturated)
                    try:
                        v = float(s)
                        return None if v != v else v
                    except ValueError:
                        return None
                row.update(lux=numok(lux), light_sat=(lux == "sat"),
                           light_ch0=int(ch0), light_ch1=int(ch1),
                           panel_temp_c=numok(ptc),
                           panel_rh_pct=(None if int(prh) < 0 else int(prh)),
                           batt_temp_c=numok(btc))
            if ipv is not None:  # onboard INA meters; -32768 = channel absent
                def ina_ok(s):
                    v = int(s)
                    return None if v == -32768 else v
                pv, pa, bv2, ba = ina_ok(ipv), ina_ok(ipa), ina_ok(ibv), ina_ok(iba)
                row.update(ina_panel_mv=pv, ina_panel_ma=pa,
                           ina_batt_mv=bv2, ina_batt_ma=ba)
                if pv is not None and pa is not None:
                    row["ina_panel_w"] = round(pv * pa / 1e6, 3)  # ground-truth harvest
            if cap is not None:
                row.update(config_capacity_mah=int(cap), config_charge_ma=int(chg))
            if dd is not None:
                row.update(drawdown_mah=float(dd), drawdown_budget_mah=int(ddb),
                           drawdown_active=bool(int(dda)))
            if fw is not None:
                row["firmware_rev"] = fw
            if mt is not None:
                row["maint_status"] = int(mt)
            fh.write(json.dumps(row) + "\n"); fh.flush(); n += 1
            if n % 50 == 0:
                extra = (f" | panel {float(sv):.2f}V*{sma}mA={float(sv)*int(sma)/1000:.2f}W "
                         f"sgood={sgood}" if sv is not None else "")
                print(f"+{el:6.0f}s  peer {pid} pdr={pdr} rssi={rssi} soc={soc} "
                      f"batt_ma={ima}{extra} reboots={reb}", flush=True)
            continue
        m = rx_master.search(text)
        if m:
            pid, ch, frames, sok, sfail, up, bv = m.groups()
            row = dict(meta, ts_utc=ts, elapsed_s=el, src="master", master_ip=addr[0],
                       master_id=pid, channel=int(ch), frames=int(frames),
                       send_ok=int(sok), send_fail=int(sfail), uptime_ms=int(up),
                       battery_v=float(bv))
            fh.write(json.dumps(row) + "\n"); fh.flush(); n += 1
            continue
        m = rx_scanap.search(text)
        if m:
            (frm, scan, idx, cnt, bssid, ap_rssi, ch, enc, linkrssi, ssid) = m.groups()
            row = dict(meta, ts_utc=ts, elapsed_s=el, src="scanap", master_ip=addr[0],
                       field_id=frm, scan_id=int(scan), idx=int(idx), ap_count=int(cnt),
                       bssid=bssid, ap_rssi_dbm=int(ap_rssi), ap_channel=int(ch),
                       enc=int(enc), link_rssi_dbm=int(linkrssi), ssid=ssid.rstrip())
            fh.write(json.dumps(row) + "\n"); fh.flush(); n += 1
            if int(idx) == 0:  # log the strongest AP of each batch as a heartbeat of progress
                print(f"+{el:6.0f}s  scan#{scan} from {frm}: best {ssid.rstrip()} "
                      f"{ap_rssi}dBm ch{ch} ({cnt} APs, link {linkrssi}dBm)", flush=True)
s.close()
print(f"=== DONE rows={n} reboots={reb} -> {out} ===", flush=True)
