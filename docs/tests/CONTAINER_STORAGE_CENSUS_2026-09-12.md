# Nevada City container census and USB storage, 2026-09-12

Ben requested a census after roughly a week in a dark shipping container and
storage until physical service. At 17:08 PDT he explicitly authorized fleet
USB-wake storage, then direct laptop operation without a T-Deck tap. His later
dinner plan put the expected return at roughly 18:30-19:00 PDT.

## Completed operation, 19:18 PDT

- 106 of the 114 previously installed fixtures were heard. All 106 passed
  exact-revision OTA verification beyond the pending-verification window on
  `fx-260912-f951ae9-b`; no failed, deferred or commission target remained.
- All 106 acknowledged untimed USB-wake storage, with matching source/sequence
  receipts and durable cause-8, zero-duration audits. No refusal, entry failure
  or later radio rejoin was observed.
- The final five acknowledged by 18:39:20 PDT. The last job stopped sending at
  18:39:37. The full 960-second quiet interval elapsed at 18:55:20; the final
  live check at 19:17:57 established 38 minutes 37 seconds of quiet after the
  last receipt, uninterrupted capture, and no controller restart.
- A fresh controller status confirmed job 3718EB1B inactive, 17/17 prepared,
  zero refused. Dashboard/logger processes were stopped at 19:18 and the
  temporary Windows awake hold was released. Laptop, T-Deck and network are
  ready to pack; none is needed to maintain storage.
- Eight installed fixtures never responded and were not commanded. Their
  condition remains unknown; exact names and IDs appear below.

Ponyta F2B7DC and Chunli 9F2714 both acknowledged storage. Ponyta's last report
at storage entry was 2.562 V; an earlier 2.547 V sample was late in a protected
listening window. These readings do not diagnose a ruined cell.

Final evidence is `storage-final.json` and CSV, `pack-up-controller-status.json`,
all eight storage job ledgers, and the SHA-256 evidence manifest under
`ops/bench/data/ca/20260912-155847-container/`. The read-only reconciler required
all 106 receipts, durable audits, stopped jobs, at least 960 seconds of quiet,
live capture and no bridge restart. Its final capture contained 48,755 rows;
maximum controller-report gap was 10.024 seconds. Radio evidence is not an
assembled-current measurement or a physical USB-wake test; both remain open.
The compact evidence is retained on `codex/container-storage-host-20260912`;
large raw captures and private device backups remain local.

## Wake and controller handoff

The selected mode is untimed USB-wake ship mode. Good USB or solar input wakes
it; QON is an alternative. RESET/BOOT alone cannot wake an unpowered ESP32.
The laptop, network and T-Deck are not needed to maintain this storage state.

T-Deck Home -> Rest -> Store now offers USB storage, RESET-wake deep sleep,
and a Pacific date/time wake picker using fresh GPS time. The timer option is
limited to 30 minutes through seven days. Pure-core calendar/campaign/receipt
and parser tests, the full native suite and the embedded build passed. Actual
LCD layout, physical wake behavior and assembled storage current still need
service-time qualification.

Retained T-Deck artifact: `tdeck-0.3.0-storage4`, 1,578,224 bytes, SHA-256
`11b98b33ca46babf21ad27cb1f2edd7cccdf6319a5e130df3e5442fd6260c1b1`.
Firmware source: `556c2f6cdbefd152a1f79437b9e7f859b7b7fd44`; artifact commit
`a53181a` on `codex/container-storage-host-20260912`. Exact application,
bootloader, partitions, build options and manifest are retained in
`../resonance-tree-storage-host-20260912/firmware/tdeck_bridge/build/storage-host-20260912-r4/`.
The private pre-flash NVS/apps backup is retained locally and is not committed.
The storage3 build was not flashed; no firmware source changed during the
storage4 build. Its post-build test-only CRLF comparison fix passed the suite.

## Census

Read-only capture through T-Deck `8EB508`, channel 11, COM152, while the laptop
was on the known Party In The Woods network. Compare against the 114 installed
fixtures in the 2026-09-01 rollout report, not the larger inventory registry.

The first host capture stopped after 399 seconds without an error. The same
logical run was resumed with a recorded segment boundary; capture gaps are not
counted as continuous observation. The second segment completed 1,020 seconds
uninterrupted, ending about 16:27:57 PDT. The whole run spans 15:58:47 through
16:27:57 PDT with the gap explicitly retained.

Final result: **106 of 114 heard**, all FIELD profile. No latest report between
0 and 0.6 V. Lowest latest readings are Ponyta `F2B7DC`, **2.582 V**, and Chunli
`9F2714`, **2.618 V**. Median is 3.244 V; maximum 3.323 V. These are radio/gauge observations, not cell-terminal
measurements or a determination that a battery is ruined. Their current
firmware reports PROTECT, 900-second sleeps and LED rails off. All 106 latest
retained LED reports are off and all supply-good reports are false. Latest
tiers: 74 PROTECT, 5 DIM, 27 NORMAL. Last sleep records: 75 at 900 seconds and
31 at 120 seconds; sleep history and current tier are not necessarily sampled
at the same moment.

Revisions: 69 `fx-260831-f121868-b`, 23 `fx-260831-dc82da7-b`, and 14
`fx-260831-b3e2738-b`. Ten fixtures report below 3.05 V: Ralts `9E5AE0`,
Torchic `9E5B04`, Tauros `9F26AC`, Chunli `9F2714`, Ponyta `F2B7DC`, Toad
`F2BEE4`, Spock `F3FD60`, Pacman `F40174`, Raven `F402B8`, and Guile `F40350`.

The full 12,484-row JSONL, final summary JSON, CSV with field ages, and exact
observed/missing rosters are retained under
`ops/bench/data/ca/20260912-155847-container/`. The read-only
`ops/bench/storage_census.py` rejects pre-run bridge cache as fresh evidence
and distinguishes capture gaps. Seven focused tests pass. It does not open a
serial port or transmit commands.

Not heard in the completed observation:

| ID | Callsign | Context |
|---|---|---|
| 9F0E30 | Wooper | Already failed maintenance discovery on September 1 |
| 9F26B4 | Kairi | Already failed maintenance discovery on September 1 |
| F2BCF4 | Gambit | Earlier sub-1 V cell was removed; a healthy replacement was later recorded |
| F2BDD4 | Gengar | Unobserved |
| F2BDFC | Magmar | Unobserved |
| F2BE10 | Donkey | Unobserved |
| F2BE94 | Milotic | Unobserved |
| F3FD28 | Skitty | Unobserved |

Silence does not distinguish RF obstruction, disconnected power, an old sleep
command, or depleted cells. The previously reported 0-0.6 V fixtures cannot be
declared recovered from this census. Do not infer remaining capacity from LFP
plateau voltage or the fuel gauge's low SOC percentage alone (ADR 0023).

Historical very-low reports need a separate service check. The August 27
recovery report recorded Tidus `F40424` near 0.01 V with recovery refused;
Tidus is outside the 114-ID installed baseline used here and was not heard.
That report also recorded Clank `F2BF60` at 0.86 V; today's Clank report is
3.267 V in FIELD on the August 31 image, so the old cell reading must not be
assigned to its current cell without checking its service history. Gambit's
old sub-1 V cell was explicitly removed. None of those historical cases is
proved resolved merely by today's absence of 0-0.6 V telemetry.

## Battery tradeoff

These are planning calculations, not measured storage current in the assembled
fixtures. Radio examples assume 130 mA while awake, based on the bench range
in ADR 0045. They exclude LEDs and sleep current; boot/listen variation changes
the estimate. A week is 168 hours.

The manufacturer's board-current measurements use a 3.7 V source; they are
references, not measurements of these assembled LFP fixtures.

| Posture | Assumption | Per-fixture draw in 7 days |
|---|---|---:|
| Continuously awake, LEDs dark | 130 mA | 21,840 mAh |
| Ordinary radio duty cycle | 12 s awake / 120 s asleep | 1,985 mAh, radio only |
| PROTECT duty cycle | 9 s awake / 900 s asleep | 216 mAh, radio only |
| Uninterrupted deep sleep | Vendor V2 board, gauge on, 24 uA | 4.032 mAh |
| Conservative assembled sleep budget | ADR 0045 placeholder, 1 mA | 168 mAh |
| Ship mode reference | Vendor V2 board, gauge off, 1 uA | 0.168 mAh |

Applying those radio assumptions to the observed 31 short-cycle and 75
PROTECT-cycle fixtures gives about **77.77 Ah per week across 106 fixtures**
(about 249 Wh at 3.2 V). That projects the observed posture forward; it is not
an energy measurement or an estimate of remaining capacity. Uninterrupted
sleep removes this periodic-radio component. Attached-hardware leakage still
needs measurement and continues in addition to any board-current figure.

Eliminating periodic wakeups is worthwhile. The illustrative 24-to-1 uA
difference is only 3.864 mAh per fixture per week, or 0.440 Ah across 114
fixtures (about 1.4 Wh at 3.2 V). That alone does not justify cumbersome QON
access. The prepared ship implementation keeps the gauge enabled; its actual
current must be measured and is not the vendor's 1 uA gauge-disabled number.

Ship mode is attractive because good USB or solar input wakes it. QON is an
alternative, not required if USB can be applied. RESET/bootloader buttons do
not wake ship mode because the ESP32 has no power. Indefinite deep sleep uses
physical RESET; USB alone is not a wake source in the prepared implementation.
Neither mode can be recalled by radio. Battery replacement at service should
be done with external power removed; PowerFeather does not support live battery
swaps. Check the actual cell voltage separately from an isolated/protected
board node before deciding the condition of a previously very-low report.

Primary references checked 2026-09-12:

- [PowerFeather V2 current specifications](https://docs.powerfeather.dev/#current-consumption)
- [PowerFeather power and battery handling](https://docs.powerfeather.dev/sdk/usage-notes/)
- [PowerFeather power modes](https://docs.powerfeather.dev/guides/reduce_power_usage/)

## Prepared controls and deployment boundary

Source is isolated on `codex/container-storage-20260912` in the adjacent
`resonance-tree-storage-20260912` worktree, preserving unfinished conductor
work in the main checkout. ADR 0080 defines the new exact-target USB/RESET
storage modes, capability advertisement, pre-entry receipts and durable audit.
Firmware installation does not select storage or bypass battery protection.

T-Deck Rest -> Store also offers a Pacific date/time picker using the existing
timed-transport command, with fresh GPS time, a seven-day limit, and a
16-minute exact-roster catch-up campaign. The controller cannot recall asleep
fixtures, and the ESP32's timer may drift. A PREPARED receipt is not measured
electrical power-off proof. Physical wake/current qualification remains open.

The OTA permission alone did not authorize storage. Ben separately approved
fleet USB-wake storage at 17:08 PDT and then direct laptop operation; those
permissions are satisfied and remain in force for this rollout.

## Validation and live canary

Fixture source commit `fcc5ef07ccf65f0f1d034bfe523e9f1d7f68bf10` passed the full
fixture native suite and embedded PowerFeather build. The full T-Deck native
suite also passed, including capability revocation, calendar, campaign and
receipt-ordering checks. Logger output-safety tests: 5 passed; storage-event
decoding: 4 passed; census: 7 passed.

Immutable fixture artifact:

- Revision: `fx-260912-f951ae9-b` (same exact artifact used in the authorized storage rollout).
- Bytes: 1,218,752.
- SHA-256: `ec03b074c6feae7ffc3f6bb2e178ec213ea5d241b56b3e24c95bfbb24d315001`.
- FIELD, channel 11, LFP, 300 mA precharge, 120-second day sleep,
  12,000 ms listen grace, basic listener; same flags as `fx-260831-f121868-b`.
- Local artifact directory:
  `../resonance-tree-storage-20260912/firmware/fixture/build/fx-260912-f951ae9-b/`.

Ben + Codex declared the single operator for exact target Hellboy `9F26C4`.
Preflight reported 3.312 V; shared-WiFi discovery found it at 3.316 V.
Job `84DEEB42` uploaded this exact image at 16:39 PDT and verified a fresh
matching-revision heartbeat at 25,953 ms uptime at 16:40:08 PDT. Reset reason
was software, class 1, recovery state 0, and FIELD profile persisted. This is
successful OTA evidence, not a storage or USB-wake test.

At approximately 16:52 PDT it also reported a deep-sleep reset and a short
12.6-second awake boot, consistent with returning to the existing 120-second
day cycle after the normal ten-minute cold-boot listen window. The old
controller initially showed an unset automatic sleep-audit tail. After its
receiver/cache restart, a fresh 17:04 PDT report confirmed cause 2, 120 seconds,
3.308 V at entry and a deep-sleep reset at 3.313 V. Ordinary RTC sleep-audit
retention was therefore confirmed. At that checkpoint, indefinite storage and
physical wake were still untested; the final storage result is recorded above.
Evidence: `hellboy-rtc-confirmed.json` in the census data directory.

Job ledger: `ops/bench/data/ca/20260912-155847-container/hellboy-canary-ota.jsonl`.
At this initial canary checkpoint, only Hellboy had been updated and no
deliberate storage command had been sent. Later rollout updates above
supersede that initial deployment count. The successful job froze its maintenance gather before
upload and completed without deferred or failed targets.

Completed census JSONL SHA-256:
`d0bd4f235cfe52e66c338ca59d57da29b4aa46fd0325d712955e1b77ec28bd5f`.

## 2026-09-14 location follow-up

Ben found a separate lantern bin and brought it to Oakland. Kairi 9F26B4 and
Wooper 9F0E30 are now heard there, accounting for two of the historical eight
unobserved fixtures. Groot 9F2724, Kiki F2BF5C and backyard prototype 9F26F8
are also heard, but were outside the 114-ID baseline. None of these five was
in the stored 106; their present radio activity does not demonstrate a storage
failure. Other bin lanterns are not yet mapped and may overlap the stored
cohort. Radio reach on September 12 did not prove physical container location.
The exact 106 storage receipts and eight-ID absence record above stay intact.
Current inventory and remaining six unknowns are recorded in
`OAKLAND_LANTERN_INVENTORY_2026-09-14.md`.
