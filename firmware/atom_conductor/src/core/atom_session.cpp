#include "atom_session.h"

#include <string.h>

static bool reached(uint32_t nowMs, uint32_t deadlineMs) {
  return (int32_t)(nowMs - deadlineMs) >= 0;
}

static bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t intervalMs) {
  return (uint32_t)(nowMs - sinceMs) >= intervalMs;
}

void atomSessionInit(AtomSession &session) {
  memset(&session, 0, sizeof(session));
  session.phase = ATOM_SESSION_IDLE;
}

void atomSessionStart(AtomSession &session, uint32_t nowMs) {
  atomSessionInit(session);
  session.phase = ATOM_SESSION_GATHER;
  session.startedMs = nowMs;
  session.lastInputMs = nowMs;
  session.lastWakeMs = nowMs;
  session.gatherUntilMs = nowMs + ATOM_GATHER_MS;
  session.nextGatherWakeMs = nowMs;
  session.nextLocateMs = nowMs;
}

void atomSessionNoteInput(AtomSession &session, uint32_t nowMs) {
  if (session.phase == ATOM_SESSION_GATHER ||
      session.phase == ATOM_SESSION_READY)
    session.lastInputMs = nowMs;
}

uint16_t atomSessionRequestEnd(AtomSession &session, uint32_t nowMs) {
  if (session.phase == ATOM_SESSION_IDLE ||
      session.phase == ATOM_SESSION_SLEEP_DUE)
    return ATOM_ACTION_NONE;
  if (session.phase == ATOM_SESSION_CLEANUP) return ATOM_ACTION_NONE;
  session.phase = ATOM_SESSION_CLEANUP;
  session.cleanupStartedMs = nowMs;
  session.nextAutoMs = nowMs + ATOM_AUTO_REPEAT_MS;
  return ATOM_ACTION_STOP_LOCATE | ATOM_ACTION_SEND_AUTO;
}

uint16_t atomSessionTick(AtomSession &session, uint32_t nowMs) {
  uint16_t actions = ATOM_ACTION_NONE;
  switch (session.phase) {
  case ATOM_SESSION_IDLE:
  case ATOM_SESSION_SLEEP_DUE:
    return actions;

  case ATOM_SESSION_GATHER:
    if (elapsed(nowMs, session.startedMs, ATOM_ABSOLUTE_MS) ||
        elapsed(nowMs, session.lastInputMs, ATOM_INACTIVITY_MS))
      return atomSessionRequestEnd(session, nowMs);
    if (reached(nowMs, session.gatherUntilMs)) {
      session.phase = ATOM_SESSION_READY;
      return ATOM_ACTION_STOP_LOCATE | ATOM_ACTION_BECAME_READY;
    }
    if (reached(nowMs, session.nextGatherWakeMs)) {
      actions |= ATOM_ACTION_SEND_WAKE;
      session.lastWakeMs = nowMs;
      session.nextGatherWakeMs = nowMs + ATOM_GATHER_WAKE_PERIOD_MS;
    }
    if (reached(nowMs, session.nextLocateMs)) {
      actions |= ATOM_ACTION_START_LOCATE;
      session.nextLocateMs = nowMs + ATOM_LOCATE_REFRESH_MS;
    }
    return actions;

  case ATOM_SESSION_READY:
    if (elapsed(nowMs, session.startedMs, ATOM_ABSOLUTE_MS) ||
        elapsed(nowMs, session.lastInputMs, ATOM_INACTIVITY_MS))
      return atomSessionRequestEnd(session, nowMs);
    if (elapsed(nowMs, session.lastWakeMs, ATOM_WAKE_RENEW_MS)) {
      // This explicit packet is the lease renewal. User effects only update
      // lastInputMs and therefore cannot become an unbounded Wake stream.
      actions |= ATOM_ACTION_SEND_WAKE;
      session.lastWakeMs = nowMs;
    }
    return actions;

  case ATOM_SESSION_CLEANUP:
    if (elapsed(nowMs, session.cleanupStartedMs, ATOM_CLEANUP_MS)) {
      session.phase = ATOM_SESSION_SLEEP_DUE;
      return ATOM_ACTION_SLEEP;
    }
    if (reached(nowMs, session.nextAutoMs)) {
      actions |= ATOM_ACTION_SEND_AUTO;
      session.nextAutoMs = nowMs + ATOM_AUTO_REPEAT_MS;
    }
    return actions;
  }
  return actions;
}
