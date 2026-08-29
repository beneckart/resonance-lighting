#pragma once

#include <stddef.h>
#include <stdint.h>

#include "census.h"
#include "health_model.h"

// Pure model for the Fleet app's operator-selectable roster. Keeping this out
// of LVGL makes the stable-order, filtering, and absent-row contracts native
// testable.
enum class FleetRowScope : uint8_t {
  ROSTER_AND_LIVE = 0,
  SEEN_SINCE_BOOT,
  LIVE_NOW,
};

enum class FleetClassFilter : uint8_t {
  ALL = 0,
  DOWNLIGHT = 1,
  PERIMETER = 2,
  UPLIGHT = 3,
  CHANDELIER = 4,
  UNKNOWN = 5,
};

enum class FleetBatteryFilter : uint8_t {
  ALL = 0,
  GOOD,
  NEAR_LOW,
  LOW_BATTERY,
  OFF_AIR,
  UNKNOWN,
};

enum class FleetChargeFilter : uint8_t {
  ALL = 0,
  CHARGING_CC,
  CHARGING_CV,
  TOP_OFF,
  DONE_OFF,
  FAULT,
  UNKNOWN,
  OFF_AIR,
};

enum class FleetProgramFilter : uint8_t {
  ALL = 0,
  IDLE,
  CA,
  BRIDGE,
  DIRECT,
  DARK,
  VIRUS,
  UNKNOWN,
};

enum class FleetFirmwareFilter : uint8_t {
  ALL = 0,
  KNOWN,
  UNKNOWN,
  MATCH_REFERENCE,
  NOT_REFERENCE,
};

enum class FleetSortMode : uint8_t {
  CALLSIGN_STABLE = 0,
  SHORT_ID_STABLE,
  BATTERY_LOW_FIRST,
  BATTERY_HIGH_FIRST,
  RECENT_FIRST,
  SIGNAL_STRONG_FIRST,
};

struct FleetViewSettings {
  FleetRowScope scope;
  FleetClassFilter classFilter;
  FleetBatteryFilter batteryFilter;
  FleetChargeFilter chargeFilter;
  FleetProgramFilter programFilter;
  FleetFirmwareFilter firmwareFilter;
  char firmwareReference[24];
  FleetSortMode sort;
};

struct FleetViewRow {
  CensusView view;
  const HealthRegistryEntry *registry;
  BatteryHealthBand batteryBand;
  ChargeStatus chargeStatus;
  uint8_t fixtureClass;
  bool observed;
  bool fresh;
};

FleetViewSettings fleetViewDefaults();

// Live class telemetry is authoritative. A registry role supplies the class
// only while a fixture is absent or has not yet emitted a full heartbeat.
uint8_t fleetClassFromRole(const char *role);

// ROSTER_AND_LIVE keeps every registry fixture in a stable row and appends
// fresh unregistered peers. SEEN and LIVE are census-only scopes.
size_t fleetBuildView(const HealthRegistryEntry *registry,
                      size_t registryCount, const CensusView *observations,
                      size_t observationCount, uint32_t freshMs,
                      const FleetViewSettings &settings, FleetViewRow *out,
                      size_t maxOut);

int fleetFindRowById(const FleetViewRow *rows, size_t count,
                     const uint8_t id[3]);
