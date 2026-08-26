#pragma once

#include <stdint.h>

// Pure edge detector for the explicit legacy Contagion bridge mode. One
// selected program-5 source may start one old-protocol targeted strike roll
// when it changes into the infected state. Repeated RF/state frames stay quiet.
struct ContagionFanoutGate {
  bool enabled;
  bool wasInfected;
  uint8_t source[3];
};

void contagionFanoutGateInit(ContagionFanoutGate &gate);
void contagionFanoutGateConfigure(ContagionFanoutGate &gate,
                                  const uint8_t source[3], bool enabled);
bool contagionFanoutGateObserve(ContagionFanoutGate &gate,
                                const uint8_t source[3], uint8_t programId,
                                uint8_t state);
