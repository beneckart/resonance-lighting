// Bounded raw-motion recorder used only by exact-target field test images.
// The ring itself is platform-independent so overwrite/cursor behavior is
// native-tested; ESP32 glue owns allocation, sampling, and HTTP serialization.
#pragma once

#include <stdint.h>

#define MOTION_TRACE_ZONE_COUNT 16

enum MotionTraceRangeSensor : uint8_t {
  MOTION_TRACE_RANGE_NONE = 0,
  MOTION_TRACE_RANGE_TMF8820 = 1,
  MOTION_TRACE_RANGE_VL53L5CX = 2,
};

struct __attribute__((packed)) MotionTraceSample {
  uint32_t seq;
  uint32_t uptimeMs;
  int16_t accelMg[3];
  int16_t gravityMg[3];
  uint16_t tiltCdeg;
  uint16_t swayMg;
  uint8_t fixtureClass;
  uint8_t rangeSensor;
  uint32_t rangeReads;
  uint32_t rangeFrameMs;
  uint16_t closestMm;
  uint16_t closestConfidence;
  // TMF: primary=3x3 nearest range, auxiliary=confidence (first 9 slots).
  // VL53: primary=4x4 nearest range, auxiliary=farthest ground candidate.
  uint16_t zonePrimary[MOTION_TRACE_ZONE_COUNT];
  uint16_t zoneAuxiliary[MOTION_TRACE_ZONE_COUNT];
  // VL53 plane fit for the current frame. Slopes are a/b * 1000; c is mm.
  int16_t planeAMilli;
  int16_t planeBMilli;
  uint16_t planeCMm;
  uint16_t planeTiltCdeg;
  uint8_t planeValid;
  uint8_t planeZones;
  uint8_t validZones;
  uint8_t targetZones;
  uint8_t nearZones;
  uint8_t presenceActive;
  uint8_t presenceRising;
  uint8_t rangeInteractionActive;
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
