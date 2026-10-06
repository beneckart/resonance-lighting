#pragma once

#include <stdint.h>

// Pure session policy for the Atom Conductor. Effects call
// atomSessionNoteInput(); only atomSessionTick() schedules explicit lifecycle
// packets. Keeping those paths separate prevents an RGB/strike loop from
// becoming an accidental unbounded Wake publisher.

enum AtomSessionPhase : uint8_t {
  ATOM_SESSION_IDLE = 0,
  ATOM_SESSION_GATHER = 1,
  ATOM_SESSION_READY = 2,
  ATOM_SESSION_CLEANUP = 3,
  ATOM_SESSION_SLEEP_DUE = 4,
};

enum AtomSessionAction : uint16_t {
  ATOM_ACTION_NONE = 0,
  ATOM_ACTION_SEND_WAKE = 1u << 0,
  ATOM_ACTION_START_LOCATE = 1u << 1,
  ATOM_ACTION_STOP_LOCATE = 1u << 2,
  ATOM_ACTION_SEND_AUTO = 1u << 3,
  ATOM_ACTION_BECAME_READY = 1u << 4,
  ATOM_ACTION_SLEEP = 1u << 5,
};

static constexpr uint32_t ATOM_GATHER_MS = 6UL * 60UL * 1000UL;
static constexpr uint32_t ATOM_GATHER_WAKE_PERIOD_MS = 2000UL;
static constexpr uint32_t ATOM_LOCATE_REFRESH_MS = 30000UL;
static constexpr uint32_t ATOM_WAKE_RENEW_MS = 5UL * 60UL * 1000UL;
static constexpr uint32_t ATOM_INACTIVITY_MS = 15UL * 60UL * 1000UL;
static constexpr uint32_t ATOM_ABSOLUTE_MS = 60UL * 60UL * 1000UL;
static constexpr uint32_t ATOM_CLEANUP_MS = 10000UL;
static constexpr uint32_t ATOM_AUTO_REPEAT_MS = 2000UL;

struct AtomSession {
  AtomSessionPhase phase;
  uint32_t startedMs;
  uint32_t lastInputMs;
  uint32_t lastWakeMs;
  uint32_t gatherUntilMs;
  uint32_t cleanupStartedMs;
  uint32_t nextGatherWakeMs;
  uint32_t nextLocateMs;
  uint32_t nextAutoMs;
};

void atomSessionInit(AtomSession &session);
void atomSessionStart(AtomSession &session, uint32_t nowMs);
void atomSessionNoteInput(AtomSession &session, uint32_t nowMs);
uint16_t atomSessionRequestEnd(AtomSession &session, uint32_t nowMs);
uint16_t atomSessionTick(AtomSession &session, uint32_t nowMs);
