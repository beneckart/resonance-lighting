# Fixture USB bootstrap and PROTECT rescue handoff

Use this runbook when USB-flashing production PowerFeather V2 fixtures that may
be either factory-fresh or parked by the earlier durable-PROTECT bug. This is the
handoff for Elliot's bench. It deliberately installs the simple supervised
basic-listener image, not the `Lighting-Controller` boot-salute/presence image.

## Source of truth

- Repository: `https://github.com/beneckart/resonance-lighting`
- Branch: `codex/basic-listener`
- Firmware version: `fx-260816-prtrel1-b`
- Sketch: `firmware/fixture`
- Board FQBN: `esp32:esp32:esp32s3_powerfeather`
- Production ESP-NOW and maintenance-WiFi channel: 11
- Commission profile: bridge-authoritative, always reachable while adequately
  powered, steady red at linear level 128 when no bridge command is active
- No gamma correction, boot salute, USB carousel, identity pop, or
  sensor-created color

Do not build this rescue image from `Lighting-Controller`. The required source,
version string, build wrapper, automatic post-PROTECT reboot, and USB recovery
tool are all on `codex/basic-listener` (and the firmware itself is also present
on `main` at `969689c`).

The compiled `.bin` and WiFi credentials are intentionally not tracked by Git.
Build once, inspect the result, and reuse that exact build directory for the
whole batch.

## Make a clean checkout without disturbing other work

From an existing clone:

```bash
git fetch origin
git worktree add ../resonance-usb-rescue origin/codex/basic-listener
cd ../resonance-usb-rescue
```

The detached worktree is intentional: this is a build/flash station, not the
place to mix in controller development.

## Supply the production WiFi profile locally

The artifact must contain the `Party In The Woods` credentials so its
maintenance OTA endpoint remains usable after USB rescue. Obtain the password
through the existing private team channel. Do not commit it.

Create a local file outside the repository, for example
`~/resonance-party-wifi.h`:

```cpp
#pragma once
#define RES_WIFI_SSID "Party In The Woods"
#define RES_WIFI_PASSWORD "<shared password>"
```

## Build the exact recipe once

Run from `firmware/fixture`:

```bash
./build.sh \
  --artifact-dir build/fx-260816-prtrel1-b \
  --channel 11 \
  --profile commission \
  --basic-listener \
  --wifi-source ~/resonance-party-wifi.h
```

An uncached build normally takes 2-3 minutes. Do not start a second compile in
parallel against the same Arduino cache. A valid build ends with flash/RAM usage
and produces:

```text
firmware/fixture/build/fx-260816-prtrel1-b/fixture.ino.bin
firmware/fixture/build/fx-260816-prtrel1-b/build.options.json
```

Before flashing, inspect `build.options.json` and require all three flags:

```text
-DRES_BASIC_LISTENER=1
-DRES_CHANNEL=11
-DRES_PROFILE_DEFAULT=PROFILE_DEV
```

The version string alone is not enough to prove those flags were used. Ben's
known artifact was 1,169,424 bytes with SHA-256:

```text
6305E9713CE1DD3FDDD37C66FEF4FAB84FB1D6782AEBD8723BB12E989243297E
```

Use that hash as an exact-match requirement only when copying Ben's binary or
when the Arduino core/toolchain is known to be identical. A clean rebuild with
a different toolchain may legitimately have a different hash; preserve and log
the hash actually deployed.

## Classify the physical batch before running the tool

Do not mix different battery capacities or fixture roles in one commissioning
command because the command applies one capacity and one optional class check to
every selected port.

- Large hanging downlight with 33140 LFP: 15,000 mAh; expected class
  `downlight`; require MSA311, TMF8820, and BMP581 for the current production
  build.
- Small-enclosure fixture with 32700 LFP: 6,000 mAh; use its actual role if
  known, otherwise leave the class check off and record the unit as unassigned.
- Factory-fresh assembled fixture: treat it as battery-installed if its LFP is
  physically connected, even if firmware telemetry has never been seen.
- Bare board with no battery and no VDC: use the default bare-board path; do not
  add `--allow-battery-present`.

## Installed-battery lanterns: normal batch command

This example is for a batch of large downlights. Run from the repository root,
replace `N` and the port list, and use a new evidence filename for every batch:

```bash
python3 ops/bench/fleet_usb_bringup.py commission \
  --out ops/bench/data/usb/2026-08-16-elliot-downlight-rescue-01.jsonl \
  --build-path firmware/fixture/build/fx-260816-prtrel1-b \
  --sketch-dir fixture \
  --expect-fw fx-260816-prtrel1-b \
  --expect-count N \
  --ports /dev/cu.usbmodemPORT1 /dev/cu.usbmodemPORT2 \
  --max-parallel N \
  --wifi-check --wifi-parallel 1 \
  --battery-chemistry Generic_LFP \
  --capacity-mah 15000 \
  --charge-ma 2000 \
  --maintain-v 4.6 \
  --ota-profile "Party In The Woods" \
  --allow-battery-present \
  --expect-class downlight \
  --require-bmp581
```

Always pass explicit fixture ports. A CoreS3 bridge can share the same Espressif
USB VID/PID as a PowerFeather; never let a bridge enter a fixture batch.

For a single board, the exact prebuilt artifact can also be uploaded directly:

```bash
arduino-cli upload \
  --fqbn esp32:esp32:esp32s3_powerfeather \
  --port /dev/cu.usbmodemPORT \
  --build-path firmware/fixture/build/fx-260816-prtrel1-b \
  firmware/fixture
```

Direct upload does not configure capacity or verify telemetry/WiFi, so the batch
tool is preferred for a recorded production handoff.

## Durable-PROTECT rescue rule

For an assembled fixture with a battery installed:

1. Leave the LFP connected.
2. Connect the enclosure rescue USB port to a proven supply.
3. Flash `fx-260816-prtrel1-b` over normal USB if the port is available.
4. Keep USB connected for at least 90 seconds after the upload.
5. Require a valid battery at or above 3.25 V (ADR 0046), good external supply, no charger
   fault, and at least +20 mA charge current continuously for 60 seconds.
6. The firmware persists the qualified release and performs an automatic clean
   reboot. The clean reboot is necessary because the parked boot skipped sensor,
   LED, and rail initialization.
7. With no competing bridge command, steady red confirms the basic listener is
   rendering after release.

Do not use the serial `X` command on an installed-battery lantern. `X` is a
guarded bare-board-only clear. Do not erase NVS; that destroys fixture settings
and bypasses the intended safety qualification. A physical RESET alone does not
clear durable PROTECT.

Images built after ADR 0047 (2026-08-18) cannot acquire a durable PROTECT
latch from a bare board or floating BAT node in the first place: the durable
write waits for a corroborated battery (charge/discharge current, a BQ
presence test, or a recovery-lane detection), and an uncorroborated park stays
awake on any verified external supply. Latches encountered on pre-0047 images
still follow the full rescue rule above.

The commissioning tool can verify the upload and OTA endpoint before the full
60-second release has completed. Therefore, a tool `PASS` is not by itself proof
that an installed-battery PROTECT latch released. Keep the unit powered and
perform the post-release checks below.

## If USB is absent or disappears quickly

Use this failure ladder, in order:

1. Confirm the cable carries data and the USB supply is stable.
2. Keep USB connected and tap RESET once. An older sleeping image may expose its
   port only briefly; have the upload command or watcher armed before RESET.
3. If normal USB CDC never enumerates or ordinary upload cannot connect, enter
   ROM download mode: hold BOOT, tap RESET, then release BOOT. Flash the same
   prebuilt artifact.
4. Do not erase flash or NVS as a routine recovery step.
5. After upload, leave battery and USB attached for the qualified release and
   automatic reboot described above.

Factory-fresh and PROTECT-latched fixtures receive the same image. The
difference is only the recovery/verification procedure.

## Per-fixture success gate

Do not mark a lantern ready merely because upload returned success. Record its
MAC-derived fixture ID and require:

- firmware exactly `fx-260816-prtrel1-b`;
- PowerFeather initialization passed;
- expected 8 MB flash and 2 MB physical PSRAM preflight;
- correct LFP capacity and 2,000 mA charge ceiling;
- installed battery is present when expected;
- `guard_stage` is no longer PROTECT after the qualified wait;
- automatic clean reboot occurred after a PROTECT release;
- expected class and sensor health, when the physical class is known;
- ESP-NOW rejoins on channel 11;
- `Party In The Woods` `/telemetry`, `/resume`, and OTA endpoint checks pass;
- steady red appears with no competing bridge command.

If another bridge or controller is active, shut it down during the visual check
so a live color stream cannot overwrite the steady-red idle state.

## Handoff evidence

Keep the JSONL output append-only, update `ops/fleet/registry.csv`, and add a
dated `LOG.md` entry listing fixture IDs, physical class/capacity, deployed
artifact hash, USB result, WiFi result, sensor result, and any unit that required
ROM download mode or extended PROTECT recovery.
