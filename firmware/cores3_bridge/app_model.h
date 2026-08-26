#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Pure, native-testable state shared by the CoreS3 launcher and its two apps.
// Cambium remains a separate binary transport and does not use this model.
enum CoreS3App : uint8_t {
  CORES3_APP_HOME = 0,
  CORES3_APP_LISTENER,
  CORES3_APP_AUDIO,
  CORES3_APP_LISTENER_DETAIL,
};

enum CoreS3AudioInput : uint8_t {
  CORES3_AUDIO_INPUT_AMBIENT = 0,
  CORES3_AUDIO_INPUT_AUX,
};

enum CoreS3BatteryBand : uint8_t {
  CORES3_BATTERY_OFF_AIR = 0,
  CORES3_BATTERY_GOOD,
  CORES3_BATTERY_NEAR_LOW,
  CORES3_BATTERY_LOW,
  CORES3_BATTERY_UNKNOWN,
};

static constexpr size_t CORES3_LISTENER_PAGE_SIZE = 24;
static constexpr int16_t CORES3_BATTERY_GOOD_ABOVE_MV = 3200;
static constexpr int16_t CORES3_BATTERY_NEAR_LOW_ABOVE_MV = 3100;

inline CoreS3BatteryBand cores3BatteryBand(bool onAir, int16_t battMv) {
  if (!onAir) return CORES3_BATTERY_OFF_AIR;
  if (battMv < 2000 || battMv > 5000) return CORES3_BATTERY_UNKNOWN;
  if (battMv > CORES3_BATTERY_GOOD_ABOVE_MV)
    return CORES3_BATTERY_GOOD;
  if (battMv > CORES3_BATTERY_NEAR_LOW_ABOVE_MV)
    return CORES3_BATTERY_NEAR_LOW;
  return CORES3_BATTERY_LOW;
}

inline size_t cores3PageCount(size_t itemCount) {
  return itemCount ? (itemCount + CORES3_LISTENER_PAGE_SIZE - 1) /
                         CORES3_LISTENER_PAGE_SIZE
                   : 1;
}

inline size_t cores3ClampPage(size_t page, size_t itemCount) {
  size_t count = cores3PageCount(itemCount);
  return page < count ? page : count - 1;
}

inline size_t cores3PageStart(size_t page, size_t itemCount) {
  return cores3ClampPage(page, itemCount) * CORES3_LISTENER_PAGE_SIZE;
}

inline CoreS3AudioInput cores3NextAudioInput(CoreS3AudioInput current,
                                              bool auxSupported) {
  if (!auxSupported) return CORES3_AUDIO_INPUT_AMBIENT;
  return current == CORES3_AUDIO_INPUT_AUX ? CORES3_AUDIO_INPUT_AMBIENT
                                           : CORES3_AUDIO_INPUT_AUX;
}

inline bool cores3IsFixtureFirmware(const char *revision) {
  if (!revision || !revision[0]) return false;
  // ADR 0040 immutable fleet artifacts use fx-*. Retain the older fixture-*
  // development identity and the fixture build cache's explicit dev-local
  // identity, while excluding legacy net-bench peers and bridge firmware.
  return strncmp(revision, "fx-", 3) == 0 ||
         strncmp(revision, "fixture-", 8) == 0 ||
         strcmp(revision, "dev-local") == 0;
}

inline bool cores3AudioPeerEligible(bool hasFirmwareIdentity,
                                    const char *revision) {
  // Before the infrequent full heartbeat arrives, optimistically include the
  // radio peer. Once identified, only fixture firmware may consume a slot.
  return !hasFirmwareIdentity || cores3IsFixtureFirmware(revision);
}
