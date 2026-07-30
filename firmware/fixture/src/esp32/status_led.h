// Onboard user LED (GPIO46): battery-level indicator + identify/locate blink.
#pragma once

#include <stdint.h>

void statusLedInit();
void statusLedTick();
void statusLedIdentify(uint8_t secs); // "..-" locate pattern window
