# 0004 — ESP-NOW for mesh

**Date:** 2026-05-06
**Status:** Accepted
**Owners:** Ben

## Context

100 fixtures distributed across a ~20 ft bamboo tree need to coordinate light state (cellular automata, wand-presence-driven ripples, day-of-week mode pushes, OTA updates). At Burning Man, no WiFi infrastructure (APs) can be assumed. Mesh must self-heal under fixture swap.

## Options considered

- **ESP-NOW** — Espressif's WiFi-frame-level protocol, peer-to-peer, no AP required, low latency, broadcast-capable. Built into ESP32-C3.
- **WiFi mesh (ESP-MESH-LITE / ESP-WIFI-MESH)** — proper mesh stack, requires AP-style root node, higher complexity.
- **BLE Mesh** — mature mesh standard, many implementations. Slower per-hop than ESP-NOW; profile is overkill for our message types.
- **LoRa** — long range, low rate. Used in 2018 Talisman v2. Adds an SX1276/78 module to BOM, more power, more design surface. Range overkill for 20 ft.
- **Painless Mesh (Arduino lib over WiFi)** — community mesh on top of WiFi, requires every node to act as both AP and STA. Higher power, more complex.

## Decision

**ESP-NOW** with a small custom protocol on top.

- Each fixture broadcasts its state every ~1 s.
- Each fixture listens for neighbors' broadcasts and maintains a local "neighbor map" with RSSI and last-heard timestamps.
- For wand-interaction propagation: special unicast or directed-broadcast messages with a TTL counter; receiving fixtures decrement TTL and re-broadcast, producing a wavefront across the mesh.
- For OTA: special "OTA available" broadcast carries a version number; fixtures with older firmware pull the new image from a designated peer (or a fallback known endpoint when the team is on-site with a laptop on the same WiFi).

## Consequences

- ESP-NOW does not give us "the internet" — pushing OTA images requires a local source (a laptop / Pi at the base of the tree during deploy week, or a peer pre-loaded). Acceptable for art-installation context.
- Simulating the mesh in Wokwi works: Wokwi has multi-instance ESP-NOW support.
- Range: ESP-NOW packets carry standard 2.4 GHz WiFi range — comfortably more than the tree's 20 ft scale even with bamboo attenuation. No range concerns.
- Packet payload max is 250 bytes (ESP-NOW limit). Fine for our fixture-state messages (a handful of bytes: device ID, state, brightness, battery %, RSSI of neighbors).
- Encryption available natively (ESP-NOW supports peer-keyed AES-128). Probably skip for art-installation use; keep for review.
- The 2018 Talisman v2 used LoRa with single-hop and wanted multi-hop but didn't ship it. ESP-NOW + custom flood protocol is the version of that vision (see `BACKGROUND.md`).
