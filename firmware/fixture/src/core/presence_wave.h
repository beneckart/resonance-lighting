// Native-testable TMF8820 presence gate and small helpers for the field-demo
// RSSI wave. RF scheduling and rendering remain in behavior_glue.cpp.
#pragma once

#include <stdint.h>

#define PRESENCE_WARMUP_READS 90
#define PRESENCE_MIN_CONFIDENCE 20
#define PRESENCE_MAX_MM 2200
#define PRESENCE_DELTA_MM 300
#define PRESENCE_HIT_READS 3
#define PRESENCE_CLEAR_READS 4
#define PRESENCE_ZONE_COUNT 9

struct TmfPresenceGate {
  uint32_t lastReadSeq;
  uint16_t baselineMm[PRESENCE_ZONE_COUNT];
  uint8_t warmupReads;
  uint8_t clearReads;
  uint8_t closeStreak[PRESENCE_ZONE_COUNT];
  bool latched;
};

void tmfPresenceInit(TmfPresenceGate &gate);

// Call for every loop; a sample is consumed only when readSeq changes. Returns
// true once on the rising edge after two consecutive close/confident returns.
bool tmfPresenceObserve(TmfPresenceGate &gate, uint32_t readSeq,
                        const uint16_t zoneMm[PRESENCE_ZONE_COUNT],
                        const uint16_t zoneConfidence[PRESENCE_ZONE_COUNT]);

void waveHueToRgb(uint8_t hue, uint8_t value,
                  uint8_t &r, uint8_t &g, uint8_t &b);

bool waveIdSeen(const uint8_t visited[][3], uint8_t count,
                const uint8_t id[3]);
