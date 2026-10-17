# Daisy BenPhone OTA canary, October 16, 2026

Operator: Ben + Codex on Ben's laptop, sole writer for Daisy F40308
(`68:EE:8F:F4:03:08`). Skitty F3FD28 is excluded from all serial opens,
maintenance commands, resets, configuration and firmware writes in this test.
Its independent natural-recovery observer retains its original deadline.

Status: PASS for Daisy's USB bootstrap, remote maintenance entry and BenPhone
OTA. The nine canopies and Marill have not received this image.

## Connection and original image

The first 17:40:21-17:43:52 PDT enumeration capture found no perimeter port.
Ben had used the enclosure extension and the laptop's spare USB-A port.
Moving T-Deck 8EB508 onto that USB-A port with the same laptop cable produced
COM10 at 17:44:29; a read-only query returned valid bridge telemetry. Returning
the cable to the perimeter produced Daisy COM8 at 17:45:10. This proves the
port/cable can carry data and the enclosure path worked in this comparison;
it does not establish why the first connection remained silent.

Daisy reported field profile, perimeter class, FULL, a present 6 Ah-configured
battery and no charger fault. Settled samples were about 3.587 V and +355 mA.
The installed app0 was valid `fx-260912-f951ae9-b`; its retained binary contains
Party In The Woods but not BenPhone. No new physical battery-capacity claim
is made. Ben's nine canopies are connected to a USB hub on Brian's laptop;
they were not part of this write session.

## Exact artifact

- Revision: `fx-261017-abe3c6e-b`, ordinary perimeter/field image.
- Source: `bc9de328385df00dc242b267e4a1d1a20f69388d`, clean managed worktree.
- SHA-256: `85b594b003d2e89afe3a551218164e2bedc6efce9d608a0809519d534b9bad52`.
- Length: 1,226,288 bytes.
- Recipe: `abe3c6e842c1ceb8dac1984dde67c932c0240fbb0715ea1f91e63523b28159b5`.
- WiFi label: `benphone-party-v1`; BenPhone plus retained private bench fallback.
- Field/channel 11/basic-listener, 300 mA precharge, 120-second day interval,
  12-second radio window. No decompression or exact-target experiment flags.
- Local retained directory: `firmware/fixture/build/fx-261017-abe3c6e-b/`.
- Shared manifest: `ops/bench/data/Oakland/fx-261017-abe3c6e-b-manifest.json`.

The build completed with 36% flash and 21% static RAM usage. Manifest, recipe,
binary hash/length, embedded revision/SSID strings, actual compiler options
and both 0x330000-byte app slots were checked. Preserved output copies match
the build byte-for-byte. This is not the separate bfb3ab9 canopy image and
does not add Marill's presence controller or staged PROTECT experiment.

## USB bootstrap

At 17:50:35 PDT, exact USB identity and fresh healthy power telemetry gated
ROM entry. ROM identity matched Daisy. The first 64 KiB were saved privately;
the partition table matched the inspected artifact. ROM digest verification
matched the existing active app to the retained original f951ae9 binary.
Only app0 at 0x10000 was written. ROM verification subsequently matched both
the new application and the unchanged first 64 KiB. App1 was not written.

The watchdog-reset command returned a Windows serial-port abort after issuing
the reset. It was not treated as proof of failure or success: a subsequent
no-reset serial observer captured the new application boot, field/perimeter
identity, valid app0, no pending verification and later uptime above 50 seconds.
Both VL53 and MSA reads were healthy after initialization, with no charger fault.
The initial DIM/guard-stage-2 posture was recorded; it was not manually cleared.
Capacity, charge setting and original 3026 mV low-VBAT protection history matched
the preflight record. No post-boot setting was changed.

Evidence under `ops/bench/data/Oakland/`:

- `20261017T004021Z-perimeter-benphone-intake.jsonl`
- `20261017T004356Z-usb-a-port-compare.jsonl`
- `20261017T004429Z-8EB508-usb-a-proof.jsonl`
- `20261017T004452Z-perimeter-benphone-intake.jsonl`
- `20261017T005035Z-F40308-benphone-usb.jsonl`
- `20261017T005240Z-F40308-benphone-usb-boot.jsonl`

Private settings backup:
`ops/bench/private-backups/20261017T005035Z-F40308-benphone/`.

### Why the first boot was DIM

The preserved pre-write NVS explains the initial reduced-power posture:
CRC-checked `fc_stage=1` (FULL), `load_arm=1`, and `boots=0`. Live preflight
also reported the LED rail on and FULL at 3.584 V. The first post-USB-flash
boot reported `reset_reason=poweron` and guard stage 2 (DIM). In the exact
artifact source, an unexpected reset with FULL and an armed-load marker
consumes the reduced-load retry, even if the interruption was deliberate
USB service rather than a power collapse. The initial boot-guard text was
not captured; this attribution combines the saved inputs, reset telemetry
and deterministic source rule.

DIM can return to FULL after 60 seconds above the clear threshold (3.30 V
with default thresholds). The passive trace captured `power: tier -> 0
(bv=3.586 ma=326)` at 17:53:36.531906 PDT without an intervention. The later
OTA software boot explicitly logged `stage=1 stored=1 park=0 retry=0
interrupted=0 nvs=1 reset=software`, and remained FULL. This DIM episode is
consistent with the USB service interruption; it is not evidence of a new
low-battery trip or PROTECT entry. The original protection history survived.

Decoded backup evidence:
`ops/bench/data/Oakland/20261017T005035Z-F40308-benphone-nvs-audit.json`.

## OTA setup

Windows joined BenPhone using WPA2-Personal on 2.4 GHz channel 6, with address
10.60.100.36/24. The T-Deck remains on mesh channel 11. A bridge wrapper allows
maintenance/campaign writes only for F40308. The runner waits for a fresh
Daisy radio window, gathers only Daisy, and requires a post-job reset plus
fresh exact-revision evidence past 30 seconds. A separate Daisy-only USB
observer records the transfer and reboot without sending any bytes. Upload
acknowledgement alone will not be reported as successful qualification.

## Wireless result

Job `87B90584` waited for Daisy's next ordinary listening window, which opened
at 17:54:49 PDT. The targeted radio request entered maintenance at 17:54:56.
At 17:55:02 the passive serial trace reported profile 1/2, IP 10.60.100.228,
channel 6. Profile 1 is BenPhone in this retained artifact; the laptop was on
that same hotspot. This tests actual BenPhone association, not the fallback.

The bridge acknowledged a one-target frozen campaign before upload. The
1,226,288-byte WiFi transfer completed in 7.43 seconds at 17:55:18. Daisy
software-rebooted, initialized both sensors, and rejoined ESP-NOW on channel
11. The trace explicitly recorded PENDING_VERIFY, network self-test PASS and
`ota-verify: VALID (rollback cancelled)` at 17:55:39. Fresh exact-revision
radio evidence then passed the 30-second uptime gate. The job finished with
one verified target, none failed/deferred, and field profile preserved.

The separate postcheck at 17:56:04 read app1 VALID with no pending verification,
uptime 44.95 seconds, FULL, both sensors reading, charger fault zero and a
present charging battery. Thus OTA moved from the USB-bootstrapped app0 to
app1. This exercised normal A/B acceptance; it did not deliberately inject a
bad image to test rollback. No button press or second USB flash was required.

The final 17:56:43 check reached 83.665 seconds of continuous uptime, with
app1 still VALID, FULL, both sensors healthy, no charger fault, 3.576 V and
about +640 mA. Fifteen configuration/history fields, including the complete
last-PROTECT entry, matched the preflight snapshot. The local observers were
closed, and only Daisy's registry row was updated with this exact artifact
and fresh OTA qualification.

Wireless evidence under `ops/bench/data/Oakland/`:

- `20261017T004700Z-8EB508-daisy-benphone-ota-radio.jsonl`
- `20261017T005320Z-F40308-benphone-ota-serial.jsonl` (passive, zero TX)
- `20261017T005456Z-F40308-benphone-ota-job.jsonl`
- `20261017-005511-87B90584-fleet-ota-results.jsonl`
- `20261017T005605Z-F40308-benphone-ota-postcheck.jsonl`

This establishes the hotspot path for one healthy battery-backed perimeter
with the new credentials. Brian's installed canopy images still need a known
maintenance network or USB credential bootstrap before they can use BenPhone.
The prepared canopy artifact is separate; Marill needs a build retaining its
exact-target presence and staged-recovery flags.
