#include "motion_trace.h"

#include <string.h>

void motionTracePresenceSentinelFrame(FrameBuffer &frame,
                                      bool presenceActive) {
  frameClear(frame);
  frame.count = 1;
  frame.px[0][0] = 255;
  if (presenceActive) {
    frame.px[0][1] = 255;
    frame.px[0][2] = 255;
  }
}

void motionTraceBufferInit(MotionTraceBuffer &buffer,
                           MotionTraceSample *storage, uint32_t capacity) {
  memset(&buffer, 0, sizeof(buffer));
  buffer.samples = storage;
  buffer.capacity = storage ? capacity : 0;
  buffer.nextSeq = 1;
}

uint32_t motionTraceBufferAppend(MotionTraceBuffer &buffer,
                                 const MotionTraceSample &sample) {
  if (!buffer.samples || !buffer.capacity) return 0;
  MotionTraceSample value = sample;
  value.seq = buffer.nextSeq++;
  buffer.samples[buffer.writeIndex] = value;
  buffer.writeIndex = (buffer.writeIndex + 1U) % buffer.capacity;
  if (buffer.count < buffer.capacity)
    ++buffer.count;
  else
    ++buffer.overwrites;
  return value.seq;
}

static uint32_t oldestIndex(const MotionTraceBuffer &buffer) {
  return buffer.count < buffer.capacity ? 0U : buffer.writeIndex;
}

uint32_t motionTraceBufferOldestSeq(const MotionTraceBuffer &buffer) {
  if (!buffer.samples || !buffer.count) return 0;
  return buffer.samples[oldestIndex(buffer)].seq;
}

uint32_t motionTraceBufferNewestSeq(const MotionTraceBuffer &buffer) {
  if (!buffer.samples || !buffer.count) return 0;
  uint32_t index = buffer.writeIndex ? buffer.writeIndex - 1U
                                     : buffer.capacity - 1U;
  return buffer.samples[index].seq;
}

uint32_t motionTraceBufferCollectAfter(const MotionTraceBuffer &buffer,
                                       uint32_t afterSeq,
                                       MotionTraceSample *out,
                                       uint32_t maxOut) {
  if (!buffer.samples || !buffer.count || !out || !maxOut) return 0;
  uint32_t copied = 0;
  uint32_t index = oldestIndex(buffer);
  for (uint32_t i = 0; i < buffer.count && copied < maxOut; ++i) {
    const MotionTraceSample &sample = buffer.samples[index];
    if (sample.seq > afterSeq) out[copied++] = sample;
    index = (index + 1U) % buffer.capacity;
  }
  return copied;
}
