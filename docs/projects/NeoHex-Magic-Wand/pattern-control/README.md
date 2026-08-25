# NeoHex Magic Wand -- Pattern Control

**Status:** Probable path / design notes. Pending Steve's confirmation with Ben before implementation.

**Recorded:** 2026-08-23

## Goal

Give the Magic Wand an intuitive, playa-friendly way to change and control light patterns without requiring a phone, while preserving a more powerful operator interface through Ben's Resonance handheld bridge.

The wand should be fun to hand to another participant: normal movement should affect the current pattern, but ordinary bumps, walking, dancing, handoffs, or setting the wand down should not accidentally change patterns.

## Probable control hierarchy

1. **Normal movement = play within the current pattern.**
   - Tilt, wave, lift, lower, bump, or dance can influence animation parameters.
   - Ordinary movement does **not** switch to another pattern.

2. **Deliberate gesture sequence = change among a small set of favorites.**
   - Keep roughly 6-10 favorite patterns available directly from the wand.
   - Use an intentionally unusual gesture sequence to enter pattern-selection mode.
   - Candidate unlock: **raise -> hold -> lower -> hold**.
   - MSA311 acceleration/motion and BMP581 relative-pressure/elevation should corroborate the sequence so accidental activation is unlikely.
   - Give clear visual feedback when selection mode is entered.
   - During a short selection window, a deliberate upward motion could mean next pattern and a downward motion previous pattern.
   - After a short period of inactivity, lock the selected pattern and give a visual confirmation.
   - Exact thresholds, timing, and gestures are not decided yet and should be tuned on the physical wand.

3. **LilyGO T-Deck Plus = full operator control.**
   - Ben has two T-Deck Plus handhelds and the `firmware/tdeck_bridge` Bridge OS is already hardware-verified through M0-M4 as of 2026-08-20.
   - Use the T-Deck for access to the full pattern library (target roughly 20-40 excellent patterns, about 30 as a working goal), plus palette, speed, brightness, motion response, and Tree-interaction choices.
   - A future Wand page/app on the T-Deck could target the Magic Wand fixture directly and show its current pattern/settings.
   - The phone is not intended to be the normal artistic control surface; reserve phone use primarily for maintenance if needed.

4. **Wand <-> Tree interaction should be direct over ESP-NOW once configured.**
   - The wand is already an ESP-NOW peer on fleet channel 11.
   - Prefer the T-Deck as the device that **configures/selects** the wand's Tree-interaction behavior, not as a permanent relay.
   - After a Tree-interaction mode is chosen, the wand should be able to broadcast its own events directly to the Tree so the T-Deck can be put away.

## Candidate Tree-interaction ideas

These are examples, not commitments:

- Slow lift: light appears to rise through the Tree.
- Sharp upward flick: launch a light pulse/ripple outward.
- Intentional bump/tap: nearby fixtures sparkle or react briefly.
- Hold wand high: attract or concentrate light toward the wand's vicinity.
- Lower wand: settle/release the Tree back toward its normal state.
- Movement intensity can modulate the strength, speed, or radius of a Tree response.

## Accidental-change guardrails

- Do not map a single bump, shake, tilt, or ordinary lift directly to pattern switching.
- Require a multi-step gesture and timing window for mode entry.
- Prefer agreement between accelerometer-derived motion and BMP581 relative-elevation change for gesture unlock.
- Require a visible acknowledgement before pattern-selection gestures are accepted.
- Automatically exit selection mode after a short timeout/inactivity period.
- Keep gesture-selected patterns to a short curated favorites list; full library selection belongs on the T-Deck.

## Pattern-library direction

- Storage is not expected to be the practical constraint; procedural patterns reuse the same 740-pixel frame buffer.
- Favor approximately **30 strong core patterns** rather than a very large list of minor variations.
- Build variations through palettes, speed, direction, intensity, motion response, and Tree-interaction parameters rather than treating every variation as a separate pattern.
- Roughly 6-10 patterns should be designated as easy-access wand favorites.

## Relevant existing hardware/software

### Magic Wand

- Fixture ID: `F40344`
- ESP-NOW channel: 11
- Sensors: MSA311 accelerometer + BMP581 pressure/temperature sensor
- LEDs: 20 M5Stack NeoHex boards / 740 pixels
- Current wand work branch: `codex/NeoHex-Magic-Wand`
- Current renderer: `firmware/net_bench/magic_wand_mode.h`

### Ben's handheld bridge

Primary handheld: **LilyGO T-Deck Plus (LCD variant, not T-Deck Pro)**.

Repo implementation:

- `firmware/tdeck_bridge/`
- `firmware/tdeck_bridge/README.md`
- ADR 0037: `docs/decisions/0037-claude-mesh-bridge-handheld.md`
- ADR 0047: `docs/decisions/0047-bridge-os-tdeck-app-platform.md`

The T-Deck Bridge OS already acts as an ESP-NOW mesh citizen and command transmitter while also supporting Wi-Fi/Claude services. The dedicated Patterns functionality was still listed as remaining work in the 2026-08-20 status, making wand pattern control a natural extension rather than a separate handheld architecture.

## Decision checkpoint with Ben

Before implementing this control architecture, Steve plans to confirm the direction with Ben. In particular, confirm:

- whether the T-Deck should own the full Wand control UI;
- whether a new wand-specific ESP-NOW command/event type is appropriate or existing fleet packet types can be extended/reused;
- which gestures are safe and practical given the MSA311/BMP581 sampling already present;
- how many wand-favorite patterns should be exposed through gestures;
- which Tree-interaction effects are desirable for the 2026 playa deployment versus later experimentation.
