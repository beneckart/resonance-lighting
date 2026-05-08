# System Block Diagram + Power Budget

**Status:** First pass. Numbers below are sized to LiFePO4 chemistry, ESP32-C3 platform, 1–9 WS2812B LEDs (typically 1–3 lit at a time). Refine as bench measurements come in.

## Block Diagram

```
                    ┌────────────────────┐
                    │   Solar panel      │
                    │   1–2 W, ~5–6 V    │
                    │   Connector: JST-PH│
                    └─────────┬──────────┘
                              │
                              ▼
                    ┌────────────────────┐
                    │ Reverse-polarity   │
                    │ protection         │
                    │ (Schottky diode)   │
                    └─────────┬──────────┘
                              │
                              ▼
                    ┌────────────────────┐
                    │   CN3058 charger   │
                    │   LiFePO4 profile  │
                    │   3.6 V max charge │
                    │   ~500 mA Iset     │◄─── status LED (charging / done)
                    └─────────┬──────────┘
                              │
                              ▼
                    ┌────────────────────┐         ┌──────────────────┐
                    │   Power-path       │◄────────┤ LiFePO4 cell     │
                    │ (ideal-diode MOSFET│         │ 14430 ~400 mAh   │
                    │  on battery side)  │         │ or 18650 1500mAh │
                    └─────────┬──────────┘         │ JST-PH connector │
                              │                    └──────────────────┘
                              │ Vbat = 2.5–3.6 V
                              │
              ┌───────────────┼───────────────────────┐
              ▼               ▼                       ▼
    ┌──────────────┐  ┌────────────────┐    ┌─────────────────────┐
    │ AP2112K-3.3  │  │ Battery sense  │    │ WS2812B chain       │
    │ LDO          │  │ ADC divider    │    │ 1–9 LEDs (1–3 typ.) │
    │ 450 mV drop  │  │ (gated by GPIO │    │ Direct from Vbat    │
    │ ~600 mA cap. │  │  to save power)│    │ (3.3 V data sat.)   │
    └──────┬───────┘  └────────┬───────┘    │ JST-PH out          │
           │                   │            └─────────┬───────────┘
           │ 3V3                                      │
           ▼                                          │ Data + Vbat + GND
    ┌──────────────────────────────────────┐         │
    │  ESP32-C3-MINI-1                     │─────────┘ (GPIO data line)
    │  RISC-V single-core 160 MHz          │
    │  WiFi + BLE + ESP-NOW                │
    │  ~22 GPIO available                  │◄─── USB-C (D+/D-) ─── flashing + charge fallback
    │                                      │
    │  Tasks: led_render, ca_tick,         │
    │         mesh_rx, mesh_tx,            │
    │         housekeeping                 │
    └──────────────────────────────────────┘
```

## Voltage rails

| Rail | Source | Range | Loads |
|------|--------|-------|-------|
| Vsolar | Panel | 0–7 V (Voc when unloaded) | Charger input only |
| Vbat | LiFePO4 | 2.5–3.6 V | LEDs (direct), LDO input, battery sense |
| 3V3 | LDO | 3.3 V (when Vbat ≥ 3.55 V) | ESP32-C3, pull-ups, status LEDs |

**LiFePO4-specific design notes:**

- LiFePO4 max charge voltage is **3.6 V**, not 4.2 V. CN3058 is tuned for this. Do not substitute LiPo chargers (TP4056, bq24074, CN3791).
- Discharge cutoff for LiFePO4 longevity is **2.5 V** absolute / **2.8–3.0 V** practical.
- AP2112K-3.3 has 450 mV dropout. Output regulates cleanly when Vbat ≥ 3.55 V; below that, 3.3 V rail droops with the battery. ESP32-C3 runs down to ~3.0 V on the 3.3 V rail without issue. Effective system cutoff: ~3.05 V Vbat (matches LiFePO4 longevity target).
- WS2812B data-line threshold: logic high needs ≥0.7 × Vcc. With Vcc = 3.6 V max, threshold = 2.52 V. With 3.3 V GPIO data, margin = 780 mV. **No level shifter required.** Verified on Talisman v2 (2018 build, same trick).

## Current draw — measured + estimated

Per-LED current numbers derived from 2018 Talisman v2 measurements on a 16-LED ring (`beneckart/future-robotics`). WS2812B current scales linearly per LED — the 16-LED measurements divide cleanly to per-LED values that match the datasheet.

**Per-LED reference table** (at Vbat = 3.3 V):

| Brightness | Color | Current per LED |
|------------|-------|-----------------|
| 255 (full) | White (R+G+B) | ~31 mA |
| 255 (full) | Single color | ~12.5 mA |
| 128 (~50%) | White | ~17.5 mA |
| 26 (~10%) | White | ~4.4 mA |
| 1 (~0.4%) | White | ~1.25 mA |

**Resonance fixture has 1–9 WS2812B per lantern.** Real usage model (per Ben):

- *Default ambient:* 1 LED (the center of a 3×3 grid) at ~10% brightness.
- *Showy "color-fringing" mode:* 3 neighboring LEDs at moderate brightness for the chromatic offset effect.
- *Wand-burst peaks:* up to 9 LEDs lit simultaneously, briefly.

**Subsystem current table:**

| Subsystem | Mode | Current @ Vbat=3.3V | Notes |
|-----------|------|---------------------|-------|
| ESP32-C3 | WiFi TX active | ~80 mA | Brief bursts during ESP-NOW sends |
| ESP32-C3 | WiFi RX listening | ~20 mA | ESP-NOW receive-window |
| ESP32-C3 | Light sleep, radio off | ~0.8 mA | Wake on timer or interrupt |
| ESP32-C3 | Deep sleep | ~5 µA | No RTC RAM, full restart on wake |
| WS2812B (1 LED) | Ambient: 10% white | ~4 mA | Default operational mode |
| WS2812B (3 LEDs) | Showy: 30% white | ~30 mA | Color-fringing mode |
| WS2812B (9 LEDs) | Burst: 100% white | ~280 mA peak | Wand-interaction event, brief |
| AP2112K-3.3 | Quiescent | ~55 µA | Add to all sleep estimates |
| Battery sense | ADC measurement | ~0.5 mA × ms | Gate on/off via GPIO; sample once/min |
| CN3058 | Quiescent (charging) | ~150 µA | Negligible when sun is up |
| CN3058 | Standby (no input) | ~10 µA | At night |

## Power budget — single fixture, single night

**Assumptions:**

- 100 fixtures; each is autonomous on its own battery and panel.
- Active operation from dusk (~7:30 PM) to dawn (~6:00 AM) = **10.5 hr active**
- Daylight charging window: ~6 hr of strong sun, ~2 hr of partial sun (panel partially shaded by bamboo skin and the tree's canopy)
- Mesh duty cycle: ESP-NOW sends own state every 1 s, listens between. Effective average MCU current: ~5 mA (mostly light sleep with periodic wake).
- LED usage averaged over the night: **time-weighted average of ~5 mA**. Most of the night is "default ambient" (1 LED at ~10% = 4 mA), interspersed with brief "showy" mode (3 LEDs at ~30% = 30 mA). Wand-burst peaks (9 LEDs full) are brief and rare.

**Night drain:**

| Component | Current avg | Hours | Charge consumed |
|-----------|-------------|-------|-----------------|
| ESP32-C3 (mostly light sleep, periodic wake) | 5 mA | 10.5 | 52.5 mAh |
| WS2812B (1–3 LEDs at low/mid brightness, time-weighted avg) | 5 mA | 10.5 | 52.5 mAh |
| LDO + miscellaneous quiescent | 0.2 mA | 10.5 | 2 mAh |
| **Total night drain** | | | **~107 mAh** |

**Day standby + charge:**

| Component | Current avg | Hours | Charge consumed |
|-----------|-------------|-------|-----------------|
| ESP32-C3 deep sleep + periodic wake to maintain mesh sync | 0.5 mA | 13.5 | 7 mAh |
| Charger quiescent | 0.2 mA | 13.5 | 3 mAh |
| **Total day standby drain** | | | **~10 mAh** |

**Daily total drain: ~120 mAh.**

**Solar harvest estimate:**

| Panel | Strong sun (6 hr) | Partial sun (2 hr) | Daily harvest |
|-------|-------------------|---------------------|---------------|
| 1 W | ~150 mA | ~30 mA | 960 mAh |
| 2 W | ~300 mA | ~60 mA | 1920 mAh |
| 3 W | ~450 mA | ~90 mA | 2880 mAh |

(Numbers assume ~5 V panel, charger conversion ~80%, partial bamboo shading derated.)

**Verdict:** A 1 W panel covers the 120 mAh daily drain with 8× margin. A 2 W panel gives ~16× margin and tolerates heavy shading or much higher LED duty cycle (e.g. for sustained wand-interaction events). **Recommendation: target 1–2 W panel — 1 W is sufficient if hat layout constrains panel area; 2 W provides margin for the 2027 brighter-mode spec.**

**Battery sizing:**

| Cell | Capacity | Total Wh | Nights of autonomy at 120 mAh / night, no sun |
|------|----------|----------|------------------------------------------------|
| LiFePO4 14430 | ~400 mAh | 1.3 Wh | 3.3 nights |
| LiFePO4 18650 | ~1500 mAh | 4.8 Wh | 12.5 nights |
| LiFePO4 26650 | ~3000 mAh | 9.6 Wh | 25 nights |

**Recommendation: LiFePO4 18650** — well-known cell format, robust holders available, gives ~12 nights of dark-survival autonomy if panels ever fail. Weight is ~50 g, well inside the 1 kg per-fixture structural budget. The 14430 (~3 nights autonomy) is now reasonable and would save weight + cost if cell sourcing is tight, but 18650 is the safer call for a 2-year-life deployment where battery aging is a factor and where the desert can easily produce multi-day dust storms blocking panels.

## Power budget — back-of-envelope max-stress check

What if the wand-interaction events trigger every fixture into "9 LEDs full white" mode for 30 min total over a night?

- 9 WS2812B at full white = 280 mA. 30 min × 280 mA = 140 mAh.
- Same fixture's regular 10 hr ambient at 5 mA avg = 50 mAh.
- ESP32-C3 active during the burst (extra mesh traffic): ~30 mA × 30 min = 15 mAh.
- Total stress night: ~210 mAh.

Still inside 18650 capacity (1500 mAh — using 14% in the worst night). Still inside 1 W panel daily harvest (960 mAh — replacing ~22% of capacity in one good day). System tolerates considerable showy-mode duty cycle without battery deficit. The 14430 (400 mAh) starts to feel marginal here (52% drain in stress night) — another reason to prefer 18650.

## What's NOT in this budget

- Cold-start charging from depleted battery. CN3058 handles this with trickle-charge below 2 V; tested behavior should be verified on bench.
- Long sleep / shipping mode. Battery self-discharge of LiFePO4 is ~3% / month; storage for 1+ year between BM 2026 and 2027 means the 100 fixtures need to be brought up to charge before deployment. Plan for a desert-arrival "charge day" at BRC.
- BMS / overcurrent protection. CN3058 has internal over-temp and over-voltage protection but no short-circuit cutoff on the load side. A polyfuse on the battery output is cheap insurance and worth adding.
- PCB and connector resistance losses. Accounted for in ~80% charger conversion estimate.

## Sanity check against `INV_2026_00401`

When the cost decomposition for INV_2026_00401 is done (see `TODO.md`), the BOM-cost target should be substantially below what's quoted there. Rough sketch:

- ESP32-C3-MINI-1 module: ~$1.50
- CN3058 charger + supporting passives: ~$1.00
- AP2112K-3.3 LDO + caps: ~$0.30
- 4× WS2812B: ~$0.50
- USB-C connector: ~$0.50
- JST-PH connectors × 3: ~$0.60
- Misc passives, MOSFET, Schottky, LED indicator, fuse: ~$1.50
- **PCB SMT-assembled at qty 100 (JLCPCB): ~$3 / board**
- LiFePO4 18650 cell with holder: ~$3
- 2 W solar panel (Voltaic or generic): ~$5
- **Per-fixture electronics target: ~$17**

If the invoice is 3–5× this number, the cost driver is probably labor or hand-assembly assumptions; pushing to JLCPCB SMT assembly closes the gap. If the invoice is <2× this, the gap may be enclosure + battery + panel, which we control here.
