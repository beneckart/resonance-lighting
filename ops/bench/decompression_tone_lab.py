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

REPO = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser()
ap.add_argument("--site", default="Brian bench")
ap.add_argument("--port", type=int, default=8765)
ap.add_argument("--pulse-ms", type=int, default=40)
ARGS = ap.parse_args()

DATA_DIR = REPO / "ops" / "bench" / "data" / ARGS.site
STATE_FILE = DATA_DIR / "decompression-tone-map.json"
STRIKE_LOG = DATA_DIR / "decompression-tone-strikes.jsonl"
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


def send_exact(fid, line, verify_strike=False):
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
    res = {"ok": True, "fixture_id": fid, "cmd": line, "utc": sent, "reply": reply.strip()[-200:],
           "strikes_before": before.get("solenoid_strikes"), "strikes_after": after and after.get("solenoid_strikes"),
           "blocked": after and after.get("solenoid_blocked"), "failsafes": after and after.get("solenoid_failsafes"),
           "battery_v": after and after.get("battery_v"), "session_left_s": after and after.get("decompression_left_s")}
    if verify_strike:
        res["ok"] = res["strikes_after"] is not None and res["strikes_after"] == (res["strikes_before"] or 0) + 1
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(STRIKE_LOG, "a") as f:
        f.write(json.dumps(res) + "\n")
    return res


def play(order, gap_ms):
    """Fast run: verify every identity in parallel, then fire one exact-target strike
    per lantern on an absolute schedule, then confirm all counters in parallel."""
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
        t0 = time.perf_counter() + 0.25
        for i, fid in enumerate(ready):
            due = t0 + i * gap_ms / 1000.0
            while time.perf_counter() < due:
                pass
            conns[fid].write(f"K{fid}:{ARGS.pulse_ms}\n".encode())
            results[fid]["sent_offset_ms"] = round((time.perf_counter() - t0) * 1000, 1)
            results[fid]["utc"] = utc()

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
        f.write(json.dumps({"kind": "play", "utc": utc(), "gap_ms": gap_ms, "results": out}) + "\n")
    return {"ok": all(r.get("ok") for r in out), "gap_ms": gap_ms, "results": out}


def snapshot():
    with state_lock:
        return {"lanterns": sorted(lanterns.values(), key=lambda l: l["hub"]), "state": STATE,
                "state_file": str(STATE_FILE.relative_to(REPO)), "pulse_ms": ARGS.pulse_ms}


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
            fid = b["id"].upper()
            return self.reply(send_exact(fid, f"K{fid}:{ARGS.pulse_ms}", verify_strike=True))
        if self.path == "/api/play":
            gap = int(b.get("gap_ms", 400))
            if not 100 <= gap <= 5000:
                return self.reply({"ok": False, "error": "gap must be 100..5000 ms"}, 400)
            return self.reply(play([i.upper() for i in b.get("order", [])], gap))
        if self.path == "/api/session":
            fid, minutes = b["id"].upper(), int(b.get("minutes", 60))
            if not 0 <= minutes <= 120:
                return self.reply({"ok": False, "error": "minutes must be 0..120"}, 400)
            return self.reply(send_exact(fid, f"!D{fid}:{minutes}"))
        if self.path == "/api/state":
            with state_lock:
                STATE["lanterns"] = b.get("lanterns", STATE["lanterns"])
                STATE["order"] = b.get("order", STATE["order"])
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
.panel{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px;align-self:start;position:sticky;top:64px}
.panel h2{font-size:14px;margin:0 0 8px}.ol{list-style:none;margin:0;padding:0}
.ol li{display:flex;align-items:center;gap:6px;padding:6px 4px;border-bottom:1px solid var(--line);font-size:14px;cursor:grab}
.ol li.drag{opacity:.4}.ol .k{width:20px;color:var(--muted);font-size:12px}.ol .t{flex:1}
.tag{font-size:11px;padding:1px 6px;border-radius:6px}
#log{font:12px ui-monospace,monospace;color:var(--muted);max-height:180px;overflow:auto;margin-top:10px;white-space:pre-wrap}
.status{font-size:12px;color:var(--muted)}.ok{color:var(--ok)}.bad{color:var(--bad)}
</style></head><body>
<header><h1>Tone lab</h1>
<button id="scan">Rescan lanterns</button>
<button id="play" class="on-l">▶ Play in order</button>
<label class="status">gap <input id="gapms" type="range" min="100" max="1500" step="50" value="400" style="width:110px;vertical-align:middle"> <span id="gapv">400</span> ms</label>
<button id="seq">Slow check (one by one)</button>
<label class="status">gap <input id="gap" type="number" value="3" min="1" max="15" step="0.5" style="width:52px"> s</label>
<button id="sess">Extend all sessions 60 min</button>
<span class="status" id="saved"></span></header>
<main><section><div class="grid" id="grid"></div></section>
<aside class="panel"><h2>Tone order (1 = highest) — drag to reorder</h2><ul class="ol" id="order"></ul><div id="log"></div></aside></main>
<script>
let L=[],S={lanterns:{},order:[]},busy=false;
const $=id=>document.getElementById(id);
const api=(p,b)=>fetch(p,{method:b?"POST":"GET",headers:{"Content-Type":"application/json"},body:b?JSON.stringify(b):undefined}).then(r=>r.json());
function log(m,cls){const d=document.createElement("div");d.className=cls||"";d.textContent=new Date().toLocaleTimeString()+"  "+m;$("log").prepend(d)}
function meta(id){return S.lanterns[id]||(S.lanterns[id]={type:null,note:""})}
function save(){api("/api/state",{lanterns:S.lanterns,order:S.order}).then(r=>{$("saved").textContent="saved "+new Date().toLocaleTimeString()})}
function syncOrder(){const ids=L.map(l=>l.fixture_id);S.order=S.order.filter(i=>ids.includes(i));ids.forEach(i=>{if(!S.order.includes(i))S.order.push(i)})}
function name(id){const l=L.find(x=>x.fixture_id===id);return l?(l.callsign||id):id}
function tag(t){return t==="large"?'<span class="tag on-l">large</span>':t==="small"?'<span class="tag on-s">small</span>':'<span class="tag" style="color:var(--muted)">?</span>'}
function render(){
 $("grid").innerHTML=L.map(l=>{const m=meta(l.fixture_id),mins=l.session_left_s!=null?Math.round(l.session_left_s/60):"–";
 return `<div class="card ${l.offline?"offline":""}" id="c-${l.fixture_id}"><div class="name">${l.callsign||l.fixture_id}</div>
 <div class="meta">${l.fixture_id} · hub ${l.hub} · ${l.battery_v!=null?l.battery_v.toFixed(2)+" V":"?"} · session ${mins} min · strikes ${l.strikes??"?"}</div>
 <div class="row"><button data-strike="${l.fixture_id}">Strike</button>
 <button class="${m.type==="large"?"on-l":""}" data-type="${l.fixture_id}:large">Large</button>
 <button class="${m.type==="small"?"on-s":""}" data-type="${l.fixture_id}:small">Small</button></div>
 <input type="text" placeholder="Tone notes, e.g. bright, slight beat" data-note="${l.fixture_id}" value="${(m.note||"").replace(/"/g,"&quot;")}"></div>`}).join("");
 $("order").innerHTML=S.order.map((id,i)=>`<li draggable="true" data-i="${i}"><span class="k">${i+1}</span><span class="t">${name(id)}</span>${tag(meta(id).type)}<button data-strike="${id}" title="Strike">▶</button></li>`).join("");
}
async function strike(id){const c=$("c-"+id);c&&c.classList.add("flash");const r=await api("/api/strike",{id});c&&c.classList.remove("flash");
 if(r.ok)log(`${name(id)} struck (${r.strikes_before}→${r.strikes_after}, ${r.battery_v} V)`,"ok");else log(`${name(id)}: ${r.error||"strike not confirmed"}`,"bad");
 const l=L.find(x=>x.fixture_id===id);if(l&&r.strikes_after!=null){l.strikes=r.strikes_after;l.battery_v=r.battery_v;l.session_left_s=r.session_left_s}render();return r}
document.addEventListener("click",async e=>{const b=e.target.closest("button");if(!b||busy)return;
 if(b.dataset.strike){busy=true;await strike(b.dataset.strike);busy=false}
 else if(b.dataset.type){const[id,t]=b.dataset.type.split(":");const m=meta(id);m.type=m.type===t?null:t;render();save()}});
document.addEventListener("change",e=>{if(e.target.dataset.note){meta(e.target.dataset.note).note=e.target.value;save()}});
let dragI=null;
$("order").addEventListener("dragstart",e=>{const li=e.target.closest("li");dragI=+li.dataset.i;li.classList.add("drag")});
$("order").addEventListener("dragover",e=>e.preventDefault());
$("order").addEventListener("drop",e=>{e.preventDefault();const li=e.target.closest("li");if(!li||dragI===null)return;const j=+li.dataset.i;const[x]=S.order.splice(dragI,1);S.order.splice(j,0,x);dragI=null;render();save()});
$("order").addEventListener("dragend",()=>{dragI=null;render()});
$("scan").onclick=async()=>{busy=true;$("scan").disabled=true;log("scanning…");const r=await api("/api/scan",{});L=r.lanterns;syncOrder();render();log(`found ${L.filter(l=>!l.offline).length} lanterns`);$("scan").disabled=false;busy=false};
$("seq").onclick=async()=>{if(busy)return;busy=true;$("seq").disabled=true;const g=Math.max(1,+$("gap").value||3)*1000;
 for(const id of S.order){await strike(id);await new Promise(r=>setTimeout(r,g))}$("seq").disabled=false;busy=false};
$("gapms").oninput=e=>$("gapv").textContent=e.target.value;
$("play").onclick=async()=>{if(busy)return;busy=true;$("play").disabled=true;const g=+$("gapms").value;
 const r=await api("/api/play",{order:S.order,gap_ms:g});
 if(r.results){const bad=r.results.filter(x=>!x.ok);log(`played ${r.results.length} at ${g} ms`+(bad.length?` — not confirmed: ${bad.map(x=>name(x.fixture_id)+(x.error?" ("+x.error+")":"")).join(", ")}`:" — all confirmed"),bad.length?"bad":"ok");
  r.results.forEach(x=>{const l=L.find(y=>y.fixture_id===x.fixture_id);if(l&&x.strikes_after!=null){l.strikes=x.strikes_after;l.battery_v=x.battery_v}});render()}
 else log(r.error||"play failed","bad");$("play").disabled=false;busy=false};
$("sess").onclick=async()=>{if(busy)return;busy=true;for(const l of L.filter(x=>!x.offline)){const r=await api("/api/session",{id:l.fixture_id,minutes:60});log(`${name(l.fixture_id)} session: ${r.ok&&/accepted/.test(r.reply)?"60 min":(r.error||r.reply||"?")}`,r.ok?"":"bad");if(r.session_left_s!=null)l.session_left_s=r.session_left_s}render();busy=false};
api("/api/snapshot").then(r=>{S=r.state;S.lanterns=S.lanterns||{};S.order=S.order||[];$("saved").textContent="saving to "+r.state_file;$("scan").click()});
</script></body></html>"""

if __name__ == "__main__":
    print(f"tone lab on http://localhost:{ARGS.port}  state -> {STATE_FILE}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", ARGS.port), H).serve_forever()
