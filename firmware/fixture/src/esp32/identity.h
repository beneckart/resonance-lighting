// MAC-derived fixture identity (ADR 0009: no per-unit provisioning).
// fixture_id = last 3 MAC bytes, uppercase hex -- the same short id used in
// ESP-NOW packets, the fleet registry, and the native-USB serial number.
#pragma once

#include <Arduino.h>
#include "esp_system.h"

extern uint8_t gMyMac[6];
extern uint8_t gMyId[3];
extern String gShortId;

void identityInit();
const char *resetReasonName(esp_reset_reason_t r);
