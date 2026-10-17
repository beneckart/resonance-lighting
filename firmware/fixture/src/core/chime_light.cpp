#include "chime_light.h"

#include "presence_wave.h"  // waveHueToRgb

uint8_t chimeLightSeedHue(const uint8_t id[3]) {
  // Small mix so consecutive MACs land far apart on the wheel.
  uint32_t h = (uint32_t)id[0] * 131u + (uint32_t)id[1] * 197u + (uint32_t)id[2] * 251u;
  return (uint8_t)(h ^ (h >> 8));
}

void chimeLightNoteStrikes(ChimeLight &c, uint32_t strikes, uint32_t nowMs) {
  if (!c.seeded) {  // first sight of the counter is not a chime
    c.lastStrikes = strikes;
    c.seeded = true;
    return;
  }
  if (strikes == c.lastStrikes) return;
  c.jumps = (uint8_t)(c.jumps + CHIME_LIGHT_JUMP);  // one jump per tick, even if several strikes landed
  c.lastStrikes = strikes;
  c.flashAtMs = nowMs;
  c.flashing = true;
}

void chimeLightRgbw(ChimeLight &c, uint32_t nowMs, uint8_t out[4]) {
  uint8_t drift = (uint8_t)(((uint64_t)nowMs * 256u / CHIME_LIGHT_TURN_MS) & 0xFF);
  uint8_t hue = (uint8_t)(c.baseHue + drift + c.jumps);
  uint8_t value = CHIME_LIGHT_AMBIENT, white = 0;
  if (c.flashing) {
    uint32_t age = nowMs - c.flashAtMs;
    if (age < CHIME_LIGHT_FLASH_MS) {
      value = CHIME_LIGHT_FLASH;
      white = CHIME_LIGHT_FLASH / 2;  // a white pop on RGBW heads, on top of the new colour
    } else if (age < CHIME_LIGHT_FLASH_MS + CHIME_LIGHT_FADE_MS) {
      uint32_t left = CHIME_LIGHT_FLASH_MS + CHIME_LIGHT_FADE_MS - age;  // fade to ambient
      value = (uint8_t)(CHIME_LIGHT_AMBIENT + (uint32_t)(CHIME_LIGHT_FLASH - CHIME_LIGHT_AMBIENT) * left / CHIME_LIGHT_FADE_MS);
      white = (uint8_t)((uint32_t)(CHIME_LIGHT_FLASH / 2) * left / CHIME_LIGHT_FADE_MS);
    } else {
      c.flashing = false;
    }
  }
  waveHueToRgb(hue, value, out[0], out[1], out[2]);
  out[3] = white;
}
