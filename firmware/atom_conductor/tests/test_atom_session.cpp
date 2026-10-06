#include <stdio.h>

#include "core/atom_session.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main() {
  AtomSession session;
  atomSessionInit(session);
  CHECK(session.phase == ATOM_SESSION_IDLE);

  atomSessionStart(session, 1000);
  uint16_t action = atomSessionTick(session, 1000);
  CHECK(action & ATOM_ACTION_SEND_WAKE);
  CHECK(action & ATOM_ACTION_START_LOCATE);

  for (uint32_t elapsed = 2000; elapsed < ATOM_GATHER_MS; elapsed += 2000)
    atomSessionTick(session, 1000 + elapsed);
  action = atomSessionTick(session, 1000 + ATOM_GATHER_MS);
  CHECK(session.phase == ATOM_SESSION_READY);
  CHECK(action & ATOM_ACTION_STOP_LOCATE);
  CHECK(action & ATOM_ACTION_BECAME_READY);

  // An effect/mode input keeps the local session alive but does not itself
  // emit Wake. Renewal remains owned by the independent five-minute clock.
  uint32_t justReady = 1000 + ATOM_GATHER_MS + 100;
  atomSessionNoteInput(session, justReady);
  action = atomSessionTick(session, justReady);
  CHECK(!(action & ATOM_ACTION_SEND_WAKE));

  uint32_t renewAt = 1000 + ATOM_GATHER_MS - 2000 + ATOM_WAKE_RENEW_MS;
  action = atomSessionTick(session, renewAt);
  CHECK(action & ATOM_ACTION_SEND_WAKE);

  uint32_t inputAt = renewAt + 100;
  atomSessionNoteInput(session, inputAt);
  action = atomSessionTick(session, inputAt);
  CHECK(!(action & ATOM_ACTION_SEND_WAKE));
  CHECK(session.phase == ATOM_SESSION_READY);

  uint32_t idleAt = inputAt + ATOM_INACTIVITY_MS;
  action = atomSessionTick(session, idleAt);
  CHECK(session.phase == ATOM_SESSION_CLEANUP);
  CHECK(action & ATOM_ACTION_SEND_AUTO);
  CHECK(action & ATOM_ACTION_STOP_LOCATE);
  CHECK(!(action & ATOM_ACTION_SLEEP));

  action = atomSessionTick(session, idleAt + ATOM_AUTO_REPEAT_MS);
  CHECK(action & ATOM_ACTION_SEND_AUTO);
  action = atomSessionTick(session, idleAt + ATOM_CLEANUP_MS);
  CHECK(session.phase == ATOM_SESSION_SLEEP_DUE);
  CHECK(action & ATOM_ACTION_SLEEP);

  atomSessionStart(session, 0xFFFF0000UL);
  atomSessionNoteInput(session, 0xFFFF0000UL + ATOM_INACTIVITY_MS - 1);
  action = atomSessionTick(session, 0xFFFF0000UL + ATOM_ABSOLUTE_MS);
  CHECK(session.phase == ATOM_SESSION_CLEANUP);
  CHECK(action & ATOM_ACTION_SEND_AUTO);

  printf("atom_session ok\n");
  return 0;
}
