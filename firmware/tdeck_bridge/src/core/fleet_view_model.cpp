#include "fleet_view_model.h"

#include <string.h>

namespace {

static int compareId(const uint8_t a[3], const uint8_t b[3]) {
  for (size_t i = 0; i < 3; ++i) {
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  }
  return 0;
}

static char asciiLower(char c) {
  return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

static int foldedCompare(const char *a, const char *b) {
  if (!a) a = "";
  if (!b) b = "";
  while (*a && *b) {
    char ca = asciiLower(*a++);
    char cb = asciiLower(*b++);
    if (ca != cb) return ca < cb ? -1 : 1;
  }
  if (*a == *b) return 0;
  return *a ? 1 : -1;
}

static bool foldedEquals(const char *a, const char *b) {
  return foldedCompare(a, b) == 0;
}

static bool foldedStartsWith(const char *text, const char *prefix) {
  if (!text || !prefix) return false;
  while (*prefix) {
    if (!*text || asciiLower(*text++) != asciiLower(*prefix++)) return false;
  }
  return true;
}

static const CensusView *findObservation(const CensusView *observations,
                                         size_t count,
                                         const uint8_t id[3]) {
  for (size_t i = 0; i < count; ++i) {
    if (memcmp(observations[i].id, id, 3) == 0) return &observations[i];
  }
  return nullptr;
}

static bool validBattery(const FleetViewRow &row) {
  return row.batteryBand == BatteryHealthBand::GOOD ||
         row.batteryBand == BatteryHealthBand::NEAR_LOW ||
         row.batteryBand == BatteryHealthBand::LOW_BATTERY;
}

static FleetBatteryFilter filterForBand(BatteryHealthBand band) {
  switch (band) {
    case BatteryHealthBand::GOOD: return FleetBatteryFilter::GOOD;
    case BatteryHealthBand::NEAR_LOW: return FleetBatteryFilter::NEAR_LOW;
    case BatteryHealthBand::LOW_BATTERY:
      return FleetBatteryFilter::LOW_BATTERY;
    case BatteryHealthBand::OFF_AIR: return FleetBatteryFilter::OFF_AIR;
    case BatteryHealthBand::UNKNOWN: return FleetBatteryFilter::UNKNOWN;
  }
  return FleetBatteryFilter::UNKNOWN;
}

static FleetChargeFilter filterForCharge(ChargeStatus status) {
  switch (status) {
    case ChargeStatus::CHARGING_CC: return FleetChargeFilter::CHARGING_CC;
    case ChargeStatus::CHARGING_CV: return FleetChargeFilter::CHARGING_CV;
    case ChargeStatus::TOP_OFF: return FleetChargeFilter::TOP_OFF;
    case ChargeStatus::NOT_CHARGING:
    case ChargeStatus::CHARGE_DISABLED:
      return FleetChargeFilter::DONE_OFF;
    case ChargeStatus::FAULT: return FleetChargeFilter::FAULT;
    case ChargeStatus::UNKNOWN: return FleetChargeFilter::UNKNOWN;
    case ChargeStatus::OFF_AIR: return FleetChargeFilter::OFF_AIR;
  }
  return FleetChargeFilter::UNKNOWN;
}

static bool rowMatches(const FleetViewRow &row,
                       const FleetViewSettings &settings) {
  if (settings.scope == FleetRowScope::SEEN_SINCE_BOOT && !row.observed)
    return false;
  if (settings.scope == FleetRowScope::LIVE_NOW && !row.fresh) return false;

  if (settings.rosterFilter != FleetRosterFilter::ALL) {
    if (!row.registry) return false;
    HealthRosterScope wanted = HealthRosterScope::SITE;
    if (settings.rosterFilter == FleetRosterFilter::CAMP)
      wanted = HealthRosterScope::CAMP;
    else if (settings.rosterFilter == FleetRosterFilter::REPAIR)
      wanted = HealthRosterScope::REPAIR;
    if (row.registry->scope != wanted) return false;
  }

  if (settings.classFilter != FleetClassFilter::ALL) {
    uint8_t wanted = settings.classFilter == FleetClassFilter::UNKNOWN
                         ? 0
                         : (uint8_t)settings.classFilter;
    if (row.fixtureClass != wanted) return false;
  }

  if (settings.batteryFilter != FleetBatteryFilter::ALL &&
      settings.batteryFilter != filterForBand(row.batteryBand))
    return false;

  if (settings.chargeFilter != FleetChargeFilter::ALL &&
      settings.chargeFilter != filterForCharge(row.chargeStatus))
    return false;

  if (settings.programFilter != FleetProgramFilter::ALL) {
    if (settings.programFilter == FleetProgramFilter::UNKNOWN) {
      if (row.view.hasFixtureState && row.view.activeProgram <= 5) return false;
    } else {
      uint8_t wanted = (uint8_t)settings.programFilter - 1;
      if (!row.view.hasFixtureState || row.view.activeProgram != wanted)
        return false;
    }
  }

  switch (settings.firmwareFilter) {
    case FleetFirmwareFilter::ALL:
      return true;
    case FleetFirmwareFilter::KNOWN:
      return row.view.fwFingerprint != 0;
    case FleetFirmwareFilter::UNKNOWN:
      return row.view.fwFingerprint == 0;
    case FleetFirmwareFilter::MATCH_REFERENCE:
      return row.view.fwFingerprint && settings.firmwareReference[0] &&
             row.view.fwFingerprint ==
                 censusFirmwareFingerprint(settings.firmwareReference);
    case FleetFirmwareFilter::NOT_REFERENCE:
      // For rollout auditing, no revision evidence belongs in the needs-
      // attention cohort alongside an explicit non-match.
      return settings.firmwareReference[0] &&
             (!row.view.fwFingerprint ||
              row.view.fwFingerprint !=
                  censusFirmwareFingerprint(settings.firmwareReference));
  }
  return false;
}

static int stableIdentityCompare(const FleetViewRow &a,
                                 const FleetViewRow &b) {
  const char *aName = a.registry ? a.registry->callsign : nullptr;
  const char *bName = b.registry ? b.registry->callsign : nullptr;
  bool aKnown = aName && aName[0];
  bool bKnown = bName && bName[0];
  if (aKnown != bKnown) return aKnown ? -1 : 1;
  if (aKnown) {
    int byName = foldedCompare(aName, bName);
    if (byName != 0) return byName;
  }
  return compareId(a.view.id, b.view.id);
}

static int batteryRank(const FleetViewRow &row) {
  if (validBattery(row)) return 0;
  if (row.batteryBand == BatteryHealthBand::UNKNOWN) return 1;
  return 2;  // off-air rows always trail actual live measurements
}

static bool comesBefore(const FleetViewRow &a, const FleetViewRow &b,
                        FleetSortMode sort) {
  if (sort == FleetSortMode::SHORT_ID_STABLE)
    return compareId(a.view.id, b.view.id) < 0;

  if (sort == FleetSortMode::BATTERY_LOW_FIRST ||
      sort == FleetSortMode::BATTERY_HIGH_FIRST) {
    int aRank = batteryRank(a);
    int bRank = batteryRank(b);
    if (aRank != bRank) return aRank < bRank;
    if (aRank == 0 && a.view.battMv != b.view.battMv) {
      if (sort == FleetSortMode::BATTERY_LOW_FIRST)
        return a.view.battMv < b.view.battMv;
      return a.view.battMv > b.view.battMv;
    }
    return stableIdentityCompare(a, b) < 0;
  }

  if (sort == FleetSortMode::RECENT_FIRST) {
    if (a.view.ageMs != b.view.ageMs) return a.view.ageMs < b.view.ageMs;
    return stableIdentityCompare(a, b) < 0;
  }

  if (sort == FleetSortMode::SIGNAL_STRONG_FIRST) {
    if (a.observed != b.observed) return a.observed;
    if (a.observed && a.view.rssiEwma != b.view.rssiEwma)
      return a.view.rssiEwma > b.view.rssiEwma;
    return stableIdentityCompare(a, b) < 0;
  }

  return stableIdentityCompare(a, b) < 0;
}

static FleetViewRow makeRow(const HealthRegistryEntry *registry,
                            const CensusView *observation,
                            uint32_t freshMs) {
  FleetViewRow row = {};
  row.registry = registry;
  row.observed = observation != nullptr;
  if (observation) {
    row.view = *observation;
  } else if (registry) {
    memcpy(row.view.id, registry->id, 3);
    row.view.ageMs = UINT32_MAX;
    row.view.soc = 255;
  }
  row.fresh = row.observed && row.view.ageMs < freshMs;
  row.batteryBand = batteryHealthBand(row.fresh, row.view.battMv);
  row.chargeStatus = chargeStatus(row.fresh, row.view.hasBq,
                                  row.view.bqReg16, row.view.bqStat1,
                                  row.view.bqFault0);
  row.fixtureClass = row.view.fixtureClass;
  if (row.fixtureClass == 0 && registry)
    row.fixtureClass = fleetClassFromRole(registry->role);
  return row;
}

static void insertSorted(FleetViewRow *rows, size_t count,
                         FleetSortMode sort) {
  FleetViewRow moving = rows[count - 1];
  size_t at = count - 1;
  while (at > 0 && comesBefore(moving, rows[at - 1], sort)) {
    rows[at] = rows[at - 1];
    --at;
  }
  rows[at] = moving;
}

}  // namespace

FleetViewSettings fleetViewDefaults() {
  FleetViewSettings settings = {};
  settings.scope = FleetRowScope::ROSTER_AND_LIVE;
  settings.rosterFilter = FleetRosterFilter::ALL;
  settings.classFilter = FleetClassFilter::ALL;
  settings.batteryFilter = FleetBatteryFilter::ALL;
  settings.chargeFilter = FleetChargeFilter::ALL;
  settings.programFilter = FleetProgramFilter::ALL;
  settings.firmwareFilter = FleetFirmwareFilter::ALL;
  settings.sort = FleetSortMode::CALLSIGN_STABLE;
  return settings;
}

uint8_t fleetClassFromRole(const char *role) {
  if (!role || !role[0]) return 0;
  if (foldedEquals(role, "downlight")) return 1;
  if (foldedEquals(role, "perimeter")) return 2;
  if (foldedEquals(role, "uplight") || foldedEquals(role, "trunk")) return 3;
  if (foldedStartsWith(role, "chandelier")) return 4;
  return 0;
}

size_t fleetBuildView(const HealthRegistryEntry *registry,
                      size_t registryCount, const CensusView *observations,
                      size_t observationCount, uint32_t freshMs,
                      const FleetViewSettings &settings, FleetViewRow *out,
                      size_t maxOut) {
  if (!out || maxOut == 0) return 0;
  size_t count = 0;

  if (settings.scope == FleetRowScope::ROSTER_AND_LIVE) {
    for (size_t i = 0; i < registryCount && count < maxOut; ++i) {
      FleetViewRow row = makeRow(
          &registry[i], findObservation(observations, observationCount,
                                        registry[i].id),
          freshMs);
      if (!rowMatches(row, settings)) continue;
      out[count++] = row;
      insertSorted(out, count, settings.sort);
    }
  }

  for (size_t i = 0; i < observationCount && count < maxOut; ++i) {
    const CensusView &observation = observations[i];
    const HealthRegistryEntry *entry =
        healthRegistryFind(registry, registryCount, observation.id);
    if (settings.scope == FleetRowScope::ROSTER_AND_LIVE) {
      if (entry || observation.ageMs >= freshMs) continue;
    }
    FleetViewRow row = makeRow(entry, &observation, freshMs);
    if (!rowMatches(row, settings)) continue;
    out[count++] = row;
    insertSorted(out, count, settings.sort);
  }
  return count;
}

int fleetFindRowById(const FleetViewRow *rows, size_t count,
                     const uint8_t id[3]) {
  if (!rows || !id) return -1;
  for (size_t i = 0; i < count; ++i) {
    if (memcmp(rows[i].view.id, id, 3) == 0) return (int)i;
  }
  return -1;
}
