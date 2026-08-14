/** CAMBIUM BRIDGE — controller ⇄ cambium daemon ⇄ CoreS3 modem ⇄ ESP-NOW fleet.
 *
 *  The third transport behind the BridgeLink seam. Speaks Justin's cambium
 *  WebSocket vocabulary (cambium/api/ws_protocol.py, verified 2026-08-13):
 *
 *  DOWN (us → cambium), one JSON object per text frame, keyed by "kind":
 *    frame    {seq, fixtures:[{id, rgb:[r,g,b]}]}  — linear floats, no clamp here
 *    drive    {on}
 *    show     {phase, hue, flags?}                 — same fields as our ShowDown
 *    identify {mac?, seconds}                      — mac omitted = whole fleet
 *    set_rate {hbHz? | frameHz?}
 *    ruleset  accepted + logged only (cambium does not forward it yet)
 *
 *  UP (cambium → us), payload keys merged flat with the kind:
 *    hb  {rssi, mac, batt_mv, batt_ma, soc_pct, dl_rssi, mode, last_seen,
 *         life_state, program, power_tier}          (uplink/fleetstate.py)
 *    evt {mac, state, prev_state, program, generation, intensity}
 *        — choreo state EDGE (first sighting counts as an edge)
 *    charging      {count, macs}        ─┐ transport/fleet meta — surfaced via
 *    bridge_status {…}                   ├ onMeta, not forced through UpFrame
 *    err           {msg}                ─┘ (a bad DOWN earns err, never a drop)
 *
 *  Field translation into the seam's HbFrame is OURS to own (cambium is
 *  snake_case and omits seq/uptime): seq is synthesized monotonically per mac,
 *  uptimeMs counts from first sighting, dlPdrX1000 is 0 (cambium does not
 *  report PDR — DataLog treats 0 as "unknown", not "0%").
 *
 *  Reconnect: exponential backoff 500 ms → 8 s while enabled. One typo'd or
 *  unknown UP frame is skipped, never fatal — same doctrine as SerialBridge. */

import type { BridgeLink, DownFrame, UpFrame } from "./bridge";

/** minimal WebSocket surface — injectable so tests never open sockets */
export type WsLike = {
  readyState: number;
  send(data: string): void;
  close(): void;
  onopen: ((ev?: unknown) => void) | null;
  onclose: ((ev?: unknown) => void) | null;
  onerror: ((ev?: unknown) => void) | null;
  onmessage: ((ev: { data: unknown }) => void) | null;
};
export type WsFactory = (url: string) => WsLike;

const WS_OPEN = 1;

export const CAMBIUM_DEFAULT_URL = "ws://localhost:8600/ws";

/** cambium meta traffic that is not fleet state (charging counts, bridge
 *  health, protocol errors). Additive channel — the BridgeLink seam is
 *  untouched for consumers that only care about hb/evt. */
export interface CambiumMeta {
  kind: "charging" | "bridge_status" | "err" | "open" | "close";
  payload: Record<string, unknown>;
}

interface PerMac { seq: number; firstSeenMs: number }

export class CambiumBridge implements BridgeLink {
  readonly transport = "cambium" as const;
  private url: string;
  private makeWs: WsFactory;
  private now: () => number;
  private ws: WsLike | null = null;
  private enabled = false; // user intent: keep (re)connecting while true
  private open = false; // socket truth
  /** true once THIS enable-cycle has had a successful open. Reconnect fights
   *  for an ESTABLISHED session only — a failed initial connect fails fast
   *  and stays failed (startup-order case B: app before daemon), so a caller
   *  that discards the bridge after a connect() rejection never leaves a
   *  zombie redial loop behind. */
  private established = false;
  private subs: ((f: UpFrame) => void)[] = [];
  private metaSubs: ((m: CambiumMeta) => void)[] = [];
  private perMac = new Map<string, PerMac>();
  private backoffMs = 500;
  private retryTimer: ReturnType<typeof setTimeout> | null = null;
  private frameSeq = 0;

  constructor(opts?: { url?: string; wsFactory?: WsFactory; now?: () => number }) {
    this.url = opts?.url ?? CAMBIUM_DEFAULT_URL;
    this.makeWs =
      opts?.wsFactory ??
      ((url: string) => new WebSocket(url) as unknown as WsLike);
    this.now = opts?.now ?? (() => Date.now());
  }

  /** ?cambium=ws://<lan-host>:8600/ws — an iPad on the LAN points here
   *  (Justin's quickstart step 6 contract). */
  static urlFromLocation(): string {
    if (typeof window === "undefined") return CAMBIUM_DEFAULT_URL;
    const q = new URLSearchParams(window.location.search).get("cambium");
    return q || CAMBIUM_DEFAULT_URL;
  }

  async connect(): Promise<void> {
    this.enabled = true;
    await this.dial();
  }

  private dial(): Promise<void> {
    return new Promise((resolve, reject) => {
      let settled = false;
      const ws = this.makeWs(this.url);
      this.ws = ws;
      ws.onopen = () => {
        this.open = true;
        this.established = true;
        this.backoffMs = 500; // healthy connection resets the ladder
        this.emitMeta({ kind: "open", payload: {} });
        if (!settled) { settled = true; resolve(); }
      };
      ws.onmessage = (ev) => this.onWire(ev.data);
      ws.onerror = () => {
        if (!settled) { settled = true; reject(new Error(`cambium unreachable at ${this.url}`)); }
      };
      ws.onclose = () => {
        const was = this.open;
        this.open = false;
        if (was) this.emitMeta({ kind: "close", payload: {} });
        // retry ONLY a session that was once established this enable-cycle
        if (this.enabled && this.established) this.scheduleRetry();
        if (!settled) { settled = true; reject(new Error(`cambium closed during connect (${this.url})`)); }
      };
    });
  }

  private scheduleRetry(): void {
    if (this.retryTimer) return;
    const delay = this.backoffMs;
    this.backoffMs = Math.min(8000, this.backoffMs * 2);
    this.retryTimer = setTimeout(() => {
      this.retryTimer = null;
      if (this.enabled) void this.dial().catch(() => { /* onclose re-arms */ });
    }, delay);
  }

  disconnect(): void {
    this.enabled = false;
    this.established = false;
    if (this.retryTimer) { clearTimeout(this.retryTimer); this.retryTimer = null; }
    this.ws?.close();
    this.ws = null;
    this.open = false;
  }

  connected(): boolean { return this.open; }

  onUp(cb: (f: UpFrame) => void): () => void {
    this.subs.push(cb);
    return () => { this.subs = this.subs.filter((s) => s !== cb); };
  }

  /** subscribe to cambium meta (charging / bridge_status / err / open / close) */
  onMeta(cb: (m: CambiumMeta) => void): () => void {
    this.metaSubs.push(cb);
    return () => { this.metaSubs = this.metaSubs.filter((s) => s !== cb); };
  }
  private emitMeta(m: CambiumMeta) { for (const s of this.metaSubs) s(m); }

  send(frame: DownFrame): void {
    if (!this.ws || this.ws.readyState !== WS_OPEN) return;
    if (frame.kind === "identify") {
      // cambium's identify takes an OPTIONAL mac; our seam uses null for
      // "whole fleet". Omit the key rather than send null — ws_protocol
      // validates field types and null is not a mac.
      const { mac, seconds } = frame;
      this.ws.send(JSON.stringify(mac === null ? { kind: "identify", seconds } : { kind: "identify", mac, seconds }));
      return;
    }
    this.ws.send(JSON.stringify(frame));
  }

  /** Stream one per-fixture color frame (the type-25 path). seq is owned
   *  here so callers can't accidentally send stale ordering. */
  sendFrame(fixtures: { id: string; rgb: [number, number, number] }[]): void {
    this.frameSeq = (this.frameSeq + 1) >>> 0;
    this.send({ kind: "frame", seq: this.frameSeq, fixtures });
  }

  // ── UP translation ─────────────────────────────────────────────────────────

  private tick(mac: string): PerMac {
    let p = this.perMac.get(mac);
    if (!p) { p = { seq: 0, firstSeenMs: this.now() }; this.perMac.set(mac, p); }
    p.seq += 1;
    return p;
  }

  private onWire(data: unknown): void {
    if (typeof data !== "string") return; // binary is not in the contract
    let msg: Record<string, unknown>;
    try { msg = JSON.parse(data) as Record<string, unknown>; } catch { return; }
    const kind = msg.kind;
    if (kind === "hb") this.onHb(msg);
    else if (kind === "evt") this.onEvt(msg);
    else if (kind === "charging" || kind === "bridge_status" || kind === "err") {
      const { kind: k, ...payload } = msg;
      this.emitMeta({ kind: k as CambiumMeta["kind"], payload });
    }
    // unknown kinds: skipped — cambium may grow vocabulary before we do
  }

  private num(v: unknown, fallback: number): number {
    return typeof v === "number" && Number.isFinite(v) ? v : fallback;
  }

  private onHb(m: Record<string, unknown>): void {
    if (typeof m.mac !== "string" || m.mac.length === 0) return;
    const mac = m.mac.toUpperCase();
    const p = this.tick(mac);
    const f: UpFrame = {
      kind: "hb",
      mac,
      seq: p.seq,
      uptimeMs: Math.max(0, this.now() - p.firstSeenMs),
      battMv: this.num(m.batt_mv, 0),
      battMa: this.num(m.batt_ma, 0), // signed: negative = discharging
      soc: this.num(m.soc_pct, 0),
      resetReason: 0, // not reported over cambium
      caState: this.num(m.life_state, 0),
      mode: this.num(m.mode, 0),
      dlPdrX1000: 0, // cambium does not report PDR — 0 means "unknown"
      dlRssi: this.num(m.dl_rssi, this.num(m.rssi, 0)),
    };
    for (const s of this.subs) s(f);
  }

  private onEvt(m: Record<string, unknown>): void {
    if (typeof m.mac !== "string" || m.mac.length === 0) return;
    const mac = m.mac.toUpperCase();
    const p = this.tick(mac);
    const f: UpFrame = {
      kind: "evt",
      mac,
      seq: p.seq,
      event: "state", // cambium evt IS a choreo state edge
      value: this.num(m.state, 0),
    };
    for (const s of this.subs) s(f);
  }
}

/** 8 Hz frame pump: reads the provider each tick and streams non-empty frames.
 *  Kept OUTSIDE the class so the twin can pump from its render telemetry
 *  (telemetry.states) without CambiumBridge importing UI modules. Returns a
 *  stop function. */
export function startFramePump(
  bridge: CambiumBridge,
  provider: () => { id: string; rgb: [number, number, number] }[],
  hz = 8,
  schedule: (fn: () => void, ms: number) => ReturnType<typeof setTimeout> = setTimeout,
): () => void {
  let live = true;
  const period = 1000 / Math.max(1, Math.min(8, hz)); // cambium contract: ≤8 Hz
  const loop = () => {
    if (!live) return;
    if (bridge.connected()) {
      const fixtures = provider();
      if (fixtures.length > 0) bridge.sendFrame(fixtures);
    }
    schedule(loop, period);
  };
  schedule(loop, period);
  return () => { live = false; };
}
