#include <assert.h>
#include <stdio.h>

#include "motion_trace.h"

static MotionTraceSample sampleAt(uint32_t uptimeMs) {
  MotionTraceSample sample = {};
  sample.uptimeMs = uptimeMs;
  return sample;
}

int main() {
  FrameBuffer sentinel = {};
  sentinel.count = 4;
  sentinel.px[0][3] = 25;
  sentinel.px[1][1] = 99;
  motionTracePresenceSentinelFrame(sentinel, false);
  assert(sentinel.count == 1);
  assert(sentinel.px[0][0] == 255 && sentinel.px[0][1] == 0);
  assert(sentinel.px[0][2] == 0 && sentinel.px[0][3] == 0);
  assert(sentinel.px[1][0] == 0 && sentinel.px[1][1] == 0);
  motionTracePresenceSentinelFrame(sentinel, true);
  assert(sentinel.px[0][0] == 255 && sentinel.px[0][1] == 255);
  assert(sentinel.px[0][2] == 255 && sentinel.px[0][3] == 0);

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
