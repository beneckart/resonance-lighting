# 0018 -- LED module/interface selection

**Date:** 2026-05-10
**Status:** Accepted 2026-05-10; **substantially revised 2026-06-04** after V2 battery
bench work. Net: IS31FL3741 13x9 is **ruled out** for the V2 battery build; the LED
module is **deliberately left undecided** -- SK6812 "HEX" direct-GPIO and the 4 W RGBW
point source are both still live and serve *different optical roles*. The original
2026-05-10 decision (IS31 primary, NeoHEX secondary) is preserved at the bottom as
superseded.
**Owners:** Ben + Steve

## Context

The project wants future-proof LED behavior that serves **two distinct optical modes**
through the bamboo-node gobo/filter:

1. **Crisp mandala shadows** -- a stencil cast on the ground/canopy. Shadow sharpness is
   set by the source's *angular size as seen from the gobo*: penumbra width grows with
   source size and shrinks as the source moves away from the gobo. This wants a **small
   point source**. A spatially-extended array cannot throw a crisp shadow no matter how
   it's driven, and a multi-die RGB pixel additionally throws *colored* penumbra fringes
   (R/G/B emit from offset dies -> three offset shadows).
2. **General ambient wash / glow** -- soft, diffuse light, optionally with the gobo
   *washed out* on purpose. This wants an **area source** (or a point source close to /
   without the gobo).

So "best LED" is partly an **application question**, not a single electrical winner -- a
point source and an area source are good at opposite ends. A third, untested idea sits in
the middle: animate a **single lit pixel swept across an array** so the *cast shadow
moves* -- possibly a compelling effect, possibly just mushy. Needs gobo testing.

Interface distinctions (unchanged, still relevant):

- **STEMMA-QT/Qwiic** is an I2C connector family (JST-SH): power, ground, SDA, SCL.
- **Grove/HY2.0** is a *physical* connector family; it can carry I2C, UART, GPIO, analog,
  or custom signals depending on the device.
- **M5Stack NeoHEX** uses Grove/HY2.0 physically but is a WS2812C single-wire LED board,
  **not** an I2C device.

New context from the 2026-06-04 V2 battery bench work (see LOG 2026-06-04 entries +
`docs/tests/BATTERY_BROWNOUT_INVESTIGATION_2026-06-03.md`):

- The **IS31FL3741 on the PowerFeather V2's shared charger/gauge I2C bus reliably browns
  out the board on battery under WiFi** -- a VSYS power-on-reset loop. Proven IS31-*specific*
  (n=2 boards; the same cell/bus/WiFi is stable with the IS31 unplugged, and stable with an
  Adafruit NeoDriver on the same bus), and it triggers even with LEDs off, so it's the
  IS31 chip's behavior on SDA/SCL, not LED current and not a general bus property.
- **Direct-GPIO WS2812/SK6812 (off the I2C bus) is brownout-safe by construction** and
  validated on hardware (HEX on GPIO10/A0).
- Efficiency numbers measured so far (e.g. HEX ~1.6x the PAR/mA of WS2812C NeoHEX) are
  **muddied** because each run sat at a different battery SOC -> a different buck-boost
  operating point. Treat them as *system* efficiency at as-measured conditions, not a
  clean intrinsic ranking; re-rank at a fixed VBAT before trusting the slopes.

## Decision (2026-06-04 -- current)

1. **IS31FL3741 13x9: ruled OUT for the V2 battery build.** Shared-bus brownout, well
   characterized and IS31-specific. Revisit only if the 13x9 *grid* form factor becomes a
   hard requirement -- and then only via an untested mitigation (VSYS bulk cap, or moving
   it off the shared bus onto the second I2C bus, GPIO35/36).

2. **No single LED module is selected yet.** Two live, *complementary* candidates remain;
   the choice depends on gobo testing and on better low-voltage characterization, and may
   end up being "both, for different modes":
   - **SK6812 "HEX", direct-GPIO @ 3.3 V (off the I2C bus)** -- the **area / ambient-glow**
     candidate. No boost, fewest parts, brownout-safe by construction, software-cuttable if
     fed from the switchable 3V3 rail. Apparent efficiency edge over WS2812C NeoHEX, subject
     to the SOC-confound caveat above. Washes the gobo (a feature for ambient, not for crisp
     shadows).
   - **4 W RGBW single emitter (Adafruit 5163)** -- the **point-source / crisp-gobo**
     candidate, and the only one that can throw sharp mandala shadows. Brightest and most
     efficient at high brightness. **Undervolting at 3.3 V is viable -- 5 V is NOT strictly
     required** (Ben, prior experience); the one bench run that looked "voltage-starved"
     simply did not cleanly characterize its low-V curve (non-monotonic mid-range current
     near its Vf). Its usable 3.3 V dimming range, color balance, and max brightness -- and
     therefore whether any boost is worth it -- remain **open**.

3. **NeoHEX (WS2812C-2020) via Adafruit NeoDriver** stays a viable **no-solder fallback**
   LED path (stable on the shared bus, self-contained data level-shift), but it's the
   *least* efficient option measured, and the NeoDriver only level-shifts the **data**
   signal -- it does **not** boost pixel power (pixels run at whatever feeds Vin). Demoted
   from "leading" to fallback.

4. **Pixel-power architecture: undecided.** Options under consideration (see TODO):
   (a) switchable 3V3 header -- dim but software-cuttable, zero extra parts;
   (b) VBAT -- brighter but always-live, needs a load switch;
   (c) 5 V boost fed *from* the switchable 3V3 -- full brightness and still cuttable, +1 part.

5. **Keep the LED module choice flexible** in the custom-board / enclosure design until
   optics (gobo) and the RGBW low-V characterization are done. Do not freeze a single
   emitter into the BOM yet.

## Consequences

- The custom board and hat must accommodate **either** a direct-GPIO addressable strip/hex
  **or** a single high-power RGBW emitter on the optical axis -- and possibly a 5 V boost as
  a populate/DNP option.
- A free GPIO for direct pixel data (off the I2C bus) is now a hard requirement, not an
  option -- that path is what makes the LED brownout-safe.
- Firmware must support a single-pixel / swept-pixel mode (for the moving-shadow test) in
  addition to full-array -- a small add on top of the existing `--pixel-pin` + brightness/
  sweep bench modes.
- WS2812/SK6812 **latch their last frame**; firmware must send an explicit all-off on
  shutdown/sleep or the pixels stay lit and keep drawing (see ADR 0013, LED fail-safe).
- Efficiency-based BOM claims are on hold until a fixed-VBAT re-rank removes the buck-boost
  SOC confound.

## Open questions / tests required

- **Gobo projection test** (point vs area source; crisp-shadow vs wash; chromatic fringing
  on RGB pixels; the swept-single-pixel moving-shadow idea). Rig available: lantern inverted,
  flat sample filters, source resting under the cylinder, shadow on the ceiling.
- **RGBW low-voltage characterization** at 3.3 V -- usable dimming range, color balance, max
  brightness -- to decide whether a 5 V boost is warranted at all.
- **Fixed-VBAT efficiency re-rank** (bench supply or SOC-correction) to clean the PAR/mA
  comparison.
- **Overnight / multi-hour direct-GPIO stability run** (now unblocked by the auto-sleep
  loop-breaker) to confirm the HEX-direct path the way the IS31 never was.
- **Pixel-power architecture bench check**: does cutting the 3V3 rail kill the pixels while
  the I2C devices stay alive (ideal: LEDs off, bus up)?
- Per-module: current draw at representative brightness, rail-off leakage / back-powering,
  sleep current with module connected, mechanical placement on the optical axis, diffuser
  on/off behavior.

---

## Superseded -- original decision (2026-05-10)

> Preserved for the record. This was the COTS-testing-phase plan, before the V2 battery
> bench work ruled out the IS31 and reframed the choice around gobo optics.

**Original context:** the project wanted future-proof LED geometry -- ideally 3x3, 5x5, or
larger -- while keeping default optical behavior crisp through the gobo/filter. The first
COTS survey identified several LED boards, but not all were no-solder or compatible with
PowerFeather's STEMMA-QT connector.

**Original decision:** Use two LED paths in parallel for testing:

1. **Adafruit IS31FL3741 13x9 matrix** as the primary no-solder PowerFeather/STEMMA-QT LED
   module.
2. **M5Stack NeoHEX** as the primary no-solder WS2812/GPIO geometry experiment.

FeatherS2 Neo and Atom Matrix remained integrated 5x5 fallback/optics boards.

**Original consequences:**

- PowerFeather + IS31FL3741 was expected to be the cleanest no-solder COTS stack: STEMMA-QT
  cable, I2C control, switchable `VSQT` rail. **(Invalidated: shared-bus brownout on
  battery -- see the revised decision above.)**
- NeoHEX must be wired as GPIO data + power + ground; may require a 5 V or otherwise suitable
  LED rail and is not plug-and-play on STEMMA-QT.
- The IS31FL3741 matrix is multiplexed PWM over I2C, not NeoPixel; must be tested for gobo
  projection artifacts, brightness, and current.
- NeoHEX has many LEDs and significant current in full-white modes; firmware must cap
  brightness/current from the start.
- The final custom board should keep LED module choice flexible until optics tests complete.
  **(Still true -- reaffirmed above.)**
