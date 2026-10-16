#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "../core/packet.h"
#ifndef RES_DECOMPRESSION
#define RES_DECOMPRESSION 0
#endif
inline bool decompressionBuild() { return RES_DECOMPRESSION != 0; }
bool decompressionActive();
uint32_t decompressionSecondsLeft();
bool decompressionSetSession(uint16_t minutes);
bool decompressionPlaySignature();
bool decompressionPowerReady(); // battery, FULL/DIM tier, comms, no OTA/boot-guard veto
void decompressionButtonWake();
void decompressionTick();
void decompressionOnRadio(const NbDecompressionControl &command, uint32_t receivedMs);
uint32_t decompressionRadioSecondsLeft();
// Atom press reporting for host tools (tone lab). Relay mode (RAM-only) reports
// PLAY presses without starting the local phrase; telemetry carries the counts.
void decompressionSetRelay(bool on);
bool decompressionRelay();
void decompressionRadioTelemetry(String &json);
