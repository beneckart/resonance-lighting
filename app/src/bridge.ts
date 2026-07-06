/** TWO-WAY BRIDGE — controller ⇄ bridge PowerFeather ⇄ ESP-NOW fleet.
 *
 *  Physical picture (Elliot 2026-07-05): the controller device (iPad/laptop
 *  running the twin) has a BRIDGE PowerFeather plugged in over USB. The bridge
 *  runs Ben's master role with NB_FRAME_HZ=0 (pure bridge/mesh node — this
 *  mode EXISTS in net_bench.ino) and translates between the fleet's packed
 *  ESP-NOW structs and JSON lines on the USB CDC serial port.
 *
 *  Wire reality this file mirrors 1:1 (net_bench.ino, upstream verified):
 *   - NbHeader { ver, type, src_id[3], seq, uptime_ms } — 3-byte compact MAC
 *     is the fleet-wide identity (same id our calibration map keys on).
 *   - NbHeartbeat @ 2 Hz ±30% jitter: batt_mv/ma/soc, reset_reason, ca_state,
 *     mode, dl_pdr, dl_rssi, supply telemetry. Fleet state ALREADY rides the
 *     heartbeat — knowing state costs ZERO extra radio duty. The 2026-07-05
 *     46 h battery soak ran exactly this duty cycle.
 *   - NB_IDENTIFY (locate-blink), NB_SET_RATE (survey can raise HB rate,
 *     conservation can lower it), NB_SHOWFRAME (params only).
 *
 *  PROPOSED additions (flagged, need Ben's buy-in — append-only NbType):
 *   - NB_STATE_EVT: edge-triggered tiny frame a peer sends THE MOMENT its
 *     local state changes (tap/presence/mode/fault). Heartbeats bound state
 *     staleness at ~500 ms; events make it instant, at negligible duty
 *     (one small frame per edge).
 *   - NB_CAL_*: the survey/assign/lock frames of syncproto.ts.
 *
 *  Uplink latency budget: HB worst case ≈ 1/2 Hz + jitter ≈ 650 ms;
 *  event path ≈ ESP-NOW airtime (µs) + serial (ms) → twin reacts in <50 ms. */

// ── host-side frame vocabulary (JSON lines over serial; bridge fw translates) ─

export interface HbFrame {
  kind: "hb";
  mac: string; // compact 3-byte id, e.g. "A1B2C3" (NbHeader.src_id)
  seq: number;
  uptimeMs: number;
  battMv: number;
  battMa: number;
  soc: number; // 0..100
  resetReason: number;
  caState: number; // the rules' local state — patterns run ON the fixture
  mode: number;
  dlPdrX1000: number; // downlink PDR as seen by this peer
  dlRssi: number; // RSSI of bridge frames at this peer (the survey signal!)
}

/** PROPOSED NB_STATE_EVT — instant edge report. */
export interface EvtFrame {
  kind: "evt";
  mac: string;
  seq: number;
  event: "state" | "tap" | "boot" | "fault" | "identify_ack";
  value: number; // new ca_state / fault code / tap strength
}

export type UpFrame = HbFrame | EvtFrame;

export interface ShowDown { kind: "show"; phase: number; hue: number; flags: number } // NbShowFrame
export interface IdentifyDown { kind: "identify"; mac: string | null; seconds: number } // NB_IDENTIFY (null = all)
export interface SetRateDown { kind: "set_rate"; hbHz: number; frameHz: number } // NB_SET_RATE
export type DownFrame = ShowDown | IdentifyDown | SetRateDown;

// ── the seam ──────────────────────────────────────────────────────────────────

export interface BridgeLink {
  readonly transport: "mock" | "serial";
  connect(): Promise<void>;
  disconnect(): void;
  send(frame: DownFrame): void;
  /** subscribe to uplink frames; returns unsubscribe */
  onUp(cb: (f: UpFrame) => void): () => void;
  connected(): boolean;
}

// ── MOCK bridge: a tick-driven fleet sim behind the same seam ────────────────
/** Simulates N fixtures running their rules LOCALLY (the control-plane
 *  contract): each node has a deterministic pattern clock advancing ca_state,
 *  battery slowly draining, heartbeats at hbHz ±30% jitter (Ben's
 *  NB_JITTER_PCT), and INSTANT EvtFrames on every local state edge.
 *  Tick-driven (no wall clock) so tests and the panel drive it explicitly. */
export interface MockNodeSpec { mac: string; role: string }

interface MockNode {
  spec: MockNodeSpec;
  seq: number;
  uptimeMs: number;
  battMv: number;
  soc: number;
  caState: number;
  mode: number;
  nextHbMs: number;
  patternPeriodMs: number; // local rules clock — state edges WITHOUT radio
  nextEdgeMs: number;
  identifyUntilMs: number;
}

function hash32(s: string): number {
  let h = 0x811c9dc5;
  for (let i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = Math.imul(h, 0x01000193) >>> 0; }
  return h >>> 0;
}

export class MockBridge implements BridgeLink {
  readonly transport = "mock" as const;
  private nodes: MockNode[] = [];
  private subs: ((f: UpFrame) => void)[] = [];
  private isConnected = false;
  private nowMs = 0;
  private hbHz = 2; // Ben's NB_HB_HZ default
  private rnd: () => number;

  constructor(specs: MockNodeSpec[], seed = 1) {
    let s = (seed >>> 0) || 1;
    this.rnd = () => { s = (Math.imul(s, 1664525) + 1013904223) >>> 0; return s / 4294967296; };
    this.nodes = specs.map((spec) => {
      const h = hash32(spec.mac);
      return {
        spec,
        seq: 0,
        uptimeMs: 0,
        battMv: 3250 + (h % 150), // LFP mid-charge spread
        soc: 55 + (h % 40),
        caState: h % 8,
        mode: 1,
        nextHbMs: (h % 500), // desynchronized boots
        patternPeriodMs: 1500 + (h % 2500), // each node's rules tick locally
        nextEdgeMs: 300 + (h % 1500),
        identifyUntilMs: 0,
      };
    });
  }

  async connect(): Promise<void> { this.isConnected = true; }
  disconnect(): void { this.isConnected = false; }
  connected(): boolean { return this.isConnected; }
  onUp(cb: (f: UpFrame) => void): () => void {
    this.subs.push(cb);
    return () => { this.subs = this.subs.filter((s) => s !== cb); };
  }
  private emit(f: UpFrame) { for (const s of this.subs) s(f); }

  send(frame: DownFrame): void {
    if (!this.isConnected) return;
    if (frame.kind === "identify") {
      for (const n of this.nodes) {
        if (frame.mac !== null && n.spec.mac !== frame.mac) continue;
        n.identifyUntilMs = this.nowMs + frame.seconds * 1000;
        n.seq += 1;
        this.emit({ kind: "evt", mac: n.spec.mac, seq: n.seq, event: "identify_ack", value: frame.seconds });
      }
    } else if (frame.kind === "set_rate") {
      this.hbHz = Math.max(0.1, Math.min(10, frame.hbHz));
    }
    // "show" frames: params only — nodes render locally; no uplink needed
  }

  /** A person touches/triggers a physical light (presence, tap sensor).
   *  This is the fleet→controller INSTANT path: the node emits an event the
   *  moment its local state flips — no polling involved. */
  tap(mac: string): void {
    const n = this.nodes.find((x) => x.spec.mac === mac);
    if (!n || !this.isConnected) return;
    n.caState = (n.caState + 1) % 8;
    n.seq += 1;
    this.emit({ kind: "evt", mac, seq: n.seq, event: "tap", value: n.caState });
  }

  /** advance the sim clock; emits due heartbeats + local rule edges */
  tick(dtMs: number): void {
    if (!this.isConnected) return;
    this.nowMs += dtMs;
    const hbPeriod = 1000 / this.hbHz;
    for (const n of this.nodes) {
      n.uptimeMs += dtMs;
      // local rules advance state with NO radio (the whole point) — but a
      // state edge emits one tiny event frame (the proposed NB_STATE_EVT)
      if (this.nowMs >= n.nextEdgeMs) {
        n.caState = (n.caState + 1) % 8;
        n.seq += 1;
        this.emit({ kind: "evt", mac: n.spec.mac, seq: n.seq, event: "state", value: n.caState });
        n.nextEdgeMs += n.patternPeriodMs;
      }
      if (this.nowMs >= n.nextHbMs) {
        n.battMv = Math.max(2900, n.battMv - 0.01 * (dtMs / 1000)); // slow drain
        n.soc = Math.max(0, n.soc - 0.0005);
        n.seq += 1;
        this.emit({
          kind: "hb", mac: n.spec.mac, seq: n.seq, uptimeMs: n.uptimeMs,
          battMv: Math.round(n.battMv), battMa: -60 - (n.identifyUntilMs > this.nowMs ? 120 : 0),
          soc: Math.round(n.soc), resetReason: 1, caState: n.caState, mode: n.mode,
          dlPdrX1000: 985 + Math.floor(this.rnd() * 15), dlRssi: -35 - Math.floor(this.rnd() * 30),
        });
        // ±30% jitter like NB_JITTER_PCT — desynchronizes the fleet's TX
        n.nextHbMs += hbPeriod * (0.7 + this.rnd() * 0.6);
      }
    }
  }
}

// ── SERIAL bridge: the real thing (Web Serial → bridge PowerFeather) ─────────
/** JSON-lines over USB CDC at 115200. UNTESTED against hardware — the framing
 *  contract for the bridge firmware is in docs/research/17-*.md; when the
 *  bridge sketch lands, this class is the only thing that touches it. */
type SerialLike = {
  open(opts: { baudRate: number }): Promise<void>;
  close(): Promise<void>;
  readable: ReadableStream<Uint8Array> | null;
  writable: WritableStream<Uint8Array> | null;
};

export class SerialBridge implements BridgeLink {
  readonly transport = "serial" as const;
  private port: SerialLike | null = null;
  private subs: ((f: UpFrame) => void)[] = [];
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private live = false;

  static available(): boolean {
    return typeof navigator !== "undefined" && "serial" in navigator;
  }

  async connect(): Promise<void> {
    const nav = navigator as Navigator & { serial?: { requestPort(): Promise<SerialLike> } };
    if (!nav.serial) throw new Error("Web Serial unavailable (Chrome/Edge on desktop required)");
    this.port = await nav.serial.requestPort();
    await this.port.open({ baudRate: 115200 });
    this.writer = this.port.writable!.getWriter();
    this.reader = this.port.readable!.getReader();
    this.live = true;
    void this.readLoop();
  }

  private async readLoop() {
    const dec = new TextDecoder();
    let buf = "";
    while (this.live && this.reader) {
      const { value, done } = await this.reader.read();
      if (done) break;
      buf += dec.decode(value, { stream: true });
      let nl;
      while ((nl = buf.indexOf("\n")) >= 0) {
        const line = buf.slice(0, nl).trim();
        buf = buf.slice(nl + 1);
        if (!line) continue;
        try {
          const f = JSON.parse(line) as UpFrame;
          if (f && (f.kind === "hb" || f.kind === "evt")) for (const s of this.subs) s(f);
        } catch { /* skip malformed line — untrusted wire */ }
      }
    }
  }

  disconnect(): void {
    this.live = false;
    this.reader?.cancel().catch(() => {});
    this.writer?.close().catch(() => {});
    this.port?.close().catch(() => {});
    this.port = null;
  }
  connected(): boolean { return this.live; }
  onUp(cb: (f: UpFrame) => void): () => void {
    this.subs.push(cb);
    return () => { this.subs = this.subs.filter((s) => s !== cb); };
  }
  send(frame: DownFrame): void {
    if (!this.writer) return;
    void this.writer.write(new TextEncoder().encode(JSON.stringify(frame) + "\n"));
  }
}
