import type { Control, SimFixture } from "./store";
import type { Override } from "./command";

// Protocol v1 (the seam to Ben's mesh). The cortex broadcasts CONTROL PARAMS — the
// recipe each fixture renders ITSELF — NOT a per-pixel stream (ADR 0004/0010).
// A fixture gets {pattern_id, brightness, hue, optional static rgb}; it runs the
// pattern on-device. Low-rate, channel-pinned, broadcast.
export interface ParamPacket {
  id: string;
  pattern: string; // pattern the fixture runs (or "off"/"static")
  bri: number; // 0..255
  hue: number; // 0..255
  sat: number; // 0..255 — without this, white/pastels are unrepresentable (white ≡ red on the fleet)
  rgb?: [number, number, number]; // 0..255, only for a static override color
}

export interface ShowFrame {
  proto: 1;
  channel: number; // ESP-NOW channel (pinned; must match the mesh)
  epoch: number; // monotonically increasing show epoch
  fixtures: ParamPacket[];
}

export function encodeFixture(c: Control, f: SimFixture, ov?: Override): ParamPacket {
  // BLACKOUT is a state the fleet must hear about — the twin renders black, so the
  // packet must command "off" too, or the physical tree stays lit (mirror rule).
  if (c.blackout) return { id: f.id, pattern: "off", bri: 0, hue: 0, sat: 0 };
  const bri = Math.round(Math.min(1, Math.max(0, c.brightness * c.master)) * 255);
  const sat = Math.round(Math.min(1, Math.max(0, c.sat)) * 255);
  if (ov?.mode === "off") return { id: f.id, pattern: "off", bri: 0, hue: 0, sat: 0 };
  if (ov?.rgb) {
    return {
      id: f.id, pattern: "static", bri, hue: 0, sat: 0,
      rgb: [Math.round(ov.rgb[0] * 255), Math.round(ov.rgb[1] * 255), Math.round(ov.rgb[2] * 255)],
    };
  }
  return { id: f.id, pattern: c.pattern, bri, hue: Math.round(c.hue * 255), sat };
}

export function buildShowFrame(
  c: Control,
  fixtures: SimFixture[],
  overrides: Record<number, Override>,
  channel: number,
  epoch: number
): ShowFrame {
  return {
    proto: 1,
    channel,
    epoch,
    fixtures: fixtures.map((f, i) => encodeFixture(c, f, overrides[i])),
  };
}
