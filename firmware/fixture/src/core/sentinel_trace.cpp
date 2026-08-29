#include "sentinel_trace.h"

#include <string.h>

void sentinelTraceBufferInit(SentinelTraceBuffer &buffer,
                             SentinelTraceSample *storage,
                             uint32_t capacity) {
  memset(&buffer, 0, sizeof(buffer));
  buffer.samples = storage;
  buffer.capacity = storage ? capacity : 0;
  buffer.nextSeq = 1;
}

uint32_t sentinelTraceBufferAppend(SentinelTraceBuffer &buffer,
                                   const SentinelTraceSample &sample) {
  if (!buffer.samples || !buffer.capacity) return 0;
  SentinelTraceSample value = sample;
  value.seq = buffer.nextSeq++;
  buffer.samples[buffer.writeIndex] = value;
  buffer.writeIndex = (buffer.writeIndex + 1U) % buffer.capacity;
  if (buffer.count < buffer.capacity)
    ++buffer.count;
  else
    ++buffer.overwrites;
  return value.seq;
}

static uint32_t oldestIndex(const SentinelTraceBuffer &buffer) {
  return buffer.count < buffer.capacity ? 0U : buffer.writeIndex;
}

uint32_t sentinelTraceBufferOldestSeq(const SentinelTraceBuffer &buffer) {
  if (!buffer.samples || !buffer.count) return 0;
  return buffer.samples[oldestIndex(buffer)].seq;
}

uint32_t sentinelTraceBufferNewestSeq(const SentinelTraceBuffer &buffer) {
  if (!buffer.samples || !buffer.count) return 0;
  uint32_t index = buffer.writeIndex ? buffer.writeIndex - 1U
                                     : buffer.capacity - 1U;
  return buffer.samples[index].seq;
}

uint32_t sentinelTraceBufferCollectAfter(const SentinelTraceBuffer &buffer,
                                         uint32_t afterSeq,
                                         SentinelTraceSample *out,
                                         uint32_t maxOut) {
  if (!buffer.samples || !buffer.count || !out || !maxOut) return 0;
  uint32_t copied = 0;
  uint32_t index = oldestIndex(buffer);
  for (uint32_t i = 0; i < buffer.count && copied < maxOut; ++i) {
    const SentinelTraceSample &sample = buffer.samples[index];
    if (sample.seq > afterSeq) out[copied++] = sample;
    index = (index + 1U) % buffer.capacity;
  }
  return copied;
}

const char *sentinelTracePhaseName(uint8_t phase) {
  switch (phase) {
  case SENTINEL_TRACE_RADIO_SETTLE: return "radio-settle";
  case SENTINEL_TRACE_BASELINE_A: return "baseline-a";
  case SENTINEL_TRACE_TOF_WARMUP: return "tof-warmup";
  case SENTINEL_TRACE_TOF_ACTIVE: return "tof-active";
  case SENTINEL_TRACE_BASELINE_B: return "baseline-b";
  case SENTINEL_TRACE_RETRIEVAL: return "retrieval";
  case SENTINEL_TRACE_ERROR: return "error";
  default: return "disabled";
  }
}
