# Roadmap

The canonical roadmap is [../ROADMAP.md](../ROADMAP.md).

This file used to carry an older custom-PCBA-first timeline with ESP32-C3, CN3058, and
IS31FL3741 assumptions. Those assumptions are superseded by the June 2026 bench findings:

- PowerFeather V2 is the validated COTS/reference architecture (ADR 0021).
- The IS31FL3741 matrix is ruled out for the V2 battery build (ADR 0018).
- The LED fleet is mixed by role: HEX plus 4 W RGBW point source (ADR 0022).
- The active gates are energy sizing, panel choice, hat RF/thermal, and production path.

Keep roadmap edits in the root `ROADMAP.md` to avoid two drifting schedules.
