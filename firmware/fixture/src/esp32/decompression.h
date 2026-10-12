#pragma once
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
void decompressionButtonWake();
void decompressionTick();
void decompressionOnRadio(const NbDecompressionControl &command, uint32_t receivedMs);
uint32_t decompressionRadioSecondsLeft();
