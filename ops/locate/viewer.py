"""Self-contained interactive 3D HTML viewer for solve results.

Hand-rolled canvas renderer (no CDN, no external deps -- repo convention):
drag = orbit, wheel = zoom, shift-drag = pan. Layer toggles for CAD slots,
estimates, truth (sim only), assignment lines and error lines; color by class
or by position error; flagged devices ringed.

write_viewer(path, payload) where payload is the dict built by locate_run.py:
  {"title": str, "devices": [{"id","role","est":[x,y,z],
    "truth":[x,y,z]|None,"cad":[x,y,z]|None,"flagged":bool,"correct":bool|None}],
   "cad_all": [{"id","role","pos":[x,y,z]}], "meta": {...}}
"""

import json

_TEMPLATE = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>__TITLE__</title>
<style>
 body { margin:0; background:#111; color:#ddd; font:13px monospace; }
 #hud { position:fixed; top:8px; left:8px; background:#000a; padding:8px 10px;
        border-radius:6px; line-height:1.7; user-select:none; }
 #hud label { display:block; cursor:pointer; }
 #meta { position:fixed; bottom:8px; left:8px; background:#000a; padding:6px 10px;
         border-radius:6px; white-space:pre; color:#9a9; }
 canvas { display:block; }
 .sw { display:inline-block; width:10px; height:10px; border-radius:5px;
       margin-right:6px; vertical-align:-1px; }
</style></head><body>
<canvas id="c"></canvas>
<div id="hud">
 <b>__TITLE__</b><br>
 <label><input type="checkbox" id="cad" checked> CAD slots (hollow)</label>
 <label><input type="checkbox" id="est" checked> estimates (solid)</label>
 <label><input type="checkbox" id="truth" checked> truth (small dots)</label>
 <label><input type="checkbox" id="alines" checked> assignment lines</label>
 <label><input type="checkbox" id="elines"> error lines (est-truth)</label>
 <label><input type="checkbox" id="byerr"> color by error</label>
 <label><input type="checkbox" id="flags" checked> ring flagged</label>
 <div id="legend"></div>
 <span style="color:#777">drag orbit / wheel zoom / shift-drag pan</span>
</div>
<div id="meta">__META__</div>
<script>
const DATA = __DATA__;
const ROLE_COLORS = {downlight:"#3987e5", perimeter:"#199e70", uplight:"#c98500",
                     chandelier:"#9085e9"};   // dark-surface categorical steps
const cv = document.getElementById("c"), ctx = cv.getContext("2d");
let W, H; function resize(){ W=cv.width=innerWidth; H=cv.height=innerHeight; draw(); }
addEventListener("resize", resize);

// scene center/extent
let cx=0, cy=0, cz=0, n=0, ext=1;
const pts=[];
for (const d of DATA.devices){ if(d.est){pts.push(d.est);} if(d.truth){pts.push(d.truth);} }
for (const f of DATA.cad_all){ pts.push(f.pos); }
for (const p of pts){ cx+=p[0]; cy+=p[1]; cz+=p[2]; n++; }
cx/=n; cy/=n; cz/=n;
for (const p of pts){ ext=Math.max(ext, Math.hypot(p[0]-cx,p[1]-cy,p[2]-cz)); }

let yaw=0.7, pitch=0.35, dist=3.2, panX=0, panY=0;
function project(p){
  const x=p[0]-cx, y=p[1]-cy, z=p[2]-cz;
  const cyw=Math.cos(yaw), syw=Math.sin(yaw), cp=Math.cos(pitch), sp=Math.sin(pitch);
  const x1=cyw*x+syw*y, y1=-syw*x+cyw*y;          // rotate about z (world up)
  const y2=cp*y1-sp*z,  z2=sp*y1+cp*z;            // pitch
  const depth=dist*ext + y2;
  if (depth<=0.05*ext) return null;
  const f=(0.9*Math.min(W,H))/ (depth/ext);
  return [W/2+panX+f*(x1/ext), H/2+panY-f*(z2/ext), depth];
}
function errColor(e){
  if (e==null) return "#888";
  const t=Math.min(e/2.0, 1.0);                    // 0..2 m ramp
  const r=Math.round(60+195*t), g=Math.round(200*(1-t)+40), b=60;
  return `rgb(${r},${g},${b})`;
}
const ui={}; for (const id of ["cad","est","truth","alines","elines","byerr","flags"])
  { ui[id]=document.getElementById(id); ui[id].onchange=draw; }

function draw(){
  ctx.clearRect(0,0,W,H);
  const items=[];
  if (ui.cad.checked) for (const f of DATA.cad_all){
    const q=project(f.pos); if(!q) continue;
    items.push({z:q[2], fn:()=>{ ctx.strokeStyle=ROLE_COLORS[f.role]||"#aaa";
      ctx.lineWidth=1; ctx.beginPath(); ctx.arc(q[0],q[1],4,0,7); ctx.stroke(); }});
  }
  for (const d of DATA.devices){
    const col = ui.byerr.checked && d.err!=null ? errColor(d.err)
              : (ROLE_COLORS[d.role]||"#aaa");
    if (ui.est.checked && d.est){
      const q=project(d.est); if(q) items.push({z:q[2], fn:()=>{
        ctx.fillStyle=col; ctx.beginPath(); ctx.arc(q[0],q[1],3.4,0,7); ctx.fill();
        if (ui.flags.checked && d.flagged){ ctx.strokeStyle="#ff4444";
          ctx.lineWidth=1.4; ctx.beginPath(); ctx.arc(q[0],q[1],6.4,0,7); ctx.stroke(); }
        if (d.correct===false){ ctx.strokeStyle="#fff"; ctx.lineWidth=1;
          ctx.beginPath(); ctx.moveTo(q[0]-4,q[1]-4); ctx.lineTo(q[0]+4,q[1]+4);
          ctx.moveTo(q[0]+4,q[1]-4); ctx.lineTo(q[0]-4,q[1]+4); ctx.stroke(); }
      }});
    }
    if (ui.truth.checked && d.truth){
      const q=project(d.truth); if(q) items.push({z:q[2], fn:()=>{
        ctx.fillStyle="#ffffff55"; ctx.beginPath(); ctx.arc(q[0],q[1],1.6,0,7); ctx.fill(); }});
    }
    if (ui.alines.checked && d.est && d.cad){
      const a=project(d.est), b=project(d.cad);
      if(a&&b) items.push({z:(a[2]+b[2])/2, fn:()=>{ ctx.strokeStyle=col+"66";
        ctx.lineWidth=1; ctx.beginPath(); ctx.moveTo(a[0],a[1]); ctx.lineTo(b[0],b[1]); ctx.stroke(); }});
    }
    if (ui.elines.checked && d.est && d.truth){
      const a=project(d.est), b=project(d.truth);
      if(a&&b) items.push({z:(a[2]+b[2])/2, fn:()=>{ ctx.strokeStyle="#ff666688";
        ctx.lineWidth=1; ctx.beginPath(); ctx.moveTo(a[0],a[1]); ctx.lineTo(b[0],b[1]); ctx.stroke(); }});
    }
  }
  items.sort((u,v)=>v.z-u.z);
  for (const it of items) it.fn();
}
let drag=null;
cv.onmousedown=e=>{ drag={x:e.clientX,y:e.clientY,shift:e.shiftKey}; };
addEventListener("mouseup",()=>drag=null);
addEventListener("mousemove",e=>{
  if(!drag) return;
  const dx=e.clientX-drag.x, dy=e.clientY-drag.y; drag.x=e.clientX; drag.y=e.clientY;
  if (drag.shift){ panX+=dx; panY+=dy; }
  else { yaw+=dx*0.008; pitch=Math.max(-1.5,Math.min(1.5,pitch+dy*0.008)); }
  draw();
});
cv.onwheel=e=>{ e.preventDefault(); dist=Math.max(1.2,Math.min(12,dist*(1+e.deltaY*0.001))); draw(); };
const lg=document.getElementById("legend");
lg.innerHTML = Object.entries(ROLE_COLORS).map(([r,c])=>
  `<span class="sw" style="background:${c}"></span>${r}`).join("<br>");
resize();
</script></body></html>
"""


def write_viewer(path: str, payload: dict):
    html = (_TEMPLATE
            .replace("__TITLE__", payload.get("title", "locate result"))
            .replace("__META__", payload.get("meta_text", ""))
            .replace("__DATA__", json.dumps(payload, separators=(",", ":"))))
    with open(path, "w") as fh:
        fh.write(html)
