# Ness overnight check: voltage and gauge observations

Date: 2026-10-12 PDT
Target: Ness, 9F2738, D8:85:AC:9F:27:38, COM14
Firmware: fx-261012-5164506-b
Operator: Ben + this Codex laptop; read-only observation

## Conditions and method

Ben connected a retrofitted lantern the next morning to check drawdown. He
reports unplugging it after flashing/service, leaving it indoors in a dark
room, with no extra chimes. He confirmed a physical 15 Ah cell. The precise
disconnect time was not logged; Ness's last available service telemetry is
2026-10-11 18:38:42 PDT, after the last logged signature replay.

The watcher started at 10:31:28 PDT with no serial port. Ness enumerated at
10:31:55 and returned telemetry at uptime 1400 ms with reset reason
`deepsleep`. No reset, chime, event-session, configuration or charger command
was issued; only read-only `t` queries. A temporary host awake hold was
released after capture. USB was already physically connected, so this is
not an isolated battery-only measurement.

## Observations

| Observation | Battery voltage | Gauge SOC | Conditions |
| --- | --- | --- | --- |
| Last logged service sample, Oct 11 18:38:42 | 3.334 V | 89% | USB charging, +184.8 mA reported, event session active |
| Retained sample before the preceding 120 s sleep | 3.278 V | Not recorded | Ordinary day-charge sleep; no UTC timestamp in retained record |
| First morning sample, Oct 12 10:31:55 | 3.303 V | 62% | USB present, charging disabled during startup, LED rail off |
| Five startup samples before charging enabled | 3.300-3.303 V | 62% | Same wake, uptime 1400-5818 ms |
| After charging enabled, Oct 12 10:32:04-05 | 3.314 V | 62% | About +237.5 mA reported charging current |

The interval between the two logged endpoints is 15 h 53 min 13 s, not a
precisely measured unplugged interval. Relative to the last service sample,
the first morning voltage is 31 mV lower; the retained pre-sleep value is
56 mV lower. The retained sleep duration and wake uptime place the preceding
sleep entry approximately at 10:29:54 PDT, likely before Ben's just-reported
USB connection. This is an estimate, not a recorded wall-clock timestamp,
and the retained battery value may be a recently cached sample.

Morning state: FULL power-policy tier, event session expired (`0` seconds),
healthy downlight sensors, LED rail off, no charger fault and no interrupted
boot flag. FULL denotes power allowance, not a full battery. All retained
PROTECT fields exactly match the prior historical record: reset-load-armed,
3242 mV, predecessor DIM, brownout reason 9, streak 1 at historical uptime
2603 ms. There is no newly recorded PROTECT entry. A current natural wake
does not independently prove uninterrupted operation throughout the night.

## What this establishes and what it cannot measure

Ness remained operational into the morning and is now using ordinary
120-second sleep with no active event session. The observed voltage change
and 27-percentage-point gauge decrease are recorded, but neither is a
validated measurement of total capacity consumed.

The night sample was charging and the morning startup sample already had
USB powering the board, so their voltages are not comparable resting-cell
measurements. The gauge is still configured for 6,000 mAh despite the actual
15,000 mAh cell. LiFePO4 SOC is model-sensitive on its voltage plateau, and
the VBAT-fed capboard bypasses the gauge current shunt. Thus the 27-point
change cannot be treated as 27% of the 15 Ah battery, or converted reliably
to Ah, Wh, average current or remaining runtime. The 6 Ah setting was left
unchanged to preserve this observation.

For a quantitative repeat, measure charge/energy at the cell connection
upstream of both the PowerFeather and capboard branches, with recorded
start/end times and controlled initial charge, event duration and chime
count. Record capboard standby current and the complete sleep/wake cycle.
Correcting the gauge's capacity alone will not measure the bypass branch.

References: `firmware/POWERFEATHER_NOTES.md` (VBAT shunt bypass and RepSOC),
`hardware/capboard/README.md`, ADR 0023, and the prior
[Ness commissioning record](DECOMPRESSION_NESS_2026-10-11.md).

## Evidence

Under `ops/bench/data/Bodhi Hive/`:

- `20261012T013839Z-9F2738-signature-replay.jsonl` (prior endpoint)
- `20261012T173128Z-morning-intake.jsonl` (unaltered morning serial trace)
- `20261012T173128Z-9F2738-overnight-comparison.json` (conditions and derived comparison)
