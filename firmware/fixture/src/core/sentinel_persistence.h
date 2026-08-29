// Platform-independent journal contract for the exact-target sentinel trace.
// The ESP32 glue owns flash I/O; this module pins the durable layouts, CRCs,
// and validation reasons so a reset can never silently restart a completed or
// interrupted field campaign.
#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr uint32_t SENTINEL_RUN_MAGIC = 0x5352554EUL; // "SRUN"
static constexpr uint32_t SENTINEL_TRACE_MAGIC = 0x53454E54UL; // "SENT"
static constexpr uint16_t SENTINEL_RUN_SCHEMA = 1;
static constexpr uint16_t SENTINEL_TRACE_SCHEMA = 2;

enum SentinelRunState : uint8_t {
  SENTINEL_RUN_STARTED = 1,
};

struct __attribute__((packed)) SentinelRunMarker {
  uint32_t magic;
  uint16_t schema;
  uint8_t state;
  uint8_t reserved;
  uint32_t artifactTag;
  uint32_t markerCrc32;
};

struct __attribute__((packed)) SentinelPersistHeader {
  uint32_t magic;
  uint16_t schema;
  uint16_t sampleSize;
  uint32_t artifactTag;
  uint32_t count;
  uint32_t overwrites;
  uint32_t newestSeq;
  uint32_t samplesCrc32;
  uint32_t headerCrc32;
};

static_assert(sizeof(SentinelRunMarker) == 16,
              "sentinel run-marker layout changed");
static_assert(sizeof(SentinelPersistHeader) == 32,
              "sentinel persistence header layout changed");

enum SentinelPersistVerdict : uint8_t {
  SENTINEL_PERSIST_VALID = 0,
  SENTINEL_PERSIST_BAD_MAGIC,
  SENTINEL_PERSIST_BAD_SCHEMA,
  SENTINEL_PERSIST_BAD_SAMPLE_SIZE,
  SENTINEL_PERSIST_BAD_ARTIFACT,
  SENTINEL_PERSIST_EMPTY,
  SENTINEL_PERSIST_CAPACITY,
  SENTINEL_PERSIST_OVERWRITTEN,
  SENTINEL_PERSIST_BAD_SEQUENCE,
  SENTINEL_PERSIST_BAD_HEADER_CRC,
  SENTINEL_PERSIST_BAD_SAMPLES_CRC,
};

uint32_t sentinelPersistCrc32Update(uint32_t crc, const void *data,
                                    size_t length);
uint32_t sentinelPersistCrc32(const void *data, size_t length);
uint32_t sentinelPersistArtifactTag(const char *revision);

void sentinelRunMarkerBuild(SentinelRunMarker &marker, uint32_t artifactTag);
bool sentinelRunMarkerValid(const SentinelRunMarker &marker,
                            uint32_t artifactTag);

void sentinelPersistHeaderBuild(SentinelPersistHeader &header,
                                uint32_t artifactTag, uint16_t sampleSize,
                                uint32_t count, uint32_t overwrites,
                                uint32_t newestSeq, uint32_t samplesCrc32);
SentinelPersistVerdict sentinelPersistHeaderValidate(
    const SentinelPersistHeader &header, uint32_t artifactTag,
    uint16_t sampleSize, uint32_t capacity, uint32_t samplesCrc32);
const char *sentinelPersistVerdictName(SentinelPersistVerdict verdict);
