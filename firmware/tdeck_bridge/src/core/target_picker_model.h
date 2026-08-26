#pragma once

#include <stddef.h>
#include <stdint.h>

struct TargetPickerItem {
  uint8_t id[3];
  const char *callsign;
};

// Known callsigns sort first, alphabetically (case-insensitive). Unknown
// fixtures follow in short-ID order.
void targetPickerSort(TargetPickerItem *items, size_t count);

// Empty queries match everything. Otherwise match a callsign substring or a
// contiguous part of the six-digit short ID, case-insensitively.
bool targetPickerMatches(const TargetPickerItem &item, const char *query);
