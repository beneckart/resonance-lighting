#pragma once

#include <stddef.h>
#include <stdint.h>

// Operator-facing battery triage bands. These deliberately use raw reported
// LiFePO4 voltage rather than the MAX17260 SOC estimate, which can be poorly
// learned in field fixtures. They are dashboard bands, not lifecycle cutoffs.
static constexpr int16_t HEALTH_GOOD_ABOVE_MV = 3200;
static constexpr int16_t HEALTH_NEAR_LOW_ABOVE_MV = 3100;

enum class BatteryHealthBand : uint8_t {
  OFF_AIR = 0,
  GOOD,
  NEAR_LOW,
  LOW_BATTERY,
  UNKNOWN,
};

enum class HealthRegistryStatus : uint8_t {
  COMMISSIONED = 1,
  COMMISSION_FAILED = 2,
};

struct HealthRegistryEntry {
  uint8_t id[3];
  HealthRegistryStatus status;
  uint16_t capacityMah;
  const char *callsign;
  const char *role;
};

struct HealthObservation {
  uint8_t id[3];
  uint32_t ageMs;
  int16_t battMv;
};

struct HealthTile {
  uint8_t id[3];
  BatteryHealthBand band;
  int16_t battMv;
  uint32_t ageMs;
  const HealthRegistryEntry *registry;  // null for an unexpected live ID
};

struct HealthSummary {
  uint16_t good;
  uint16_t nearLow;
  uint16_t low;
  uint16_t offAir;
  uint16_t unknown;
  uint16_t unregisteredLive;
};

BatteryHealthBand batteryHealthBand(bool onAir, int16_t battMv);

// Registry entries always occupy stable, registry-sorted positions. Any live
// observation not in the registry is appended in short-ID order; stale foreign
// observations are omitted so old bench visitors cannot crowd the field view.
size_t healthBuildTiles(const HealthRegistryEntry *registry,
                        size_t registryCount,
                        const HealthObservation *observations,
                        size_t observationCount, uint32_t freshMs,
                        HealthTile *out, size_t maxOut);

HealthSummary healthSummarize(const HealthTile *tiles, size_t count);

const HealthRegistryEntry *healthRegistryFind(
    const HealthRegistryEntry *registry, size_t registryCount,
    const uint8_t id[3]);

const HealthRegistryEntry *healthRegistryFindCallsign(
    const HealthRegistryEntry *registry, size_t registryCount,
    const char *callsign);
