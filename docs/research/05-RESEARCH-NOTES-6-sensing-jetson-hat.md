# DOCUMENT 5 of 5 — RESEARCH NOTES 6: Sensing, Jetson-as-Master, the Hat

**Sensor suite:** motion via LD2410-class mmWave at the crown (sees still humans, dust-immune) + per-fixture PIR elsewhere; BME280 (temp/humidity/baro — pressure trend = storm early warning); wind via crown anemometer or $2 chime IMUs (sway IS wind, gives gust mapping not a point reading); esp-csi crowd sensing as a P2 spike. All readings ride the heartbeat; fusion at the crown brain.

**Element-based lighting (art + protective reflex):** high wind → IMU-driven ripples + 20% dim to bank energy; heat (day) → pause charging; cold → ember palette; rain → silver-blue cascade; dust whiteout → **BEACON lighthouse mode** (genuine safety feature); storm forecast → amber hours ahead.

**Jetson as master — what it opens:** flash ceiling gone (full app on-site, OTA hosted, unlimited logging, heart-adoption queue); sensor fusion + ML at the hub; Voice and hub on one machine. Catch: Jetson has no ESP-NOW radio → Jetson hub + PowerFeather as USB radio modem (Ben's T7 topology, validated). Principle: the mesh stays a self-sufficient brainstem; the Jetson is the cortex — wakes at dusk, dies invisibly. Resolves power too (Jetson night-only; daytime = pure-mesh design).

**The Hat — centralized crown power:** the 124 canopy/root lights stay per-fixture solar-autonomous; the 16-light chandelier cluster centralizes under a hat (shared solar on top, central LiFePO4, sensor suite, DJI/audio electronics, 12V bus) so chandelier lights drop their own panels/batteries (lighter, freer chimes). Connector honesty: USB-C inside the sealed hat only; exposed = locking/soldered. Brain placement: (a) in the hat = best RF + shortest audio runs, but service-by-climbing + heat soak + 8–12kg aloft; **(b, recommended)** hat = energy + senses + antennas, brain at the trunk base on one cable (keeps the serviceability that saved MIRA). Protection: positive-pressure MERV-13 enclosure, parts ≥150°F, white shell, panels double as shade.

---

## Cross-doc reconciliation (strategist note, 2026-06-13)
This 5-doc dossier (the "controller/cortex" design) and Ben's repo `github.com/beneckart/resonance-lighting` (the "brainstem") describe ONE coherent two-tier system, NOT two rival systems:
- **Brainstem = Ben's build** (canonical, build-ready): ~100 autonomous solar fixtures, PowerFeather V2 / ESP32-S3 / SK6812, ESP-NOW, control-params-only. THE critical path (all 100 in hand ~Aug 20).
- **Cortex = this dossier** (optional night layer): Jetson at trunk base + PowerFeather-as-ESP-NOW-modem + iPad R3F twin/console + MIDI/DJ-VJ + voice + sensing + camp bridge. Explicitly non-required; "dies invisibly." Aug-1 soak gate; fallback = ship brainstem-only.
- **Keystone dependency (gates ALL controller work): OQ-1** — Rhino/Grasshopper → `fixtures.json` + glTF export. Blocks the Blender placement pipeline (Addendum C §C3.8) and the twin. Inputs already on the Mac: `~/Downloads/Tree_Rhino7.3dm` (Ed, Jun 12) + `Tree_Resonance_packed_2026-05-29.blend` (Mia).
- **Count to reconcile:** dossier = 150 (100 down + 24 up + 16 chandelier); Ben's repo scopes ~100 downlights only. Who owns uplights + chandelier is open.
- **Hard near-term risk (Addendum B B9 #1):** PIR sensors may need to be IN fixtures before they ship from Bali, or per-fixture sensing slips to 2027.
- The Blender→fixtures.json work (Addendum C §C3.8) IS the natural first job of the proposed "lighting specialist agent."
