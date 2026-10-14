"use strict";
// Tone lab front end. State of record lives on the server (tone-map JSON);
// this page mirrors it in S and saves on every change.

const $ = id => document.getElementById(id);
const api = (p, b) => fetch(p, {
  method: b ? "POST" : "GET", headers: {"Content-Type": "application/json"},
  body: b ? JSON.stringify(b) : undefined,
}).then(r => r.json());
const esc = s => String(s ?? "").replace(/[&<>"]/g, c => ({"&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;"}[c]));
const lsGet = k => { try { return localStorage.getItem("tonelab." + k); } catch (e) { return null; } };
const lsSet = (k, v) => { try { localStorage.setItem("tonelab." + k, v); } catch (e) { /* per-viewer only */ } };

let L = [];                                   // live lanterns from the last scan
let S = {lanterns: {}, order: [], presets: {}, patterns: {}, pulse_ms: 40};
let CFG = {pulse_min: 5, pulse_max: 100, audio: false, min_gap: 250};
let busy = false;
const UI = {tab: lsGet("tab") || "lanterns", set: "all", pattern: lsGet("pattern")};

// ---------- helpers ----------
function log(m, cls) {
  const d = document.createElement("div");
  d.className = cls || "";
  d.textContent = new Date().toLocaleTimeString() + "  " + m;
  $("log").prepend(d);
}
function meta(id) { return S.lanterns[id] || (S.lanterns[id] = {type: null, note: ""}); }
function live(id) { return L.find(l => l.fixture_id === id); }
function name(id) { const l = live(id); return (l && l.callsign) || meta(id).callsign || id; }
function online(id) { const l = live(id); return !!l && !l.offline; }
function pulseFor(id) { return meta(id).pulse_ms || S.pulse_ms; }
function validPulse(v) { return Number.isInteger(v) && v >= CFG.pulse_min && v <= CFG.pulse_max; }
function tag(t) {
  return t === "large" ? '<span class="tag on">large</span>'
    : t === "small" ? '<span class="tag on-s">small</span>'
    : '<span class="tag status">?</span>';
}
let saveTimer = null;
function save(now) {
  clearTimeout(saveTimer);
  const go = () => api("/api/state", {lanterns: S.lanterns, order: S.order, presets: S.presets,
    patterns: S.patterns, pulse_ms: S.pulse_ms})
    .then(r => { $("saved").textContent = r.ok ? "saved " + new Date().toLocaleTimeString() : "save failed"; })
    .catch(() => { $("saved").textContent = "save failed"; });
  if (now) go(); else saveTimer = setTimeout(go, 300);
}
// Keep every lantern ever classified in the ranking, even when unplugged, so a
// partial hub never reshuffles the order; append newly seen lanterns at the end.
function syncOrder() {
  const known = new Set([...S.order, ...L.map(l => l.fixture_id), ...Object.keys(S.lanterns)]);
  known.forEach(id => { if (!S.order.includes(id)) S.order.push(id); });
  L.forEach(l => { if (l.callsign) meta(l.fixture_id).callsign = l.callsign; });
}
function setList(dir) {
  const ids = S.order.filter(id => online(id) && (UI.set === "all" || meta(id).type === UI.set));
  return dir === "up" ? ids.slice().reverse() : ids;
}
function flash(id, cls, ms) {
  document.querySelectorAll(`[data-card="${id}"]`).forEach(c => {
    c.classList.add(cls); setTimeout(() => c.classList.remove(cls), ms || 500);
  });
}

// ---------- tabs ----------
function showTab(t) {
  UI.tab = t; lsSet("tab", t);
  document.querySelectorAll(".tabs button").forEach(b => b.setAttribute("aria-selected", b.dataset.tab === t));
  document.querySelectorAll(".tab").forEach(s => s.classList.toggle("active", s.id === "tab-" + t));
  render();
}
document.querySelectorAll(".tabs button").forEach(b => b.onclick = () => showTab(b.dataset.tab));
function render() {
  const n = L.filter(l => !l.offline).length;
  $("count").textContent = `${n} online · ${S.order.length} known`;
  if (UI.tab === "lanterns") renderCards();
  else if (UI.tab === "order") renderRanks();
  else renderPatterns();
}

// ---------- shared text ----------
function latText(id) {
  const m = meta(id);
  if (m.latency_ms == null) return "";
  const stale = m.latency_pulse_ms !== pulseFor(id);
  return ` · <span style="color:${stale ? "var(--warn)" : "inherit"}" title="${stale ? "pulse changed since sync; re-sync" : "measured at " + m.latency_pulse_ms + " ms pulse"}">latency ${Math.round(m.latency_ms)} ms ±${Math.round((m.latency_jitter_ms || 0) / 2)}${stale ? " (re-sync)" : ""}</span>`;
}
let PX = {on: false, live: {}, no_sensor: []}, pxSince = 0, pxTimer = null;
function proxText(id) {
  if (!PX.on) return "";
  const l = live(id);
  if (l && !l.tmf) return ' · <span class="status">no sensor</span>';
  const v = PX.live[id];
  if (!v) return " · sensor starting";
  if (v.status === "learning") return " · learning background…";
  if (v.status === "blocked") return ` · <span class="bad" title="Something sits right in front of this sensor">sensor blocked at ${Math.round(v.background_mm / 10)} cm</span>`;
  if (v.status === "hand") return ` · <span class="ok">hand ${Math.round(v.depth_mm / 10)} cm</span>`;
  if (v.status === "ready") return " · ready" + (v.background_mm ? ` (background ${Math.round(v.background_mm / 10)} cm)` : "");
  return ` · <span class="bad">${esc(v.status)}</span>`;
}
function metaText(id) {
  const l = live(id);
  if (!l) return `${id} · not connected${latText(id)}`;
  const mins = l.session_left_s != null ? Math.round(l.session_left_s / 60) : "–";
  return `${id} · hub ${l.hub} · ${l.battery_v != null ? l.battery_v.toFixed(2) + " V" : "?"} · session ${mins} min · strikes ${l.strikes ?? "?"}${latText(id)}${proxText(id)}`;
}

// ---------- Lanterns tab ----------
function cardIds() {
  const q = $("filter").value.trim().toLowerCase(), show = $("show").value, sort = $("sort").value;
  let ids = S.order.slice();
  if (q) ids = ids.filter(id => id.toLowerCase().includes(q) || name(id).toLowerCase().includes(q));
  if (show === "online") ids = ids.filter(online);
  else if (show === "unset") ids = ids.filter(id => !meta(id).type);
  else if (show !== "all") ids = ids.filter(id => meta(id).type === show);
  if (sort === "hub") ids.sort((a, b) => ((live(a) || {}).hub || "~").localeCompare((live(b) || {}).hub || "~"));
  else if (sort === "name") ids.sort((a, b) => name(a).localeCompare(name(b)));
  return ids;
}
function renderCards() {
  $("cards").innerHTML = cardIds().map(id => {
    const m = meta(id);
    return `<div class="card ${online(id) ? "" : "offline"}" data-card="${id}">
      <div class="name">${esc(name(id))}</div><div class="meta" data-meta="${id}">${metaText(id)}</div>
      <div class="row"><button data-strike="${id}">Strike</button>
        <button class="${m.type === "large" ? "on" : ""}" data-type="${id}:large">Large</button>
        <button class="${m.type === "small" ? "on-s" : ""}" data-type="${id}:small">Small</button>
        <label class="status">pulse <input type="number" min="${CFG.pulse_min}" max="${CFG.pulse_max}" step="1" style="width:56px" placeholder="${S.pulse_ms}" data-pulse="${id}" value="${m.pulse_ms ?? ""}"> ms</label></div>
      <input type="text" placeholder="Tone notes, e.g. bright, slight beat" data-note="${id}" value="${esc(m.note)}"></div>`;
  }).join("") || '<p class="status">No lanterns match.</p>';
}
function refreshMeta() {
  document.querySelectorAll("[data-meta]").forEach(el => { el.innerHTML = metaText(el.dataset.meta); });
}
["filter", "sort", "show"].forEach(k => $(k).addEventListener("input", renderCards));

// ---------- Tone order tab ----------
function renderRanks() {
  $("ranks").innerHTML = S.order.map((id, i) => {
    const l = live(id), m = meta(id);
    return `<li draggable="true" data-i="${i}" data-card="${id}" class="${online(id) ? "" : "offline"}">
      <span class="k">${i + 1}</span><span>${esc(name(id))} ${tag(m.type)}</span>
      <span class="dim">${l ? "hub " + l.hub : "not connected"}</span>
      <span class="dim">${pulseFor(id)} ms</span>
      <span class="dim">${m.latency_ms != null ? Math.round(m.latency_ms) + " ms lat" : ""}</span>
      <button data-strike="${id}" title="Strike" aria-label="Strike ${esc(name(id))}">▶</button></li>`;
  }).join("");
  renderPresets();
  document.querySelectorAll("#set button").forEach(b => b.classList.toggle("on", b.dataset.set === UI.set));
}
let dragI = null;
$("ranks").addEventListener("dragstart", e => { const li = e.target.closest("li"); dragI = +li.dataset.i; li.classList.add("drag"); });
$("ranks").addEventListener("dragover", e => {
  e.preventDefault();
  document.querySelectorAll("#ranks li.over").forEach(x => x.classList.remove("over"));
  const li = e.target.closest("li"); if (li) li.classList.add("over");
});
$("ranks").addEventListener("drop", e => {
  e.preventDefault();
  const li = e.target.closest("li"); if (!li || dragI === null) return;
  const [x] = S.order.splice(dragI, 1); S.order.splice(+li.dataset.i, 0, x);
  dragI = null; render(); save();
});
$("ranks").addEventListener("dragend", () => { dragI = null; render(); });
$("set").onclick = e => { const b = e.target.closest("button"); if (b) { UI.set = b.dataset.set; renderRanks(); } };
$("rev").onclick = () => { S.order.reverse(); render(); save(); };
$("grp").onclick = () => {
  const r = t => t === "large" ? 0 : t === "small" ? 2 : 1;
  S.order = S.order.map((id, i) => [id, i]).sort((a, b) => r(meta(a[0]).type) - r(meta(b[0]).type) || a[1] - b[1]).map(x => x[0]);
  render(); save();
};
function renderPresets() {
  const v = $("psel").value;
  $("psel").innerHTML = '<option value="">Saved orders…</option>' +
    Object.keys(S.presets).map(k => `<option ${k === v ? "selected" : ""}>${esc(k)}</option>`).join("");
}
$("psave").onclick = () => {
  const n = (prompt("Name for this order:") || "").trim(); if (!n) return;
  S.presets[n] = S.order.slice(); renderPresets(); $("psel").value = n; save(); log(`saved order "${n}"`);
};
$("pload").onclick = () => {
  const n = $("psel").value; if (!n || !S.presets[n]) return;
  const rest = S.order.filter(id => !S.presets[n].includes(id));
  S.order = S.presets[n].concat(rest); render(); save(); log(`loaded order "${n}"`);
};
$("pdel").onclick = () => {
  const n = $("psel").value; if (!n || !confirm(`Delete saved order "${n}"?`)) return;
  delete S.presets[n]; renderPresets(); save();
};
$("gapms").oninput = e => { $("gapv").textContent = e.target.value; };
document.querySelectorAll(".comp").forEach(cb => cb.addEventListener("change", e => {
  document.querySelectorAll(".comp").forEach(o => { o.checked = e.target.checked; });
}));
const compOn = () => document.querySelector(".comp").checked;

// ---------- strikes and play ----------
async function strike(id) {
  flash(id, "flash", 400);
  const ms = pulseFor(id), r = await api("/api/strike", {id, ms});
  if (r.ok) log(`${name(id)} struck at ${ms} ms (${r.strikes_before}→${r.strikes_after}, ${r.battery_v} V)`, "ok");
  else log(`${name(id)}: ${r.error || "strike not confirmed"}`, "bad");
  const l = live(id);
  if (l && r.strikes_after != null) Object.assign(l, {strikes: r.strikes_after, battery_v: r.battery_v, session_left_s: r.session_left_s});
  refreshMeta();
  return r;
}
document.addEventListener("click", async e => {
  const b = e.target.closest("button");
  if (!b || busy) return;
  if (b.dataset.strike) { busy = true; await strike(b.dataset.strike); busy = false; }
  else if (b.dataset.type) {
    const [id, t] = b.dataset.type.split(":"), m = meta(id);
    m.type = m.type === t ? null : t; render(); save();
  }
});
document.addEventListener("change", e => {
  const t = e.target;
  if (t.dataset.note) { meta(t.dataset.note).note = t.value; save(); }
  else if (t.dataset.pulse) {
    const id = t.dataset.pulse, v = t.value === "" ? null : +t.value;
    if (v !== null && !validPulse(v)) { log(`${name(id)}: pulse must be ${CFG.pulse_min}..${CFG.pulse_max} ms`, "bad"); t.value = meta(id).pulse_ms ?? ""; }
  }
});
let pulseTimer = null;
document.addEventListener("input", e => {
  const t = e.target; if (!t.dataset.pulse) return;
  const id = t.dataset.pulse, v = t.value === "" ? null : +t.value;
  if (v !== null && !validPulse(v)) return;
  meta(id).pulse_ms = v; clearTimeout(pulseTimer);
  pulseTimer = setTimeout(() => { save(true); log(`${name(id)} pulse ${v === null ? "default" : v + " ms"} saved`); refreshMeta(); }, 400);
});
document.querySelectorAll("[data-dir]").forEach(btn => btn.onclick = async () => {
  if (busy) return;
  const ids = setList(btn.dataset.dir);
  if (!ids.length) { log(`no online ${UI.set === "all" ? "" : UI.set + " "}lanterns`, "bad"); return; }
  busy = true; btn.disabled = true;
  const g = +$("gapms").value, pulses = Object.fromEntries(ids.map(id => [id, pulseFor(id)]));
  const r = await api("/api/play", {order: ids, gap_ms: g, pulses, comp: compOn(), measure: $("meas").checked});
  if (r.results) {
    const bad = r.results.filter(x => !x.ok);
    log(`played ${UI.set} ${btn.dataset.dir === "up" ? "low→high" : "high→low"} (${r.results.length}) at ${g} ms gap` +
      (bad.length ? ` — not confirmed: ${bad.map(x => name(x.fixture_id) + (x.error ? " (" + x.error + ")" : "")).join(", ")}` : " — all confirmed"), bad.length ? "bad" : "ok");
    r.results.forEach(x => { const l = live(x.fixture_id); if (l && x.strikes_after != null) Object.assign(l, {strikes: x.strikes_after, battery_v: x.battery_v}); });
    if (r.timing) {
      if (r.timing.error) log("timing: " + r.timing.error, "bad");
      else log(`timing ${r.compensated ? "(compensated)" : "(uncompensated)"}: max ${Math.round(r.timing.max_abs_ms)} ms, rms ${r.timing.rms_ms} ms — ` +
        r.timing.errors_ms.map(x => x.error_ms == null ? `${name(x.fixture_id)} not heard` : `${name(x.fixture_id)} ${x.error_ms > 0 ? "+" : ""}${Math.round(x.error_ms)}`).join(", "),
        r.timing.max_abs_ms <= 10 ? "ok" : "");
    }
  } else log(r.error || "play failed", "bad");
  btn.disabled = false; busy = false; render();
});
$("seq").onclick = async () => {
  if (busy) return; busy = true; $("seq").disabled = true;
  for (const id of setList("down")) { await strike(id); await new Promise(r => setTimeout(r, 2500)); }
  $("seq").disabled = false; busy = false;
};
$("cal").onclick = async () => {
  if (busy) return;
  const ids = setList("down"); if (!ids.length) return;
  if (!CFG.audio) { log("microphone support missing: pip install numpy sounddevice", "bad"); return; }
  busy = true; $("cal").disabled = true;
  log(`syncing latency for ${ids.length} lanterns — keep the room quiet`);
  for (const id of ids) {
    flash(id, "flash", 8000);
    const r = await api("/api/calibrate", {id, reps: +$("reps").value, ms: pulseFor(id)});
    if (r.ok) {
      Object.assign(meta(id), {latency_ms: r.latency_ms, latency_jitter_ms: r.jitter_ms, latency_pulse_ms: r.pulse_ms, latency_utc: r.utc});
      log(`${name(id)} latency ${Math.round(r.latency_ms)} ms (spread ${Math.round(r.jitter_ms)} ms; ${r.samples_ms.map(v => v == null ? "–" : v).join(", ")})`, "ok");
    } else { log(`${name(id)}: ${r.error}`, "bad"); if (/silent/.test(r.error || "")) break; }
    render();
  }
  const got = ids.map(id => meta(id).latency_ms).filter(v => v != null);
  if (got.length > 1) {
    log(`latency range ${Math.round(Math.min(...got))}–${Math.round(Math.max(...got))} ms; compensation on`);
    document.querySelectorAll(".comp").forEach(o => { o.checked = true; });
  }
  save(true); $("cal").disabled = false; busy = false;
};

// ---------- header: scan, sessions, proximity ----------
$("scan").onclick = async () => {
  if (busy) return; busy = true; $("scan").disabled = true; log("scanning…");
  const r = await api("/api/scan", {});
  if (r.lanterns) { L = r.lanterns; syncOrder(); render(); log(`found ${L.filter(l => !l.offline).length} lanterns`); save(); }
  else log(r.error || "scan failed", "bad");
  $("scan").disabled = false; busy = false;
};
$("sess").onclick = async () => {
  if (busy) return; busy = true;
  for (const l of L.filter(x => !x.offline)) {
    const r = await api("/api/session", {id: l.fixture_id, minutes: 60});
    log(`${name(l.fixture_id)} session: ${r.ok && /accepted/.test(r.reply) ? "60 min" : (r.error || r.reply || "?")}`, r.ok ? "" : "bad");
    if (r.session_left_s != null) l.session_left_s = r.session_left_s;
  }
  render(); busy = false;
};
function setProxBtn() { $("prox").textContent = "Proximity ding: " + (PX.on ? "on" : "off"); $("prox").classList.toggle("on", PX.on); }
async function pollProx() {
  const r = await api("/api/prox?since=" + pxSince); PX = r;
  for (const ev of r.events) {
    pxSince = Math.max(pxSince, ev.id);
    log(`${name(ev.fixture_id)} ding (hand at ${Math.round(ev.depth_mm / 10)} cm, ${ev.pulse_ms} ms)`, "ok");
    flash(ev.fixture_id, "ding", 600);
  }
  refreshMeta();
  if (!PX.on) { clearInterval(pxTimer); pxTimer = null; setProxBtn(); }
}
$("prange").oninput = e => { $("prangev").textContent = Math.round(e.target.value / 10); };
$("prange").onchange = e => { api("/api/prox").then(r => { if (r.on) api("/api/proximity", {on: true, range_mm: +e.target.value}); }); };
$("prox").onclick = async () => {
  const cur = await api("/api/prox?since=" + pxSince), on = !cur.on;
  const r = await api("/api/proximity", {on, range_mm: +$("prange").value}); PX = r; setProxBtn();
  if (on) {
    log(`proximity ding on: learning each background for 1.5 s (keep hands away), then dinging within ${Math.round(r.range_mm / 10)} cm` +
      (r.no_sensor.length ? ` (no sensor: ${r.no_sensor.map(name).join(", ")})` : ""));
    pxTimer = pxTimer || setInterval(pollProx, 300);
  } else { log("proximity ding off"); clearInterval(pxTimer); pxTimer = null; }
  refreshMeta();
};

// ---------- Patterns tab ----------
function newPattern(nm) { return {name: nm, bpm: 100, beats_per_bar: 4, bars: 1, steps_per_beat: 2, steps: 8, hits: {}}; }
const MAX_STEPS = 256; // server limit
// Older patterns only had steps + steps_per_beat; derive bars/beats so the grid always adds up.
function normalize(p) {
  p.steps_per_beat = p.steps_per_beat || 4;
  p.beats_per_bar = p.beats_per_bar || 4;
  p.bars = p.bars || Math.max(1, Math.round(p.steps / (p.beats_per_bar * p.steps_per_beat)));
  p.steps = p.bars * p.beats_per_bar * p.steps_per_beat;
  for (const id of Object.keys(p.hits)) {
    p.hits[id] = [...new Set(p.hits[id])].filter(i => i < p.steps).sort((a, b) => a - b);
    if (!p.hits[id].length) delete p.hits[id];
  }
  return p;
}
function curPattern() {
  if (!Object.keys(S.patterns).length) { S.patterns["Pattern 1"] = newPattern("Pattern 1"); save(); }
  if (!UI.pattern || !S.patterns[UI.pattern]) UI.pattern = Object.keys(S.patterns)[0];
  return normalize(S.patterns[UI.pattern]);
}
function stepMs(p) { return 60000 / (p.bpm * p.steps_per_beat); }
// Mirror the server rule: hits on one lantern at least CFG.min_gap ms apart, counting the loop wrap.
function violations(p) {
  const bad = {}, sm = stepMs(p);
  for (const [id, raw] of Object.entries(p.hits)) {
    const idx = [...new Set(raw)].filter(i => i < p.steps).sort((a, b) => a - b);
    for (let k = 0; k < idx.length && idx.length > 1; k++) {
      const nxt = k + 1 < idx.length ? idx[k + 1] : idx[0] + p.steps;
      if ((nxt - idx[k]) * sm < CFG.min_gap) {
        (bad[id] = bad[id] || new Set()).add(idx[k]).add(idx[(k + 1) % idx.length]);
      }
    }
  }
  return bad;
}
// Would adding step i to this lantern put two of its hits closer than the
// minimum gap at the current tempo (counting the loop wrap)? Returns the
// offending gap in ms, or 0 when the hit is allowed.
function placementGap(p, id, i) {
  const idx = [...new Set([...(p.hits[id] || []), i])].sort((a, b) => a - b);
  if (idx.length < 2) return 0;
  const sm = stepMs(p);
  let worst = Infinity;
  for (let k = 0; k < idx.length; k++) {
    const nxt = k + 1 < idx.length ? idx[k + 1] : idx[0] + p.steps;
    worst = Math.min(worst, (nxt - idx[k]) * sm);
  }
  return worst < CFG.min_gap ? worst : 0;
}
function deny(c, id, gap) {
  c.classList.remove("deny"); void c.offsetWidth; c.classList.add("deny");
  setTimeout(() => c.classList.remove("deny"), 1200);
  if (!paint || !paint.warned) {
    log(`${name(id)}: hits ${Math.round(gap)} ms apart at ${curPattern().bpm} BPM — needs ${CFG.min_gap} ms; slow the tempo or space them out`, "bad");
    if (paint) paint.warned = true;
  }
}
let cellEls = [], playhead = -1;
// Firmware pattern state from the last scan (ADR 0084 builds only).
function fwBadge(id) {
  const l = live(id);
  if (!l || !l.fw_pattern) return "";
  if (l.pattern_playing) return `<span class="fw play" title="standalone: ${esc(l.pattern_role)}${l.pattern_locked ? ", in time" : ", syncing"}">${l.pattern_role === "conduct" ? "♪ cond" : l.pattern_locked ? "♪" : "…"}</span>`;
  return l.pattern_id && l.pattern_id !== "00000000" ? `<span class="fw" title="stored pattern ${esc(l.pattern_id)}">●</span>` : "";
}
let saPatternId = lsGet("saPatternId") || "";
function renderPatterns() {
  const p = curPattern();
  $("patsel").innerHTML = Object.keys(S.patterns).map(k => `<option ${k === UI.pattern ? "selected" : ""}>${esc(k)}</option>`).join("");
  $("bpm").value = p.bpm; $("bpb").value = p.beats_per_bar; $("spb").value = p.steps_per_beat; $("bars").value = p.bars;
  const bad = violations(p), rows = S.order, spb = p.steps_per_beat, barSteps = spb * p.beats_per_bar;
  const g = $("seq-grid");
  g.style.gridTemplateColumns = `180px repeat(${p.steps}, 26px)`;
  let h = '<div class="rh status">lantern</div>';
  for (let i = 0; i < p.steps; i++) {
    const beat = i % spb === 0, bar = i % barSteps === 0;
    const label = bar ? `${i / barSteps + 1}` : beat ? `${(i % barSteps) / spb + 1}` : "·";
    h += `<div class="hd ${bar ? "bar" : beat ? "beat" : ""}" title="${bar ? "bar " + (i / barSteps + 1) : ""}">${label}</div>`;
  }
  rows.forEach(id => {
    const m = meta(id), hits = new Set(p.hits[id] || []), off = online(id) ? "" : " offline";
    h += `<div class="rh" data-card="${id}"><span class="nm" title="${esc(id)}">${esc(name(id))}</span>${fwBadge(id)}${tag(m.type)}<button data-strike="${id}" aria-label="Strike ${esc(name(id))}">▶</button></div>`;
    for (let i = 0; i < p.steps; i++) {
      const cls = ["c"];
      if (i && i % barSteps === 0) cls.push("bar"); else if (i && i % spb === 0) cls.push("beat");
      if (hits.has(i)) cls.push("on");
      if (m.type === "small") cls.push("small");
      if (bad[id] && bad[id].has(i)) cls.push("bad");
      h += `<div class="${cls.join(" ")}${off}" data-cell="${id}:${i}"><i></i></div>`;
    }
  });
  g.innerHTML = h;
  cellEls = [];
  g.querySelectorAll("[data-cell]").forEach(el => {
    const i = +el.dataset.cell.split(":")[1]; (cellEls[i] = cellEls[i] || []).push(el);
  });
  playhead = -1;
  const nHits = Object.values(p.hits).reduce((a, v) => a + v.length, 0), nBad = Object.keys(bad).length;
  const minSteps = Math.ceil(CFG.min_gap / stepMs(p) - 1e-9);
  $("patinfo").innerHTML = `loop ${(p.steps * stepMs(p) / 1000).toFixed(2)} s · step ${Math.round(stepMs(p))} ms · ${nHits} hits · ` +
    (minSteps <= 1 ? "any step allowed on one lantern" : `one lantern needs ${minSteps} steps between hits`) +
    (nBad ? ` · <span class="bad">${nBad} lantern${nBad > 1 ? "s" : ""} too fast (under ${CFG.min_gap} ms)</span>` : "");
}
function setCell(id, i, on) {
  const p = curPattern(), set = new Set(p.hits[id] || []);
  if (on) set.add(i); else set.delete(i);
  if (set.size) p.hits[id] = [...set].sort((a, b) => a - b); else delete p.hits[id];
}
let paint = null;
$("seq-grid").addEventListener("pointerdown", e => {
  const c = e.target.closest("[data-cell]"); if (!c) return;
  const [id, i] = c.dataset.cell.split(":"), on = !c.classList.contains("on");
  paint = {on, seen: new Set([c.dataset.cell]), warned: false};
  e.preventDefault();
  const gap = on ? placementGap(curPattern(), id, +i) : 0;
  if (gap) { deny(c, id, gap); return; }
  setCell(id, +i, on); c.classList.toggle("on", on);
});
document.addEventListener("pointermove", e => {
  if (!paint) return;
  const el = document.elementFromPoint(e.clientX, e.clientY), c = el && el.closest("[data-cell]");
  if (!c || paint.seen.has(c.dataset.cell)) return;
  paint.seen.add(c.dataset.cell);
  const [id, i] = c.dataset.cell.split(":");
  const gap = paint.on ? placementGap(curPattern(), id, +i) : 0;
  if (gap) { deny(c, id, gap); return; }
  setCell(id, +i, paint.on); c.classList.toggle("on", paint.on);
});
document.addEventListener("pointerup", () => { if (paint) { paint = null; renderPatterns(); save(); } });
$("patsel").onchange = e => { UI.pattern = e.target.value; lsSet("pattern", UI.pattern); renderPatterns(); };
function uniqueName(base) { let n = base, k = 2; while (S.patterns[n]) n = `${base} ${k++}`; return n; }
$("patnew").onclick = () => {
  const nm = (prompt("Name for the new pattern:", uniqueName("Pattern")) || "").trim(); if (!nm) return;
  if (S.patterns[nm]) { log(`a pattern called "${nm}" already exists`, "bad"); return; }
  S.patterns[nm] = newPattern(nm); UI.pattern = nm; lsSet("pattern", nm); renderPatterns(); save();
};
$("patdup").onclick = () => {
  const p = curPattern(), nm = uniqueName(p.name + " copy");
  S.patterns[nm] = JSON.parse(JSON.stringify({...p, name: nm})); UI.pattern = nm; lsSet("pattern", nm); renderPatterns(); save();
};
$("patren").onclick = () => {
  const p = curPattern(), nm = (prompt("Rename pattern:", p.name) || "").trim();
  if (!nm || nm === p.name) return;
  if (S.patterns[nm]) { log(`a pattern called "${nm}" already exists`, "bad"); return; }
  delete S.patterns[p.name]; p.name = nm; S.patterns[nm] = p; UI.pattern = nm; lsSet("pattern", nm); renderPatterns(); save();
};
$("patdel").onclick = () => {
  const p = curPattern(); if (!confirm(`Delete pattern "${p.name}"?`)) return;
  delete S.patterns[p.name]; UI.pattern = null; renderPatterns(); save();
};
$("patclear").onclick = () => { const p = curPattern(); if (confirm(`Clear every hit in "${p.name}"?`)) { p.hits = {}; renderPatterns(); save(); } };
$("bpm").onchange = e => {
  const v = Math.round(+e.target.value);
  if (!(v >= 20 && v <= 300)) { log("BPM must be 20..300", "bad"); e.target.value = curPattern().bpm; return; }
  const p = curPattern(); p.bpm = v;
  const nBad = Object.keys(violations(p)).length;
  if (nBad) log(`at ${v} BPM ${nBad} lantern${nBad > 1 ? "s are" : " is"} too fast (blinking red); the pattern won't play until fixed`, "bad");
  renderPatterns(); save();
};
// Grid changes keep hits at the same musical position where the new grid has one.
function regrid(change) {
  const p = curPattern(), old = {spb: p.steps_per_beat, bpb: p.beats_per_bar, bars: p.bars};
  const next = {...old, ...change};
  if (next.bars * next.bpb * next.spb > MAX_STEPS) {
    log(`that grid would be ${next.bars * next.bpb * next.spb} steps; the limit is ${MAX_STEPS}`, "bad"); renderPatterns(); return;
  }
  let moved = 0;
  for (const id of Object.keys(p.hits)) {
    const out = new Set();
    for (const i of p.hits[id]) {
      const beatPos = i / old.spb;                       // position in beats from the loop start
      const j = beatPos * next.spb;
      if (Math.abs(j - Math.round(j)) > 1e-6) moved++;   // no exact slot on the new grid
      out.add(Math.round(j));
    }
    p.hits[id] = [...out];
  }
  Object.assign(p, {steps_per_beat: next.spb, beats_per_bar: next.bpb, bars: next.bars});
  normalize(p);
  if (moved) log(`${moved} hit${moved > 1 ? "s" : ""} had no exact slot on the new grid and moved to the nearest step`);
  const nBad = Object.keys(violations(p)).length;
  if (nBad) log(`${nBad} lantern${nBad > 1 ? "s" : ""} now too fast for this tempo (blinking red); the pattern won't play until fixed`, "bad");
  renderPatterns(); save();
}
$("spb").onchange = e => regrid({spb: +e.target.value});
$("bpb").onchange = e => regrid({bpb: +e.target.value});
$("bars").onchange = e => regrid({bars: +e.target.value});

let patPoll = null, patAnim = null, patStatus = null;
function animate() {
  if (!patStatus || UI.tab !== "patterns") { patAnim = requestAnimationFrame(animate); return; }
  const p = curPattern(), t = Date.now() - patStatus.start_epoch_ms - (patStatus.lead_ms || 0);
  let col = -1;
  if (t >= 0 && patStatus.loop_ms) col = Math.floor((t % patStatus.loop_ms) / patStatus.step_ms);
  if (patStatus.name !== p.name || col >= p.steps) col = -1;
  if (col !== playhead) {
    (cellEls[playhead] || []).forEach(el => el.classList.remove("ph"));
    (cellEls[col] || []).forEach(el => el.classList.add("ph"));
    playhead = col;
  }
  patAnim = requestAnimationFrame(animate);
}
function stopAnim() {
  cancelAnimationFrame(patAnim); patAnim = null; patStatus = null;
  (cellEls[playhead] || []).forEach(el => el.classList.remove("ph")); playhead = -1;
}
async function watchPattern() {
  const st = await api("/api/pattern/status");
  if (st.running) { patStatus = st; return; }
  clearInterval(patPoll); patPoll = null; stopAnim();
  $("patplay").disabled = false;
  if (st.sent != null) {
    log(`pattern "${st.name}" finished: ${st.loops_done} loop${st.loops_done === 1 ? "" : "s"}, ${st.sent} strikes` +
      (st.confirmed ? ", all confirmed" : `, not confirmed: ${(st.unconfirmed || []).join(", ")}`) +
      (st.note ? ` (${st.note})` : "") + (st.errors ? ` — ${st.errors.join("; ")}` : ""), st.confirmed ? "ok" : "bad");
  }
}
$("patplay").onclick = async () => {
  if (busy) return;
  const p = curPattern();
  const r = await api("/api/pattern/play", {pattern: p, loops: +$("loops").value, comp: compOn()});
  if (!r.ok) { log(r.error || "could not start", "bad"); return; }
  const skipped = Object.keys(p.hits).filter(id => !online(id));
  log(`playing "${p.name}"${+$("loops").value ? ` × ${$("loops").value}` : " until stopped"}${compOn() ? " (compensated)" : ""}` +
    (skipped.length ? ` — not connected: ${skipped.map(name).join(", ")}` : ""));
  patStatus = r; $("patplay").disabled = true;
  if (!patAnim) patAnim = requestAnimationFrame(animate);
  patPoll = patPoll || setInterval(watchPattern, 500);
};
$("patstop").onclick = () => api("/api/pattern/stop", {});

function reportResults(verb, r) {
  (r.results || []).forEach(x => log(`${name(x.fixture_id)}: ${x.ok ? (x.reply || "ok") : (x.error || x.reply || "failed")}`, x.ok ? "" : "bad"));
  const bad = (r.results || []).filter(x => !x.ok).length, n = (r.results || []).length;
  log(`${verb}: ${n - bad} of ${n} lanterns ok`, bad ? "bad" : "ok");
}
$("saload").onclick = async () => {
  if (busy) return; busy = true; $("saload").disabled = true;
  const p = curPattern();
  log(`loading "${p.name}" onto connected lanterns…`);
  const r = await api("/api/pattern/upload", {pattern: p, lead: $("salead").checked});
  if (r.error && !r.results) log(r.error, "bad");
  else {
    saPatternId = r.pattern_id; lsSet("saPatternId", saPatternId); reportResults(`load "${p.name}" (pattern ${r.pattern_id})`, r);
    if ((r.skipped_old_firmware || []).length) log(`skipped (no pattern-mode firmware yet): ${r.skipped_old_firmware.map(name).join(", ")}`);
  }
  $("sainfo").textContent = saPatternId ? `loaded pattern ${saPatternId}` : "";
  $("saload").disabled = false; busy = false; $("scan").click();
};
$("saplay").onclick = async () => {
  if (busy) return;
  if (!saPatternId) { log("load a pattern onto the lanterns first", "bad"); return; }
  busy = true;
  const r = await api("/api/pattern/standalone", {action: "play", pattern_id: saPatternId, minutes: +$("samin").value});
  if (r.error && !r.results) log(r.error, "bad"); else reportResults(`standalone play ${saPatternId}`, r);
  busy = false; setTimeout(() => $("scan").click(), 3500);
};
$("sastop").onclick = async () => {
  if (busy) return; busy = true;
  const r = await api("/api/pattern/standalone", {action: "stop"});
  if (r.error && !r.results) log(r.error, "bad"); else reportResults("standalone stop", r);
  busy = false; $("scan").click();
};

// ---------- log bar and init ----------
$("logbar").onclick = () => $("logbar").classList.toggle("open");
(async function init() {
  const r = await api("/api/snapshot");
  S = Object.assign({lanterns: {}, order: [], presets: {}, patterns: {}, pulse_ms: 40}, r.state);
  CFG = {pulse_min: r.pulse_min, pulse_max: r.pulse_max, audio: r.audio, min_gap: r.min_hit_spacing_ms || 250};
  L = r.lanterns || [];
  syncOrder();
  $("saved").textContent = "saving to " + r.state_file;
  const px = await api("/api/prox");
  PX = px; $("prange").value = px.range_mm; $("prangev").textContent = Math.round(px.range_mm / 10); setProxBtn();
  if (px.on) pxTimer = setInterval(pollProx, 300);
  const st = await api("/api/pattern/status");
  if (st.running) { patStatus = st; $("patplay").disabled = true; patAnim = requestAnimationFrame(animate); patPoll = setInterval(watchPattern, 500); }
  $("sainfo").textContent = saPatternId ? `loaded pattern ${saPatternId}` : "";
  showTab(["lanterns", "order", "patterns"].includes(UI.tab) ? UI.tab : "lanterns");
  $("scan").click();
})();
