#include "sentinel_persistence.h"

#include <string.h>

uint32_t sentinelPersistCrc32Update(uint32_t crc, const void *data,
                                    size_t length) {
  const uint8_t *bytes = static_cast<const uint8_t *>(data);
  while (length--) {
    crc ^= *bytes++;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320UL & (0U - (crc & 1U)));
  }
  return crc;
}

uint32_t sentinelPersistCrc32(const void *data, size_t length) {
  return sentinelPersistCrc32Update(0xFFFFFFFFUL, data, length) ^
         0xFFFFFFFFUL;
}

uint32_t sentinelPersistArtifactTag(const char *revision) {
  return revision ? sentinelPersistCrc32(revision, strlen(revision)) : 0;
}

void sentinelRunMarkerBuild(SentinelRunMarker &marker, uint32_t artifactTag) {
  memset(&marker, 0, sizeof(marker));
  marker.magic = SENTINEL_RUN_MAGIC;
  marker.schema = SENTINEL_RUN_SCHEMA;
  marker.state = SENTINEL_RUN_STARTED;
  marker.artifactTag = artifactTag;
  marker.markerCrc32 =
      sentinelPersistCrc32(&marker, offsetof(SentinelRunMarker, markerCrc32));
}

bool sentinelRunMarkerValid(const SentinelRunMarker &marker,
                            uint32_t artifactTag) {
  return marker.magic == SENTINEL_RUN_MAGIC &&
         marker.schema == SENTINEL_RUN_SCHEMA &&
         marker.state == SENTINEL_RUN_STARTED &&
         marker.artifactTag == artifactTag &&
         marker.markerCrc32 ==
             sentinelPersistCrc32(&marker,
                                  offsetof(SentinelRunMarker, markerCrc32));
}

void sentinelPersistHeaderBuild(SentinelPersistHeader &header,
                                uint32_t artifactTag, uint16_t sampleSize,
                                uint32_t count, uint32_t overwrites,
                                uint32_t newestSeq, uint32_t samplesCrc32) {
  memset(&header, 0, sizeof(header));
  header.magic = SENTINEL_TRACE_MAGIC;
  header.schema = SENTINEL_TRACE_SCHEMA;
  header.sampleSize = sampleSize;
  header.artifactTag = artifactTag;
  header.count = count;
  header.overwrites = overwrites;
  header.newestSeq = newestSeq;
  header.samplesCrc32 = samplesCrc32;
  header.headerCrc32 = sentinelPersistCrc32(
      &header, offsetof(SentinelPersistHeader, headerCrc32));
}

SentinelPersistVerdict sentinelPersistHeaderValidate(
    const SentinelPersistHeader &header, uint32_t artifactTag,
    uint16_t sampleSize, uint32_t capacity, uint32_t samplesCrc32) {
  if (header.magic != SENTINEL_TRACE_MAGIC)
    return SENTINEL_PERSIST_BAD_MAGIC;
  if (header.schema != SENTINEL_TRACE_SCHEMA)
    return SENTINEL_PERSIST_BAD_SCHEMA;
  if (header.sampleSize != sampleSize)
    return SENTINEL_PERSIST_BAD_SAMPLE_SIZE;
  if (header.artifactTag != artifactTag)
    return SENTINEL_PERSIST_BAD_ARTIFACT;
  if (!header.count) return SENTINEL_PERSIST_EMPTY;
  if (header.count > capacity) return SENTINEL_PERSIST_CAPACITY;
  if (header.overwrites != 0) return SENTINEL_PERSIST_OVERWRITTEN;
  if (header.newestSeq != header.count)
    return SENTINEL_PERSIST_BAD_SEQUENCE;
  if (header.headerCrc32 != sentinelPersistCrc32(
                                &header,
                                offsetof(SentinelPersistHeader, headerCrc32)))
    return SENTINEL_PERSIST_BAD_HEADER_CRC;
  if (header.samplesCrc32 != samplesCrc32)
    return SENTINEL_PERSIST_BAD_SAMPLES_CRC;
  return SENTINEL_PERSIST_VALID;
}

const char *sentinelPersistVerdictName(SentinelPersistVerdict verdict) {
  switch (verdict) {
  case SENTINEL_PERSIST_VALID: return "valid";
  case SENTINEL_PERSIST_BAD_MAGIC: return "bad-magic";
  case SENTINEL_PERSIST_BAD_SCHEMA: return "bad-schema";
  case SENTINEL_PERSIST_BAD_SAMPLE_SIZE: return "bad-sample-size";
  case SENTINEL_PERSIST_BAD_ARTIFACT: return "bad-artifact";
  case SENTINEL_PERSIST_EMPTY: return "empty";
  case SENTINEL_PERSIST_CAPACITY: return "capacity";
  case SENTINEL_PERSIST_OVERWRITTEN: return "overwritten";
  case SENTINEL_PERSIST_BAD_SEQUENCE: return "bad-sequence";
  case SENTINEL_PERSIST_BAD_HEADER_CRC: return "bad-header-crc";
  case SENTINEL_PERSIST_BAD_SAMPLES_CRC: return "bad-samples-crc";
  default: return "unknown";
  }
}
