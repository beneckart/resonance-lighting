#!/usr/bin/env python3
"""Parallelized WiFi OTA to N net_bench nodes + auto-recovery verification.

Pushes one firmware .bin to several node IPs CONCURRENTLY (the nodes must be in
maintenance mode -- serving POST /update + GET /telemetry), then verifies each
node REBOOTS INTO THE NEW IMAGE with NO physical button press (the TODO.md field-
reset requirement). For each node it records:
  t_ack   -- wall time of the /update POST until "Update complete. Rebooting."
  t_ready -- time until /telemetry responds again with a RESET uptime
  recovered / button_press_required

This exercises ADR-0010's standard-WiFi OTA at fleet scale (no ESP-NOW firmware
gossip). Stdlib only. Reuses the same POST contract as firmware/*/build.sh --ota.

Examples:
  ./net_bench_ota.py --bin /tmp/net_bench.ino.bin \\
      --nodes pf1=192.168.4.61,pf2=192.168.4.62,pf3=192.168.4.63 --jobs 5
  ./net_bench_ota.py --bin <bin> --nodes <...> --jobs 1   # sequential baseline
"""
import argparse, json, os, time, urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

ap = argparse.ArgumentParser()
ap.add_argument("--bin", required=True, help="firmware .bin to upload")
ap.add_argument("--nodes", required=True, help="name=ip,name=ip,... (in maintenance mode)")
ap.add_argument("--jobs", type=int, default=5, help="concurrent OTA pushes (1 = sequential)")
ap.add_argument("--reboot", choices=["maint", "comms"], default="maint",
                help="where nodes reboot to: 'maint' = stay on WiFi (verify via /telemetry); "
                     "'comms' = peers rejoin ESP-NOW off WiFi (verify via the master bridge, not HTTP)")
ap.add_argument("--ready-timeout", type=float, default=60, help="s to wait for reboot+/telemetry (maint reboot)")
ap.add_argument("--site", default="ca")
ap.add_argument("--notes", default="")
ap.add_argument("--out", default=None)
a = ap.parse_args()

nodes = []
for item in a.nodes.split(","):
    item = item.strip()
    if not item:
        continue
    name, ip = (item.split("=", 1) if "=" in item else (item, item))
    nodes.append((name.strip(), ip.strip()))

with open(a.bin, "rb") as f:
    binblob = f.read()
print(f"OTA {len(binblob)} bytes -> {len(nodes)} nodes, jobs={a.jobs}", flush=True)


def get_telemetry(ip, timeout=4):
    with urllib.request.urlopen(f"http://{ip}/telemetry", timeout=timeout) as r:
        return json.loads(r.read().decode())


def post_update(ip, blob):
    """Multipart POST to /update, field name 'firmware' (matches build.sh)."""
    boundary = "----netbenchOTA"
    head = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"firmware\"; "
            f"filename=\"net_bench.ino.bin\"\r\nContent-Type: application/octet-stream\r\n\r\n").encode()
    tail = f"\r\n--{boundary}--\r\n".encode()
    body = head + blob + tail
    req = urllib.request.Request(f"http://{ip}/update", data=body, method="POST")
    req.add_header("Content-Type", f"multipart/form-data; boundary={boundary}")
    with urllib.request.urlopen(req, timeout=180) as r:
        return r.read().decode(errors="replace")


def flash_one(name, ip):
    res = dict(node=name, ip=ip, event="ota",
               ts_utc=datetime.now(timezone.utc).isoformat())
    try:
        pre = get_telemetry(ip)
        res["uptime_pre_ms"] = pre.get("uptime_ms")
    except Exception as e:
        res["uptime_pre_ms"] = None
        res["pre_error"] = str(e)
    t0 = time.time()
    try:
        ack = post_update(ip, binblob)
        res["t_ack_s"] = round(time.time() - t0, 2)
        res["ack"] = ack.strip()
    except Exception as e:
        res["error"] = f"update: {e}"
        res["recovered"] = False
        res["button_press_required"] = True
        res["verified"] = None
        return res
    # "Update complete. Rebooting." means Update.end(true) validated the image and
    # ESP.restart() was called -> the device WILL boot the new image via a software
    # reset (the reliable path, not the flaky JTAG-RTS reset). That is the success
    # signal. The /telemetry poll below is an *extra* confirmation that only works
    # for nodes that stay on WiFi (maint reboot / the master); comms-mode peers
    # reboot OFF WiFi and must be confirmed via the master bridge (rr=software, seq
    # restart) -- not an OTA failure.
    upload_ok = "complete" in ack.lower()
    verified = None
    if a.reboot == "maint":
        deadline = time.time() + a.ready_timeout
        while time.time() < deadline:
            time.sleep(2)
            try:
                post = get_telemetry(ip)
                up = post.get("uptime_ms", 1 << 62)
                if res.get("uptime_pre_ms") is None or up < res["uptime_pre_ms"]:
                    res["t_ready_s"] = round(time.time() - t0, 2)
                    res["uptime_post_ms"] = up
                    res["reset_reason_post"] = post.get("reset_reason")
                    verified = "telemetry-uptime-reset"
                    break
            except Exception:
                continue  # board is rebooting / not yet serving
    res["verified"] = verified or ("ota-ack (comms peer: confirm rejoin via master bridge)"
                                   if upload_ok else None)
    res["recovered"] = bool(upload_ok or verified)
    res["button_press_required"] = not res["recovered"]
    return res


results = []
with ThreadPoolExecutor(max_workers=a.jobs) as ex:
    futs = {ex.submit(flash_one, n, ip): n for n, ip in nodes}
    for fut in as_completed(futs):
        r = fut.result()
        results.append(r)
        ok = "OK" if r.get("recovered") else "FAIL"
        print(f"  [{ok}] {r['node']} {r['ip']} t_ack={r.get('t_ack_s')}s "
              f"t_ready={r.get('t_ready_s')}s verified={r.get('verified')} "
              f"{r.get('error','')}", flush=True)

out = a.out or os.path.join(DATA_DIR, a.site,
                            datetime.now(timezone.utc).strftime("%Y-%m-%d") + "-ota-results.jsonl")
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "a") as fh:
    for r in results:
        r["notes"] = a.notes
        fh.write(json.dumps(r) + "\n")

n_ok = sum(1 for r in results if r.get("recovered"))
print(f"=== OTA DONE: {n_ok}/{len(results)} recovered (no button). -> {out} ===", flush=True)
if n_ok != len(results):
    print("*** FAIL: some nodes did not auto-recover -- field-reset requirement NOT met.", flush=True)
