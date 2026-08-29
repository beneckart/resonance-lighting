// Bounded exact-target radio-off + perimeter-ToF power experiment. The ring
// is platform-independent; ESP32 glue owns phase transitions and retrieval.
#pragma once

#include <stdint.h>

enum SentinelTracePhase : uint8_t {
  SENTINEL_TRACE_DISABLED = 0,
  SENTINEL_TRACE_RADIO_SETTLE = 1,
  SENTINEL_TRACE_BASELINE_A = 2,
  SENTINEL_TRACE_TOF_WARMUP = 3,
  SENTINEL_TRACE_TOF_ACTIVE = 4,
  SENTINEL_TRACE_BASELINE_B = 5,
  SENTINEL_TRACE_RETRIEVAL = 6,
  SENTINEL_TRACE_ERROR = 7,
};

struct __attribute__((packed)) SentinelTraceSample {
  uint32_t seq;
  uint32_t uptimeMs;
  uint32_t phaseElapsedMs;
  uint32_t vlReads;
  int16_t batteryMa;
  int16_t batteryRawMa;
  int16_t supplyMa;
  uint16_t batteryMv;
  uint16_t supplyMv;
  uint16_t vlClosestMm;
  int8_t socPct;
  uint8_t phase;
  uint8_t powerFlags;
  uint8_t supplyGood;
  uint8_t chargingEnabled;
  uint8_t chargerPhase;
  uint8_t chargerFault;
  uint8_t radioOn;
  uint8_t sensorRailOn;
  uint8_t vlOk;
  uint8_t vlNearZones;
  uint8_t vlValidZones;
  uint8_t presenceRising;
};

struct SentinelTraceBuffer {
  SentinelTraceSample *samples;
  uint32_t capacity;
  uint32_t count;
  uint32_t writeIndex;
  uint32_t nextSeq;
  uint32_t overwrites;
};

void sentinelTraceBufferInit(SentinelTraceBuffer &buffer,
                             SentinelTraceSample *storage,
                             uint32_t capacity);
uint32_t sentinelTraceBufferAppend(SentinelTraceBuffer &buffer,
                                   const SentinelTraceSample &sample);
uint32_t sentinelTraceBufferOldestSeq(const SentinelTraceBuffer &buffer);
uint32_t sentinelTraceBufferNewestSeq(const SentinelTraceBuffer &buffer);
uint32_t sentinelTraceBufferCollectAfter(const SentinelTraceBuffer &buffer,
                                         uint32_t afterSeq,
                                         SentinelTraceSample *out,
                                         uint32_t maxOut);

const char *sentinelTracePhaseName(uint8_t phase);
