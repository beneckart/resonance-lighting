# Firmware artifact identity and shared-bench handoff

This runbook is the coordination contract for Ben's, Elliot's, and Justin's
benches. It applies to fixture USB flashing, fixture OTA, bridge firmware, and
any host tool that selects an image. ADR 0040 records the architectural decision.

## The two identities that must travel together

Every shared or fleetable build has:

1. `fw_rev`: a short recipe identity reported by the fixture; and
2. `sha256`: the exact binary identity recorded in the artifact manifest and
   flash log.

Neither is sufficient alone. `fw_rev` tells an operator what source/configuration
recipe is intended. SHA-256 proves which bytes were uploaded.

The legacy manually incremented form, for example
`fixture-2026-08-15.4`, is historical only. On 2026-08-15 two materially
different images used that string, so telemetry could not distinguish Ben's
strict commission artifact from Elliot's listener/presence artifact.

## Revision format

New shared fixture builds use the generated form:

```
fx-YYMMDD-RRRRRRR-V
```

- `YYMMDD` is the UTC date of the clean source commit.
- `RRRRRRR` is the first seven hex characters of a recipe SHA-256.
- `V` is one reviewed build-class code:
  - `p`: normal fleetable fixture image; listener, strict diagnostic, and field
    postures are runtime settings inside this one image;
  - `b`: safe bench/canary image that is not approved for fleet promotion; or
  - `t`: targeted safety-bypass/test image -- never fleetable.

Example:

```
fx-260815-3a91c2e-p
```

This is at most 19 ASCII characters and fits the existing 24-byte heartbeat
field with a terminating NUL.

The recipe hash is computed from canonical, ordered inputs:

- full clean git commit SHA;
- normalized compile flags and variant;
- FQBN;
- Arduino ESP32 platform version;
- PowerFeather SDK version;
- firmware-affecting Arduino library versions; and
- manifest schema version.

Build time, hostname, and branch name are metadata, not recipe inputs. Branch
names move. The fixture artifact wrapper refuses dirty source for every
automatically named artifact, including variant `t`; make a clean checkpoint
first. A `t` image remains targeted and may not be offered by fleet OTA tooling.

The recipe serialization is compact ASCII JSON followed by exactly one LF.
Do not calculate the hash or type the revision manually. Use the fixture build
wrapper, which writes those exact bytes before compilation and pins the
serialization with a golden test against a previously accepted artifact:

```bash
cd firmware/fixture
./build.sh --artifact-variant b \
  --wifi-profile-label party-in-the-woods-v1 \
  --profile field --channel 11 --basic-listener \
  --precharge-ma 300 --day-sleep-s 120 --wake-listen-ms 12000
```

`--artifact-variant` requires explicit profile, channel, and non-secret WiFi
profile label. It refuses direct USB/OTA upload; flash the retained artifact
with exact-target tooling after inspection. Manual `--artifact-dir` and
`--fw-rev` are disabled.

The build-class suffix is not a runtime behavior selector. The normal `p` image
contains commission-listener, strict commission-dark, and field postures behind
runtime configuration. While listener behavior still exists only behind the
temporary `RES_QUIET_AUTONOMY` compile flag, that image is a `b` canary, not the
final one-image fleet artifact.

## Immutable artifact directory and manifest

Build once into a new directory named for `fw_rev`; never resume a killed build
directory and never overwrite a revision that has been flashed:

```
firmware/fixture/build/<fw_rev>/
  recipe.json
  fixture.ino.bin
  build.options.json
  manifest.json
  sha256.txt
```

`manifest.json` contains at least:

```json
{
  "schema": 1,
  "fw_rev": "fx-260815-3a91c2e-p",
  "variant": "p",
  "git_commit": "full-40-character-sha",
  "git_dirty": false,
  "fqbn": "esp32:esp32:esp32s3_powerfeather",
  "compile_flags": ["canonical", "ordered", "flags"],
  "channel_default": 11,
  "profile_default": "commission",
  "commission_idle_default": "listener",
  "wifi_profile": "party-in-the-woods-v1",
  "toolchain": {
    "arduino_cli": "version",
    "esp32_platform": "version",
    "powerfeather_sdk": "version"
  },
  "binary": {
    "file": "fixture.ino.bin",
    "bytes": 0,
    "sha256": "64 lowercase hex characters"
  },
  "built_utc": "ISO-8601 timestamp"
}
```

Never put an SSID password in the manifest. `wifi_profile` is a non-secret
credential-set label.

One `fw_rev` may map to only one binary SHA-256. If nominally identical builds
produce different bytes, stop and select one immutable artifact; do not publish
both under the same revision. Fleet tooling uploads the artifact, not a rebuild.

## Shared-bench ownership and callout

Live color frames may remain intentionally leaseless: two operators can see
flicker and coordinate in person. State-changing operations are different.
OTA, USB flash, profile/channel persistence, reboot, NVS changes, and firmware
promotion have one operator at a time across all bridges and laptops.

The preferred diagnostic for intentional last-writer-wins control is visibility,
not another lease: bridges get stable source identities and the operator UI
surfaces the latest direct-frame source/age and rapid source changes.

Before a state-changing session, post or record:

```
OWNER: <person/bench>
SOURCE: <repo commit>
ARTIFACT: <fw_rev> <sha256>
TARGETS: <explicit short MACs>
OPERATION: <USB | OTA | profile | channel | other>
```

Rules:

- Name explicit target MACs. A roster slot or nickname is supporting context;
  the short MAC is device identity.
- Never offer or select an image by newest modification time or the word
  `latest`.
- A UI may preselect an image, but it displays `fw_rev`, variant, commit, and a
  SHA-256 prefix before confirmation.
- A typo or unknown MAC must fail before broadcasting maintenance to the fleet.
- `--profile commission` and `--channel 11` are compile defaults only. Existing
  NVS wins; verify the reported profile and channel or persist them explicitly.
- After the session, record each target, result, reported revision, exact binary
  SHA-256, and any rollback or failure. Do not infer rollback merely because a
  different version appears; competing OTA is also possible.

## OTA completion contract

An HTTP upload acknowledgement is not completion. Cambium or another OTA runner
may say success only after all of these are true:

1. the target identity matched before upload;
2. a heartbeat newer than the job start arrived after reboot;
3. the reported `fw_rev` matches the manifest;
4. the fixture remained on that revision through the 20-second pending-verify
   window; and
5. a later fresh heartbeat confirms it is still present.

A cached `online=true` roster entry is not a rejoin. When practical, include
fresh uptime/reset evidence as well. A rollback is a safe OTA outcome but not a
successful promotion.

## USB boot salute

The optional boot salute has two deliberately different meanings:

- **Automatic USB boot salute:** the fixture detected USB specifically (not
  merely solar/VDC `supply_good`) and reached a stable firmware boot. This is a
  useful liveness cue, not proof that commissioning passed.
- **Final completion salute:** host tooling verified the expected artifact,
  MAC, profile, channel, power state, class, required sensors, and
  `ota_pending_verify=false`, then explicitly requested the completion signal.
  This is the signal that may mean "unplug and install."

An autonomous salute based only on USB voltage must not claim the second
meaning. It does not know which artifact the operator expected, which fixture
class/roster row is intended, or whether host-side checks passed.

The success pattern must be distinct from the low-red listener beacon, the
periodic identity pulse, and every error/PROTECT indication. A parked boot,
failed power initialization, or pending OTA image must never emit the final
completion salute.
