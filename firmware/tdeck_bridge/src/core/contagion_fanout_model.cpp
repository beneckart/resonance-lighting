#include "contagion_fanout_model.h"

#include <string.h>

void contagionFanoutGateInit(ContagionFanoutGate &gate) {
  memset(&gate, 0, sizeof(gate));
}

void contagionFanoutGateConfigure(ContagionFanoutGate &gate,
                                  const uint8_t source[3], bool enabled) {
  contagionFanoutGateInit(gate);
  gate.enabled = enabled && source;
  if (source) memcpy(gate.source, source, sizeof(gate.source));
}

bool contagionFanoutGateObserve(ContagionFanoutGate &gate,
                                const uint8_t source[3], uint8_t programId,
                                uint8_t state) {
  if (!gate.enabled || !source || memcmp(gate.source, source, 3) != 0)
    return false;
  bool infected = programId == 5 && (state & 0x03) == 1;
  bool rising = infected && !gate.wasInfected;
  gate.wasInfected = infected;
  return rising;
}
