#include "test_util.h"

#include "../src/core/ota_verify_policy.h"

int main() {
  // COMMS owns ESP-NOW. WiFi/HTTP state is irrelevant in this mode.
  CHECK(otaNetworkSelfTestPass(false, false, true, 1));
  CHECK(otaNetworkSelfTestPass(false, true, true, 9));
  CHECK(!otaNetworkSelfTestPass(false, true, false, 1));
  CHECK(!otaNetworkSelfTestPass(false, true, true, 0));

  // MAINT deliberately deinitializes ESP-NOW and owns WiFi/HTTP instead.
  CHECK(otaNetworkSelfTestPass(true, true, false, 0));
  CHECK(!otaNetworkSelfTestPass(true, false, true, 9));

  return testReport("test_ota_verify_policy");
}
