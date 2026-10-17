"""Flash a perimeter that only wakes briefly (ordinary field firmware, ~12 s every
2 min). Waits for the exact target's USB port to appear, runs Ben's ADR 0087
prepared USB service at once (ROM entry needs a READY ack + loads off; no raw
reset fallback), then from ROM: private backup (prefix, coredump, whole running
slot), partition-table check, app-slot write + verify, prefix unchanged,
watchdog reset. Usage: flash_sleepy.py <FIXTURE_ID> <NEW_REV> [wait_s]"""
import json, os, subprocess, sys, time, hashlib
REPO = "/Users/brianthomas/Projects/Resonance"
S = os.path.dirname(os.path.abspath(__file__))
PY = "/Users/brianthomas/.venvs/tonelab/bin/python"; ESPT = "/Users/brianthomas/.venvs/tonelab/bin/esptool"
fid, new_rev = sys.argv[1].upper(), sys.argv[2]; wait_s = float(sys.argv[3]) if len(sys.argv) > 3 else 300
NEW = f"{REPO}/firmware/fixture/build/{new_rev}/fixture.ino.bin"; PART = f"{REPO}/firmware/fixture/build/{new_rev}/fixture.ino.partitions.bin"
ts = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime()); OUT = f"{REPO}/ops/bench/private-backups/{ts}-{fid}"; os.makedirs(OUT, exist_ok=True)
log = open(f"{S}/{ts}-{fid}-flash.jsonl", "w")
def rec(kind, **d):
    d.update(utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), kind=kind); log.write(json.dumps(d) + "\n"); log.flush()
def die(m): print("STOP:", m); rec("stop", reason=m); sys.exit(2)
sys.path.insert(0, f"{REPO}/ops/bench"); import fleet_usb_bringup as fub
port = None; end = time.time() + wait_s
while time.time() < end and not port:
    hits = [b for b in fub.discover() if b.fixture_id == fid]
    port = hits[0].port if hits else None
    if not port: time.sleep(0.3)
if not port: die(f"{fid} did not wake within {wait_s:.0f} s")
print(f"{fid} awake on {port}; preparing service now")
env = dict(os.environ, PATH="/Users/brianthomas/.venvs/tonelab/bin:" + os.environ.get("PATH", ""))
svc = f"{OUT}/usb-service.jsonl"
r = subprocess.run([PY, f"{REPO}/ops/bench/fixture_usb_service.py", "--target", fid, "--enter-rom", "--out", svc],
                   capture_output=True, text=True, env=env, cwd=f"{REPO}/ops/bench", timeout=90)
rec("usb_service", rc=r.returncode, tail=(r.stdout + r.stderr).strip().splitlines()[-3:])
if r.returncode or "ROM identity verified" not in r.stdout: die(f"prepared service refused/failed: {(r.stdout + r.stderr).strip()[-300:]}")
prep = next(json.loads(l)["value"] for l in open(svc) if '"prepared"' in l)
before = prep["before"]; slot = before.get("ota_partition"); old_rev = before.get("fw")
addr = {"app0": 0x10000, "app1": 0x340000}.get(slot)
if addr is None: die(f"unknown slot {slot}")
rec("pre_flash_telemetry", telemetry={k: before.get(k) for k in ["fw", "fixture_id", "ota_partition", "ota_state", "battery_v", "power_tier", "bq_fault0", "battery_capacity_mah", "profile", "sensor_bits"]})
print(f"{fid}: {old_rev} on {slot}, {before.get('battery_v')} V tier {before.get('power_tier')}; in ROM")
def esp(*args, after="no-reset"):
    rr = subprocess.run([ESPT, "--port", port, "--before", "no-reset", "--after", after, *args], capture_output=True, text=True)
    rec("esptool", args=list(args), after=after, rc=rr.returncode, tail=[l for l in (rr.stdout + rr.stderr).splitlines() if l.strip() and "━" not in l][-3:])
    return rr.returncode, rr.stdout + rr.stderr
for a, n, f in [(0x0, 0x10000, "flash-0x0-64k.bin"), (0x7f0000, 0x10000, "flash-0x7f0000-coredump-64k.bin"), (addr, 0x330000, f"running-slot-{slot}-{old_rev}.bin")]:
    rc, _ = esp("read-flash", hex(a), hex(n), f"{OUT}/{f}")
    if rc: die(f"backup read {hex(a)} failed")
pre = open(f"{OUT}/flash-0x0-64k.bin", "rb").read(); newpart = open(PART, "rb").read()
if pre[0x8000:0x8000 + len(newpart)] != newpart: die("partition table differs from the new build")
rec("backup", dir=OUT, sha256={f: hashlib.sha256(open(f"{OUT}/{f}", "rb").read()).hexdigest() for f in os.listdir(OUT) if f.endswith(".bin")})
rc, _ = esp("write-flash", hex(addr), NEW)
if rc: die("write failed")
rc, out = esp("verify-flash", hex(addr), NEW)
if rc or "successful" not in out: die("new app verify failed")
rc, out = esp("verify-flash", "0x0", f"{OUT}/flash-0x0-64k.bin")
if rc or "successful" not in out: die("64 KiB prefix changed")
esp("read-mac", after="watchdog-reset")
print(f"written and verified {new_rev} into {slot}; reset out of ROM")
rec("done", new=new_rev, slot=slot, old=old_rev)
print("OK", fid)
