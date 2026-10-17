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
// Event mode (NB_EVENT_MODE from the T-Deck): 1 = perimeter cycle, 2 = lantern
// dings. Mode 2 keeps a sensing downlight awake and chimes it (plus its roster
// partner) when someone is under it. A mode change flashes every light blue
// (1) or green (2) for 1.5 s.
uint8_t decompressionEventMode();
void decompressionOnEventMode(const NbEventMode &mode);
bool decompressionModeFlash(uint32_t nowMs, uint8_t rgb[3]);
void decompressionDingTick(uint32_t nowMs);
