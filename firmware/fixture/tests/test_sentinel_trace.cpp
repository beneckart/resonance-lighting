#include "test_util.h"

#include "../src/core/sentinel_trace.h"
#include "../src/core/sentinel_persistence.h"

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

  const uint32_t artifact = sentinelPersistArtifactTag("fx-test-t");
  CHECK(artifact != 0);
  SentinelRunMarker marker = {};
  sentinelRunMarkerBuild(marker, artifact);
  CHECK(sentinelRunMarkerValid(marker, artifact));
  CHECK(!sentinelRunMarkerValid(marker, artifact + 1));
  marker.markerCrc32 ^= 1;
  CHECK(!sentinelRunMarkerValid(marker, artifact));

  const char payload[] = "durable sentinel samples";
  const uint32_t payloadCrc = sentinelPersistCrc32(payload, sizeof(payload));
  SentinelPersistHeader header = {};
  sentinelPersistHeaderBuild(header, artifact, sizeof(SentinelTraceSample),
                             1830, 0, 1830, payloadCrc);
  CHECK_EQ(sentinelPersistHeaderValidate(
               header, artifact, sizeof(SentinelTraceSample), 4096, payloadCrc),
           SENTINEL_PERSIST_VALID);
  CHECK_EQ(sentinelPersistHeaderValidate(
               header, artifact, sizeof(SentinelTraceSample), 1024, payloadCrc),
           SENTINEL_PERSIST_CAPACITY);
  CHECK_EQ(sentinelPersistHeaderValidate(
               header, artifact, sizeof(SentinelTraceSample), 4096,
               payloadCrc ^ 1),
           SENTINEL_PERSIST_BAD_SAMPLES_CRC);
  header.newestSeq++;
  header.headerCrc32 = sentinelPersistCrc32(
      &header, offsetof(SentinelPersistHeader, headerCrc32));
  CHECK_EQ(sentinelPersistHeaderValidate(
               header, artifact, sizeof(SentinelTraceSample), 4096, payloadCrc),
           SENTINEL_PERSIST_BAD_SEQUENCE);

  return testReport("test_sentinel_trace");
}
