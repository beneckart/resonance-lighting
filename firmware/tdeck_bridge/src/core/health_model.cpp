#include "health_model.h"

#include <string.h>

static int compareId(const uint8_t a[3], const uint8_t b[3]) {
  for (int i = 0; i < 3; ++i) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return 0;
}

BatteryHealthBand batteryHealthBand(bool onAir, int16_t battMv) {
  if (!onAir) return BatteryHealthBand::OFF_AIR;
  // A live heartbeat with no plausible 1S LFP voltage is different from a
  // radio-silent fixture. Keep it visible as UNKNOWN instead of a false red.
  if (battMv < 2000 || battMv > 5000) return BatteryHealthBand::UNKNOWN;
  if (battMv > HEALTH_GOOD_ABOVE_MV) return BatteryHealthBand::GOOD;
  if (battMv > HEALTH_NEAR_LOW_ABOVE_MV)
    return BatteryHealthBand::NEAR_LOW;
  return BatteryHealthBand::LOW_BATTERY;
}

ChargeStatus chargeStatus(bool onAir, bool hasBq, uint8_t bqReg16,
                          uint8_t bqStat1, uint8_t bqFault0) {
  if (!onAir) return ChargeStatus::OFF_AIR;
  if (!hasBq || bqReg16 == 0xFF || bqStat1 == 0xFF || bqFault0 == 0xFF)
    return ChargeStatus::UNKNOWN;
  if (bqFault0 != 0) return ChargeStatus::FAULT;
  if ((bqReg16 & (1u << 5)) == 0) return ChargeStatus::CHARGE_DISABLED;
  switch ((bqStat1 >> 3) & 0x03) {
    case 1: return ChargeStatus::CHARGING_CC;
    case 2: return ChargeStatus::CHARGING_CV;
    case 3: return ChargeStatus::TOP_OFF;
    default: return ChargeStatus::NOT_CHARGING;
  }
}

const char *chargeStatusName(ChargeStatus status) {
  switch (status) {
    case ChargeStatus::CHARGING_CC: return "CHARGING_CC";
    case ChargeStatus::CHARGING_CV: return "CHARGING_CV";
    case ChargeStatus::TOP_OFF: return "TOP-OFF";
    case ChargeStatus::NOT_CHARGING: return "DONE/OFF";
    case ChargeStatus::CHARGE_DISABLED: return "DONE/OFF";
    case ChargeStatus::FAULT: return "FAULT";
    case ChargeStatus::UNKNOWN: return "UNKNOWN";
    case ChargeStatus::OFF_AIR: return "OFF_AIR";
  }
  return "UNKNOWN";
}

const HealthRegistryEntry *healthRegistryFind(
    const HealthRegistryEntry *registry, size_t registryCount,
    const uint8_t id[3]) {
  for (size_t i = 0; i < registryCount; ++i) {
    if (memcmp(registry[i].id, id, 3) == 0) return &registry[i];
  }
  return nullptr;
}

static char asciiLower(char c) {
  return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

static bool callsignEquals(const char *a, const char *b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (asciiLower(*a++) != asciiLower(*b++)) return false;
  }
  return *a == 0 && *b == 0;
}

const HealthRegistryEntry *healthRegistryFindCallsign(
    const HealthRegistryEntry *registry, size_t registryCount,
    const char *callsign) {
  for (size_t i = 0; i < registryCount; ++i) {
    if (callsignEquals(registry[i].callsign, callsign)) return &registry[i];
  }
  return nullptr;
}

size_t healthRegistryCountScope(const HealthRegistryEntry *registry,
                                size_t registryCount,
                                HealthRosterScope scope) {
  size_t count = 0;
  for (size_t i = 0; registry && i < registryCount; ++i)
    if (registry[i].scope == scope) ++count;
  return count;
}

const char *healthRosterScopeName(HealthRosterScope scope) {
  switch (scope) {
    case HealthRosterScope::SITE: return "site";
    case HealthRosterScope::CAMP: return "camp";
    case HealthRosterScope::REPAIR: return "repair";
  }
  return "unknown";
}

static const HealthObservation *findObservation(
    const HealthObservation *observations, size_t count, const uint8_t id[3]) {
  for (size_t i = 0; i < count; ++i) {
    if (memcmp(observations[i].id, id, 3) == 0) return &observations[i];
  }
  return nullptr;
}

static void fillTile(HealthTile *tile, const uint8_t id[3],
                     const HealthRegistryEntry *registry,
                     const HealthObservation *observation, uint32_t freshMs) {
  memcpy(tile->id, id, 3);
  tile->registry = registry;
  tile->ageMs = observation ? observation->ageMs : UINT32_MAX;
  tile->battMv = observation ? observation->battMv : 0;
  tile->battMa = observation ? observation->battMa : 0;
  tile->battMaValid = observation && observation->battMaValid;
  tile->supplyMv = observation ? observation->supplyMv : 0;
  tile->supplyMa = observation ? observation->supplyMa : 0;
  tile->supplyGood = observation && observation->supplyGood;
  bool onAir = observation && observation->ageMs < freshMs;
  tile->band = batteryHealthBand(observation && observation->ageMs < freshMs,
                                tile->battMv);
  tile->chargeStatus = chargeStatus(
      onAir, observation && observation->hasBq,
      observation ? observation->bqReg16 : 0xFF,
      observation ? observation->bqStat1 : 0xFF,
      observation ? observation->bqFault0 : 0xFF);
}

size_t healthBuildTiles(const HealthRegistryEntry *registry,
                        size_t registryCount,
                        const HealthObservation *observations,
                        size_t observationCount, uint32_t freshMs,
                        HealthTile *out, size_t maxOut) {
  size_t n = 0;
  for (size_t i = 0; i < registryCount && n < maxOut; ++i) {
    if (registry[i].scope != HealthRosterScope::SITE) continue;
    const HealthObservation *obs =
        findObservation(observations, observationCount, registry[i].id);
    fillTile(&out[n++], registry[i].id, &registry[i], obs, freshMs);
  }

  // Append only currently live foreign IDs. Insertion keeps this small tail
  // deterministic without allocating a second full-fleet work buffer.
  const size_t tailStart = n;
  for (size_t i = 0; i < observationCount && n < maxOut; ++i) {
    const HealthObservation &obs = observations[i];
    if (obs.ageMs >= freshMs ||
        healthRegistryFind(registry, registryCount, obs.id))
      continue;
    fillTile(&out[n++], obs.id, nullptr, &obs, freshMs);
    for (size_t j = n - 1;
         j > tailStart && compareId(out[j].id, out[j - 1].id) < 0; --j) {
      HealthTile tmp = out[j];
      out[j] = out[j - 1];
      out[j - 1] = tmp;
    }
  }
  return n;
}

HealthSummary healthSummarize(const HealthTile *tiles, size_t count) {
  HealthSummary s = {};
  for (size_t i = 0; i < count; ++i) {
    switch (tiles[i].band) {
      case BatteryHealthBand::GOOD: ++s.good; break;
      case BatteryHealthBand::NEAR_LOW: ++s.nearLow; break;
      case BatteryHealthBand::LOW_BATTERY: ++s.low; break;
      case BatteryHealthBand::OFF_AIR: ++s.offAir; break;
      case BatteryHealthBand::UNKNOWN: ++s.unknown; break;
    }
    if (!tiles[i].registry && tiles[i].band != BatteryHealthBand::OFF_AIR)
      ++s.unregisteredLive;
  }
  return s;
}
