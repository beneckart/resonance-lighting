// Live per-light TELEMETRY — the "behind the scenes" data log: what each light is
// ACTUALLY doing (the control-plane truth — brightness + colour that would really
// be sent to the fixture). Written by the render loop (throttled ~5 Hz) and read
// by the DataLog panel. Module-level (NOT zustand) so writing it every tick never
// triggers React re-renders at frame rate.
export interface LightState {
  num: number; // addressable light number
  id: string; // fixture id (F012)
  bri: number; // 0..1 output level (max channel)
  rgb: [number, number, number]; // 0..1 reported colour
}

export const telemetry: { states: LightState[]; t: number } = { states: [], t: 0 };
