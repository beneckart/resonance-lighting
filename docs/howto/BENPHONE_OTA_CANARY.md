# BenPhone maintenance OTA canary

Ben authorized the temporary Loom hotspot credentials in
`firmware/fixture/wifi_benphone.h` on 2026-10-16. The shared profile selects
BenPhone on 2.4 GHz; use WPA2-Personal. The fixture scans/joins it only during
explicit maintenance, after leaving ESP-NOW. Normal mesh operation remains
on channel 11; fixture maintenance can use the hotspot's actual channel.

The profile is build-time configuration. Merely pulling this file does not
change any installed fixture. Select it with `--wifi-source wifi_benphone.h`
for a hotspot-only build. To retain a yard fallback, create the ignored
`wifi_secrets.h` with `#include "wifi_benphone.h"` and the private
`RES_WIFI_SSID_2` / `RES_WIFI_PASSWORD_2` pair. Do not publish that private pair.

For the October 16 canary, retain the existing Party In The Woods network
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
The current task has prepared source only; the canary target and deployment
result must be recorded separately after actual verification.
