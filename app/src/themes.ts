/** COLOUR THEMES for interactive mode (Elliot): curated moods — the living field's
 *  births, drift and touch-reactions all stay INSIDE the picked theme's colour
 *  world, so the tree holds an atmosphere while still evolving freely. */
export interface ColorTheme {
  id: string;
  name: string;
  emoji: string;
  blurb: string;
  hues: number[]; // the theme's colour anchors (0..1 on the wheel)
  sat: number; // saturation character
}

export const THEMES: ColorTheme[] = [
  { id: "ember", name: "Ember", emoji: "🔥", blurb: "the tree's warm amber heart", hues: [0.02, 0.05, 0.09, 0.12], sat: 0.9 },
  { id: "energize", name: "Energize", emoji: "⚡", blurb: "electric blues & greens", hues: [0.5, 0.56, 0.62, 0.35, 0.44], sat: 0.95 },
  { id: "intimate", name: "Intimate", emoji: "🕯", blurb: "deep reds, warm yellows & oranges", hues: [0.0, 0.04, 0.09, 0.13], sat: 0.85 },
  { id: "love", name: "Love", emoji: "💗", blurb: "pinks, purples & reds", hues: [0.88, 0.94, 0.8, 0.0, 0.75], sat: 0.85 },
  { id: "forest", name: "Forest", emoji: "🌲", blurb: "living greens & teals", hues: [0.3, 0.36, 0.42, 0.47], sat: 0.8 },
  { id: "ocean", name: "Ocean", emoji: "🌊", blurb: "deep blues & cyans", hues: [0.5, 0.55, 0.6, 0.65], sat: 0.85 },
  { id: "sunset", name: "Sunset", emoji: "🌅", blurb: "orange melting into pink & violet", hues: [0.05, 0.09, 0.9, 0.82], sat: 0.9 },
  { id: "random", name: "Wild", emoji: "🎲", blurb: "every colour, always different", hues: [], sat: 0.85 },
];

export const themeById = (id: string): ColorTheme => THEMES.find((t) => t.id === id) ?? THEMES[0];

/** Pick a hue from the theme (with a little organic jitter). `avoid` supports the
 *  "never the same as the last" rule — a different anchor is chosen when possible. */
export function themeHue(t: ColorTheme, rnd: number, avoid = -1, minDist = 0.12): number {
  if (!t.hues.length) return rnd % 1; // Wild: anything
  const dist = (a: number, b: number) => { const d = Math.abs(a - b) % 1; return Math.min(d, 1 - d); };
  const ok = avoid >= 0 ? t.hues.filter((h) => dist(h, avoid) >= minDist) : t.hues;
  const pool = ok.length ? ok : t.hues;
  const base = pool[Math.floor((rnd * 9973) % pool.length)];
  return ((base + (((rnd * 7919) % 1) - 0.5) * 0.05) % 1 + 1) % 1; // ±0.025 jitter
}
