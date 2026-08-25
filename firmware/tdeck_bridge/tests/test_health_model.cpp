#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>

#include "core/fleet_registry_generated.h"
#include "core/health_model.h"

static HealthRegistryEntry entry(uint32_t id) {
  HealthRegistryEntry e = {};
  e.id[0] = (uint8_t)(id >> 16);
  e.id[1] = (uint8_t)(id >> 8);
  e.id[2] = (uint8_t)id;
  e.status = HealthRegistryStatus::COMMISSIONED;
  return e;
}

static HealthObservation observation(uint32_t id, uint32_t ageMs,
                                     int16_t battMv) {
  HealthObservation o = {};
  o.id[0] = (uint8_t)(id >> 16);
  o.id[1] = (uint8_t)(id >> 8);
  o.id[2] = (uint8_t)id;
  o.ageMs = ageMs;
  o.battMv = battMv;
  return o;
}

int main() {
  assert(batteryHealthBand(false, 3400) == BatteryHealthBand::OFF_AIR);
  assert(batteryHealthBand(true, 3201) == BatteryHealthBand::GOOD);
  assert(batteryHealthBand(true, 3200) == BatteryHealthBand::NEAR_LOW);
  assert(batteryHealthBand(true, 3101) == BatteryHealthBand::NEAR_LOW);
  assert(batteryHealthBand(true, 3100) == BatteryHealthBand::LOW_BATTERY);
  assert(batteryHealthBand(true, 0) == BatteryHealthBand::UNKNOWN);

  HealthRegistryEntry registry[] = {entry(0x100001), entry(0x100002)};
  registry[0].callsign = "Luigi";
  registry[1].callsign = "Ponyta";
  HealthObservation observations[] = {
      observation(0x100001, 100, 3201),
      observation(0x100002, 5000, 3050),  // exactly stale
      observation(0x200002, 100, 3150),   // live foreign, sorted second
      observation(0x200001, 100, 3000),   // live foreign, sorted first
      observation(0x200003, 6000, 3300),  // stale foreign, omitted
  };
  HealthTile tiles[8] = {};
  size_t n = healthBuildTiles(registry, 2, observations, 5, 5000, tiles, 8);
  assert(n == 4);
  assert(tiles[0].registry == &registry[0]);
  assert(tiles[0].band == BatteryHealthBand::GOOD);
  assert(tiles[1].registry == &registry[1]);
  assert(tiles[1].band == BatteryHealthBand::OFF_AIR);
  assert(tiles[1].battMv == 3050);  // stale detail survives greying
  assert(tiles[2].registry == nullptr && tiles[2].id[2] == 0x01);
  assert(tiles[2].band == BatteryHealthBand::LOW_BATTERY);
  assert(tiles[3].registry == nullptr && tiles[3].id[2] == 0x02);
  assert(tiles[3].band == BatteryHealthBand::NEAR_LOW);

  HealthSummary summary = healthSummarize(tiles, n);
  assert(summary.good == 1);
  assert(summary.nearLow == 1);
  assert(summary.low == 1);
  assert(summary.offAir == 1);
  assert(summary.unknown == 0);
  assert(summary.unregisteredLive == 2);
  assert(healthRegistryFindCallsign(registry, 2, "luigi") == &registry[0]);
  assert(healthRegistryFindCallsign(registry, 2, "PONYTA") == &registry[1]);
  assert(healthRegistryFindCallsign(registry, 2, "missing") == nullptr);

  // The generated production roster is stable, sorted, and intentionally
  // excludes quarantined, bench-only, merely enumerated, and bridge hardware.
  assert(kHealthRegistryCount == 144);
  for (size_t i = 1; i < kHealthRegistryCount; ++i) {
    assert(std::memcmp(kHealthRegistry[i - 1].id, kHealthRegistry[i].id, 3) < 0);
  }
  for (size_t i = 0; i < kHealthRegistryCount; ++i) {
    size_t len = std::strlen(kHealthRegistry[i].callsign);
    assert(len >= 3 && len <= 7);
    for (size_t j = i + 1; j < kHealthRegistryCount; ++j)
      assert(std::strcmp(kHealthRegistry[i].callsign,
                         kHealthRegistry[j].callsign) != 0);
  }
  assert(std::strlen(kHealthRegistryCsvSha256) == 64);
  assert(std::strlen(kCallsignsCsvSha256) == 64);
  const uint8_t ponytaId[3] = {0xF2, 0xB7, 0xDC};
  const HealthRegistryEntry *ponyta =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, ponytaId);
  assert(ponyta && std::strcmp(ponyta->callsign, "Ponyta") == 0);
  const uint8_t astroId[3] = {0x9E, 0x5B, 0x44};
  const HealthRegistryEntry *astro =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, astroId);
  assert(astro && std::strcmp(astro->callsign, "Astro") == 0);
  const uint8_t bidoofId[3] = {0x9F, 0x26, 0xD8};
  const HealthRegistryEntry *bidoof =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, bidoofId);
  assert(bidoof && std::strcmp(bidoof->callsign, "Bidoof") == 0);

  std::printf("health_model ok (%zu registry fixtures)\n", kHealthRegistryCount);
  return 0;
}
