// Bounded raw-motion recorder used only by exact-target field test images.
// The ring itself is platform-independent so overwrite/cursor behavior is
// native-tested; ESP32 glue owns allocation, sampling, and HTTP serialization.
#pragma once

#include <stdint.h>

#define MOTION_TRACE_ZONE_COUNT 9

struct __attribute__((packed)) MotionTraceSample {
  uint32_t seq;
  uint32_t uptimeMs;
  int16_t accelMg[3];
  int16_t gravityMg[3];
  uint16_t tiltCdeg;
  uint16_t swayMg;
  uint32_t tmfReads;
  uint16_t tofDepthMm;
  uint16_t tofConfidence;
  uint16_t tofZoneMm[MOTION_TRACE_ZONE_COUNT];
  uint16_t tofZoneConfidence[MOTION_TRACE_ZONE_COUNT];
  uint8_t presenceActive;
  uint8_t presenceRising;
  uint8_t lifeState;
  uint8_t program;
  uint8_t powerTier;
  uint8_t ledRailOn;
  uint8_t ledR;
  uint8_t ledG;
  uint8_t ledB;
  uint8_t ledW;
  uint8_t ledLitPixels;
};

struct MotionTraceBuffer {
  MotionTraceSample *samples;
  uint32_t capacity;
  uint32_t count;
  uint32_t writeIndex;
  uint32_t nextSeq;
  uint32_t overwrites;
};

void motionTraceBufferInit(MotionTraceBuffer &buffer,
                           MotionTraceSample *storage, uint32_t capacity);
uint32_t motionTraceBufferAppend(MotionTraceBuffer &buffer,
                                 const MotionTraceSample &sample);
uint32_t motionTraceBufferOldestSeq(const MotionTraceBuffer &buffer);
uint32_t motionTraceBufferNewestSeq(const MotionTraceBuffer &buffer);

// Copy up to maxOut records with seq > afterSeq, oldest first. A cursor older
// than the retained window naturally starts at the oldest retained record.
uint32_t motionTraceBufferCollectAfter(const MotionTraceBuffer &buffer,
                                       uint32_t afterSeq,
                                       MotionTraceSample *out,
                                       uint32_t maxOut);
