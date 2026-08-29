// Native-testable TMF8820 presence gate and small helpers for the field-demo
// RSSI wave. RF scheduling and rendering remain in behavior_glue.cpp.
#pragma once

#include <stdint.h>

#define PRESENCE_WARMUP_READS 90
#define PRESENCE_MIN_CONFIDENCE 20
#define PRESENCE_SENSOR_MAX_MM 5000
#define PRESENCE_MAX_MM 4500
#define PRESENCE_DELTA_MM 300
#define PRESENCE_HIT_READS 3
#define PRESENCE_CLEAR_READS 4
#define PRESENCE_EMPTY_REBASE_READS 12
#define PRESENCE_ZONE_COUNT 9

// Deliberate perimeter easter egg. A palm held 5-10 cm over F2BDFC produced
// 15-16 near zones against a stable clear baseline of zero. Debounce the broad
// multi-zone signature so a fleeting return cannot originate a program seed.
#define VL53_COVER_MIN_ZONES 4
#define VL53_COVER_HIT_READS 2
#define VL53_COVER_CLEAR_READS 4

struct TmfPresenceGate {
  uint32_t lastReadSeq;
  uint16_t baselineMm[PRESENCE_ZONE_COUNT];
  uint8_t warmupReads;
  uint8_t clearReads;
  uint8_t closeStreak[PRESENCE_ZONE_COUNT];
  uint8_t emptyStreak[PRESENCE_ZONE_COUNT];
  bool latched;
};

void tmfPresenceInit(TmfPresenceGate &gate);

// Call for every loop; a sample is consumed only when readSeq changes. Returns
// true once on the rising edge after three consecutive close/confident returns.
bool tmfPresenceObserve(TmfPresenceGate &gate, uint32_t readSeq,
                        const uint16_t zoneMm[PRESENCE_ZONE_COUNT],
                        const uint16_t zoneConfidence[PRESENCE_ZONE_COUNT]);

// Deliberately aggressive exact-target installed-height predicate. This
// removes background learning, hit debounce, and hold/release hysteresis. Any
// one confident zone from 1 m to the 5 m sensor limit is active; the lower bound
// excludes Gible's already-known 166-363 mm bamboo/sensor self-returns. Normal
// builds do not use this helper.
bool tmfDistantRangePresent(
    const uint16_t zoneMm[PRESENCE_ZONE_COUNT],
    const uint16_t zoneConfidence[PRESENCE_ZONE_COUNT]);

struct Vl53CoverGate {
  uint32_t lastReadSeq;
  uint8_t hitReads;
  uint8_t clearReads;
  bool latched;
};

void vl53CoverInit(Vl53CoverGate &gate);

// Consume each read sequence once. Returns one rising edge after a deliberate
// multi-zone cover, then requires four clear frames before another edge.
bool vl53CoverObserve(Vl53CoverGate &gate, uint32_t readSeq,
                      uint8_t nearZones);

void waveHueToRgb(uint8_t hue, uint8_t value,
                  uint8_t &r, uint8_t &g, uint8_t &b);

bool waveIdSeen(const uint8_t visited[][3], uint8_t count,
                const uint8_t id[3]);
