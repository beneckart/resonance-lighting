#include "ota_verify_policy.h"

bool otaNetworkSelfTestPass(bool maintenanceMode, bool maintenanceReady,
                            bool espNowUp, uint32_t espNowSendOk) {
  if (maintenanceMode) return maintenanceReady;
  return espNowUp && espNowSendOk > 0;
}
