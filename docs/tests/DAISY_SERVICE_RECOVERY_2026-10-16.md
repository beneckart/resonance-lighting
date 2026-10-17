# Daisy USB service and power confirmation qualification

Date: 2026-10-16, PDT
Operator: Ben + Codex, sole writer for Daisy F40308 / 68:EE:8F:F4:03:08.
Skitty F3FD28's independent natural-recovery test is excluded and untouched.

Status: implementation and native tests in progress; hardware not yet qualified.
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

The unresolved dusk fallback deserves a separate observed-evidence design,
not an automatic 30-minute awake hold or an assumption that darkness was
continuous while sleeping. This task does not alter it.
