/** LANTERN NICKNAMES — human names for physical lanterns (Elliot 08-15:
 *  "change the names of the ones that are hard plugged to mario and luigi").
 *
 *  MAC → name, per device (localStorage). Names are an OPERATOR overlay: they
 *  never touch fixtures.json ids, the calibration map, or anything another
 *  system joins on — a nickname can never break a slot join. Resolution is
 *  case-insensitive both ways so "blink mario" works from voice and the AI. */

const KEY = "resonance.macnames.v1";

/** THE MARIO BROTHERS — the OG connections: the first two lanterns ever
 *  hard-plugged for bench work (Luigi corrected F2BDB0→F2B7DC 08-15: USB serial
 *  68:EE:8F:F2:B7:DC is the board Elliot plugged; F2BDB0 was a radio misguess) (Elliot, 2026-08-15). Seeded so the names
 *  exist on every device without retyping; editable/overridable. */
const SEED: Record<string, string> = {
  F40384: "Mario",
  F2B7DC: "Luigi",
};

function store(): Storage | null {
  try {
    return typeof localStorage === "undefined" ? null : localStorage;
  } catch {
    return null;
  }
}

export function loadNames(): Record<string, string> {
  try {
    const raw = store()?.getItem(KEY);
    return { ...SEED, ...(raw ? (JSON.parse(raw) as Record<string, string>) : {}) };
  } catch {
    return { ...SEED };
  }
}

export function setName(mac: string, name: string): void {
  const s = store();
  if (!s) return;
  let cur: Record<string, string> = {};
  try {
    cur = JSON.parse(s.getItem(KEY) ?? "{}") as Record<string, string>;
  } catch { /* fresh */ }
  const m = mac.toUpperCase();
  const t = name.trim();
  if (t) cur[m] = t;
  else delete cur[m];
  s.setItem(KEY, JSON.stringify(cur));
}

export function nameFor(mac: string): string | null {
  return loadNames()[mac.toUpperCase()] ?? null;
}

/** name → MAC, case-insensitive ("mario" → "F40384"); null if unknown */
export function macFor(name: string): string | null {
  const want = name.trim().toLowerCase();
  if (!want) return null;
  for (const [mac, n] of Object.entries(loadNames())) {
    if (n.toLowerCase() === want) return mac;
  }
  return null;
}
