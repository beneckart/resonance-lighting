# 0004 -- ESP-NOW for mesh

**Date:** 2026-05-06
**Status:** Superseded by ADR 0010 -- Standard OTA only; ESP-NOW for lighting/control packets, not firmware-image transport
**Owners:** Ben

## Context

100 fixtures distributed across a ~20 ft bamboo tree need to coordinate light state with no fixed data wiring and no guaranteed WiFi infrastructure at Burning Man.

## Original decision

Use ESP-NOW with a small custom protocol on top. The original text allowed an OTA path where a bridge fixture serves an image to peers and peers gossip it forward through the mesh.

## Why this is superseded

ESP-NOW remains the correct lightweight control-plane protocol for lighting state, neighbor discovery, RSSI, wand events, and simple broadcasts. However, firmware images must not be transported via a custom ESP-NOW gossip protocol. OTA is a high-consequence subsystem where bugs can brick many fixtures. The updated decision is to use the most standard ESP32 OTA flow available and keep USB/pogo flashing as the guaranteed recovery path. See ADR 0010.
