#include "test_util.h"

#include "../src/core/sentinel_trace.h"

#include <cstring>

int main() {
  SentinelTraceSample storage[3] = {};
  SentinelTraceBuffer buffer;
  sentinelTraceBufferInit(buffer, storage, 3);
  CHECK_EQ(buffer.capacity, 3u);

  for (uint32_t i = 0; i < 5; ++i) {
    SentinelTraceSample sample = {};
    sample.uptimeMs = i * 1000;
    sample.phase = SENTINEL_TRACE_TOF_ACTIVE;
    sample.batteryMa = (int16_t)(100 + i);
    CHECK_EQ(sentinelTraceBufferAppend(buffer, sample), i + 1);
  }
  CHECK_EQ(buffer.count, 3u);
  CHECK_EQ(buffer.overwrites, 2u);
  CHECK_EQ(sentinelTraceBufferOldestSeq(buffer), 3u);
  CHECK_EQ(sentinelTraceBufferNewestSeq(buffer), 5u);

  SentinelTraceSample out[3] = {};
  uint32_t count = sentinelTraceBufferCollectAfter(buffer, 3, out, 3);
  CHECK_EQ(count, 2u);
  CHECK_EQ(out[0].seq, 4u);
  CHECK_EQ(out[1].seq, 5u);
  CHECK_EQ(out[1].batteryMa, 104);
  CHECK(std::strcmp(sentinelTracePhaseName(SENTINEL_TRACE_BASELINE_A),
                    "baseline-a") == 0);

  return testReport("test_sentinel_trace");
}
