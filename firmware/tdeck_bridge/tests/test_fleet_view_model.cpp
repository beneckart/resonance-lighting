#include <cassert>
#include <cstdio>
#include <cstring>

#include "core/fleet_view_model.h"

static HealthRegistryEntry entry(uint32_t id, const char *callsign,
                                 const char *role,
                                 HealthRosterScope scope =
                                     HealthRosterScope::SITE) {
  HealthRegistryEntry e = {};
  e.id[0] = (uint8_t)(id >> 16);
  e.id[1] = (uint8_t)(id >> 8);
  e.id[2] = (uint8_t)id;
  e.status = HealthRegistryStatus::COMMISSIONED;
  e.scope = scope;
  e.callsign = callsign;
  e.role = role;
  return e;
}

static CensusView observation(uint32_t id, uint32_t ageMs, int16_t battMv,
                              int8_t rssi, uint8_t fixtureClass = 0) {
  CensusView v = {};
  v.id[0] = (uint8_t)(id >> 16);
  v.id[1] = (uint8_t)(id >> 8);
  v.id[2] = (uint8_t)id;
  v.ageMs = ageMs;
  v.battMv = battMv;
  v.rssiEwma = rssi;
  v.fixtureClass = fixtureClass;
  v.soc = 70;
  return v;
}

int main() {
  HealthRegistryEntry registry[] = {
      entry(0x100001, "Zelda", "downlight"),
      entry(0x100002, "Abra", "perimeter"),
      entry(0x100003, "Mario", "uplight", HealthRosterScope::CAMP),
      entry(0x100004, "Epona", "chandelier_tester",
            HealthRosterScope::REPAIR),
  };
  CensusView seen[] = {
      observation(0x100001, 100, 3300, -60, 1),
      observation(0x100002, 100, 3150, -40, 2),
      observation(0x100003, 6000, 3050, -70, 3),  // seen, now off-air
      observation(0x200001, 200, 3000, -50, 4),   // fresh foreign
      observation(0x200002, 7000, 3400, -30, 1),  // stale foreign
  };
  seen[0].hasFixtureState = true;
  seen[0].activeProgram = 0;
  seen[0].hasBq = true;
  seen[0].bqReg16 = 0x20;
  seen[0].bqStat1 = 0x08;  // CHARGING_CC
  seen[0].fwFingerprint = censusFirmwareFingerprint("fx-260826-new-p");
  seen[1].hasFixtureState = true;
  seen[1].activeProgram = 3;
  seen[1].hasBq = true;
  seen[1].bqReg16 = 0x20;
  seen[1].bqStat1 = 0x10;  // CHARGING_CV
  seen[1].fwFingerprint = censusFirmwareFingerprint("fx-260819-old-p");
  seen[2].hasFixtureState = true;
  seen[2].activeProgram = 1;
  seen[2].fwFingerprint = censusFirmwareFingerprint("fx-260826-new-p");
  seen[4].hasFixtureState = true;
  seen[4].activeProgram = 4;
  seen[4].fwFingerprint = censusFirmwareFingerprint("fx-260819-old-p");
  FleetViewRow rows[12] = {};
  FleetViewSettings settings = fleetViewDefaults();

  // Default is a stable callsign roster: absent Epona keeps its place, and a
  // fresh foreign fixture follows the named roster.
  size_t n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 5);
  assert(std::strcmp(rows[0].registry->callsign, "Abra") == 0);
  assert(std::strcmp(rows[1].registry->callsign, "Epona") == 0);
  assert(std::strcmp(rows[2].registry->callsign, "Mario") == 0);
  assert(std::strcmp(rows[3].registry->callsign, "Zelda") == 0);
  assert(rows[4].registry == nullptr && rows[4].view.id[0] == 0x20);
  assert(!rows[1].observed && !rows[1].fresh);
  assert(rows[1].fixtureClass == 4);  // inferred from registry role
  assert(rows[1].batteryBand == BatteryHealthBand::OFF_AIR);
  assert(rows[2].batteryBand == BatteryHealthBand::OFF_AIR);

  // Placement filtering separates expected site peers from camp and repair
  // inventory. A placement filter also excludes foreign live observations.
  settings.rosterFilter = FleetRosterFilter::SITE;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2);
  settings.rosterFilter = FleetRosterFilter::CAMP;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].registry->scope == HealthRosterScope::CAMP);
  settings.rosterFilter = FleetRosterFilter::REPAIR;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].registry->scope == HealthRosterScope::REPAIR);

  // A class filter works for live telemetry and absent registry-role fallbacks.
  settings = fleetViewDefaults();
  settings.classFilter = FleetClassFilter::CHANDELIER;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2);
  assert(rows[0].registry && rows[0].view.id[2] == 0x04);
  assert(!rows[0].fresh);
  assert(!rows[1].registry && rows[1].view.id[0] == 0x20);

  // Battery state uses raw voltage bands and distinguishes off-air from low.
  settings = fleetViewDefaults();
  settings.batteryFilter = FleetBatteryFilter::NEAR_LOW;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].view.id[2] == 0x02);
  settings.batteryFilter = FleetBatteryFilter::OFF_AIR;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2 && !rows[0].fresh && !rows[1].fresh);

  // Charger-phase filters use BQ25628E status truth and keep radio silence
  // distinct from a live charger that is done or off.
  settings = fleetViewDefaults();
  settings.chargeFilter = FleetChargeFilter::CHARGING_CC;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].view.id[2] == 0x01);
  settings.chargeFilter = FleetChargeFilter::CHARGING_CV;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].view.id[2] == 0x02);
  settings.chargeFilter = FleetChargeFilter::OFF_AIR;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2 && !rows[0].fresh && !rows[1].fresh);

  CensusView phases[] = {
      observation(0x300001, 100, 3300, -40),
      observation(0x300002, 100, 3300, -40),
      observation(0x300003, 100, 3300, -40),
      observation(0x300004, 100, 3300, -40),
      observation(0x300005, 100, 3300, -40),
  };
  for (CensusView &phase : phases) {
    phase.hasBq = true;
    phase.bqReg16 = 0x20;
  }
  phases[0].bqStat1 = 0x08;
  phases[1].bqStat1 = 0x10;
  phases[2].bqStat1 = 0x18;
  phases[3].bqStat1 = 0x00;
  phases[4].bqFault0 = 0x01;
  const FleetChargeFilter phaseFilters[] = {
      FleetChargeFilter::CHARGING_CC, FleetChargeFilter::CHARGING_CV,
      FleetChargeFilter::TOP_OFF, FleetChargeFilter::DONE_OFF,
      FleetChargeFilter::FAULT,
  };
  for (size_t i = 0; i < 5; ++i) {
    settings = fleetViewDefaults();
    settings.chargeFilter = phaseFilters[i];
    n = fleetBuildView(nullptr, 0, phases, 5, 5000, settings, rows, 12);
    assert(n == 1 && rows[0].view.id[2] == i + 1);
  }

  // Program zero is explicitly IDLE only when a full-heartbeat sample exists;
  // absent full state belongs to the distinct unknown cohort.
  settings = fleetViewDefaults();
  settings.programFilter = FleetProgramFilter::IDLE;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].view.id[2] == 0x01);
  settings.programFilter = FleetProgramFilter::DIRECT;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 1 && rows[0].view.id[2] == 0x02);
  settings.programFilter = FleetProgramFilter::UNKNOWN;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2);  // absent Epona plus the fresh foreign peer

  // OTA audit filters compare the exact selected artifact. Unknown revision
  // evidence is intentionally included in the not-reference cohort.
  settings = fleetViewDefaults();
  settings.firmwareFilter = FleetFirmwareFilter::MATCH_REFERENCE;
  std::strcpy(settings.firmwareReference, "fx-260826-new-p");
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 2);
  settings.firmwareFilter = FleetFirmwareFilter::NOT_REFERENCE;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 3);  // old Abra, absent Epona, unknown fresh foreign peer

  // Seen/live scopes do not synthesize absent rows. The stale foreign peer is
  // retained only in Seen; it is omitted by Roster and Live.
  settings = fleetViewDefaults();
  settings.scope = FleetRowScope::SEEN_SINCE_BOOT;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 5);
  settings.scope = FleetRowScope::LIVE_NOW;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 3);
  for (size_t i = 0; i < n; ++i) assert(rows[i].fresh);

  // Voltage sorts keep invalid/off-air rows after real live readings and use
  // stable identity ties. This is intentionally opt-in; the default never
  // shuffles when heartbeat age or RSSI changes.
  settings = fleetViewDefaults();
  settings.sort = FleetSortMode::BATTERY_LOW_FIRST;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(n == 5);
  assert(rows[0].view.battMv == 3000);
  assert(rows[1].view.battMv == 3150);
  assert(rows[2].view.battMv == 3300);
  assert(rows[3].batteryBand == BatteryHealthBand::OFF_AIR);
  assert(rows[4].batteryBand == BatteryHealthBand::OFF_AIR);
  settings.sort = FleetSortMode::BATTERY_HIGH_FIRST;
  n = fleetBuildView(registry, 4, seen, 5, 5000, settings, rows, 12);
  assert(rows[0].view.battMv == 3300 && rows[2].view.battMv == 3000);

  const uint8_t zelda[3] = {0x10, 0x00, 0x01};
  assert(fleetFindRowById(rows, n, zelda) == 0);
  assert(fleetClassFromRole("Chandelier_tester") == 4);
  assert(fleetClassFromRole("magic_wand") == 0);

  std::puts("fleet_view_model ok");
  return 0;
}
