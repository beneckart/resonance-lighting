#include "action_gate.h"

AtomActionFamily atomActionFamily(bool timeValid, bool civilNight) {
  if (!timeValid) return ATOM_ACTION_UNKNOWN;
  return civilNight ? ATOM_ACTION_COLOR : ATOM_ACTION_CHIME;
}
