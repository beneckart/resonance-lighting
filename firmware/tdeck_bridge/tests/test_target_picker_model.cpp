#include "core/target_picker_model.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
  TargetPickerItem items[] = {
      {{0xF2, 0xBD, 0xFC}, "Magmar"},
      {{0x00, 0x00, 0x02}, nullptr},
      {{0x01, 0x02, 0x03}, "abra"},
      {{0x00, 0x00, 0x01}, nullptr},
      {{0x10, 0x20, 0x30}, "Zubat"},
      {{0x00, 0x10, 0x00}, "Abra"},
  };
  targetPickerSort(items, sizeof(items) / sizeof(items[0]));

  assert(strcmp(items[0].callsign, "Abra") == 0);
  assert(strcmp(items[1].callsign, "abra") == 0);
  assert(strcmp(items[2].callsign, "Magmar") == 0);
  assert(strcmp(items[3].callsign, "Zubat") == 0);
  assert(items[4].callsign == nullptr && items[4].id[2] == 0x01);
  assert(items[5].callsign == nullptr && items[5].id[2] == 0x02);

  assert(targetPickerMatches(items[2], "mag"));
  assert(targetPickerMatches(items[2], "GMAR"));
  assert(targetPickerMatches(items[2], "  mag  "));
  assert(targetPickerMatches(items[2], "F2BD"));
  assert(targetPickerMatches(items[2], "bdfc"));
  assert(targetPickerMatches(items[2], ""));
  assert(!targetPickerMatches(items[2], "abra"));

  puts("target_picker_model: ok");
  return 0;
}
