#include "target_picker_model.h"

#include <string.h>

namespace {

static char asciiLower(char c) {
  return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

static int foldedCompare(const char *a, const char *b) {
  while (*a && *b) {
    char ca = asciiLower(*a++);
    char cb = asciiLower(*b++);
    if (ca != cb) return ca < cb ? -1 : 1;
  }
  if (*a == *b) return 0;
  return *a ? 1 : -1;
}

static int idCompare(const uint8_t a[3], const uint8_t b[3]) {
  for (size_t i = 0; i < 3; ++i) {
    if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
  }
  return 0;
}

static bool comesBefore(const TargetPickerItem &a,
                        const TargetPickerItem &b) {
  const bool aKnown = a.callsign && a.callsign[0];
  const bool bKnown = b.callsign && b.callsign[0];
  if (aKnown != bKnown) return aKnown;
  if (aKnown) {
    int byName = foldedCompare(a.callsign, b.callsign);
    if (byName != 0) return byName < 0;
  }
  return idCompare(a.id, b.id) < 0;
}

static bool containsFolded(const char *text, const char *needle) {
  if (!needle[0]) return true;
  for (; *text; ++text) {
    const char *candidate = text;
    const char *wanted = needle;
    while (*candidate && *wanted &&
           asciiLower(*candidate) == asciiLower(*wanted)) {
      ++candidate;
      ++wanted;
    }
    if (!*wanted) return true;
  }
  return false;
}

} // namespace

void targetPickerSort(TargetPickerItem *items, size_t count) {
  if (!items) return;
  for (size_t i = 1; i < count; ++i) {
    TargetPickerItem moving = items[i];
    size_t at = i;
    while (at > 0 && comesBefore(moving, items[at - 1])) {
      items[at] = items[at - 1];
      --at;
    }
    items[at] = moving;
  }
}

bool targetPickerMatches(const TargetPickerItem &item, const char *query) {
  if (!query) return true;
  while (*query == ' ') ++query;
  size_t queryLen = strlen(query);
  while (queryLen > 0 && query[queryLen - 1] == ' ') --queryLen;
  if (queryLen == 0) return true;

  char trimmed[32];
  if (queryLen >= sizeof(trimmed)) queryLen = sizeof(trimmed) - 1;
  memcpy(trimmed, query, queryLen);
  trimmed[queryLen] = 0;

  if (item.callsign && containsFolded(item.callsign, trimmed)) return true;

  static const char hex[] = "0123456789ABCDEF";
  char shortId[7];
  for (size_t i = 0; i < 3; ++i) {
    shortId[i * 2] = hex[item.id[i] >> 4];
    shortId[i * 2 + 1] = hex[item.id[i] & 0x0F];
  }
  shortId[6] = 0;
  return containsFolded(shortId, trimmed);
}
