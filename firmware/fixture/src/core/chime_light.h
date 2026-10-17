#pragma once

#include <stdint.h>

// Decompression lighting that follows the chimes. While a lantern is awake in
// decompression mode its light drifts slowly through the colour wheel (one turn
// per CHIME_LIGHT_TURN_MS, each lantern starting at its own hue). Every
// solenoid strike, whatever sent it (USB, radio, pattern, signature), flashes
// the light and snaps it a third of the way round the wheel, so a pattern reads
// as a ripple of colour changes on the beat. Pure core: no Arduino.

static constexpr uint32_t CHIME_LIGHT_TURN_MS = 60000;  // ambient drift: one turn a minute
static constexpr uint32_t CHIME_LIGHT_FLASH_MS = 120;   // full flash
static constexpr uint32_t CHIME_LIGHT_FADE_MS = 600;    // then fade back to ambient
static constexpr uint8_t CHIME_LIGHT_JUMP = 85;         // hue jump per strike (~1/3 turn)
static constexpr uint8_t CHIME_LIGHT_AMBIENT = 90;      // ambient colour value (0..255)
static constexpr uint8_t CHIME_LIGHT_FLASH = 255;       // flash peak

struct ChimeLight {
  uint8_t baseHue = 0;      // per-lantern starting hue
  uint8_t jumps = 0;        // accumulated strike jumps (wraps)
  uint32_t lastStrikes = 0; // solenoid strike counter last seen
  bool seeded = false;
  uint32_t flashAtMs = 0;
  bool flashing = false;
};

// Starting hue from the lantern's short id, so neighbours differ.
uint8_t chimeLightSeedHue(const uint8_t id[3]);

// Feed the solenoid strike counter every tick; a change means a chime.
void chimeLightNoteStrikes(ChimeLight &c, uint32_t strikes, uint32_t nowMs);

// Current colour for an RGBW (or RGB: ignore out[3]) light.
void chimeLightRgbw(ChimeLight &c, uint32_t nowMs, uint8_t out[4]);
