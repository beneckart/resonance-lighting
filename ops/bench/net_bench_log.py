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
New outputs are exclusive-create by default: an existing trace is never truncated
unless --overwrite is explicit. Use --append to continue an existing run with a
machine-readable segment boundary after a host/logger outage. Stdlib only.

Examples:
  ./net_bench_log.py --site ca --operator ben --battery liion-4400 \\
      --topology master-multicast --tx-rate 10 --notes "tree-scale 1-6m" --duration 7200
  ./net_bench_log.py --site ca --master-ip 192.168.4.50   # filter one master
"""
import argparse, json, os, re, socket, sys, time
from datetime import datetime, timezone

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
UDP_RECV_BYTES = 65535
RUN_IDENTITY_FIELDS = (
    "run_id", "site", "operator", "battery", "topology", "tx_rate_hz", "notes"
)


def first_jsonl_row(path):
    with open(path, "r", encoding="utf-8") as fh:
        for line_number, line in enumerate(fh, 1):
            if not line.strip():
                continue
            try:
                return json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(
                    f"first non-empty line {line_number} is not valid JSON: {exc}"
                ) from exc
    raise ValueError("file is empty")


def last_jsonl_row(path):
    """Read the last non-empty JSONL row without scanning a potentially huge trace."""
    with open(path, "rb") as fh:
        end = fh.seek(0, os.SEEK_END)
        pos = end
        partial = b""
        while pos:
            size = min(8192, pos)
            pos -= size
            fh.seek(pos)
            block = fh.read(size) + partial
            lines = block.splitlines()
            if pos and lines:
                partial = lines.pop(0)
            else:
                partial = b""
            for raw in reversed(lines):
                if not raw.strip():
                    continue
                try:
                    return json.loads(raw.decode("utf-8"))
                except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                    raise ValueError(f"last non-empty line is not valid JSON: {exc}") from exc
        if partial.strip():
            try:
                return json.loads(partial.decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                raise ValueError(f"last non-empty line is not valid JSON: {exc}") from exc
    raise ValueError("file is empty")

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
output_mode = ap.add_mutually_exclusive_group()
output_mode.add_argument(
    "--append", action="store_true",
    help="continue an existing non-empty JSONL run; preserve its identity and add a segment boundary")
output_mode.add_argument(
    "--overwrite", action="store_true",
    help="deliberately replace an existing output (destructive)")
ap.add_argument(
    "--segment-notes", default="",
    help="outage/restart context recorded on the start/resume segment boundary")
a = ap.parse_args()

now0 = datetime.now(timezone.utc)
run_id = a.run_id or "-".join(
    [now0.strftime("%Y-%m-%d"), a.site, a.battery, "net", a.topology,
     (f"{a.tx_rate}hz" if a.tx_rate else "rNA"), now0.strftime("%H%M")])
out = a.out or os.path.join(DATA_DIR, a.site, run_id + ".jsonl")
out = os.path.abspath(out)
os.makedirs(os.path.dirname(out), exist_ok=True)

requested_meta = dict(run_id=run_id, site=a.site, operator=a.operator,
                      battery=a.battery, topology=a.topology,
                      tx_rate_hz=a.tx_rate, notes=a.notes)
segment_started_utc = datetime.now(timezone.utc).isoformat()
previous_ts_utc = None
needs_leading_newline = False

if a.append:
    identity_args = ("--site", "--operator", "--battery", "--topology", "--tx-rate",
                     "--notes", "--run-id")
    used_identity_args = [flag for flag in identity_args if flag in sys.argv[1:]]
    if used_identity_args:
        ap.error("--append preserves metadata from the existing run; omit " +
                 ", ".join(used_identity_args))
    if not os.path.isfile(out):
        ap.error(f"--append requires an existing regular file: {out}")
    try:
        first_row = first_jsonl_row(out)
        last_row = last_jsonl_row(out)
    except ValueError as exc:
        ap.error(f"cannot append safely to {out}: {exc}")
    missing = [field for field in RUN_IDENTITY_FIELDS if field not in first_row]
    if missing:
        ap.error(f"cannot append safely to {out}: first row lacks run identity fields " +
                 ", ".join(missing))
    meta = {field: first_row[field] for field in RUN_IDENTITY_FIELDS}
    segment_index = int(last_row.get("segment_index", 1)) + 1
    previous_ts_utc = last_row.get("ts_utc")
    file_mode = "a"
    segment_event = "resume"
    with open(out, "rb") as existing:
        existing.seek(-1, os.SEEK_END)
        needs_leading_newline = existing.read(1) not in (b"\n", b"\r")
else:
    if os.path.exists(out) and not a.overwrite:
        ap.error(f"output already exists: {out}; use --append to resume it, choose a new "
                 "--out/--run-id, or use destructive --overwrite")
    meta = requested_meta
    segment_index = 1
    file_mode = "w" if a.overwrite else "x"
    segment_event = "overwrite" if a.overwrite and os.path.exists(out) else "start"

meta = dict(meta, segment_index=segment_index,
            segment_started_utc=segment_started_utc)

rx_master = re.compile(
    r"nb-master id=(\w+) ch=(\d+) frames=(\d+) sendok=(\d+) sendfail=(\d+) up=(\d+) bv=([\d.]+)"
    r"(?: fw=(\S+))?"
    r"(?: act=(\d+) actv=(\d+) actseq=(\d+) actup=(\d+) actutc=(\d+) actf=([0-9A-Fa-f]{2})"
    r" acttgt=([0-9A-Fa-f]{6}) actn=(\d+))?")
rx_peer = re.compile(
    r"nb-peer id=(\w+) seq=(\d+) rx=(\d+) gaps=(\d+) pdr=([\d.]+) rssi=(-?\d+) bv=([\d.-]+) "
    r"ima=(-?\d+) soc=(-?\d+) rr=(\w+) ca=(\d+) mode=(\d+) dlpdr=([\d.]+) dlrssi=(-?\d+) up=(\d+) age=(\d+)"
    r"(?: sv=([\d.-]+) sma=(-?\d+) sgood=(\d+))?"   # supply (panel) side; optional (pre-.7 peers omit it)
    r"(?: lux=([\w.\-]+) ch0=(\d+) ch1=(\d+) ptc=([\w.\-]+) prh=(-?\d+) btc=([\w.\-]+))?"   # env sensors (2026-06-10.1+); lux: number|sat|nan
    r"(?: ipv=(-?\d+) ipa=(-?\d+) ibv=(-?\d+) iba=(-?\d+))?"  # onboard INA meters (2026-06-11.2+); -32768 = absent
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
    r"(?: sens=(\d+) cmis=(\d+) rec=(\d+) recmv=(\d+))?"
    r"(?: audf=(\d+) slpr=(\d+) slps=(\d+) slpmv=(-?\d+) slpprof=(\d+) slplife=(\d+)"
    r" slptier=(\d+) slpsrc=([0-9A-Fa-f]{6}) slpseq=(\d+) cmdslpr=(\d+) cmdslps=(\d+)"
    r" cmdslpsrc=([0-9A-Fa-f]{6}) cmdslpseq=(\d+) protmv=(-?\d+))?"
    r"(?: protorig=(\d+) protprev=(\d+) protrst=(\d+) protarm=(\d+) protstreak=(\d+))?")
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
print(f"net_bench_log -> {out}  ({meta['topology']}, {a.duration:.0f}s, "
      f"segment {segment_index} {segment_event}). reboots flagged inline.", flush=True)
with open(out, file_mode, encoding="utf-8") as fh:
    if needs_leading_newline:
        fh.write("\n")
    boundary = dict(meta, ts_utc=segment_started_utc, elapsed_s=0.0,
                    src="segment", segment_event=segment_event,
                    segment_notes=a.segment_notes)
    if previous_ts_utc is not None:
        boundary["previous_ts_utc"] = previous_ts_utc
    fh.write(json.dumps(boundary) + "\n")
    fh.flush()
    while time.time() - t0 < a.duration:
        try:
            d, addr = s.recvfrom(UDP_RECV_BYTES)
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
             cap, chg, dd, ddb, dda, fw, mt,
             fc, fcr, fcc, fce, fcchg, fcdis, fcmin, fcmax,
             bqv, bqichg, bqvreg, bq16, bq18, bq1d, bq1e, bq1f,
             bq20, bq21, bq22, bq38,
             fcwhc, fcwhd, fcpw, fcbw, fcdw, fclow, fcmchg, fcmwait,
             fcmdraw, fcmprot,
              mppts, mpptr, mpptn, mpptv, mpptbest, mpptlast, mppt46, mppt48,
              mppt50, fcdim, fclat,
              profile, life, power_tier, active_program, night_min,
              fixture_class, led_rail_on, led_r, led_g, led_b, led_w,
              led_lit_pixels, sensor_bits, class_mismatch, recovery_state,
              recovery_detect_mv, sleep_audit_flags, last_sleep_cause,
              last_sleep_s, last_sleep_batt_mv, last_sleep_profile,
              last_sleep_life_state, last_sleep_power_tier, last_sleep_source,
              last_sleep_source_seq, last_command_sleep_cause,
              last_command_sleep_s, last_command_sleep_source,
              last_command_sleep_source_seq, last_protect_batt_mv,
              last_protect_origin, last_protect_predecessor_stage,
              last_protect_reset_reason, last_protect_load_armed,
              last_protect_reset_streak) = m.groups()
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
            if fc is not None:
                row.update(field_phase=int(fc), field_reason=int(fcr),
                           field_cycle=int(fcc), field_elapsed_s=int(fce),
                           field_charge_mah=int(fcchg),
                           field_discharge_mah=int(fcdis),
                           field_min_mv=int(fcmin), field_max_mv=int(fcmax))
            if bq16 is not None:
                def u16_or_none(s):
                    v = int(s)
                    return None if v == 65535 else v
                r16, r18, s1 = int(bq16, 16), int(bq18, 16), int(bq1e, 16)
                row.update(bq_vindpm_mv=u16_or_none(bqv),
                           bq_ichg_ma=u16_or_none(bqichg),
                           bq_vreg_mv=u16_or_none(bqvreg),
                           bq_reg16=r16, bq_reg18=r18,
                           bq_stat0=int(bq1d, 16), bq_stat1=s1,
                           bq_fault0=int(bq1f, 16),
                           bq_flag0=int(bq20, 16),
                           bq_flag1=int(bq21, 16),
                           bq_fault_flag0=int(bq22, 16),
                           bq_part=int(bq38, 16),
                           bq_chg_en=bool(r16 & (1 << 5)),
                           bq_en_hiz=bool(r16 & (1 << 4)),
                           bq_batfet_ctrl=r18 & 0x03,
                           bq_vbus_stat=s1 & 0x07,
                           bq_chg_stat=(s1 >> 3) & 0x03)
            if fcwhc is not None:
                row.update(field_charge_wh=round(int(fcwhc) / 10.0, 1),
                           field_discharge_wh=round(int(fcwhd) / 10.0, 1),
                           field_peak_panel_w=round(int(fcpw) / 100.0, 2),
                           field_peak_charge_w=round(int(fcbw) / 100.0, 2),
                           field_peak_draw_w=round(int(fcdw) / 100.0, 2),
                           field_low_s=int(fclow),
                           field_charge_min=int(fcmchg),
                           field_wait_min=int(fcmwait),
                           field_draw_min=int(fcmdraw),
                           field_protect_min=int(fcmprot))
            if mppts is not None:
                row.update(mppt_status=int(mppts),
                           mppt_reason=int(mpptr),
                           mppt_runs=int(mpptn),
                           mppt_active_v=round(int(mpptv) / 10.0, 1),
                           mppt_best_v=round(int(mpptbest) / 10.0, 1),
                           mppt_last_v=round(int(mpptlast) / 10.0, 1),
                           mppt_p46_w=round(int(mppt46) / 100.0, 2),
                           mppt_p48_w=round(int(mppt48) / 100.0, 2),
                           mppt_p50_w=round(int(mppt50) / 100.0, 2))
            if fcdim is not None:
                row.update(field_load_dimmed=bool(int(fcdim)),
                            field_protect_latched=bool(int(fclat)))
            if profile is not None:
                row.update(profile=int(profile), life_state=int(life),
                           power_tier=int(power_tier),
                           active_program=int(active_program),
                           night_min=int(night_min))
            if fixture_class is not None:
                row.update(fixture_class=int(fixture_class),
                           led_rail_on=bool(int(led_rail_on)), led_r=int(led_r),
                           led_g=int(led_g), led_b=int(led_b), led_w=int(led_w),
                           led_lit_pixels=int(led_lit_pixels))
            if sensor_bits is not None:
                row.update(sensor_bits=int(sensor_bits),
                           class_mismatch=bool(int(class_mismatch)),
                           recovery_state=int(recovery_state),
                           recovery_detect_mv=(None if int(recovery_detect_mv) == 65535
                                               else int(recovery_detect_mv)))
            if sleep_audit_flags is not None:
                row.update(sleep_audit_flags=int(sleep_audit_flags),
                           last_sleep_cause=int(last_sleep_cause),
                           last_sleep_s=int(last_sleep_s),
                           last_sleep_batt_mv=int(last_sleep_batt_mv),
                           last_sleep_profile=int(last_sleep_profile),
                           last_sleep_life_state=int(last_sleep_life_state),
                           last_sleep_power_tier=int(last_sleep_power_tier),
                           last_sleep_source=last_sleep_source,
                           last_sleep_source_seq=int(last_sleep_source_seq),
                           last_command_sleep_cause=int(last_command_sleep_cause),
                           last_command_sleep_s=int(last_command_sleep_s),
                           last_command_sleep_source=last_command_sleep_source,
                           last_command_sleep_source_seq=int(last_command_sleep_source_seq),
                           last_protect_batt_mv=int(last_protect_batt_mv))
            if last_protect_origin is not None:
                row.update(
                    last_protect_origin=int(last_protect_origin),
                    last_protect_predecessor_stage=int(last_protect_predecessor_stage),
                    last_protect_reset_reason=int(last_protect_reset_reason),
                    last_protect_load_armed=bool(int(last_protect_load_armed)),
                    last_protect_reset_streak=int(last_protect_reset_streak),
                )
            fh.write(json.dumps(row) + "\n"); fh.flush(); n += 1
            if n % 50 == 0:
                extra = (f" | panel {float(sv):.2f}V*{sma}mA={float(sv)*int(sma)/1000:.2f}W "
                         f"sgood={sgood}" if sv is not None else "")
                print(f"+{el:6.0f}s  peer {pid} pdr={pdr} rssi={rssi} soc={soc} "
                      f"batt_ma={ima}{extra} reboots={reb}", flush=True)
            continue
        m = rx_master.search(text)
        if m:
            (pid, ch, frames, sok, sfail, up, bv, fw, action, action_value,
             action_seq, action_up, action_utc, action_flags, action_target,
             action_count) = m.groups()
            row = dict(meta, ts_utc=ts, elapsed_s=el, src="master", master_ip=addr[0],
                       master_id=pid, channel=int(ch), frames=int(frames),
                       send_ok=int(sok), send_fail=int(sfail), uptime_ms=int(up),
                       battery_v=float(bv))
            if fw is not None:
                row["firmware_rev"] = fw
            if action is not None:
                row.update(action=int(action), action_value=int(action_value),
                           action_mesh_seq=int(action_seq),
                           action_bridge_uptime_ms=int(action_up),
                           action_utc_s=int(action_utc),
                           action_flags=int(action_flags, 16),
                           action_target=action_target,
                           action_count=int(action_count))
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
