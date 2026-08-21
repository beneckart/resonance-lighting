// Platform-independent network half of the deferred OTA self-test.
// Each runtime mode proves the radio path it intentionally owns.
#pragma once

#include <stdint.h>

bool otaNetworkSelfTestPass(bool maintenanceMode, bool maintenanceReady,
                            bool espNowUp, uint32_t espNowSendOk);
