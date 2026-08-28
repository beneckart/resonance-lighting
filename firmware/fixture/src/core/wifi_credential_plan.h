#pragma once

#include <stddef.h>
#include <stdint.h>

// WiFi scans use this sentinel when a configured SSID was not observed. Keep
// it outside the real ESP32 RSSI range so ordinary integer comparisons remain
// unambiguous.
static constexpr int16_t WIFI_CREDENTIAL_UNSEEN_RSSI = -32768;

inline bool wifiCredentialComesFirst(uint8_t lhs, uint8_t rhs,
                                     const int16_t *bestRssi) {
  const bool lhsSeen = bestRssi[lhs] != WIFI_CREDENTIAL_UNSEEN_RSSI;
  const bool rhsSeen = bestRssi[rhs] != WIFI_CREDENTIAL_UNSEEN_RSSI;
  if (lhsSeen != rhsSeen) return lhsSeen;
  if (lhsSeen && bestRssi[lhs] != bestRssi[rhs])
    return bestRssi[lhs] > bestRssi[rhs];
  return lhs < rhs;
}

// Rank visible credentials by strongest RSSI. Configured-but-unseen entries
// follow in declaration order so a failed/partial scan still falls back to the
// historical primary credential first.
inline void wifiCredentialOrder(const int16_t *bestRssi, size_t count,
                                uint8_t *order) {
  for (size_t i = 0; i < count; ++i) order[i] = (uint8_t)i;
  for (size_t i = 1; i < count; ++i) {
    uint8_t key = order[i];
    size_t j = i;
    while (j > 0 && wifiCredentialComesFirst(key, order[j - 1], bestRssi)) {
      order[j] = order[j - 1];
      --j;
    }
    order[j] = key;
  }
}
