# BenPhone maintenance OTA canary

Ben authorized the temporary Loom hotspot credentials in
`firmware/fixture/wifi_benphone.h` on 2026-10-16. The shared profile selects
BenPhone on 2.4 GHz; use WPA2-Personal. The fixture scans/joins it only during
explicit maintenance, after leaving ESP-NOW. Normal mesh operation remains
on channel 11; fixture maintenance can use the hotspot's actual channel.

The profile is build-time configuration. Merely pulling this file does not
change any installed fixture. Select it with `--wifi-source wifi_benphone.h`
for a hotspot-only build. To retain an existing-network fallback, create the ignored
`wifi_secrets.h` with `#include "wifi_benphone.h"` and the private
`RES_WIFI_SSID_2` / `RES_WIFI_PASSWORD_2` pair. Do not publish that private pair.

For the October 16 canary, retain the earlier Party In The Woods bench network
as that secondary profile and use label `benphone-party-v1`. With both
networks visible the existing planner prefers stronger RSSI; a successful
maintenance session must identify the actual joined network before claiming
a BenPhone test. If necessary, make only the intended AP available.

Build once from a clean commit using the standard immutable wrapper:

```bash
cd firmware/fixture
./build.sh --artifact-variant b --wifi-profile-label benphone-party-v1 \
  --profile field --channel 11 --basic-listener --decompression \
  --precharge-ma 300 --day-sleep-s 120 --wake-listen-ms 12000
```

Retain and share the exact binary and manifest; do not rebuild a flashed
revision with different credentials. This command builds a canopy receiver,
not Marill's presence controller. A perimeter/controller needs its own
appropriate recipe and artifact; do not replace its behavior with this image.

Select one healthy battery-backed canopy by exact short MAC and name the
operator before any write. If it does not already know BenPhone, its first
update needs its existing maintenance network or one supervised USB update.
Then enter maintenance through BenPhone, upload the same retained artifact,
and verify fresh exact-revision telemetry, the full 20-second pending-verify
window, valid slot state and later liveness. Upload acknowledgement alone
is not completion. A USB-triggered maintenance test validates the WiFi OTA
path; it does not also prove remote radio entry.

Keep Skitty F3FD28 excluded from all discovery writers, maintenance commands,
reboots and uploads while its natural PROTECT recovery experiment runs.
The retained canopy artifact below has compiled and passed host-side
identity/hash checks but is not deployed. A separate ordinary-perimeter
artifact subsequently passed the Daisy test described below.

## October 16 initial radio check

T-Deck 8EB508 (`tdeck-0.3.0-storage4`, channel 11, COM10 on Ben's laptop)
heard all nine canopies. Pinsir 9F26BC was chosen for the initial discovery:
fresh telemetry showed FULL, about 3.53 V and charger fault zero on d572656.
One exact-target 35-second maintenance request reached it at 17:34 PDT.
No matching HTTP endpoint appeared on the laptop's BubbyNet /22, and Pinsir
returned to the mesh with `maint_status=3` (WiFi startup failed), unchanged
firmware and continuing uptime. No upload occurred. This does not identify
which network names or passwords Brian's installed image contains.

The current laptop network is BubbyNet; Party In The Woods is a retained
earlier bench profile, not a verified reachable network in this yard.
The next step is to identify and provide a maintenance network that the
installed image already knows, or use one supervised USB bootstrap on a
healthy canopy. USB is not inherently required for OTA. Brian's current
compile-time credentials have not been established from the shared records.
After bootstrap, verify an actual BenPhone association and remote OTA using
the T-Deck. Skitty is excluded. Initial evidence:
`20261017T003500Z-9F26BC-initial-maint-discovery.jsonl`
and `20261017T003300Z-8EB508-benphone-ota-radio.jsonl` under the Oakland bench
data directory.

## Retained October 16 canary artifact

- Revision: `fx-261017-bfb3ab9-b` (UTC date in the generated name).
- Source: `8ce02b6296b185ecf8457f96897993a0be62a317`, clean checkout.
- Binary SHA-256: `7b8c95e96aae34e2ecbf02975e5166d91159818c63a1ff2cfd30b1845fb67100`.
- Binary length: 1,225,168 bytes; recipe SHA-256:
  `bfb3ab9543cb65d98c20d1f2f22864fa10d0ec5efb221fa2817faf171912722f`.
- Local artifact directory: `firmware/fixture/build/fx-261017-bfb3ab9-b/`.
- Shared manifest: `ops/bench/data/Oakland/fx-261017-bfb3ab9-b-manifest.json`.
- Build finished at 17:37 PDT; flash usage 36%, static RAM usage 21%.

Verified the recipe digest, binary digest/length, embedded revision and both
SSID names, exact decompression flags and two 0x330000-byte OTA app slots.
The output files were copied from the clean managed worktree and verified
byte-for-byte. The binary includes a private fallback credential, so the
repository shares its manifest and digest; transfer the retained binary
privately if another operator needs it. Do not rebuild this revision with
different credentials. Hardware association, upload, rollback-window
survival and later liveness remain untested for this canopy artifact.

## Daisy hotspot path qualified

On October 16, Daisy F40308 received the separate ordinary-perimeter artifact
`fx-261017-abe3c6e-b` by supervised USB bootstrap, preserving its settings and
alternate app. It then accepted a T-Deck radio maintenance request, joined
BenPhone at 10.60.100.228 on 2.4 GHz channel 6, and received the same retained
image by WiFi. The 7.43-second transfer was followed by software reboot,
channel-11 mesh return, explicit self-test/VALID acceptance, a fresh radio
uptime gate and a separate app1-valid USB check. Both sensors were reading
and Daisy was FULL. No manual reset was needed for the wireless update.

Perimeter SHA-256:
`85b594b003d2e89afe3a551218164e2bedc6efce9d608a0809519d534b9bad52`.
See [the Daisy test record](../tests/BENPHONE_OTA_DAISY_2026-10-16.md) for exact
source, manifest, evidence and scope. This does not qualify the nine canopies
or replace Marill's special controller build. Skitty was untouched.
