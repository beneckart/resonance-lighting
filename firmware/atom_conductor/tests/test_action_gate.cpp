#include <stdio.h>

#include "core/action_gate.h"
#include "../../fixture/src/core/show_schedule.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main() {
  CHECK(atomActionFamily(false, true) == ATOM_ACTION_UNKNOWN);
  CHECK(atomActionFamily(true, showScheduleAt(1788098400UL).civilNight) ==
        ATOM_ACTION_CHIME); // 07:00 PDT
  CHECK(atomActionFamily(true, showScheduleAt(1788143400UL).civilNight) ==
        ATOM_ACTION_CHIME); // visible-light pre-dusk overlap
  CHECK(atomActionFamily(true, showScheduleAt(1788147000UL).civilNight) ==
        ATOM_ACTION_COLOR); // after civil dusk
  CHECK(atomActionFamily(true, showScheduleAt(1788087600UL).civilNight) ==
        ATOM_ACTION_COLOR); // before civil dawn
  printf("action_gate ok\n");
  return 0;
}
