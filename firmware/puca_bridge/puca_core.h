#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct PucaPcmStats {
  uint16_t peak = 0;
  size_t clipped = 0;
};

struct PucaRgbw {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t w = 0;
};

enum BridgeMode : uint8_t {
  MODE_CLASSIC = 0, // user-facing DJ: per-slot R/G/B envelope
  MODE_HEARTBEAT,
  MODE_EMBER,
  MODE_HUE,
  MODE_OFF,
  MODE_COUNT,
};

inline BridgeMode pucaNextLiveMode(BridgeMode current) {
  switch (current) {
  case MODE_CLASSIC: return MODE_HEARTBEAT;
  case MODE_HEARTBEAT: return MODE_EMBER;
  case MODE_EMBER: return MODE_HUE;
  default: return MODE_CLASSIC;
  }
}

inline uint8_t pucaModeStatusCode(BridgeMode current) {
  switch (current) {
  case MODE_CLASSIC: return 1;
  case MODE_HEARTBEAT: return 2;
  case MODE_EMBER: return 3;
  case MODE_HUE: return 4;
  default: return 5;
  }
}

inline bool pucaPublisherShouldArmAtBoot(bool pawHeld, bool codecReady,
                                         bool audioInputReady) {
  return pawHeld && codecReady && audioInputReady;
}

inline bool pucaIsFixtureFirmware(const char *revision) {
  if (!revision || !revision[0]) return false;
  // ADR 0040 immutable fleet artifacts use fx-*. Retain the older fixture-*
  // development identity and the fixture build cache's explicit dev-local
  // identity, while excluding legacy net-bench peers and bridge firmware.
  return strncmp(revision, "fx-", 3) == 0 ||
         strncmp(revision, "fixture-", 8) == 0 ||
         strcmp(revision, "dev-local") == 0;
}

inline bool pucaAudioPeerEligible(bool hasFirmwareIdentity,
                                  const char *revision) {
  // Before the infrequent full heartbeat arrives, optimistically include the
  // radio peer. Once identified, only fixture firmware may consume a slot.
  return !hasFirmwareIdentity || pucaIsFixtureFirmware(revision);
}

// PUCA is a one-off publisher, not a fleet fixture. It must never leave the
// mesh because somebody issued the legacy all-zero/fleet-wide maintenance
// command. Only an exact short-ID match may open its WiFi OTA endpoint.
inline bool pucaMaintenanceTargetMatches(const uint8_t target[3],
                                         const uint8_t myId[3]) {
  return target && myId && memcmp(target, myId, 3) == 0;
}

inline size_t pucaChunkCount(size_t total, size_t capacity) {
  return capacity && total ? (total + capacity - 1) / capacity : 0;
}

inline size_t pucaChunkSize(size_t total, size_t offset, size_t capacity) {
  if (!capacity || offset >= total) return 0;
  size_t remaining = total - offset;
  return remaining < capacity ? remaining : capacity;
}

inline PucaPcmStats pucaPcmStats(const int16_t *samples, size_t count,
                                 uint16_t clipThreshold = 32700) {
  PucaPcmStats stats;
  if (!samples) return stats;
  for (size_t i = 0; i < count; ++i) {
    int32_t magnitude = samples[i] < 0 ? -(int32_t)samples[i] : samples[i];
    if (magnitude > 32768) magnitude = 32768;
    if ((uint16_t)magnitude > stats.peak) stats.peak = (uint16_t)magnitude;
    if ((uint16_t)magnitude >= clipThreshold) ++stats.clipped;
  }
  return stats;
}

// Collapse interleaved L/R PCM in place. The output occupies the first half of
// the same buffer; an unmatched trailing sample is ignored.
inline size_t pucaStereoToMono(int16_t *samples, size_t sampleCount) {
  if (!samples) return 0;
  size_t pairs = sampleCount / 2;
  for (size_t i = 0; i < pairs; ++i) {
    int32_t mixed = (int32_t)samples[2 * i] + samples[2 * i + 1];
    samples[i] = (int16_t)(mixed / 2);
  }
  return pairs;
}

// Peak follower for deterministic line-level waveforms. Unlike the adaptive
// room-audio envelope, this path needs no quiet calibration and therefore does
// not learn a continuously repeating generator waveform as its noise floor.
// At the current 100 ms block rate, a quick release keeps closely spaced pulses
// distinct while the peak statistic prevents a narrow pulse from disappearing
// inside a long RMS window.
struct PucaPeakFollower {
  float level = 0.0f;

  void reset() { level = 0.0f; }

  float update(uint16_t peak, float sensitivity, float attack = 0.90f,
               float release = 0.45f) {
    float target = ((float)peak / 32768.0f) * sensitivity;
    if (target < 0.0f) target = 0.0f;
    if (target > 1.0f) target = 1.0f;
    float blend = target > level ? attack : release;
    level += (target - level) * blend;
    if (level < 0.002f) level = 0.0f;
    return level;
  }
};

inline PucaRgbw pucaHeartbeatColor(float level, float ceiling) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  if (ceiling < 0.0f) ceiling = 0.0f;
  if (ceiling > 1.0f) ceiling = 1.0f;
  uint8_t value = (uint8_t)(level * ceiling * 255.0f + 0.5f);
  return {value, (uint8_t)(value / 24), 0, 0};
}

enum PucaTouchEvent : uint8_t {
  PUCA_TOUCH_NONE = 0,
  PUCA_TOUCH_SHORT,
  PUCA_TOUCH_LONG,
};

// Debounce the carrier paw while treating its post-boot level as the baseline.
// A short touch is reported only on release, so a deliberate long hold never
// also performs the short-touch action. A settled-high input is suppressed
// until release rather than becoming a synthetic press after the boot guard.
struct PucaTouchGesture {
  bool primed = false;
  bool raw = false;
  bool stable = false;
  bool suppressCurrentPress = false;
  bool longReported = false;
  uint32_t lastChangeMs = 0;
  uint32_t pressedMs = 0;

  PucaTouchEvent update(uint32_t now, bool nextRaw, uint32_t guardMs = 2500,
                        uint32_t debounceMs = 30,
                        uint32_t longHoldMs = 1500) {
    if (now < guardMs) return PUCA_TOUCH_NONE;
    if (!primed) {
      primed = true;
      raw = nextRaw;
      stable = nextRaw;
      suppressCurrentPress = nextRaw;
      longReported = nextRaw;
      lastChangeMs = now;
      pressedMs = now;
      return PUCA_TOUCH_NONE;
    }
    if (nextRaw != raw) {
      raw = nextRaw;
      lastChangeMs = now;
    }
    if (raw != stable && now - lastChangeMs >= debounceMs) {
      stable = raw;
      if (stable) {
        pressedMs = now;
        longReported = false;
        suppressCurrentPress = false;
      } else {
        bool reportShort = !longReported && !suppressCurrentPress;
        suppressCurrentPress = false;
        return reportShort ? PUCA_TOUCH_SHORT : PUCA_TOUCH_NONE;
      }
    }
    if (stable && !longReported && !suppressCurrentPress &&
        now - pressedMs >= longHoldMs) {
      longReported = true;
      return PUCA_TOUCH_LONG;
    }
    return PUCA_TOUCH_NONE;
  }
};

// Require a continuous hold during boot. Brief startup spikes reset the timer
// and cannot open the setup window.
struct PucaBootHoldDetector {
  bool tracking = false;
  bool complete = false;
  uint32_t touchedSinceMs = 0;

  bool update(uint32_t now, bool touched, uint32_t holdMs = 1200) {
    if (complete) return true;
    if (!touched) {
      tracking = false;
      return false;
    }
    if (!tracking) {
      tracking = true;
      touchedSinceMs = now;
      return false;
    }
    if (now - touchedSinceMs < holdMs) return false;
    complete = true;
    return true;
  }
};

struct PucaSetupWindow {
  bool unlocked = false;
  uint32_t deadlineMs = 0;
  uint32_t durationMs = 20000;

  void enter(uint32_t now, uint32_t duration = 20000) {
    unlocked = true;
    durationMs = duration;
    deadlineMs = now + durationMs;
  }

  void activity(uint32_t now) {
    if (unlocked) deadlineMs = now + durationMs;
  }

  void lock() { unlocked = false; }

  bool update(uint32_t now) {
    if (!unlocked || (int32_t)(now - deadlineMs) < 0) return false;
    unlocked = false;
    return true;
  }
};

// Non-blocking bottom-LED status pattern. Input is encoded first (one long
// pulse for line, two for onboard microphones), followed by 1..5 short mode
// pulses. The caller chooses the idle level, allowing setup mode to remain
// visibly armed between patterns while normal locked operation stays dark.
struct PucaLedPattern {
  static const uint8_t MAX_SEGMENTS = 20;
  bool levels[MAX_SEGMENTS] = {};
  uint16_t durationsMs[MAX_SEGMENTS] = {};
  uint8_t count = 0;
  uint8_t index = 0;
  uint32_t segmentStartedMs = 0;
  bool active = false;

  void add(bool level, uint16_t durationMs) {
    if (count >= MAX_SEGMENTS) return;
    levels[count] = level;
    durationsMs[count] = durationMs;
    ++count;
  }

  void startStatus(uint32_t now, bool lineInput, uint8_t modeCode) {
    count = 0;
    index = 0;
    uint8_t inputPulses = lineInput ? 1 : 2;
    for (uint8_t i = 0; i < inputPulses; ++i) {
      add(true, 500);
      add(false, 180);
    }
    add(false, 420);
    if (modeCode < 1) modeCode = 1;
    if (modeCode > 5) modeCode = 5;
    for (uint8_t i = 0; i < modeCode; ++i) {
      add(true, 150);
      add(false, 180);
    }
    segmentStartedMs = now;
    active = count > 0;
  }

  bool level(uint32_t now, bool idleLevel = false) {
    while (active && index < count &&
           now - segmentStartedMs >= durationsMs[index]) {
      segmentStartedMs += durationsMs[index];
      ++index;
      if (index >= count) active = false;
    }
    return active ? levels[index] : idleLevel;
  }
};
