# Daisy USB service and power confirmation qualification

Date: 2026-10-16, PDT
Operator: Ben + Codex, sole writer for Daisy F40308 / 68:EE:8F:F4:03:08.
Skitty F3FD28's independent natural-recovery test is excluded and untouched.

Status: Daisy-only qualification PASSED; final ordinary firmware installed. Not fleet-promoted.
Decision and command contract: ADR 0087.

## Timer audit

| State/timer | Interaction with short wakes | Result |
| --- | --- | --- |
| DIM -> FULL, OFF -> DIM: 60 s each | RAM timers reset at sleep; 12 s windows cannot complete them | Bounded 150 s extension implemented; test both steps |
| DIM -> OFF: 60 s; FULL -> DIM: 10 s | Late first readings and short wakes can cut off downward confirmation too | Same bounded extension; immediate 3.05 V critical stop unchanged |
| Missing battery sample | Earlier DIM/OFF timers froze without clearing their start time | Reset the streak; never credit the unobserved gap |
| Legacy PROTECT release: 60 s | Power policy already defers sleep while proof qualifies; lifecycle yields sleep ownership to PROTECT | Existing rule retained; dependence on awake charging evidence remains on ordinary images |
| ADR 0085 staged PROTECT | Separate bounded coordinator and verified RTC cooldown credit | Unchanged; not enabled for Daisy or newly deployed to Skitty |
| Solar surplus entry: 60 s | Existing solarProbeActive suppresses normal sleep | Already handled |
| Fallback dusk: 1800 s without valid schedule | Timer resets at sleep; cold boot grace is 600 s, regular wake 12 s | Similar unresolved gap; valid schedule or explicit night avoids this path |
| Dawn: 300 s | NIGHT_SHOW does not ordinarily request daytime sleep | No equivalent short-wake starvation in that state |
| DAY_ACTIVE surplus exit: 300 s | State may reset across sleep, but one-wake energy carry is only renewed with current strike permission | Literal timer does not survive; fresh permission gates prevent stale authorization being carried indefinitely |
| Boot-reset streak clear: 60 s | Expected deep-sleep boot already clears streak | No dependency on reaching one minute on a normal wake |
| Quiet load disarm: 60 s | allLoadsOff explicitly disarms before normal sleep | No dependency on reaching one minute on a normal wake |
| OTA pending verification: 20 s | Ordinary sleep already blocked until validation/rollback resolves | Already handled |
| Gauge current settling: about 12 s | powerWakeSampleWindowComplete waits for valid current, with a 15 s fail-open bound | Already handled in current source; do not equate an early zero reading with asleep current |

The unresolved dusk fallback deserves a separate observed-evidence design,
not an automatic 30-minute awake hold or an assumption that darkness was
continuous while sleeping. This task does not alter it.

## Software validation

Firmware source checkpoint: `73cd619`. Host service integration: `b18b741`.
All 41 native test executables passed, including 406 power/sleep integration
checks and 195 existing power-policy checks. The combined runner initially
could not execute test_serial_mutation because Windows returned Permission
denied; a separate compile/run passed all 85 checks. The suite recorded no
assertion failures. Six mocked host tests passed, including no ROM call on
refused preparation and no application reboot between flash-ID and upload.
Build-wrapper tests reject missing/invalid test targets and non-test variants.

Local evidence: `tmp/daisy-service-native-tests.log` and
`tmp/daisy-service-serial-retry.log`. This reports the individual results,
not a clean exit from the original runner, which retained its execution error.

## Initial hardware service qualification, 18:12-18:15 PDT

Test artifact fx-261017-5c1c1ca-t, source 73cd619, SHA-256
2699328f1a6efaae5583f51089702cd86c1e1964c97578b3659e9ee894034a75
was sent by exact-target BenPhone OTA. The new app0 passed self-test and
remained VALID beyond 35 seconds, with healthy battery and sensors.

Three consecutive tests deliberately armed the LED load, required the new
service READY plus fresh all-loads-off readback, entered ROM, verified the
same full MAC, and returned to the application. Every fresh boot was FULL /
stage 1; voltage was 3.577..3.579 V. The historical 3026 mV protection entry
and its context were unchanged. Windows/esptool returned an I/O error after
issuing each watchdog reset out of ROM; the captured fresh application boots,
not those exit codes, establish the resulting state. No image was written in
these three cycles. Raw evidence:
20261017T011312Z-F40308-service-service.jsonl.

The separate timeout test prepared service and sent no reset command. At
18:14:58.877765 the firmware announced its bounded clean reboot, almost
exactly 30 seconds after preparation. It returned FULL with a software reset,
normal sensors and unchanged configuration/history. Raw evidence:
20261017T011428Z-F40308-service-timeout.jsonl.

Before the real daytime test, inspection of the test controls found that
ordinary inspection-firmware N0 opens a ten-minute operator window and keeps
AUTO. That would mask sleep starvation. Test-only source 5b650d3 makes N0
actually force daytime for the exact compiled target; 6fb263e additionally
reports when power confirmation is the sole sleep veto by evaluating lifecycle
on a copy with that one input removed. Neither change affects ordinary builds.
The DIM/OFF test requires this diagnostic and a real deep-sleep reset, rather
than treating merely remaining awake as sufficient evidence.

Ordinary artifact retained before the later test-only diagnostics:
fx-261017-c2ddd6a-b, source 73cd6199bf2a6ae3cd301c1ad778fa67506bf5eb,
SHA-256 09a718c6da858615cbe47346bfd0ea0d37ad820304391ade368391a48438530e,
1227920 bytes. Clean compile: 1227601 bytes flash (36%), 69668 RAM (21%).
Its immutable manifest contains no RES_POWER_RECOVERY_TEST_TARGET flag.
All 19 top-level build outputs and the private Wi-Fi input are retained.
It has not yet been declared hardware-qualified by this entry.

Host follow-up ba1c703 discards buffered serial input before starting the
service exchange, so a queued READY from an expired window cannot authorize
another reset. Seven host tests pass, including explicit stale-READY rejection.
This is a host-only change and does not change the firmware artifacts above.

The host stale-buffer follow-up also passed one live prepared ROM cycle on
Daisy: 20261017T012047Z-F40308-service-service.jsonl. This additional cycle
followed the host change; it did not replace or overwrite the first three.

The stricter test artifact is fx-261017-f61785e-t, source
6fb263e245465187077c687023076ff35076cce7, SHA-256
b9c45cb52f3da99587a5ac458ea6f435658c93615733d050c30ce43f3d30c433,
1228496 bytes (clean compile, 36% flash / 21% RAM). In addition to enabling
the exact-target test hook, its firmware source changes from the ordinary
artifact are the target-only N0 override and diagnostic. Host-only service
integration changes are also present in that source commit.

Hardware recovery acceptance requires: exact identity and revision; healthy
FULL preflight; !YF40308:1 or :2 followed by an actual three-second deep sleep;
N0 after the reduced-tier wake; the "confirmation is sole sleep veto" log;
continuing reduced-tier daytime telemetry beyond 35 seconds; then FULL with
no new PROTECT entry or changed capacity/charge/channel/profile. N2 restores
AUTO, and a final ordinary OTA removes the test hook entirely.

### DIM sleep/wake recovery passes

At 18:28:20 PDT the exact-target command persisted DIM and entered a real
three-second deep sleep. The next boot reported deepsleep, DIM / stage 2,
and normal field configuration. N0 selected actual day. At uptime 22365 ms
the target-only diagnostic proved confirmation was the sole sleep veto.
Daisy remained in daytime DIM beyond 35 seconds, then reached FULL with
fresh telemetry at uptime 72341 ms (18:29:35.438391 PDT). No history or
battery/charge/profile/channel fields changed. AUTO was restored afterward.
Raw evidence: 20261017T012819Z-F40308-service-dim.jsonl.

### OFF -> DIM -> FULL sleep/wake recovery passes

The exact-target OFF test entered deep sleep at 18:29:39 PDT. The next boot
reported OFF / stage 3. At uptime 22331 ms the diagnostic again proved power
confirmation was the sole sleep veto. OFF -> DIM occurred at 18:30:53.131618,
and the same uninterrupted boot continued through the second minute to FULL.
Final fresh telemetry at 18:31:54.648115 reported uptime 132068 ms, FULL,
normal power and unchanged historical protection/configuration. AUTO was
restored. Raw evidence: 20261017T012937Z-F40308-service-off.jsonl.

These two real sleep tests demonstrate the integration beyond the native
model. No physical brownout was induced: preservation of the armed-collapse
ladder and immediate low-voltage behavior is covered by the native regression
suite and unchanged production guard logic, not a destructive power test.

The exact host qualification driver is retained as a non-executable text
record in 20261017-daisy-service-test-driver.txt; native runner output and the
Windows execution retry are retained alongside it. Paths are under
ops/bench/data/Oakland. The driver is an archived transcript of this session's
Daisy-specific test, not a general fleet command.


## Final ordinary deployment

At 2026-10-17T01:33:29.545277+00:00, Daisy reported fx-261017-c2ddd6a-b, partition app0
VALID, uptime 77157 ms, FULL / stage 1, VBAT 3.576 V,
net battery current 681.1 mA, and no charger fault. VL53L5CX
and MSA311 were present and reading. Field profile, channel 11, 6000 mAh /
2000 mA configuration and the historical 3026 mV PROTECT context are unchanged.
The service window is inactive and the final artifact has no test-target flag.
Raw completion evidence: 20261017T013156Z-F40308-service-ota.jsonl.

The app passed its explicit OTA self-test, then remained valid beyond 75 seconds.
Only Daisy's registry row was updated. This is not permission to fleet-promote
or a claim that physical RESET outside the prepared service workflow cannot
consume a collapse retry. Skitty was not changed; the independent recovery
watch ended after fresh passive radio reports confirmed natural recovery.
