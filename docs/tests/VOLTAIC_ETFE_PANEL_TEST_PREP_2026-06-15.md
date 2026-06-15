# Voltaic ETFE panel test prep

**Date:** 2026-06-15
**Owner:** Ben
**Status:** Ready for outdoor deployment test

## Why this matters

The panel choice is now a mechanical/BOM blocker, not just an electrical question. The
two Voltaic ETFE candidates are both ruggedized enough to be plausible for playa service,
but they imply different hat footprints:

- **P105 / 5 W ETFE:** safest energy margin, mechanically friendlier because it has
  mounting holes, but the footprint is large.
- **P126 / 2 W ETFE:** much nicer footprint for a compact hat, but it leaves less
  budget for RGBW show loads and forces us to be honest about duty cycle.

My bias: split the BOM by optical role if testing confirms it. Use the smaller P126 on
HEX fixtures where the typical illuminated count is low, and use the P105 on point-source
RGBW fixtures unless the bottom-up show budget proves the RGBW duty cycle is tiny.

## Source specs

Voltaic periodically revises cells while keeping SKU/mechanical compatibility. The product
page values and datasheet current-revision values do not perfectly match, so keep both in
the bench sheet and measure the actual units in hand.

| SKU | Source | Pmpp | Vmpp | Impp | Voc | Isc | Size | Mass | Notes |
|---|---:|---:|---:|---:|---:|---:|---|---:|---|
| P105 5 W ETFE | Product page | 5.75 W | 6.12 V | 0.94 A | 7.13 V | n/a | 148 x 223 x 4 mm | 188 g | IP67, ETFE, 3.5 x 1.1 mm male plug, 50 cm cable, 6 mounting holes |
| P105 5 W ETFE | Datasheet R7E nominal | 5.51 W | 5.21 V | 1.06 A | 6.61 V | 0.95 A | 223 x 148 x 3.6 mm | 188 g | IPX7, -40 to 85 deg C, 21.9% cells |
| P105 5 W ETFE | Datasheet expected | 4.61 W | 4.69 V | 0.98 A | 5.94 V | 1.06 A | same | 188 g | Expected values include real-world stack/cell-cut losses |
| P126 2 W ETFE | Product page | 2.37 W | 7.28 V | 0.33 A | 8.51 V | n/a | 112 x 136 x 2.7 mm | 79 g | IP67, ETFE, attached 3.5 x 1.1 mm cable |
| P126 2 W ETFE | Datasheet R1L nominal | 2.38 W | 7.09 V | 0.34 A | 8.59 V | 0.37 A | 136 x 112 x 3.1 mm | 79 g | IPX7, -40 to 85 deg C, 21.5% cells, VHB gasket mount |
| P126 2 W ETFE | Datasheet expected | 2.31 W | 6.84 V | 0.29 A | 8.34 V | 0.33 A | same | 79 g | Expected values include real-world stack/cell-cut losses |

Derived:

- P126 is about **43% of P105's nominal wattage** and about **46% of its area**, so the
  smaller panel is not magically denser. It is mainly a mechanical/aesthetic win.
- Weight per nominal watt is basically tied: P105 about 34 g/W, P126 about 33 g/W.
- Retail cost per watt favors P105: $35 / 5.51 W = about $6.35/W vs $21 / 2.38 W =
  about $8.82/W. Bulk still favors P105: at the listed 100+ breaks, P105 is about
  $4.80/W and P126 about $7.62/W.
- P105 has physical mounting holes. P126's datasheet calls out a VHB gasket mount, so the
  enclosure should still provide a lip, pocket, or mechanical backup plus pigtail strain
  relief rather than trusting adhesive alone in heat/dust.

## PowerFeather gotchas for tomorrow

Both panels have Voc above the BQ25628E charger's default low input-OVP threshold. That is
not a panel disqualification, but it means a bright-sun connect can look like a dead panel
until the firmware sets VBUS_OVP=1 and/or toggles HIZ to synthesize a fresh input
qualification edge. If `supply_v` is present but `supply_good` stays false, suspect this
before blaming the panel.

Maintain/VINDPM matters:

- P105 should be swept around both the old hot-panel winner and its datasheet MPP:
  4.6, 4.9, 5.2, 5.5, 6.1 V.
- P126 should be swept near its higher MPP: 5.8, 6.5, 6.8, 7.1, 7.3 V.
- `net_bench` now accepts live `m<v10>` values from 4.0 to 16.8 V, so `m71` sets 7.1 V
  without reflashing after this firmware is on the peer.

Battery config note: both current test boards are on LFP 2000 mAh. A wrong capacity mainly
hurts SOC/time-to-empty learning and any SOC-based decisions. The dangerous mismatch is
chemistry/charge voltage. Charge current is still controlled separately by `--charge-ma`.

## Suggested outdoor run

Use COM7 as the USB serial bridge/master and the INA-instrumented board as the field peer.
Keep the bridge on USB near the laptop; put the peer on battery plus the test panel outside.

Baseline flash/config:

```bash
cd firmware/net_bench
./build.sh --role master --channel 11 --serial-bridge --chem lfp --cap 2000 --port COM7
./build.sh --role peer --channel 11 --maint-ap --chem lfp --cap 2000 --maintain 4.6 --hb-hz 1 --port COM4
```

Logging, from the repo root in a second terminal:

```bash
python ops/bench/net_bench_serial_bridge.py --port COM7
python ops/bench/net_bench_log.py --site travel --notes "Voltaic ETFE P105/P126 outdoor MPP prep"
```

Run shape:

1. Start with the peer battery below roughly 80% SOC so the charger is not demand-limited.
2. For each panel, record open-circuit voltage with a meter before connecting to the board.
3. Shade the panel while plugging it into VDC, then uncover after telemetry is flowing.
4. Hold each maintain setpoint for at least 2-5 minutes; use panel-side INA power as the
   sizing source when available because BQ supply telemetry has under-reported by about 10%.
5. Log panel orientation, sky state, panel-back temperature, and any shading from the
   temporary enclosure or cable routing.
6. Repeat one setpoint after moving the panel hot/cold if the temperature changed a lot.

## Decision read

If P126 reliably nets enough Wh/day for the HEX profile, it is the more elegant enclosure
choice for those fixtures. But I would not make it the universal panel until RGBW load
budget is pinned down. The P105 buys real margin against dust, imperfect angle, heat, and
aging, and those are exactly the playa failure modes. Mixed panel BOMs are a little messier
operationally, but the project already has two optical roles; matching panel size to role
is a rational trade if it unlocks a better hat shape.

## Sources

- P105 product page: https://voltaicsystems.com/5-watt-panel-etfe/
- P105 datasheet: https://voltaicsystems.com/content/P105_ds.pdf
- P126 product page: https://voltaicsystems.com/P126/
- P126 datasheet: https://voltaicsystems.com/content/P126_ds.pdf
