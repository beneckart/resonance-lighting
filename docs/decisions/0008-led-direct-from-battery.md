# 0008 — WS2812B powered direct from battery rail

**Date:** 2026-05-06
**Status:** Accepted (verified in 2018 Talisman v2)
**Owners:** Ben

## Context

WS2812B LEDs need Vcc 3.3–5 V with logic-high data threshold ≥ 0.7 × Vcc. ESP32-C3 GPIO outputs 3.3 V logic-high. Battery is single-cell LiFePO4 (Vbat 2.5–3.6 V).

Three places to power LEDs from:

1. **3.3 V rail (post-LDO):** clean voltage but limited by AP2112K-3.3's ~600 mA budget, which the ESP32 also draws from. 4 LEDs at 240 mA max would saturate the LDO.
2. **5 V boost rail:** clean and standard, but requires adding a boost converter to the BOM and burning power even when not driving full LEDs.
3. **Battery rail directly (Vbat 2.5–3.6 V):** cheapest and lowest-loss. Question: does WS2812B latch on a 3.3 V GPIO data signal when Vbat = 3.6 V max?

## Decision

**Power LEDs from Vbat directly. No level shifter.**

Math:
- WS2812B threshold: 0.7 × Vcc
- Worst case: Vbat = 3.6 V (LiFePO4 fully charged) → threshold = 2.52 V
- ESP32-C3 GPIO high = 3.3 V
- Margin = 3.3 − 2.52 = **780 mV**

The 2018 Talisman v2 (`beneckart/future-robotics`) verified the same trick on bench with similar math (LiPo 4.2 V max → threshold 2.94 V, with 3.3 V GPIO → 360 mV margin). Worked reliably on production fixtures.

## Consequences

- **No level-shifter chip on BOM.** Saves ~$0.10 per fixture and one BOM line.
- **No boost converter on BOM.** Saves several parts, ~$0.50–1.00 per fixture, and the constant power loss of a boost regulator.
- **Color of LEDs varies slightly with Vbat.** As the battery drops from 3.6 V → 3.0 V, WS2812B output current per LED drops slightly (the internal constant-current driver compensates partially but not completely). Visually subtle, more important to the artistic intent than the engineering — at low battery the lantern appears dimmer, which we want anyway as a "this fixture is tired" signal.
- **Decoupling caps still required.** One 100 nF ceramic per LED, plus a bulk 10 µF on the rail. Per the 2018 datasheet study, missing inter-LED decoupling caps on Steve's first PCB attempt caused issues — explicitly catch this on the new design.
- **If bench testing on the new TTGO+LiFePO4 setup ever shows latching glitches**, the fallback is SN74AHCT125 quad level shifter (~$0.10, JLCPCB Basic) on the data line. Easy to add later if needed.
