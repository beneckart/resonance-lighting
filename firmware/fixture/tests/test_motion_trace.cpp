#include <assert.h>
#include <stdio.h>

#include "motion_trace.h"

static MotionTraceSample sampleAt(uint32_t uptimeMs) {
  MotionTraceSample sample = {};
  sample.uptimeMs = uptimeMs;
  return sample;
}

int main() {
  MotionTraceSample storage[3] = {};
  MotionTraceBuffer buffer;
  motionTraceBufferInit(buffer, storage, 3);
  assert(motionTraceBufferOldestSeq(buffer) == 0);
  assert(motionTraceBufferNewestSeq(buffer) == 0);

  assert(motionTraceBufferAppend(buffer, sampleAt(40)) == 1);
  assert(motionTraceBufferAppend(buffer, sampleAt(80)) == 2);
  assert(motionTraceBufferAppend(buffer, sampleAt(120)) == 3);
  assert(buffer.count == 3 && buffer.overwrites == 0);

  MotionTraceSample out[4] = {};
  assert(motionTraceBufferCollectAfter(buffer, 1, out, 4) == 2);
  assert(out[0].seq == 2 && out[0].uptimeMs == 80);
  assert(out[1].seq == 3 && out[1].uptimeMs == 120);

  assert(motionTraceBufferAppend(buffer, sampleAt(160)) == 4);
  assert(buffer.count == 3 && buffer.overwrites == 1);
  assert(motionTraceBufferOldestSeq(buffer) == 2);
  assert(motionTraceBufferNewestSeq(buffer) == 4);
  assert(motionTraceBufferCollectAfter(buffer, 0, out, 4) == 3);
  assert(out[0].seq == 2 && out[1].seq == 3 && out[2].seq == 4);
  assert(motionTraceBufferCollectAfter(buffer, 3, out, 1) == 1);
  assert(out[0].seq == 4);

  puts("motion_trace: PASS");
  return 0;
}
