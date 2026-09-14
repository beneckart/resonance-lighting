# 2027 solar LFP fixture platform -- design exploration

Date: 2026-09-14

Status: discussion draft, not an accepted architecture, BOM, or deployment plan.
Ben wants to use the coming year to assess the whole firmware/tooling feature
set. The proposed T-Deck per-fixture recovery command is explicitly deferred.
No device operations, firmware changes, purchases, or existing ADR replacements
are authorized by this document. The existing fleet is the comparison baseline.

## Proposed organizing principle

The power subsystem should safely charge, measure recovery, and decide when to
wake the art controller while the ESP32 is asleep, unpowered, or broken.
Useful diagnostics must survive that controller's resets. Independently switched
loads must not be able to drain a cell indefinitely because an application hung.

This is more valuable than maximizing the number of sensors or bus speed.
Use autonomous charger/protection/RTC circuitry where it suffices. Evaluate a
small low-power supervisor only if energy accounting, recovery logic and fault
recording require one; its firmware, update path, current and failure modes also
have to be qualified. A second MCU is an option, not a prerequisite.

Conceptual power flow (not a circuit; switches, protection and measurement
placement require an electrical design):

```text
Solar -> input protection -> solar V/I measurement ->+-> charger input selector
                                                    |            ^
                                                    |            |
                                                    |       USB service input
                                                    |
                                                    +-> controlled chime-cap charge
                                                               |
                                                cap V / driver / bounded strike

Charger/power path <-> battery shunt + LFP protection <-> cell + NTC
          |
          +-> low-power gauge / clock / wake control
          +-> switched ESP32 logic supply
          +-> separately switched LED and sensor supplies
```

All material battery currents must pass through the measurement boundary;
unavoidable upstream leakage must be included in its error budget. The chime
path shown is solar-fed; whether to allow controlled battery-backed strikes is
a separate artistic/energy decision, not implicit in this sketch.

## Direct answers and priority

| Question | Proposed direction | Why |
|---|---|---|
| Solar-path INA? | Yes to solar-only measurement and an R&D monitor option; qualify the production implementation's sleep cost. | Distinguishes harvest from USB and captures power used by the separate chime branch. |
| Separate MAX bus? | Put charger and gauge on a short private power bus; put cable-connected sensors on another bus. | Contains peripheral faults; no evidence yet that gauge and charger need separate buses from each other. |
| Remove 100 kHz constraint? | Remove it from the sensors by separating buses. Keep existing PowerFeathers at 100 kHz. | Power telemetry needs little bandwidth; qualify higher speed on a new PCB only if useful. |
| Remove SOC/gauge? | Keep current sensing and evaluate characterized gauging; stop treating an unvalidated percentage as truth. | Battery energy accounting is valuable even when the percentage model is wrong. |
| Better chime readiness? | Measure capacitor voltage; instrument pulse current in R&D. | The cap is the immediate energy store; battery SOC is not its charge state. |
| More sleep modes? | Define independent charging, logic, load and wake behavior, with a small number of clear user modes. | A date wake and indefinite service storage have different wake contracts. |
| Highest priority? | LFP-correct autonomous charging, final battery cutoff, isolated buses and whole-cycle recovery evidence. | These address failure/recovery, rather than merely describing failures better. |

## What the existing evidence actually says

- The September 12 census had 40 PROTECT fixtures at >=3.25 V. It did not record
  qualified sunny recovery and does not prove 40 healthy cells or 40 software
  faults. The offline recovery reproduction shows how net-positive charging
  across sleep/wake can fail an awake-only charge proof. See the
  [recovery investigation](../../tests/PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md).
- The 400 kHz versus 100 kHz clock-only A/B and later 46-hour soak support the
  bus-speed containment rule. The exact switch-opening register upset was not
  captured on the wire. Core placement was subsequently reclassified; single
  ownership and isolation matter more than blaming a particular core. See
  [ADR 0028](../../decisions/0028-power-management-bus-integrity.md).
- Production 6 Ah cycle tests reported 1% SOC with about 40% of measured
  capacity remaining; the 2 Ah cell had a better empty edge. There is no
  demonstrated fleet-wide SOC accuracy qualification. Current calibration at
  ordinary loads was useful: one fit was gauge = 1.080 * INA + 2.4 mA. That
  is not validation at microamp storage currents. See
  [PowerFeather notes](../../../firmware/POWERFEATHER_NOTES.md) and
  [ADR 0023](../../decisions/0023-lfp-power-policy-thresholds.md).
- The current fixture initialization requests generic LFP with a configured
  capacity. The locally installed SDK is modified, so auditing historical
  artifacts is necessary before attributing past errors to a particular model
  or initialization bug.
- Chime operator attempts were deliberately decoupled from inferred energy
  permission because cap charge was not measured. See
  [ADR 0065](../../decisions/0065-operator-knocks-are-best-effort-attempts.md).

## Charging and source qualification

Require an LFP-correct charge ceiling from first power application, after charger
reset, and with the ESP32 held off. The BQ25628E has useful power-path management,
VINDPM and voltage/current ADCs, but its VREG reset default is 4.2 V. That motivates
hardware/nonvolatile chemistry defaults or an independent enable interlock in a
dedicated LFP design. This is not evidence that the deployed fleet was overcharged.
[TI BQ25628E datasheet](https://www.ti.com/lit/ds/symlink/bq25628e.pdf).

Specify cell-contact temperature sensing, cell-specific charge-temperature limits,
and a final hardware over/undervoltage/current protection boundary independent
of application firmware. Operational energy conservation should act earlier than
the emergency cutoff. Measure leakage through every path after cutoff. An extreme
low reading must lead to diagnosis, not an unconditional automatic charging retry.

Give USB and solar distinct presence/status signals and deliberate input priority.
Measure panel power before the USB join and before the chime branch. Report input
voltage, actual draw, charger limiting reason, and net battery current separately.
A high open-circuit panel voltage is not proof it can sustain the next load.

Test source capability with a bounded load/operating-point change when needed.
Low panel current can mean shade, a full battery, a configured charge limit,
temperature limiting, an input fault, or simply no demand. An INA measures drawn
power, not unused harvest potential. An irradiance sensor is optional research
instrumentation, not necessary for ordinary power control.

Compare autonomous MPPT against the present panel-specific VINDPM approach using
daily harvested Wh under real shade, dust and temperature. Better tracking is
worthwhile only if its harvest benefit exceeds electronics overhead and it
recovers reliably at dawn. LTC4162-F is an example of an integrated LFP/MPPT
architecture to study, not a selected replacement.
[ADI LTC4162-F](https://www.analog.com/en/products/ltc4162-f.html).

## Energy measurements and their own energy cost

Prefer accumulated battery charge through the entire sleep/wake cycle over
integration of sparse radio-on snapshots. Solar Wh, battery net mAh/Wh and
load energy are different quantities. Include explicit measurement boundaries,
counter resets, overflow, elapsed time, validity and gaps; never silently
bridge a missing interval with a zero reading.

INA228 is a useful R&D example: continuous mode accumulates energy while the
host sleeps; triggered mode does not provide valid accumulation. Its typical
640 uA is 5.61 Ah/year at the monitor supply (18.5 Wh/year at 3.3 V), excluding
conversion losses. Consider a lower-power device or qualified solar-powered
operation with dawn/shade reset and gap handling. Do not claim continuous
measurement after shutting off the measuring chip.
[TI INA228 datasheet](https://www.ti.com/lit/ds/symlink/ina228.pdf).

Useful arithmetic for year-long storage, not a board measurement:

| Continuous current | Charge per 365 days |
|---|---:|
| 10 uA | 87.6 mAh |
| 50 uA | 438 mAh |
| 1 mA | 8.76 Ah |

Component current at a regulated supply is not automatically battery current.
Final targets must be measured at the cell, with all connectors/accessories,
temperature, regulator losses and battery self-discharge accounted for.

## SOC, battery identity and usable energy

MAX17260 already combines current sensing and a battery model. Its datasheet
lists 5.1 uA typical hibernate and 15 uA active current. Its measurement range
starts at 2.3 V, and startup is specified at 3 V. Keep an independently powered
service measurement path for a missing/mute gauge or very low battery.
[ADI MAX17260 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17260.pdf).

There is no simple direct SOC sensor for an LFP cell. Estimate it from calibrated
charge accounting, a cell model, temperature and known reference states; LFP's
flat voltage plateau limits voltage-only inference.
[TI LFP design considerations](https://www.ti.com/lit/pdf/sluaar1).

Qualify the actual 6 Ah and 15 Ah cell types at representative loads. Check model,
shunt value, scaling, current offset, sample cadence, capacity configuration,
learned-state persistence and battery swaps. The SDK offers custom characterized
models and warns that battery changes can invalidate learned history.
[PowerFeather SDK usage notes](https://docs.powerfeather.dev/sdk/usage-notes/).

Show SOC only with a qualification/confidence state; use 'unknown' where needed.
Keep measured charge since a reference, usable capacity above the operational
floor, voltage under load, and the assumed light program alongside runtime
estimates. Do not inherit one cell's learned state onto a replacement. A simple
scan/record of a battery label may suffice; electronic pack identity is optional.

Add cell-contact NTC, keyed connectors, battery-present evidence and accessible
test points on both sides of protection/power-path switches. Connector/NTC
presence alone does not prove a healthy cell or a sound power contact. A brief
known-load test can expose path resistance, but does not measure remaining Ah.

## Load and fault observability

Prioritize these measurements over another opaque percentage:

| Measurement | Decision it supports |
|---|---|
| Panel V/I and accumulated Wh | How much solar was actually collected? |
| USB presence and input selection | Which source is powering service/charging? |
| Cell and system voltage, signed battery current, accumulated charge | Is the cell gaining energy over the whole cycle? |
| Cell temperature and charger enable/phase/fault/limit state | Why is charging allowed, limited, or stopped? |
| LED rail voltage/current and rail-enable truth | Did the commanded load actually turn on/off; is a harness suspect? |
| Chime-cap voltage before/after a strike | Was electrical strike energy available and consumed? |
| Fast minimum-voltage/overcurrent capture and reset reason | Did cell voltage, the power path, or the regulated rail fail first? |
| Sample age/source/validity and reset counters | Is the displayed evidence current and comparable? |

For a known capacitance, stored energy is E = 0.5*C*V^2; usable cap energy between
two voltages is 0.5*C*(Vhigh^2 - Vlow^2). This is electrical energy, not acoustic
output. Concurrent charging, capacitance tolerance, ESR and conversion loss must
be accounted for. Validate cap voltage against actual strike quality and instrument
pulse current during development. Current draw still cannot prove a mallet made
contact; mechanical confirmation is a separate optional sensor.

Separate ESP32, LEDs, sensors and chime charging/driver power domains. Include
hardware default-off behavior, controlled inrush, flyback protection and an
independent maximum strike pulse. Size the light supply for actual optical needs
and isolate its transients from logic. A higher-resolution light driver is worth
an artistic A/B, not an automatic replacement of the deployed pixel modules.

Use a retained event log with time, source/rail state, minimum voltages, guard
entry/release, fault and firmware revision. A regular heartbeat cannot catch a
millisecond collapse; use appropriate comparators/latches or a fast local capture.
FRAM or a small supervisor is optional if ordinary retained memory and bounded
flash checkpointing cannot meet endurance/power-loss requirements.

## Sleep and wake contracts worth exploring

| User mode | Hardware intent | Wake contract |
|---|---|---|
| Operate | Scheduled radio, art and sensing | Existing schedule/control with explicit expiry |
| Harvest/recover | Charge and measure; expensive loads off | Qualified recovery, bounded service window, or explicit service |
| Sleep until date | Main logic/art off; clock retained | Chosen date; separately define whether USB service is allowed |
| Store until service | Logic/art off indefinitely | Accessible button/magnet or USB; solar behavior chosen explicitly |
| Emergency cell cutoff | Stop further damaging discharge | Cell-specific controlled recovery, distinct from user storage |

An RTC such as RV-3028-C7 demonstrates calendar/alarm capability at 45 nA typical
at 3 V. Driving a power latch makes a date wake possible with ESP power removed;
the rest of the wake circuitry and temperature-dependent clock error still count.
The alarm's match semantics, year/month handling and retained clock validity need
qualification for an exact return date.
[Micro Crystal RV-3028-C7](https://www.microcrystal.com/en/products/real-time-clock-rtc-modules/rv-3028-c7).

A service button or magnetic switch should reach the actual wake circuit, not
just ESP RESET. Isolate signal lines to prevent back-powering an off domain.
Scheduled storage should not accidentally turn into 'wake at every dawn' when
solar returns; optional charging without waking the art controller is useful.
An unpowered ESP cannot hear ESP-NOW. Retain scheduled rendezvous windows, or
separately justify a dedicated wake receiver if instant remote wake is required.

A local RTC would also improve shared wake rendezvous and time holdover. It would
not replace network time anchors or prove chime synchronization accuracy. This
would be a future extension of ADR 0031, not a silent change to the 2026 design.

## Firmware/tooling review and retirement candidates

Keep a single model of operator intent, energy permission and hardware faults,
with one owner of recovery/sleep decisions. Preserve distinctions between low
energy, intentional storage, a hardware fault and telemetry unavailable. Surface
the actual waiting reason and next check, rather than a solitary PROTECT badge.

Candidates to consolidate or retire after an inventory and replacement proof:

- Unqualified SOC/SOH/time-to-empty numbers presented as authoritative.
- Repeated ESP boot/polling solely to learn whether charge accumulated.
- Multiple independent sleep owners and wake controls with surprising exclusions.
- Commissioning/service modes that accidentally persist into field operation.
- Duplicate/deprecated maintenance paths and old experiment flags in operator UI.
- General-purpose Feather features, always-lit indicators or unused auxiliary
  functions whose compatibility benefit does not justify board space or leakage.

Retain raw measurements, charger limiting/fault states, USB rescue, exact artifact
identity, rollback, immutable command receipts and per-target recovery evidence.
Do not remove a protection or diagnostics path merely because its old UI was poor.
The proposed explicit T-Deck recovery command remains on the design backlog.

## Evidence-driven next steps, without a rushed fleet replacement

1. Preserve Oakland specimens and map cell/harness/firmware identity. Reproduce
   the recovery defects with measured currents; audit historic gauge setup.
2. Inventory tooling and firmware features against real operator workflows:
   commissioning, diagnosis, show control, maintenance, storage and repair. Mark
   each keep/improve/consolidate/retire with evidence and a replacement path.
3. Build an instrumented bench comparison, then a small R&D board only where
   the existing hardware cannot test the requirement. Include optional monitors
   and test points; avoid selecting a full fleet BOM now.
4. Test weak dawn, intermittent shade, full battery, missing battery, supply
   transitions, charger reset with ESP held off, stuck sensor bus under radio
   TX, LED/chime pulses, battery swaps and intended storage wake sources. Measure
   whole-cycle energy and record whether recovery occurs without service.
5. Qualify SOC against external charge accounting on each cell type, including
   partial cycles, long dark periods, temperature and storage-current offsets.
   A repeatable error bound, not a plausible-looking plot, is the release gate.
6. Run outdoor comparative soaks on a small cohort. Set numeric leakage,
   harvest, recovery, reachability and show-quality targets before choosing the
   next production architecture. Record accepted choices in future ADRs.

The near-term deliverable is a requirements and test program. Hardware changes
should earn their place against a hardened version of the PowerFeather fleet.
