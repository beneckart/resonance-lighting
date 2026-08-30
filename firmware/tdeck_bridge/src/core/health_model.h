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

// Human-readable BQ25628E REG0x1E charge phases. These are charger truth, not
// an IBAT threshold guess; the separate sample-valid bit says whether the net
// battery-current number can be trusted.
enum class ChargeStatus : uint8_t {
  OFF_AIR = 0,
  CHARGING_CC,
  CHARGING_CV,
  TOP_OFF,
  NOT_CHARGING,
  CHARGE_DISABLED,
  FAULT,
  UNKNOWN,
};

enum class HealthRegistryStatus : uint8_t {
  COMMISSIONED = 1,
  COMMISSION_FAILED = 2,
  ENUMERATED = 3,
  QUARANTINED = 4,
};

// Physical-fleet placement is independent of commissioning state. SITE rows
// are expected in Health/RF. CAMP and REPAIR remain visible in Fleet without
// creating false off-air alerts at the installation.
enum class HealthRosterScope : uint8_t {
  SITE = 0,
  CAMP = 1,
  REPAIR = 2,
};

struct HealthRegistryEntry {
  uint8_t id[3];
  HealthRegistryStatus status;
  HealthRosterScope scope;
  uint16_t capacityMah;
  const char *callsign;
  const char *role;
};

struct HealthObservation {
  uint8_t id[3];
  uint32_t ageMs;
  int16_t battMv;
  int16_t battMa;
  bool battMaValid;
  int16_t supplyMv;
  int16_t supplyMa;
  bool supplyGood;
  bool hasBq;
  uint8_t bqReg16;
  uint8_t bqStat1;
  uint8_t bqFault0;
};

struct HealthTile {
  uint8_t id[3];
  BatteryHealthBand band;
  ChargeStatus chargeStatus;
  int16_t battMv;
  int16_t battMa;
  bool battMaValid;
  int16_t supplyMv;
  int16_t supplyMa;
  bool supplyGood;
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
ChargeStatus chargeStatus(bool onAir, bool hasBq, uint8_t bqReg16,
                          uint8_t bqStat1, uint8_t bqFault0);
const char *chargeStatusName(ChargeStatus status);

// SITE registry entries always occupy stable, registry-sorted positions. Any
// live observation outside the complete physical roster is appended in short-ID
// order; CAMP/REPAIR observations are known inventory and stay out of Health.
// Stale foreign observations are omitted so old bench visitors cannot crowd the
// field view.
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

size_t healthRegistryCountScope(const HealthRegistryEntry *registry,
                                size_t registryCount,
                                HealthRosterScope scope);

const char *healthRosterScopeName(HealthRosterScope scope);
