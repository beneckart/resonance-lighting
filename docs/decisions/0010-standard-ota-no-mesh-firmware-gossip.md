# 0010 — Standard OTA only; no mesh-gossiped firmware images

**Date:** 2026-05-08
**Status:** Accepted
**Owners:** Ben
**Supersedes:** OTA portions of ADR 0004 and `firmware/ARCHITECTURE.md`

## Context

Fixtures need firmware updates after fabrication and possibly during deployment week. The earlier architecture allowed firmware images to be distributed peer-to-peer through the ESP-NOW network. That is too clever for a high-consequence subsystem: a bad custom OTA protocol could brick many fixtures, waste field time, or create inconsistent firmware versions in a 100-node swarm.

ESP-NOW is still useful for small lighting/control packets. Firmware transport is different. It is large, stateful, failure-prone, and needs rollback.

## Options considered

- **Custom ESP-NOW firmware gossip:** rejected. Too much bespoke protocol surface; failure modes are catastrophic.
- **Standard ESP32 OTA from a local WiFi source:** accepted. Use existing ESP-IDF / Arduino OTA mechanisms, A/B partitions, and rollback.
- **USB-C / pogo flashing:** required fallback. This is the recovery path when OTA fails or a board is bricked.
- **Factory pre-flash:** desirable, if the assembler can do it reliably, but it does not replace a local flashing/recovery jig.

## Decision

Use the most standard ESP32 OTA flow available. Do not implement firmware-image transport over ESP-NOW.

Deployment/update model:

- Fixtures ship with one known-good firmware image flashed at the fab or via jig.
- OTA occurs only in an explicit maintenance mode.
- A local laptop or Raspberry Pi hosts the firmware image over ordinary WiFi.
- Fixtures connect to a known local AP or temporarily enter a controlled WiFi mode to fetch the image.
- Use A/B OTA partitions with validation and rollback.
- ESP-NOW may advertise small metadata only: current firmware version, maintenance window, or “update available.” It must not carry firmware image chunks.
- USB-C / pogo flashing remains the guaranteed recovery path.

## Consequences

- Firmware update protocol complexity is dramatically reduced.
- Field updates are slower and more deliberate, which is acceptable.
- The show-control mesh and firmware-update path remain decoupled.
- The smoke-test rig should report firmware version and OTA partition status for every node.
- The firmware architecture should remove all “peer serves image to peers” language.
- If future work wants distributed OTA, it is a 2027 R&D feature, not a 2026 production dependency.
