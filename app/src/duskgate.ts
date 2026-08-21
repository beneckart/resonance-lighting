/** DUSK GATE — ADR-0031 compliance for the Solar Ray auto-fire.
 *
 *  Ben's ADR 0031 (accepted 07-26) disqualified panel telemetry as a show
 *  clock: charge taper under cloud/shade reads exactly like sunset, so a
 *  charging-census edge alone could fire the sunset show mid-afternoon.
 *  The fix is his: an explicit site/date schedule is PRIMARY, telemetry is
 *  confirmation. A real-fleet charging edge may only fire Solar Ray inside
 *  the dusk window around the site's computed sunset. The SIM path (daylight
 *  slider) is exempt — previews fire instantly, hardware obeys the sun.
 *
 *  Sunset math: NOAA-style approximation, good to a few minutes — plenty for
 *  a ±45/+90-minute window. Pure + clock-injected (no Date.now in tests). */

/** Black Rock City — the Tree's site. */
export const BRC = { lat: 40.7864, lon: -119.2065 };

const RAD = Math.PI / 180;

/** Approximate sunset for the site, as a UTC timestamp (ms) on `day`'s date. */
export function sunsetUtcMs(day: Date, lat = BRC.lat, lon = BRC.lon): number {
  const start = Date.UTC(day.getUTCFullYear(), 0, 0);
  const doy = Math.floor((Date.UTC(day.getUTCFullYear(), day.getUTCMonth(), day.getUTCDate()) - start) / 86400000);
  // fractional year (radians)
  const g = (2 * Math.PI / 365) * (doy - 1 + 0.5);
  // equation of time (minutes) + solar declination (radians) — NOAA short form
  const eqt = 229.18 * (0.000075 + 0.001868 * Math.cos(g) - 0.032077 * Math.sin(g)
    - 0.014615 * Math.cos(2 * g) - 0.040849 * Math.sin(2 * g));
  const decl = 0.006918 - 0.399912 * Math.cos(g) + 0.070257 * Math.sin(g)
    - 0.006758 * Math.cos(2 * g) + 0.000907 * Math.sin(2 * g)
    - 0.002697 * Math.cos(3 * g) + 0.00148 * Math.sin(3 * g);
  // hour angle at official sunset (zenith 90.833°)
  const cosH = (Math.cos(90.833 * RAD) / (Math.cos(lat * RAD) * Math.cos(decl)))
    - Math.tan(lat * RAD) * Math.tan(decl);
  const clamped = Math.min(1, Math.max(-1, cosH));
  const haMin = (Math.acos(clamped) / RAD) * 4; // degrees → minutes
  const solarNoonMin = 720 - 4 * lon - eqt; // minutes UTC
  const sunsetMin = solarNoonMin + haMin;
  return Date.UTC(day.getUTCFullYear(), day.getUTCMonth(), day.getUTCDate()) + sunsetMin * 60000;
}

export interface DuskWindowOpts {
  lat?: number;
  lon?: number;
  beforeMin?: number; // window opens this many minutes BEFORE sunset
  afterMin?: number; // …and closes this many after
}

/** Is `nowMs` inside the dusk window around the site's sunset?
 *  Checks BOTH today's and yesterday's UTC-date sunset: at BRC (UTC-7) sunset
 *  falls AFTER UTC midnight, so anchoring only to now's UTC date points a
 *  post-00:00Z `now` at tomorrow's sunset — the rollover this fixes. */
export function inDuskWindow(nowMs: number, opts: DuskWindowOpts = {}): boolean {
  const { lat = BRC.lat, lon = BRC.lon, beforeMin = 45, afterMin = 90 } = opts;
  for (const dayOffset of [0, -1]) {
    const sunset = sunsetUtcMs(new Date(nowMs + dayOffset * 86400000), lat, lon);
    if (nowMs >= sunset - beforeMin * 60000 && nowMs <= sunset + afterMin * 60000) return true;
  }
  return false;
}
