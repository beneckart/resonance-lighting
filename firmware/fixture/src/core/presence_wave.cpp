#include "presence_wave.h"

#include <string.h>

void tmfPresenceInit(TmfPresenceGate &gate) {
  memset(&gate, 0, sizeof(gate));
}

static void learnBaseline(uint16_t &baselineMm, uint16_t depthMm) {
  if (!baselineMm) baselineMm = depthMm;
  else baselineMm =
      (uint16_t)(((uint32_t)baselineMm * 7U + depthMm) / 8U);
}

bool tmfPresenceObserve(TmfPresenceGate &gate, uint32_t readSeq,
                        const uint16_t zoneMm[PRESENCE_ZONE_COUNT],
                        const uint16_t zoneConfidence[PRESENCE_ZONE_COUNT]) {
  if (!readSeq || readSeq == gate.lastReadSeq) return false;
  gate.lastReadSeq = readSeq;

  if (gate.warmupReads < PRESENCE_WARMUP_READS) {
    ++gate.warmupReads;
    for (uint8_t zone = 0; zone < PRESENCE_ZONE_COUNT; ++zone) {
      bool valid = zoneConfidence[zone] >= PRESENCE_MIN_CONFIDENCE &&
                   zoneMm[zone] >= 80 && zoneMm[zone] <= 2500;
      if (valid) learnBaseline(gate.baselineMm[zone], zoneMm[zone]);
    }
    return false;
  }

  uint16_t closeMask = 0;
  for (uint8_t zone = 0; zone < PRESENCE_ZONE_COUNT; ++zone) {
    bool valid = zoneConfidence[zone] >= PRESENCE_MIN_CONFIDENCE &&
                 zoneMm[zone] >= 80 && zoneMm[zone] <= 2500;
    bool close = valid &&
        (gate.baselineMm[zone]
             ? (uint32_t)zoneMm[zone] + PRESENCE_DELTA_MM < gate.baselineMm[zone]
             : zoneMm[zone] <= PRESENCE_MAX_MM);
    if (close) closeMask |= (uint16_t)(1U << zone);
  }

  if (gate.latched) {
    if (closeMask) gate.clearReads = 0;
    else if (++gate.clearReads >= PRESENCE_CLEAR_READS) {
      gate.latched = false;
      gate.clearReads = 0;
    }
    gate.priorCloseMask = closeMask;
    return false;
  }

  // Slowly follow unchanged background zones after warm-up, without letting a
  // close candidate teach itself into the scene. A single noisy frame is not
  // enough: the same spatial zone must be closer on two consecutive reports.
  for (uint8_t zone = 0; zone < PRESENCE_ZONE_COUNT; ++zone) {
    bool valid = zoneConfidence[zone] >= PRESENCE_MIN_CONFIDENCE &&
                 zoneMm[zone] >= 80 && zoneMm[zone] <= 2500;
    if (valid && !(closeMask & (uint16_t)(1U << zone)))
      learnBaseline(gate.baselineMm[zone], zoneMm[zone]);
  }
  bool rising = (closeMask & gate.priorCloseMask) != 0;
  gate.priorCloseMask = closeMask;
  if (!rising) return false;
  gate.latched = true;
  gate.clearReads = 0;
  return true;
}

void waveHueToRgb(uint8_t hue, uint8_t value,
                  uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t segment = hue / 43;
  uint8_t remainder = (uint8_t)((hue % 43) * 6);
  uint8_t rising = (uint8_t)(((uint16_t)value * remainder) / 255);
  uint8_t falling = (uint8_t)(value - rising);
  switch (segment) {
  case 0: r = value; g = rising; b = 0; break;
  case 1: r = falling; g = value; b = 0; break;
  case 2: r = 0; g = value; b = rising; break;
  case 3: r = 0; g = falling; b = value; break;
  case 4: r = rising; g = 0; b = value; break;
  default: r = value; g = 0; b = falling; break;
  }
}

bool waveIdSeen(const uint8_t visited[][3], uint8_t count,
                const uint8_t id[3]) {
  for (uint8_t i = 0; i < count; ++i)
    if (memcmp(visited[i], id, 3) == 0) return true;
  return false;
}
