# Gotion 33140 15 Ah qualification on ET5406A+ (2026-08)

## Purpose

Qualify the new large-hat cell with an instrument-controlled discharge and collect
the voltage/capacity curve needed to re-derive ADR 0023 thresholds. Repeat on at
least two cells before qualifying the batch.

## Test point

- Cell: Gotion 33140 LiFePO4, 3.2 V, 15 Ah label.
- Discharge: 1.000 A constant current (0.067 C).
- Cutoff: 2.500 V under load, enforced by the ET5406A+ battery-test mode.
- Record both total capacity to 2.5 V and product-usable capacity above 3.0 V.
- Logger: `ops/bench/et5406_discharge.py`.

The 1 A point is deliberately close to the prior 32700 fixture-load test while
remaining in the ET5406A+ low-current range. It is for project qualification, not a
claim about the vendor's rating-test conditions.

## Sample 1 actual segmented protocol

Expediency for the Nevada City production decision superseded the uninterrupted
1 A trace for sample 1. Preserve and sum the host integrals across all segments;
the ET counters restart whenever its channel is reactivated.

- Segment 1: 1 A, manually paused after 0.374 Ah / 1.202 Wh host-integrated.
- Segment 2c: 5 A, manually paused after 4.501 Ah / 12.600 Wh host-integrated.
- Pre-resume cumulative result: 4.875 Ah / 13.802 Wh.
- The 22 AWG alligator leads measured about 0.093 ohm including contacts by the
  simultaneous loaded DMM-versus-ET comparison. ET-terminal voltage and energy
  therefore exclude substantial lead loss at 5 A and must not be mistaken for
  cell-terminal voltage/energy.
- Segment 3: hardware-native two-stage battery mode -- 5 A to 2.550 V at the ET
  input (approximately 3.0 V estimated at the cell using the measured lead path),
  then 1 A to the final 2.500 V ET cutoff.

This segmented result is sufficient for a rough fleet-capacity decision but is not
the original controlled 1 A qualification trace. Use sample 2 for the repeatable
protocol after fitting short, heavier test leads.

## Sample 1 charge preflight

The first connected PowerFeather sample was identified as `9E5AB8` on COM4. It had
old `net-bench-2026-07-13.3` firmware configured as `Generic_3V7`, which is unsafe
for an LFP cell because it selects the Li-ion charge profile. It was replaced with
the previously verified
`firmware/net_bench/build/serial-bridge-20260708-adr23-latch-tail` artifact, whose
build options specify `Generic_LFP`, 1,500 mA maximum charge, and 4.6 V input
maintenance voltage.

After correction, live telemetry showed approximately 3.528 V and +1.61 A into the
cell. The cell was therefore still accepting essentially the full programmed charge
current and was not full. The reported 99 percent SOC is not usable: this old image
has a 6,000 mAh gauge capacity setting, not 15,000 mAh.

Treat the charge as complete only after the cell reaches the LFP CV region near
3.60 V and net battery current tapers to approximately 120 mA or less. Then disconnect
all charging and rest the cell for at least one hour. Record rested open-circuit
voltage and ambient temperature before moving it to the electronic load.

## Wiring and start checklist

1. Leave the ET5406A+ channel OFF. Disconnect the cell from the PowerFeather.
2. Rest the cell disconnected for at least one hour.
3. Inspect the cell for damage or abnormal warmth. Do not test a suspect cell.
4. Connect cell positive to ET positive and cell negative to ET negative with short,
   adequately sized leads. Check polarity with a DMM before enabling the load.
5. Run the read-only status command and confirm a plausible single-cell voltage:

   ```powershell
   python ops/bench/et5406_discharge.py --port COM41 --status
   ```

6. Arm and verify the test while leaving the channel OFF:

   ```powershell
   python ops/bench/et5406_discharge.py --port COM41 --arm
   ```

7. Start only after the wiring and rested-voltage checks pass:

   ```powershell
   python ops/bench/et5406_discharge.py --port COM41 --run --yes `
       --battery gotion-33140-15ah-sample-1 --ambient-c <deg-C>
   ```

The logger refuses to start outside 3.30-3.70 V, exclusive-creates its JSONL output,
records ET and host Ah/Wh integrals, records capacity above 3.0 V, and forces the
channel OFF on every normal, cutoff, error, or keyboard-interrupt exit. The internal
2.500 V cutoff is primary if the host or USB connection fails.

## Immediate post-run

1. Confirm the ET channel is OFF and record the final display values.
2. Disconnect the cell from the load. Do not leave an empty cell attached overnight.
3. Reconnect it to a verified LFP charger promptly and observe the initial recovery.
4. Preserve the raw JSONL file. Analyze the knee, capacity to 3.0 V, capacity to
   2.5 V, initial loaded sag, and ET-versus-host integration agreement.
5. Repeat the same protocol on sample 2 before changing the fleet qualification or
   ADR 0023 threshold map.
