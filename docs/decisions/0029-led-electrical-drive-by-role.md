# 0029 -- LED electrical drive by role: HEX on the 3V3 rail; boost shelved; RGBW feed decision open

**Date:** 2026-07-08 (records the 2026-07-02 boost A/B campaign verdicts, r1-r9, and
Ben's 2026-07-08 production-wiring stance)
**Status:** Accepted for the HEX feed and the boost shelving. The RGBW production
feed is OPEN: currently wired from the 3V3 rail; the measured-better VBAT-direct
option is documented below with its conversion plan and costs. ADR 0013's
switchable/default-off requirement stands unchanged either way.
**Owners:** Ben + Claude

## Context

ADR 0022 split the fleet into two LED roles (SK6812 HEX close-range; 4 W RGBW
point-source long-throw) but left the electrical feed open: regulated 3V3 rail,
VBAT-direct, or a TPS63802-class 4.2 V boost. The 2026-07-02 campaign measured the
full matrix (VEML7700 lux at the tube exit, INA branch power where instrumented,
LFP bench cell, gamma 0, seating error bounded <=2 % by remount replication).

**HEX A/B (single-pixel gobo regime -- Ben's product call: >1 full-white pixel
washes out the gobo, so single white px full IS the operating point):**

| look | bare lux | boosted lux | delta | branch W bare -> boosted |
|---|---|---|---|---|
| white 1 px full | 211.5-215.6 | 216.7-217.4 | +1.6 % (in noise) | 0.134 -> 0.216 (+60 %) |
| blue single | 60.4 | 63.4-63.6 | +5.1 % (below JND) | 0.062 -> 0.113 |
| ring1 7 px bri128 | 557.9 | 596.0-596.8 | +6.9 % | 0.388 -> 0.62 |

Boost is ~40 % WORSE on lumens/W for HEX (white ~1594 -> ~1006 lux/W): the extra
branch power becomes constant-current-driver heat, not photons. Boost gain grows
with load exactly as dropout physics predicts -- but the production HEX does not
operate there. LFP spends the night at 3.2-3.3 V terminal; a single-px look barely
sags it.

**RGBW matrix (usual aim, bri=255, SOC 63-75):**

| config | W-only (clean white) | RGB-white (fringed) |
|---|---|---|
| bare, rail-fed | 470 | 1310 (no wall) |
| bare, VBAT + fat wire | 448 | **1746 (no wall)** |
| boosted, rail-fed | 1044 | wall at bri=128 (rail limit) |
| boosted, VBAT + fat wire | 1016 | 3044 (no wall) |

The "wall" was never the architecture -- rail regulator first, instrumented-harness
resistance second (~0.3 ohm loop). VBAT-direct beats the rail by +33 % on fringed
RGB-white (starved G/B dies convert every extra 100 mV into light); **clean W-only
is unchanged** (448 vs 470 -- the W die is equally starved either way). Connector
quality is worth ~25 % of top-end light. Boost's final form: 2.3x clean white /
1.7x max fringed, at ~25-30 % battery-plane efficacy tax -- it converts efficiency
into output ceiling.

**Current production wiring (as-built on the bench fleet):** both roles feed from
the switchable 3V3 header -- V+ / GND / signal on A0/GPIO10 via a simple
right-angle JST-XH 4-pin (QON pin left unconnected). One connector, one firmware
pinout, and the rail IS the hard LED kill.

## Decision

1. **SK6812 HEX stays on the regulated switchable 3V3 rail.** DECIDED -- boost is
   measurably not worth it in the single-px gobo regime, VBAT showed no meaningful
   benefit for the HEX, and the rail gives the fail-safe kill for free.
2. **TPS63802 4.2 V boost: SHELVED, with its option file complete.** DECIDED --
   revival spec if a future look needs the ceiling: VBAT-fed single conversion on
   an adapter PCB, EN -> GPIO with pull-down, fat wiring; every operating point is
   already measured.
3. **4 W RGBW feed: OPEN -- rail-fed as wired today; VBAT-direct is the
   measured-better option with real conversion costs.** Ben's 2026-07-08 stance,
   recorded so the trade is explicit when this is revisited (before the harness
   buy):
   - **For staying on the rail:** clean W-only output is unchanged; cutting the
     3V3 rail is a robust hard LED kill (ADR 0013 satisfied by construction); one
     harness and one firmware pinout across both LED roles; no clean VBAT tap
     exists on the COTS board.
   - **For converting to VBAT (+33 % fringed white, free):** the conversion plan
     is to solder a 4-pin header along {VBAT | EN | VS | D13}, pulling VBAT -> V+
     and D13 -> signal, with GND tapped from the GND pin adjacent to VDC/solar+
     via a cheap JST 2-pin Y-cable (~$0.50 each; ~100 needed for the RGBW
     fixtures -- sourcing at quantity unverified).
   - **Conversion costs:** firmware pin move A0 -> D13; LED fail-safe redesign
     (the 3V3-rail shutoff no longer kills the LED -- a stuck-on frame could run
     the battery down, so a default-off switch element and verified all-off
     behavior are mandatory per ADR 0013); Y-cable sourcing; per-board soldering
     against the ADR 0009 O(1)-ops rule.
   - **Side benefit if converted:** 3V3/GND/A0 free up for a noisemaker
     (clacker/relay) payload on the same fixture.
4. **If the VBAT option is taken:** tap downstream of the gauge's current-sense
   shunt (a bare header tap makes the gauge blind to the dominant load -- the
   planned VBAT header tap has exactly this problem and would need the coulomb
   accounting corrected or re-plumbed), fat conductors, default-off kill.
5. Pixels latch their last frame: explicit all-off before rail-cut/sleep remains
   mandatory (ADR 0013/0018) -- and becomes load-bearing, not belt-and-suspenders,
   under any VBAT feed.

## Consequences

- ADR 0013's "exact voltage rail chosen by test" clause is resolved for the HEX
  and parked-with-data for the RGBW; the fail-safe requirement itself is untouched.
- The harness buy (JST-XH right-angle set) and production firmware pinout both
  fork on the RGBW feed decision -- decide before ordering ~100 of anything.
- Coulomb/SOC telemetry (ADR 0023's coulomb-primary policy) is only trustworthy
  for the LED branch if the feed is downstream of the gauge shunt; rail-fed today
  satisfies this, the sketched VBAT header tap does not.
- ADR 0008's VBAT-direct instinct is vindicated by measurement but not (yet)
  adopted in production -- the full-circle note there stays historical.
- Open residuals: r10 battery-plane watts for the uninstrumented VBAT configs
  (spot-measure); HEX low-SOC re-check demoted (the rail is SOC-invariant by
  construction until deep discharge); rail-droop curve near the ~1 A ceiling is
  the one un-characterized corner if any multi-px HEX look ever ships.
- The boosted-build firmware count/current cap (ADR 0022 follow-up) is moot unless
  the boost is revived.

## References

- LOG 2026-07-02: boost A/B verdict (HEX), r6 gold standard, r7 VBAT-fed, r8
  bare-VBAT fat-wire (+33 %), r9 completed matrix (3044 lux), gamma/wedge audit,
  hex-V+ topology correction; LOG 2026-07-08 (cont.) -- production-wiring stance
- `docs/tests/BOOST_AB_BENCH_REPORT_2026-07-02.html` (charts, evidence-graded)
- ADR 0008 (historical), 0009 (O(1) ops), 0013 (fail-safe, stands), 0018
  (interface), 0022 (roles), 0023 (coulomb policy)
