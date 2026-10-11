#pragma once

#include <stdint.h>

enum AtomActionFamily : uint8_t {
  ATOM_ACTION_UNKNOWN = 0,
  ATOM_ACTION_CHIME = 1,
  ATOM_ACTION_COLOR = 2,
};

// The fixture inspection light starts one hour before civil dusk, but the Atom
// stays on chimes through that overlap hour. Color begins only at civil dusk.
// Invalid/stale UTC fails closed so neither actuator family is guessed.
AtomActionFamily atomActionFamily(bool timeValid, bool civilNight);
