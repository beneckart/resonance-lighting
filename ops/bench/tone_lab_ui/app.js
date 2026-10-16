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
let S = {lanterns: {}, order: [], presets: {}, patterns: {}, pulse_ms: 40, tone_links: [], interactions: []};
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
// Role: "lantern" (default) or "perimeter" (perimeter light; its sensor drives approach rules).
const isPerim = id => meta(id).role === "perimeter";
const roleTag = id => isPerim(id) ? '<span class="tag perim">perimeter</span> ' : "";
function tag(t) {
  return t === "large" ? '<span class="tag on">large</span>'
    : t === "small" ? '<span class="tag on-s">small</span>'
    : '<span class="tag status">?</span>';
}
let saveTimer = null;
function save(now) {
  clearTimeout(saveTimer);
  const go = () => api("/api/state", {lanterns: S.lanterns, order: S.order, presets: S.presets, tone_links: S.tone_links,
    interactions: S.interactions,
    patterns: S.patterns, pulse_ms: S.pulse_ms})
    .then(r => {
      const msg = r.ok ? "saved " + new Date().toLocaleTimeString() : "save failed";
      $("saved").textContent = msg; if ($("patsaved")) $("patsaved").textContent = msg; if ($("ixsaved")) $("ixsaved").textContent = "· " + msg;
    })
    .catch(() => { $("saved").textContent = "save failed"; });
  if (now) go(); else saveTimer = setTimeout(go, 300);
}
// Keep every lantern ever classified in the ranking, even when unplugged, so a
// partial hub never reshuffles the order; append newly seen lanterns at the end.
const isFixtureId = id => /^[0-9A-F]{6}$/.test(id);
function syncOrder() {
  const known = new Set([...S.order, ...L.map(l => l.fixture_id), ...Object.keys(S.lanterns)].filter(isFixtureId));
  S.order = S.order.filter(isFixtureId);
  known.forEach(id => { if (!S.order.includes(id)) S.order.push(id); });
  L.forEach(l => { if (l.callsign) meta(l.fixture_id).callsign = l.callsign; });
}
function setList(dir) {
  const ids = S.order.filter(id => online(id) && (UI.set === "all" || meta(id).type === UI.set));
  return dir === "up" ? ids.slice().reverse() : ids;
}
// ---- same-tone groups: a lantern listed in S.tone_links has the same tone as
// the lantern directly above it in S.order; consecutive links form one group.
function linked(id) { return (S.tone_links || []).includes(id); }
// Tone number badge (position in the Tone order tab, 1 = highest), shown on every tab.
function toneBadge(id) {
  const n = S.order.indexOf(id) + 1;
  if (!n) return "";
  const g = toneGroups().find(x => x.includes(id)) || [id];
  const title = g.length > 1 ? `tone #${n}; same tone as ${g.filter(x => x !== id).map(x => "#" + (S.order.indexOf(x) + 1) + " " + name(x)).join(", ")}` : `tone #${n} (1 = highest)`;
  return `<span class="tno ${g.length > 1 ? "grp " + groupColor(id) : ""}" title="${esc(title)}">${n}</span>`;
}
// Each same-tone group gets its own color (cycling through 6) so pairs are
// easy to tell apart on every tab; single lanterns get none.
function groupColor(id) {
  const multi = toneGroups().filter(g => g.length > 1);
  const k = multi.findIndex(g => g.includes(id));
  return k < 0 ? "" : "gc" + (k % 6);
}
function toneGroups() {
  const groups = [];
  S.order.forEach((id, i) => { if (i && linked(id) && groups.length) groups[groups.length - 1].push(id); else groups.push([id]); });
  return groups;
}
// Rebuild links after a reorder so lanterns that were grouped and are still
// adjacent stay grouped; `drop` lanterns (e.g. one just dragged) are unlinked.
function keepGroups(oldGroups, drop) {
  const gid = {};
  oldGroups.forEach((g, k) => g.forEach(id => { gid[id] = k; }));
  const out = (drop || []);
  S.tone_links = S.order.filter((id, i) => i > 0 && !out.includes(id) && !out.includes(S.order[i - 1]) && gid[id] === gid[S.order[i - 1]]);
}
// ---- selection-based grouping (Tone order tab) ----
const SEL = new Set();
function groupOf(id) { return toneGroups().find(g => g.includes(id)) || [id]; }
function selInfo() {
  $("selinfo").textContent = SEL.size ? `${SEL.size} selected: ${[...SEL].map(name).join(", ")}`
    : "Tick lanterns that sound the same, then Group selected. Groups always play together.";
}
document.addEventListener("change", e => {
  const t = e.target; if (!t.dataset || !t.dataset.sel) return;
  if (t.checked) SEL.add(t.dataset.sel); else SEL.delete(t.dataset.sel);
  selInfo();
});
// Group: the ticked lanterns become one group, placed together at the first
// ticked position (in their current relative order); other groups are kept.
$("grpsel").onclick = () => {
  const picked = S.order.filter(id => SEL.has(id));
  if (picked.length < 2) { log("tick at least two lanterns to group", "bad"); return; }
  const old = toneGroups(), at = S.order.indexOf(picked[0]);
  const rest = S.order.filter(id => !SEL.has(id));
  const insertAt = rest.findIndex((id, i) => S.order.indexOf(id) > at);
  S.order = insertAt < 0 ? rest.concat(picked) : rest.slice(0, insertAt).concat(picked, rest.slice(insertAt));
  keepGroups(old, picked);
  S.tone_links = (S.tone_links || []).concat(picked.slice(1));
  SEL.clear(); selInfo(); render(); save(true);
  log(`grouped as one tone: ${picked.map(name).join(", ")}`, "ok");
};
// Ungroup: each ticked lantern leaves its group and moves just after it, so the
// remaining members stay together.
$("ungrpsel").onclick = () => {
  const picked = S.order.filter(id => SEL.has(id) && groupOf(id).length > 1);
  if (!picked.length) { log("tick grouped lanterns to ungroup", "bad"); return; }
  const old = toneGroups();
  for (const id of picked) {
    const g = old.find(x => x.includes(id)), last = [...g].reverse().find(x => !SEL.has(x)) || g[g.length - 1];
    S.order.splice(S.order.indexOf(id), 1);
    S.order.splice(S.order.indexOf(last) + 1, 0, id);
  }
  keepGroups(old, picked);
  SEL.clear(); selInfo(); render(); save(true);
  log(`ungrouped: ${picked.map(name).join(", ")}`, "ok");
};
// A row's ▶ plays its whole same-tone group at once.
async function playGroup(id) {
  const ids = groupOf(id).filter(online);
  if (ids.length <= 1) return strike(id);
  const pulses = Object.fromEntries(ids.map(x => [x, pulseFor(x)]));
  ids.forEach(x => flash(x, "flash", 400));
  const r = await api("/api/play", {order: ids, gap_ms: 400, pulses, comp: compOn(), slots: ids.map(() => 0)});
  if (r.results) {
    const bad = r.results.filter(x => !x.ok);
    log(`group ${ids.map(name).join(" + ")} struck together` + (bad.length ? ` — not confirmed: ${bad.map(x => name(x.fixture_id)).join(", ")}` : ""), bad.length ? "bad" : "ok");
    r.results.forEach(x => { const l = live(x.fixture_id); if (l && x.strikes_after != null) Object.assign(l, {strikes: x.strikes_after, battery_v: x.battery_v}); });
  } else log(r.error || "group strike failed", "bad");
}
// Beat slot per lantern for Play: same-tone group members share a slot.
function groupSlots(ids) {
  const gidx = {};
  toneGroups().forEach((g, k) => g.forEach(id => { gidx[id] = k; }));
  const slots = [], seen = new Map();
  ids.forEach(id => { const g = gidx[id]; if (!seen.has(g)) seen.set(g, seen.size); slots.push(seen.get(g)); });
  return slots;
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
  else if (UI.tab === "proximity") renderProxTab();
  else if (UI.tab === "interactions") renderIx();
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
let WK = {keep: false, targets: []}, wkSince = 0;
function wakeLabel(id) {
  if (WK.targets.includes(id)) return "waking…";
  return online(id) ? "Wake" : "Wake";
}
function metaText(id) {
  const l = live(id);
  if (!l) return `${id} · not connected${WK.targets.includes(id) ? " · waking…" : ""}${latText(id)}`;
  if (l.offline) return `${id} · hub ${l.hub} · <span class="asleep">asleep${WK.targets.includes(id) ? ", waking when it next checks in" : ""}</span>${latText(id)}`;
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
  else if (show === "lantern") ids = ids.filter(id => !isPerim(id));
  else if (show === "perimeter") ids = ids.filter(isPerim);
  else if (show !== "all") ids = ids.filter(id => meta(id).type === show);
  if (sort === "hub") ids.sort((a, b) => ((live(a) || {}).hub || "~").localeCompare((live(b) || {}).hub || "~"));
  else if (sort === "name") ids.sort((a, b) => name(a).localeCompare(name(b)));
  return ids;
}
function renderCards() {
  $("cards").innerHTML = cardIds().map(id => {
    const m = meta(id);
    return `<div class="card ${online(id) ? "" : "offline"}" data-card="${id}">
      <div class="name">${toneBadge(id)}${esc(name(id))} ${roleTag(id)}</div><div class="meta" data-meta="${id}">${metaText(id)}</div>
      <div class="row"><span class="seg" role="group" aria-label="Role">
          <button class="${isPerim(id) ? "" : "on"}" data-role="${id}:lantern" title="A regular lantern">Lantern</button>
          <button class="${isPerim(id) ? "on" : ""}" data-role="${id}:perimeter" title="A perimeter light: its sensor is used for approach rules">Perimeter</button></span></div>
      <div class="row"><button data-strike="${id}">Strike</button>
        <button data-wake="${id}" class="${WK.targets.includes(id) ? "waking" : ""}">${wakeLabel(id)}</button>
        <button class="${m.type === "large" ? "on" : ""}" data-type="${id}:large">Large</button>
        <button class="${m.type === "small" ? "on-s" : ""}" data-type="${id}:small">Small</button>
        <label class="status">pulse <input type="number" min="${CFG.pulse_min}" max="${CFG.pulse_max}" step="1" style="width:56px" placeholder="${S.pulse_ms}" data-pulse="${id}" value="${m.pulse_ms ?? ""}"> ms</label></div>
      <input type="text" placeholder="Tone notes, e.g. bright, slight beat" data-note="${id}" value="${esc(m.note)}"></div>`;
  }).join("") || '<p class="status">No lanterns match.</p>';
}
function refreshMeta() {
  document.querySelectorAll("[data-meta]").forEach(el => { el.innerHTML = metaText(el.dataset.meta); });
  document.querySelectorAll("[data-wake]").forEach(b => {
    const id = b.dataset.wake; b.textContent = wakeLabel(id); b.classList.toggle("waking", WK.targets.includes(id));
  });
  document.querySelectorAll(".card[data-card]").forEach(c => c.classList.toggle("offline", !online(c.dataset.card)));
  const n = L.filter(l => !l.offline).length;
  $("count").textContent = `${n} online · ${S.order.length} known`;
  $("wakeinfo").textContent = WK.targets.length ? `waking ${WK.targets.length}: ${WK.targets.map(name).join(", ")}` : (WK.keep ? "keeping awake" : "");
}
["filter", "sort", "show"].forEach(k => $(k).addEventListener("input", renderCards));

// ---------- Tone order tab ----------
function renderRanks() {
  const rank = {}, size = {}, pos = {};
  toneGroups().forEach((g, k) => g.forEach((id, j) => { rank[id] = k + 1; size[id] = g.length; pos[id] = j; }));
  $("ranks").innerHTML = S.order.map((id, i) => {
    const l = live(id), m = meta(id), inGrp = size[id] > 1;
    const cls = [online(id) ? "" : "offline", inGrp ? "grp " + groupColor(id) : "", inGrp && pos[id] === 0 ? "gstart" : "",
                 inGrp && pos[id] === size[id] - 1 ? "gend" : ""].join(" ");
    const linkBtn = `<input type="checkbox" data-sel="${id}" ${SEL.has(id) ? "checked" : ""} aria-label="Select ${esc(name(id))}">`;
    return `<li draggable="true" data-i="${i}" data-card="${id}" class="${cls}">
      ${linkBtn}<span class="k">${i + 1}</span><span>${esc(name(id))} ${roleTag(id)}${tag(m.type)}${inGrp && pos[id] === 0 ? `<span class="glabel">same tone ×${size[id]}</span>` : ""}</span>
      <span class="dim">${l ? "hub " + l.hub : "not connected"}</span>
      <span class="dim">${pulseFor(id)} ms</span>
      <span class="dim">${m.latency_ms != null ? Math.round(m.latency_ms) + " ms lat" : ""}</span>
      <button data-gstrike="${id}" title="${inGrp ? "Strike the whole same-tone group together" : "Strike"}" aria-label="Strike ${esc(name(id))}">▶</button></li>`;
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
  const old = toneGroups();
  const [x] = S.order.splice(dragI, 1); S.order.splice(+li.dataset.i, 0, x);
  keepGroups(old, [x]); // a dragged lantern leaves its group; re-link it with ≈ if needed
  dragI = null; render(); save();
});
$("ranks").addEventListener("dragend", () => { dragI = null; render(); });
$("set").onclick = e => { const b = e.target.closest("button"); if (b) { UI.set = b.dataset.set; renderRanks(); } };
$("rev").onclick = () => { const old = toneGroups(); S.order.reverse(); keepGroups(old); render(); save(); };
$("grp").onclick = () => {
  const r = t => t === "large" ? 0 : t === "small" ? 2 : 1;
  const old = toneGroups();
  S.order = S.order.map((id, i) => [id, i]).sort((a, b) => r(meta(a[0]).type) - r(meta(b[0]).type) || a[1] - b[1]).map(x => x[0]);
  keepGroups(old); render(); save();
};
// Browser dialogs (prompt/confirm) are blocked in the app's built-in browser
// pane, so destructive actions confirm with a second click on the same button.
function confirmClick(btn, label, action) {
  if (btn.dataset.armed === "1") {
    clearTimeout(btn._t); btn.dataset.armed = ""; btn.textContent = btn.dataset.label; btn.classList.remove("confirm");
    action(); return;
  }
  btn.dataset.label = btn.textContent; btn.dataset.armed = "1"; btn.textContent = label; btn.classList.add("confirm");
  btn._t = setTimeout(() => { btn.dataset.armed = ""; btn.textContent = btn.dataset.label; btn.classList.remove("confirm"); }, 3000);
}
function renderPresets() {
  const v = $("psel").value;
  $("psel").innerHTML = '<option value="">Saved orders…</option>' +
    Object.keys(S.presets).map(k => `<option ${k === v ? "selected" : ""}>${esc(k)}</option>`).join("");
}
$("psave").onclick = () => {
  const n = $("pname").value.trim();
  if (!n) { log("type a name for this order first", "bad"); $("pname").focus(); return; }
  const existed = !!S.presets[n];
  S.presets[n] = {order: S.order.slice(), tone_links: (S.tone_links || []).slice()}; renderPresets(); $("psel").value = n; $("pname").value = ""; save(true);
  log(`${existed ? "updated" : "saved"} order "${n}"`, "ok");
};
$("pname").addEventListener("keydown", e => { if (e.key === "Enter") $("psave").click(); });
$("pload").onclick = () => {
  const n = $("psel").value; if (!n || !S.presets[n]) return;
  const saved = Array.isArray(S.presets[n]) ? {order: S.presets[n], tone_links: []} : S.presets[n]; // older saves were plain lists
  const rest = S.order.filter(id => !saved.order.includes(id));
  S.order = saved.order.concat(rest);
  S.tone_links = (saved.tone_links || []).filter(id => S.order.includes(id));
  render(); save(); log(`loaded order "${n}"`);
};
$("pdel").onclick = () => {
  const n = $("psel").value; if (!n) { log("pick a saved order to delete", "bad"); return; }
  confirmClick($("pdel"), "Click again to delete", () => { delete S.presets[n]; renderPresets(); save(true); log(`deleted order "${n}"`); });
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
  else if (b.dataset.wake) { await requestWake([b.dataset.wake]); }
  else if (b.dataset.gstrike) { busy = true; await playGroup(b.dataset.gstrike); busy = false; }
  else if (b.dataset.role) {
    const [id, r] = b.dataset.role.split(":"), m = meta(id);
    if (r === "perimeter") m.role = "perimeter"; else delete m.role;
    render(); save(); log(`${name(id)} is now a ${r === "perimeter" ? "perimeter light" : "lantern"}`);
  }
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
  // Same-tone groups always strike together on one beat.
  const r = await api("/api/play", {order: ids, gap_ms: g, pulses, comp: compOn(), measure: $("meas").checked,
                                    slots: groupSlots(ids)});
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

// ---------- wake / keep awake ----------
async function requestWake(ids) {
  const r = await api("/api/wake", {ids});
  WK = {keep: r.keep, targets: r.targets};
  log(ids === "all" ? "waking all lanterns: each is caught when it next wakes (up to ~2 min)"
                    : `waking ${ids.map(name).join(", ")} (up to ~2 min)`);
  refreshMeta();
}
$("wakeall").onclick = () => requestWake("all");
$("keepawake").onchange = async e => {
  const r = await api("/api/wake", {keep: e.target.checked}); WK = {keep: r.keep, targets: r.targets}; refreshMeta();
};
// Live view: lantern status and wake events refresh every few seconds without
// re-rendering inputs, so typing a pulse or note is never interrupted.
async function liveRefresh() {
  try {
    const w = await api("/api/wake?since=" + wkSince);
    WK = {keep: w.keep, targets: w.targets}; $("keepawake").checked = w.keep;
    for (const ev of w.events) {
      wkSince = Math.max(wkSince, ev.id);
      log(ev.fixture_id ? `${name(ev.fixture_id)}: ${ev.msg}` : ev.msg, ev.ok ? "ok" : "bad");
    }
    // Mirror the server's proximity switch so the button can never show the wrong state.
    const px = await api("/api/prox?since=" + pxSince);
    if (px.on !== PX.on) {
      PX = px; setProxBtn();
      if (px.on && !pxTimer) pxTimer = setInterval(pollProx, 300);
      if (!px.on && pxTimer) { clearInterval(pxTimer); pxTimer = null; }
      log(`proximity ding is ${px.on ? "on" : "off"}`);
    }
    const r = await api("/api/snapshot");
    if (r.lanterns) {
      const before = new Set(L.filter(l => !l.offline).map(l => l.fixture_id));
      L = r.lanterns; syncOrder();
      const after = new Set(L.filter(l => !l.offline).map(l => l.fixture_id));
      const changed = before.size !== after.size || [...after].some(id => !before.has(id));
      if (changed && UI.tab !== "lanterns" && !paint) render(); else refreshMeta();
    }
  } catch (e) { /* server restarting */ }
}
setInterval(liveRefresh, 3000);

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
function setProxBtn() {
  $("prox").textContent = "Proximity: " + (!PX.on ? "off" : PX.silent ? "watch only" : "ding");
  $("prox").classList.toggle("on", PX.on);
  if (UI.tab === "proximity") pxControls();
}
async function pollProx() {
  let r;
  try { r = await api("/api/prox?since=" + pxSince); } catch (e) { return; } // server restarting
  PX = r;
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

// ---------- Proximity tab ----------
// Live per-lantern sensor charts. The server keeps the last ~50 s of raw readings
// per lantern; this pulls only new samples every 250 ms and draws the last 15 s.
const PXV = {buf: {}, since: 0, evSince: 0, events: [], dings: {}, serverT: 0, timer: null};
function pxControls() {
  const mode = !PX.on ? "off" : PX.silent ? "watch" : "ding";
  document.querySelectorAll("#pxmode button").forEach(b => b.classList.toggle("on", b.dataset.mode === mode));
  if (PX.range_mm) { $("pxrange").value = PX.range_mm; $("pxrangev").textContent = Math.round(PX.range_mm / 10); }
}
function renderProxTab() {
  pxControls();
  $("pxgrid").innerHTML = S.order.map(id => `<div class="pxcard" data-pxcard="${id}">
      <div class="hdr"><span class="nm">${toneBadge(id)}${esc(name(id))} ${roleTag(id)}</span><span class="pxbadge" data-pxbadge="${id}">–</span>
        <button data-pxrelearn="${id}" title="Relearn this lantern's background">Relearn</button></div>
      <div class="pxbody"><canvas data-px="${id}"></canvas><div class="zgrid" data-pxz="${id}" title="The sensor's 3x3 zones: distance each sees (cm). Red = blocked by something fixed (ignored); green = hand."></div></div>
      <div class="st" data-pxst="${id}"></div></div>`).join("");
  PXV.timer = PXV.timer || setInterval(proxTick, 250);
  pxDrawAll();
}
async function proxTick() {
  if (UI.tab !== "proximity") return;
  try {
    const tr = await api("/api/proxtrace?since=" + PXV.since);
    PXV.serverT = tr.server_t;
    for (const [id, rows] of Object.entries(tr.trace)) {
      const b = PXV.buf[id] = (PXV.buf[id] || []).concat(rows);
      for (const r of rows) PXV.since = Math.max(PXV.since, r[0]);
      while (b.length && b[0][0] < PXV.serverT - 20) b.shift();
    }
    const px = await api("/api/prox?since=" + PXV.evSince);
    const wasOn = PX.on, wasSilent = PX.silent;
    PX = px;
    if (wasOn !== px.on || wasSilent !== px.silent) setProxBtn();
    for (const ev of px.events) {
      PXV.evSince = Math.max(PXV.evSince, ev.id);
      PXV.events.push(ev);
      PXV.dings[ev.fixture_id] = (PXV.dings[ev.fixture_id] || 0) + 1;
      const c = document.querySelector(`[data-pxcard="${ev.fixture_id}"]`);
      if (c) { c.classList.add("ding"); setTimeout(() => c.classList.remove("ding"), 500); }
    }
    PXV.events = PXV.events.filter(e => e.t > PXV.serverT - 20);
  } catch (e) { /* server restarting */ }
  pxDrawAll();
}
function pxDrawAll() {
  const css = getComputedStyle(document.documentElement), col = k => css.getPropertyValue(k).trim();
  const C = {muted: col("--muted"), text: col("--text"), ok: col("--ok"), bad: col("--bad"), warn: col("--warn"),
             line: col("--line"), accent: col("--accent")};
  let watching = 0, hands = 0, blocked = 0;
  S.order.forEach(id => {
    const l = live(id), v = PX.live && PX.live[id];
    const badge = document.querySelector(`[data-pxbadge="${id}"]`), st = document.querySelector(`[data-pxst="${id}"]`);
    if (!badge) return;
    let label = "off", cls = "";
    if (!l || l.offline) label = l ? "asleep" : "not connected";
    else if (!l.tmf) label = "no sensor";
    else if (PX.on && v) { label = v.status; cls = v.status; watching++; if (v.status === "hand") hands++; if (v.status === "blocked") blocked++; }
    else if (PX.on) label = "starting";
    badge.textContent = label; badge.className = "pxbadge " + cls;
    const bgTxt = v && v.background_mm ? ` · background ${Math.round(v.background_mm / 10)} cm` : (v && PX.on ? " · background clear" : "");
    const now = v && v.depth_mm && v.confidence >= (PX.learn_confidence || 20)
      ? `sees ${Math.round(v.depth_mm / 10)} cm (confidence ${v.confidence})` : (v ? "sees nothing" : "");
    const nb = v && v.zones ? v.zones.filter(z => z.state === "blocked").length : 0;
    st.textContent = (l && !l.tmf) ? "This lantern has no distance sensor."
      : nb && v.status !== "blocked" ? `${now}${bgTxt} · ${nb} of 9 zones blocked (ignored)${PXV.dings[id] ? ` · ${PXV.dings[id]} ding${PXV.dings[id] > 1 ? "s" : ""}` : ""}`
      : `${now}${bgTxt}${PXV.dings[id] ? ` · ${PXV.dings[id]} ding${PXV.dings[id] > 1 ? "s" : ""}` : ""}`;
    pxDraw(id, C, v);
    const zg = document.querySelector(`[data-pxz="${id}"]`);
    if (zg) zg.innerHTML = v && v.zones ? v.zones.map(z => {
      const seen = z.mm && z.conf >= (PX.learn_confidence || 20);
      const cls = z.state === "hand" ? "hand" : z.state === "blocked" ? "blocked" : seen ? "seen" : "";
      return `<div class="${cls}" title="zone: ${seen ? Math.round(z.mm / 10) + " cm, confidence " + z.conf : "nothing"}${z.bg ? ", background " + Math.round(z.bg / 10) + " cm" : ""}${z.state === "blocked" ? " (blocked, ignored)" : ""}">${seen ? Math.round(z.mm / 10) : "·"}</div>`;
    }).join("") : "";
  });
  $("pxsummary").textContent = PX.on ? `${watching} sensors watched · ${hands} seeing a hand · ${blocked} blocked` : "";
}
function pxDraw(id, C, v) {
  const cv = document.querySelector(`canvas[data-px="${id}"]`);
  if (!cv) return;
  const dpr = window.devicePixelRatio || 1, w = cv.clientWidth, h = cv.clientHeight;
  if (cv.width !== Math.round(w * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
  const ctx = cv.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  const span = 15, T = PXV.serverT || Date.now() / 1000, range = PX.range_mm || 600;
  const maxMm = Math.max(1500, range + 300);
  const x = t => (t - (T - span)) / span * w, y = mm => 12 + mm / maxMm * (h - 18);
  ctx.font = "10px system-ui"; ctx.fillStyle = C.muted; ctx.strokeStyle = C.line; ctx.lineWidth = 1;
  for (let mm = 500; mm < maxMm; mm += 500) {           // distance grid, sensor at the top
    ctx.beginPath(); ctx.moveTo(0, y(mm)); ctx.lineTo(w, y(mm)); ctx.stroke();
    ctx.fillText(`${mm / 10} cm`, 3, y(mm) - 2);
  }
  ctx.fillText("sensor ▾", w - 48, 10);
  const hline = (mm, color, dash) => {
    ctx.save(); ctx.strokeStyle = color; ctx.setLineDash(dash || []); ctx.lineWidth = 1.5;
    ctx.beginPath(); ctx.moveTo(0, y(mm)); ctx.lineTo(w, y(mm)); ctx.stroke(); ctx.restore();
  };
  hline(range, C.ok, [5, 4]);
  if (v && v.background_mm) {
    hline(v.background_mm, C.muted);
    const thr = v.background_mm - (PX.delta_mm || 300);
    if (thr > 0) hline(thr, C.warn, [3, 3]);
  }
  for (const ev of PXV.events.filter(e => e.fixture_id === id)) {
    ctx.save(); ctx.strokeStyle = ev.silent ? C.accent : C.bad; ctx.setLineDash(ev.silent ? [4, 3] : []); ctx.lineWidth = 2;
    ctx.beginPath(); ctx.moveTo(x(ev.t), 0); ctx.lineTo(x(ev.t), h); ctx.stroke(); ctx.restore();
  }
  const minC = PX.min_confidence || 40, learnC = PX.learn_confidence || 20;
  for (const [t, d, c, state] of PXV.buf[id] || []) {
    if (!d || c < learnC || t < T - span) continue;
    ctx.globalAlpha = c >= minC ? 1 : 0.35;
    ctx.fillStyle = state === "hand" ? C.ok : C.text;
    ctx.beginPath(); ctx.arc(x(t), y(Math.min(d, maxMm)), c >= minC ? 3 : 2.2, 0, Math.PI * 2); ctx.fill();
  }
  ctx.globalAlpha = 1;
}
$("pxmode").onclick = async e => {
  const b = e.target.closest("button"); if (!b) return;
  const m = b.dataset.mode, range_mm = +$("pxrange").value;
  const r = await api("/api/proximity", m === "off" ? {on: false} : {on: true, silent: m === "watch", range_mm});
  PX = r; setProxBtn();
  if (r.on && !pxTimer) pxTimer = setInterval(pollProx, 300);
  log(m === "off" ? "proximity off" : `proximity ${m === "watch" ? "watch only (no strikes)" : "ding"}: learning backgrounds for 1.5 s, keep hands away`);
};
$("pxrange").oninput = e => { $("pxrangev").textContent = Math.round(e.target.value / 10); };
$("pxrange").onchange = e => { if (PX.on) api("/api/proximity", {on: true, range_mm: +e.target.value}).then(r => { PX = r; }); };
$("pxrelearn").onclick = async () => {
  if (!PX.on) { log("turn proximity on (watch only or ding) first", "bad"); return; }
  PX = await api("/api/proximity", {on: true, relearn: "all"}); log("relearning every background for 1.5 s: keep hands away");
};
document.addEventListener("click", async e => {
  const b = e.target.closest("[data-pxrelearn]"); if (!b) return;
  if (!PX.on) { log("turn proximity on (watch only or ding) first", "bad"); return; }
  PX = await api("/api/proximity", {on: true, relearn: [b.dataset.pxrelearn]});
  log(`${name(b.dataset.pxrelearn)}: relearning background for 1.5 s, keep hands away`);
});

// ---------- Interactions tab ----------
// A rule maps one lantern's sensor distance onto a pattern's BPM. Rules save
// with the tone map; one rule at a time can be armed (runs on the server).
UI.ix = lsGet("ix");
let IX = {armed: false}, ixTimer = null, ixSimTimer = null;
function ixPatterns() { return Object.values(S.patterns).map(normalize); }
function newRule(nm) {
  const src = S.order.find(id => (live(id) || {}).tmf) || S.order[0];
  const pat = ixPatterns()[0];
  return {id: "ix-" + Math.random().toString(16).slice(2, 8), name: nm, source: src, far_mm: 1500, near_mm: 200,
          pattern_id: pat ? pat.id : null, bpm_from: 60, bpm_to: 180, curve: "linear", smooth_s: 0.5, idle: "stop", idle_s: 3};
}
function curRule() {
  S.interactions = S.interactions || [];
  if (!S.interactions.length) { S.interactions.push(newRule("Approach speeds up")); save(); }
  let r = S.interactions.find(x => x.id === UI.ix);
  if (!r) { r = S.interactions[0]; UI.ix = r.id; }
  return r;
}
// Fastest BPM where no lantern's hits are under the minimum gap (mirrors the server).
function bpmMax(p) {
  if (!p) return 300;
  let best = 300;
  for (const raw of Object.values(p.hits || {})) {
    const idx = [...new Set(raw)].filter(i => i < p.steps).sort((a, b) => a - b);
    if (idx.length < 2) continue;
    let k = p.steps - idx[idx.length - 1] + idx[0];
    for (let j = 1; j < idx.length; j++) k = Math.min(k, idx[j] - idx[j - 1]);
    best = Math.min(best, k * 60000 / (p.steps_per_beat * CFG.min_gap));
  }
  return best;
}
const ixShape = (x, c) => { x = Math.max(0, Math.min(1, x)); return c === "ease_in" ? x * x : c === "ease_out" ? 1 - (1 - x) * (1 - x) : x; };
const CLICK_TXT = {single: "Single click", double: "Double click", any: "Single or double click"};
const patName = id => (ixPatterns().find(p => p.id === id) || {}).name || "(no pattern)";
const ixRunning = id => IX.armed && IX.rule_id === id;
// One plain-English line per rule, for the list.
function ruleSummary(r) {
  if (r.kind === "button") {
    const a = r.action, sens = S.interactions.find(x => x.id === a.interaction_id);
    const did = {
      performance: `play ${patName(a.pattern_id)} ${a.loops || 2}×, then go back to what was running`,
      next_active_pattern: `switch ${sens ? `"${sens.name}"` : "the running sensor rule"} to the next Active pattern`,
      play_pattern: `play ${patName(a.pattern_id)} on loop`,
      next_pattern: `play the next of ${(a.pattern_ids || []).length} patterns`,
      stop: "stop everything",
      arm_interaction: `start sensor rule ${sens ? `"${sens.name}"` : "(pick one)"}`,
      toggle_proximity: "turn proximity ding on/off",
    }[a.type] || a.type;
    return `${CLICK_TXT[r.trigger.click || "single"]} on ${atomName(r.trigger.atom)} → ${did}`;
  }
  return `${name(r.source)}'s sensor, ${Math.round(r.far_mm / 10)}→${Math.round(r.near_mm / 10)} cm: ${patName(r.pattern_id)} speeds up ${r.bpm_from}→${r.bpm_to} BPM`;
}
function renderIxList() {
  const cur = curRule();
  $("ixlist").innerHTML = S.interactions.map(r => {
    const btn = r.kind === "button", run = !btn && ixRunning(r.id), off = btn && r.enabled === false;
    const ctl = btn ? `<label title="A turned-off rule ignores clicks"><input type="checkbox" data-ixon="${r.id}" ${off ? "" : "checked"}> on</label>`
      : run ? `<span class="run">● running</span><button data-ixstop="${r.id}">■ Stop</button>` : `<button data-ixstart="${r.id}">▶ Start</button>`;
    return `<div class="ixrow ${r.id === cur.id ? "sel" : ""} ${off ? "off" : ""}" data-ixrow="${r.id}">
      <span class="kind">${btn ? "button" : "sensor"}</span>
      <div class="what"><b>${esc(r.name)}</b><span class="status">${esc(ruleSummary(r))}</span></div>${ctl}</div>`;
  }).join("");
  const sig = S.interactions.map(r => r.id).join() + "|" + (IX.armed ? IX.rule_id : "");
  renderIxList.sig = sig;
}
$("ixlist").addEventListener("click", async e => {
  const t = e.target;
  if (t.dataset.ixon) return;
  if (t.dataset.ixstart) { await armRule(S.interactions.find(x => x.id === t.dataset.ixstart)); return; }
  if (t.dataset.ixstop) { await api("/api/interaction/disarm", {}); log("sensor rule stopped"); return; }
  const row = t.closest("[data-ixrow]"); if (!row) return;
  UI.ix = row.dataset.ixrow; lsSet("ix", UI.ix); renderIx();
});
$("ixlist").addEventListener("change", e => {
  const id = e.target.dataset.ixon; if (!id) return;
  const r = S.interactions.find(x => x.id === id); r.enabled = e.target.checked; save(); renderIx();
});
function renderIx() {
  const r0 = curRule();
  $("ixbtnwrap").hidden = r0.kind !== "button"; $("ixsensorwrap").hidden = r0.kind === "button";
  $("ixkind").textContent = r0.kind === "button" ? "Editing button rule" : "Editing sensor rule";
  renderIxList(); renderBxPanel();
  if (document.activeElement !== $("ixname")) $("ixname").value = r0.name;
  ixTimer = ixTimer || setInterval(ixPoll, 150);
  bxTimer = bxTimer || setInterval(bxPoll, 400);
  if (r0.kind === "button") { renderBx(); return; }
  const r = r0, pats = ixPatterns();
  const srcs = S.order.filter(id => !live(id) || live(id).tmf);
  srcs.sort((a, b) => isPerim(b) - isPerim(a));
  $("ixsrc").innerHTML = srcs.map(id =>
    `<option value="${id}" ${id === r.source ? "selected" : ""}>#${S.order.indexOf(id) + 1} ${esc(name(id))}${isPerim(id) ? " (perimeter)" : ""}</option>`).join("");
  $("ixpat").innerHTML = pats.length ? pats.map(p => `<option value="${p.id}" ${p.id === r.pattern_id ? "selected" : ""}>${esc(p.name)}</option>`).join("")
    : '<option value="">make a pattern first</option>';
  if (!r.pattern_id && pats[0]) r.pattern_id = pats[0].id;
  $("ixfar").value = Math.round(r.far_mm / 10); $("ixnear").value = Math.round(r.near_mm / 10);
  $("ixbfrom").value = r.bpm_from; $("ixbto").value = r.bpm_to; $("ixcurve").value = r.curve;
  $("ixsmooth").value = r.smooth_s; $("ixidles").value = r.idle_s; $("ixidle").value = r.idle;
  $("ixsim").max = Math.round(Math.max(r.far_mm * 1.2, 300) / 10);
  ixWarn();
  ixTimer = ixTimer || setInterval(ixPoll, 150);
  ixDraw();
}
function ixWarn() {
  const r = curRule(), p = ixPatterns().find(x => x.id === r.pattern_id), cap = bpmMax(p), msgs = [];
  if (!p) msgs.push("Pick a pattern.");
  else {
    msgs.push(`"${p.name}" can go up to ${Math.round(cap)} BPM safely (250 ms per lantern).`);
    if (r.bpm_to > cap) msgs.push(`The near BPM (${r.bpm_to}) will be capped at ${Math.round(cap)}.`);
    const slow = {...p, bpm: Math.min(r.bpm_from, r.bpm_to)};
    if (Object.keys(violations(slow)).length) msgs.push(`At ${slow.bpm} BPM some lanterns are already too fast; slow it down or edit the pattern.`);
  }
  if (r.near_mm >= r.far_mm) msgs.push("Near must be closer than far.");
  if (IX.armed && IX.rule_id === r.id) msgs.push("Running: Stop and Start again to apply changes.");
  $("ixwarn").textContent = msgs.join(" ");
}
function ixField(id, key, scale, parse) {
  $(id).addEventListener("change", e => {
    const r = curRule(); r[key] = parse ? parse(e.target.value) * (scale || 1) : e.target.value;
    save(); ixWarn(); ixDraw(); renderIxList();
  });
}
ixField("ixfar", "far_mm", 10, Number); ixField("ixnear", "near_mm", 10, Number);
ixField("ixbfrom", "bpm_from", 1, Number); ixField("ixbto", "bpm_to", 1, Number);
ixField("ixsmooth", "smooth_s", 1, Number); ixField("ixidles", "idle_s", 1, Number);
ixField("ixcurve", "curve"); ixField("ixidle", "idle"); ixField("ixsrc", "source"); ixField("ixpat", "pattern_id");
function ixUnique(base) { let n = base, k = 2; while (S.interactions.some(x => x.name === n)) n = `${base} ${k++}`; return n; }
$("ixnew").onclick = () => { const r = newRule(ixUnique("Interaction")); S.interactions.push(r); UI.ix = r.id; lsSet("ix", r.id); renderIx(); save(true); $("ixname").focus(); $("ixname").select(); };
$("ixdup").onclick = () => { const c = curRule(), r = {...JSON.parse(JSON.stringify(c)), id: "ix-" + Math.random().toString(16).slice(2, 8), name: ixUnique(c.name + " copy")};
  S.interactions.push(r); UI.ix = r.id; lsSet("ix", r.id); renderIx(); save(true); };
$("ixdel").onclick = () => confirmClick($("ixdel"), "Click again to delete", () => {
  const r = curRule(); S.interactions = S.interactions.filter(x => x.id !== r.id); UI.ix = null; renderIx(); save(true); log(`deleted interaction "${r.name}"`);
});
function ixRename() {
  const r = curRule(), nm = $("ixname").value.trim();
  if (!nm || nm === r.name) { $("ixname").value = r.name; return; }
  r.name = nm; renderIx(); save(true);
}
$("ixname").addEventListener("keydown", e => { if (e.key === "Enter") { e.preventDefault(); ixRename(); } });
$("ixname").addEventListener("blur", ixRename);
async function armRule(r) {
  const res = await api("/api/interaction/arm", {rule: r});
  if (!res.ok) { log(res.error || "could not start the rule", "bad"); return; }
  log(`started "${r.name}": ${name(r.source)}'s sensor drives "${res.pattern}" (${r.bpm_from}→${Math.min(r.bpm_to, res.bpm_max)} BPM)`, "ok");
}
$("ixarm").onclick = () => armRule(curRule());
$("ixdisarm").onclick = async () => { await api("/api/interaction/disarm", {}); log("sensor rule stopped"); };
function ixSendSim() {
  clearTimeout(ixSimTimer);
  ixSimTimer = setTimeout(() => api("/api/interaction/sim", {on: $("ixsimon").checked, mm: +$("ixsim").value * 10}), 60);
}
$("ixsimon").onchange = ixSendSim;
$("ixsim").oninput = e => { $("ixsimv").textContent = e.target.value + " cm"; if ($("ixsimon").checked) ixSendSim(); };
$("ixnobody").onclick = () => { $("ixsimon").checked = true; $("ixsimv").textContent = "nobody"; api("/api/interaction/sim", {on: true, mm: null}); };
async function ixPoll() {
  if (UI.tab !== "interactions") return;
  try { IX = await api("/api/interaction/status"); } catch (e) { return; }
  if (renderIxList.sig !== S.interactions.map(r => r.id).join() + "|" + (IX.armed ? IX.rule_id : "")) renderIxList();
  $("ixstate").textContent = IX.armed ? `running: "${IX.rule}" → "${IX.pattern}"${IX.playing === false ? " (idle, nobody there)" : ""} · ${IX.hits || 0} hits` +
    (IX.skipped ? ` · ${IX.skipped} skipped (250 ms limit)` : "") + (IX.error ? ` · ${IX.error}` : "") : (IX.error ? `stopped: ${IX.error}` : "not running");
  $("ixarm").disabled = ixRunning(curRule().id);
  $("ixdist").textContent = IX.armed ? (IX.distance_mm != null ? Math.round(IX.distance_mm / 10) + " cm" : "nobody") : "–";
  $("ixbpm").textContent = IX.armed && IX.bpm != null ? Math.round(IX.bpm) : "–";
  $("ixtarget").textContent = IX.armed && IX.target_bpm != null ? Math.round(IX.target_bpm) : "–";
  ixDraw();
}
function ixDraw() {
  const cv = $("ixplot"); if (!cv || UI.tab !== "interactions") return;
  const r = curRule(), p = ixPatterns().find(x => x.id === r.pattern_id), cap = bpmMax(p);
  const css = getComputedStyle(document.documentElement), col = k => css.getPropertyValue(k).trim();
  const dpr = window.devicePixelRatio || 1, w = cv.clientWidth, h = cv.clientHeight;
  if (cv.width !== Math.round(w * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
  const ctx = cv.getContext("2d"); ctx.setTransform(dpr, 0, 0, dpr, 0, 0); ctx.clearRect(0, 0, w, h);
  const maxD = Math.max(r.far_mm * 1.2, 300), lo = Math.min(r.bpm_from, r.bpm_to), hi = Math.max(r.bpm_from, r.bpm_to, Math.min(cap, 300));
  const yMin = Math.max(0, lo - 15), yMax = hi + 15, pad = 26;
  const x = d => pad + (1 - d / maxD) * (w - pad - 8);            // far on the left, near on the right
  const y = b => h - 18 - (b - yMin) / (yMax - yMin) * (h - 28);
  ctx.font = "10px system-ui"; ctx.fillStyle = col("--muted"); ctx.strokeStyle = col("--line");
  for (const cm of [0, 100, 200, 300, 400, 500]) if (cm * 10 <= maxD) { ctx.beginPath(); ctx.moveTo(x(cm * 10), 8); ctx.lineTo(x(cm * 10), h - 18); ctx.stroke(); ctx.fillText(cm + " cm", x(cm * 10) - 12, h - 5); }
  ctx.fillText(Math.round(yMax) + " BPM", 2, 12); ctx.fillText(Math.round(yMin), 2, h - 20);
  if (cap < yMax) { ctx.save(); ctx.strokeStyle = col("--bad"); ctx.setLineDash([4, 3]); ctx.beginPath(); ctx.moveTo(pad, y(cap)); ctx.lineTo(w - 8, y(cap)); ctx.stroke(); ctx.restore(); ctx.fillStyle = col("--bad"); ctx.fillText(`max ${Math.round(cap)}`, w - 52, y(cap) - 3); }
  ctx.strokeStyle = col("--accent"); ctx.lineWidth = 2; ctx.beginPath();
  for (let i = 0; i <= 100; i++) {
    const d = maxD * i / 100, t = ixShape((r.far_mm - d) / Math.max(1, r.far_mm - r.near_mm), r.curve);
    const b = Math.min(r.bpm_from + (r.bpm_to - r.bpm_from) * t, cap);
    i ? ctx.lineTo(x(d), y(b)) : ctx.moveTo(x(d), y(b));
  }
  ctx.stroke(); ctx.lineWidth = 1;
  if (IX.armed && IX.distance_mm != null && IX.bpm != null) {
    ctx.fillStyle = col("--ok"); ctx.beginPath(); ctx.arc(x(Math.min(IX.distance_mm, maxD)), y(IX.bpm), 6, 0, Math.PI * 2); ctx.fill();
  }
}

// ---- button rules (Atom presses) ----
let warn0 = "";
let BX = {relay: false, atoms: {}, events: [], listener: null, learn: false}, bxSince = 0, bxTimer = null, bxLog = [];
function newButtonRule(nm) {
  const pat = ixPatterns()[0];
  return {kind: "button", id: "ix-" + Math.random().toString(16).slice(2, 8), name: nm, enabled: true,
          trigger: {atom: "any"}, action: {type: "play_pattern", pattern_id: pat ? pat.id : null, pattern_ids: [], interaction_id: null}};
}
const atomName = id => id === "any" ? "any Atom" : (BX.atoms[id] || S.atoms && S.atoms[id] || `Atom ${id}`);
function renderBx() {
  const r = curRule(), a = r.action, pats = ixPatterns();
  const known = new Set([...Object.keys(BX.atoms || {}), ...(r.trigger.atom !== "any" ? [r.trigger.atom] : []),
                         ...bxLog.filter(e => e.atom).map(e => e.atom)]);
  $("bxatom").innerHTML = `<option value="any" ${r.trigger.atom === "any" ? "selected" : ""}>any Atom</option>` +
    [...known].map(id => `<option value="${id}" ${id === r.trigger.atom ? "selected" : ""}>${esc(atomName(id))}</option>`).join("");
  $("bxaction").value = a.type;
  $("bxclick").value = r.trigger.click || "single";
  $("bxpattern").hidden = !["play_pattern", "performance"].includes(a.type);
  $("bxloopswrap").hidden = a.type !== "performance"; $("bxloops").value = a.loops || 2;
  $("bxix").hidden = !["arm_interaction", "next_active_pattern"].includes(a.type);
  $("bxpattern").innerHTML = pats.map(p => `<option value="${p.id}" ${p.id === a.pattern_id ? "selected" : ""}>${esc(p.name)}</option>`).join("") || '<option value="">make a pattern first</option>';
  const sensors = S.interactions.filter(x => x.kind !== "button");
  $("bxix").innerHTML = (a.type === "next_active_pattern" ? `<option value="" ${!a.interaction_id ? "selected" : ""}>whichever sensor rule is running</option>` : "") +
    (sensors.map(x => `<option value="${x.id}" ${x.id === a.interaction_id ? "selected" : ""}>${esc(x.name)}</option>`).join("") || '<option value="">make a sensor rule first</option>');
  if (["play_pattern", "performance"].includes(a.type) && !a.pattern_id && pats[0]) a.pattern_id = pats[0].id;
  if (a.type === "arm_interaction" && !a.interaction_id && sensors[0]) a.interaction_id = sensors[0].id;
  if (a.type === "next_active_pattern" && !ixPatterns().some(p => p.active)) warn0 = "No patterns are Active yet: tick some on the Patterns tab.";
  $("bxlist").hidden = a.type !== "next_pattern";
  $("bxlist").innerHTML = '<span class="status">Each press plays the next of:</span>' + pats.map(p =>
    `<label><input type="checkbox" data-bxp="${p.id}" ${(a.pattern_ids || []).includes(p.id) ? "checked" : ""}> ${esc(p.name)}</label>`).join("");
  $("bxhelp").textContent = {
    performance: "Plays the pattern for that many loops, then goes back to whatever sensor rule was running (or silence).",
    next_active_pattern: "Each click moves the sensor rule to the next pattern ticked under Active patterns (Patterns tab) and keeps it running.",
    play_pattern: "Starts the pattern and keeps looping it until something else plays or stops.",
    next_pattern: "Each click plays the next pattern from the ticked list, looping.",
    stop: "Stops any pattern or sensor rule that's playing.",
    arm_interaction: "Starts that sensor rule (stopping any other one).",
    toggle_proximity: "Turns the Proximity tab's ding on or off.",
  }[a.type] || "";
  const warn = warn0 ? [warn0] : []; warn0 = "";
  if (r.enabled === false) warn.push("This rule is turned off (checkbox in the list).");
  if (a.type === "next_pattern" && !(a.pattern_ids || []).length) warn.push("Tick the patterns to step through.");
  $("bxwarn").textContent = warn.join(" ");
  renderBxPanel();
}
function renderBxPanel() {
  $("bxrelay").textContent = BX.relay ? "Atom buttons: ON" : "Atom buttons: off";
  $("bxrelay").classList.toggle("on", !!BX.relay); $("bxrelay").setAttribute("aria-pressed", BX.relay ? "true" : "false");
  $("bxmodetext").innerHTML = BX.relay
    ? "<b>Clicks run your button rules.</b> Lanterns won't play their own signature when an Atom is pressed."
    : "<b>Off:</b> pressing an Atom makes the lanterns play their signature, like at the show. Turn on to use the button rules below.";
  const last = bxLog.filter(e => e.kind === "press").slice(-1)[0];
  $("bxlistener").textContent = BX.learn ? "Detecting: press the Atom you want this rule to use…"
    : (BX.listener ? `Hearing Atoms through ${name(BX.listener)}.` : "No lantern on USB to hear Atoms.") + (last ? ` Last: ${last.text} at ${last.utc.slice(11, 19)}.` : "");
  const ids = Object.keys(BX.atoms || {}).concat(bxLog.filter(e => e.atom && !(e.atom in (BX.atoms || {}))).map(e => e.atom));
  $("bxatoms").innerHTML = [...new Set(ids)].map(id => `<div class="row"><span class="status" style="width:90px">Atom ${id}</span>
    <input type="text" data-atomname="${id}" value="${esc((BX.atoms || {})[id] || "")}" placeholder="name it, e.g. Red button" style="width:180px"></div>`).join("");
  $("bxlog").innerHTML = bxLog.slice(-12).reverse().map(e => `<div>${esc(e.utc.slice(11, 19))} ${esc(e.text)}</div>`).join("") || "No presses yet.";
}
function bxText(ev) {
  if (ev.kind === "learned") return `learned ${atomName(ev.atom)}`;
  if (ev.kind === "press") return `${atomName(ev.atom)} pressed${ev.rule ? ` → ${ev.rule}: ${ev.result}` : ` (${ev.result})`}`;
  return ev.result || ev.kind;
}
async function bxPoll() {
  let r;
  try { r = await api("/api/atom?since=" + bxSince); } catch (e) { return; }
  const wasLearning = BX.learn;
  BX = {...BX, ...r};
  for (const ev of r.events) {
    bxSince = Math.max(bxSince, ev.id);
    bxLog.push({...ev, text: bxText(ev)}); bxLog = bxLog.slice(-100);
    log(bxText(ev), ev.kind === "error" ? "bad" : "ok");
    if (ev.kind === "rule_updated") {  // the server switched a sensor rule's pattern (e.g. a double click)
      const rule = (S.interactions || []).find(x => x.id === ev.interaction_id);
      if (rule) rule.pattern_id = ev.pattern_id;
      if (UI.tab === "interactions") renderIx();
    }
    if (ev.kind === "learned" && UI.tab === "interactions") {
      const rule = curRule(); if (rule.kind === "button") { rule.trigger.atom = ev.atom; save(); }
    }
  }
  if (UI.tab === "interactions" && curRule().kind === "button" && (r.events.length || wasLearning !== BX.learn)) renderBx();
  else if (UI.tab === "interactions") renderBxPanel();
  if (UI.tab === "interactions" && r.events.length) renderIxList();
}
$("ixnewbtn").onclick = () => { const r = newButtonRule(ixUnique("Button")); S.interactions.push(r); UI.ix = r.id; lsSet("ix", r.id); renderIx(); save(true); $("ixname").focus(); $("ixname").select(); };
// Keep the list's one-line summaries current while editing (runs after the field handlers).
$("ixbtnwrap").addEventListener("change", () => renderIxList());
$("bxatom").onchange = e => { curRule().trigger.atom = e.target.value; save(); };
$("bxclick").onchange = e => { curRule().trigger.click = e.target.value; save(); };
$("bxloops").onchange = e => { const v = Math.round(+e.target.value); curRule().action.loops = v >= 1 && v <= 32 ? v : 2; save(); renderBx(); };
$("bxaction").onchange = e => { curRule().action.type = e.target.value; renderBx(); save(); };
$("bxpattern").onchange = e => { curRule().action.pattern_id = e.target.value; save(); };
$("bxix").onchange = e => { curRule().action.interaction_id = e.target.value; save(); };
$("bxlist").addEventListener("change", e => {
  const id = e.target.dataset.bxp; if (!id) return;
  const a = curRule().action, set = new Set(a.pattern_ids || []);
  if (e.target.checked) set.add(id); else set.delete(id);
  a.pattern_ids = ixPatterns().map(p => p.id).filter(x => set.has(x)); save(); renderBx();
});
$("bxlearn").onclick = async () => { BX = {...BX, ...(await api("/api/atom", {learn: true}))}; renderBxPanel(); log("press the Atom you want this rule to use now…"); };
$("bxrelay").onclick = async () => { BX = {...BX, ...(await api("/api/atom", {relay: !BX.relay}))}; renderIx(); };
$("bxatoms").addEventListener("change", async e => {
  const id = e.target.dataset.atomname; if (!id) return;
  BX = {...BX, ...(await api("/api/atom", {name: {atom: id, name: e.target.value}}))}; renderBx();
});

// ---------- Patterns tab ----------
function newPattern(nm) { return {id: patternId(), name: nm, bpm: 100, beats_per_bar: 4, bars: 1, steps_per_beat: 2, steps: 8, hits: {}}; }
const MAX_STEPS = 256; // server limit
// Stable pattern id: survives renames so triggers (proximity, Atom, standalone)
// can refer to a pattern without depending on its display name.
function patternId() { return "pat-" + Math.random().toString(16).slice(2, 8) + Date.now().toString(16).slice(-4); }
// Older patterns only had steps + steps_per_beat; derive bars/beats so the grid always adds up.
function normalize(p) {
  p.id = p.id || patternId();
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
  $("patsel").innerHTML = Object.keys(S.patterns).sort((a, b) => a.localeCompare(b))
    .map(k => `<option ${k === UI.pattern ? "selected" : ""}>${esc(k)}</option>`).join("");
  if (document.activeElement !== $("patname")) $("patname").value = p.name;
  $("patname").title = `pattern id ${p.id}`;
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
    h += `<div class="rh ${groupColor(id) ? "grp " + groupColor(id) : ""}" data-card="${id}"><span class="nm" title="${esc(id)}">${toneBadge(id)}${esc(name(id))}</span>${fwBadge(id)}${isPerim(id) ? '<span class="tag perim" title="perimeter light">P</span>' : ""}${tag(m.type)}<button data-strike="${id}" aria-label="Strike ${esc(name(id))}">▶</button></div>`;
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
  renderActive(p);
  const nHits = Object.values(p.hits).reduce((a, v) => a + v.length, 0), nBad = Object.keys(bad).length;
  const minSteps = Math.ceil(CFG.min_gap / stepMs(p) - 1e-9);
  $("patinfo").innerHTML = `loop ${(p.steps * stepMs(p) / 1000).toFixed(2)} s · step ${Math.round(stepMs(p))} ms · ${nHits} hits · ` +
    (minSteps <= 1 ? "any step allowed on one lantern" : `one lantern needs ${minSteps} steps between hits`) +
    (nBad ? ` · <span class="bad">${nBad} lantern${nBad > 1 ? "s" : ""} too fast (under ${CFG.min_gap} ms)</span>` : "");
}
function renderActive(p) {
  $("patactive").checked = !!p.active;
  const pats = Object.values(S.patterns).map(normalize).sort((a, b) => a.name.localeCompare(b.name));
  $("activelist").innerHTML = pats.map(x => `<label><input type="checkbox" data-active="${esc(x.name)}" ${x.active ? "checked" : ""}> ${esc(x.name)}</label>`).join("")
    || '<span class="status">No patterns yet.</span>';
}
$("patactive").onchange = e => { curPattern().active = e.target.checked; renderActive(curPattern()); save(true); };
$("activelist").addEventListener("change", e => {
  const nm = e.target.dataset.active; if (!nm || !S.patterns[nm]) return;
  S.patterns[nm].active = e.target.checked; renderActive(curPattern()); save(true);
  log(`${nm} ${e.target.checked ? "is now Active (in rotation)" : "removed from rotation"}`);
});
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
function patSaved(msg) { $("patsaved").textContent = msg || "saved " + new Date().toLocaleTimeString(); }
function selectPattern(nm, focusName) {
  UI.pattern = nm; lsSet("pattern", nm); renderPatterns(); save(true); patSaved();
  if (focusName) { $("patname").focus(); $("patname").select(); }
}
$("patnew").onclick = () => {
  const nm = uniqueName("Pattern");
  S.patterns[nm] = newPattern(nm); selectPattern(nm, true);
  log(`created "${nm}": type a name in the box and press Enter`, "ok");
};
$("patdup").onclick = () => {
  const p = curPattern(), nm = uniqueName(p.name + " copy");
  S.patterns[nm] = JSON.parse(JSON.stringify({...p, name: nm, id: patternId()}));
  selectPattern(nm, true); log(`duplicated "${p.name}" as "${nm}"`, "ok");
};
function renameCurrent() {
  const p = curPattern(), nm = $("patname").value.trim();
  if (!nm || nm === p.name) { $("patname").value = p.name; return; }
  if (S.patterns[nm]) { log(`a pattern called "${nm}" already exists`, "bad"); $("patname").value = p.name; return; }
  const old = p.name;
  delete S.patterns[old]; p.name = nm; S.patterns[nm] = p;
  selectPattern(nm); log(`renamed "${old}" to "${nm}"`, "ok");
}
$("patname").addEventListener("keydown", e => {
  if (e.key === "Enter") { e.preventDefault(); renameCurrent(); }
  if (e.key === "Escape") { $("patname").value = curPattern().name; $("patname").blur(); }
});
$("patname").addEventListener("blur", renameCurrent);
$("patdel").onclick = () => {
  const p = curPattern();
  confirmClick($("patdel"), "Click again to delete", () => {
    delete S.patterns[p.name]; UI.pattern = null; renderPatterns(); save(true); patSaved(`deleted "${p.name}"`);
    log(`deleted pattern "${p.name}"`);
  });
};
$("patclear").onclick = () => {
  const p = curPattern();
  confirmClick($("patclear"), "Click again to clear", () => { p.hits = {}; renderPatterns(); save(true); patSaved(); log(`cleared "${p.name}"`); });
};
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
  let st;
  try { st = await api("/api/pattern/status"); } catch (e) { return; } // server restarting; try again next tick
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
  S = Object.assign({lanterns: {}, order: [], presets: {}, patterns: {}, pulse_ms: 40, tone_links: [], interactions: []}, r.state);
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
  showTab(["lanterns", "order", "patterns", "proximity", "interactions"].includes(UI.tab) ? UI.tab : "lanterns");
  $("scan").click();
})();
