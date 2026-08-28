# 0066 -- Two maintenance WiFi profiles for site portability

**Date:** 2026-08-27

**Status:** Accepted and fleet-promoted; selection logic compile-tested, second
site hardware association pending

**Owner:** Ben

**Extends:** ADR 0010, ADR 0036, ADR 0040

## Context

The physical fleet is splitting earlier than planned. Perimeter fixtures are
moving to the art site while the rest of the fleet and one Starlink remain at
camp. The two locations have independently administered 2.4 GHz networks and
the camp owner declined renaming their existing SSID. Requiring one virtual
SSID would either strand one cohort from shared-WiFi OTA or require disruptive
credential changes immediately before installation.

Fixtures use infrastructure WiFi only in deliberate OTA maintenance mode.
Ordinary COMMS remains ESP-NOW-only, so remembering a second maintenance
network does not add an always-associated client or change the mesh channel.

## Decision

1. A production fixture artifact may carry two bounded maintenance credential
   profiles. The first profile remains required; the second is optional, and a
   compile-time error rejects a half-defined SSID/password pair.
2. On maintenance entry, the fixture scans once. Visible known profiles are
   tried in descending RSSI order. If no known profile appears in the scan,
   profiles are tried in declaration order as a fallback for hidden or
   transient scan results.
3. Both profiles share one 30-second association budget. Failure of one profile
   does not reset that budget or create an unbounded awake loop.
4. Logs identify only `profile 1` or `profile 2`; they do not print SSIDs or
   passwords. Real credentials remain in gitignored `wifi_secrets.h`. Recipes
   and immutable manifests record a non-secret configuration label, never
   credential values.
5. Ordinary fixture COMMS does not scan for or join either network. The
   credential plan applies only after the existing exact maintenance command
   deliberately leaves ESP-NOW.
6. ADR 0036 still governs any device using WiFi and ESP-NOW simultaneously.
   Fixture maintenance has already left the mesh, so either maintenance AP may
   use its local channel. A bridge or future uplink that stays on ESP-NOW must
   still use the channel-11 guard.
7. A single shared SSID is no longer required for fixture OTA portability.
   Operators may retain distinct camp and art-site network names and passwords.

## Consequences

- One immutable fleet image can accept OTA at either location without renaming
  an existing camp network.
- If both known APs overlap, the strongest visible one is preferred. This is a
  deterministic heuristic, not seamless roaming; fixtures are stationary and
  maintenance sessions are short.
- A credential change still requires a new immutable artifact or the separate
  bounded provisioning path. No secret is stored in the registry, job ledger,
  or git history.
- A successful rollout over the first profile proves the image and OTA path,
  but does not prove the second AP's password, security mode, DHCP, RF coverage,
  or internet path. That requires an explicit hardware association at the art
  site.

## Validation evidence -- 2026-08-27

The pure credential planner passed ten native checks. A production ESP32-S3
build passed with both local credential pairs defined. Immutable artifact
`fx-260828-658b7d2-p` is 1,208,640 bytes with SHA-256
`95de59286831bcbb9d8f610f84b09e3ac761be558f106b10b9aee8dfb01bd8cc`.
Swablu `F2BE70` passed exact-target OTA, fresh exact-revision mesh rejoin, and
the pending-verify gate. The wider rollout left 98 intended fixtures fully
verified on the same image. Uploads used the already-established maintenance
network; second-site hardware association remains open.

## References

- `firmware/fixture/src/core/wifi_credential_plan.h`
- `firmware/fixture/src/esp32/maintenance.cpp`
- `firmware/fixture/wifi_secrets.h.example`
- `firmware/fixture/tests/test_wifi_credential_plan.cpp`
- `firmware/fixture/README.md`
- ADR 0036
- `docs/howto/CAMP_NETWORK_SETUP.md`
