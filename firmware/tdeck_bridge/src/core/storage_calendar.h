#pragma once
#include <stdint.h>
struct StorageDate {
  int year, month, day, hour, minute;
  bool daylight;
};
// US Pacific rules in effect since 2007; restricted to the project's 2025-2035
// horizon. Ambiguous/repeated and nonexistent local times are refused.
StorageDate storagePacificAt(uint32_t utcS);
StorageDate storagePacificDay(uint32_t nowUtcS, unsigned daysAhead);
bool storagePacificToUtc(const StorageDate &local, uint32_t &utcS);
