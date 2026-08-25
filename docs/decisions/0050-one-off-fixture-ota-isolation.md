# 0050 -- One-off fixture OTA isolation

**Date:** 2026-08-24

**Status:** Accepted

**Owners:** Ben + Steve

## Context

Steve's NeoHex Magic Wand uses a PowerFeather V2 and participates in the
channel-11 ESP-NOW fleet, but it is not electrically or optically fungible with
the four production fixture classes. It drives 20 NeoHex boards (740 pixels)
through an external regulated 5.1 V rail and needs dedicated firmware. A normal
fleet image on the wand would not operate that hardware correctly; a wand image
on an ordinary fixture would also be wrong.

The wand's electronics identity is already fixed by its WiFi MAC. Human names,
physical location, IP address, and COM port are not stable enough for OTA
selection.

## Decision

1. The Magic Wand's canonical electronics identity is short MAC `F40344`, full
   MAC `68:EE:8F:F4:03:44`.
2. `ops/fleet/registry.csv` records it as role `magic_wand`. It remains visible
   in fleet health and mesh census views, but this role is outside the four
   fungible production classes.
3. Fleet batch OTA treats `magic_wand` as a protected role. The operator must
   explicitly repeat the exact short MAC with
   `--allow-special-target F40344`, and a protected fixture must be the only
   target in that OTA job.
4. The acknowledgement is not an artifact bypass. A wand update still uses one
   immutable dedicated artifact, exact binary SHA-256, an installed LFP for
   reboot ride-through, and fresh exact-revision evidence after the pending
   verification window (ADR 0040).
5. USB flashing follows the same exact-MAC and dedicated-artifact rule even
   though the host-side batch OTA interlock cannot police a manual USB command.

## Consequences

- Normal explicit fleet batches fail before sending maintenance if they include
  the wand.
- An intentional wand OTA is visibly exceptional and cannot share a target list
  with ordinary fixtures.
- The registry role, rather than a duplicated nickname or IP list, is the
  machine-readable source of the exception.
- The working `.1` image remains installed until a separately named dedicated
  artifact passes the current OTA completion contract.
