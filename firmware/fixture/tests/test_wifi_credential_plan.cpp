#include "test_util.h"

#include "wifi_credential_plan.h"

int main() {
  uint8_t order[2] = {99, 99};

  // A failed scan retains declaration order: existing primary, then fallback.
  int16_t unseen[2] = {WIFI_CREDENTIAL_UNSEEN_RSSI,
                       WIFI_CREDENTIAL_UNSEEN_RSSI};
  wifiCredentialOrder(unseen, 2, order);
  CHECK_EQ(order[0], 0);
  CHECK_EQ(order[1], 1);

  // A sole visible fallback is tried before an absent primary.
  int16_t fallbackOnly[2] = {WIFI_CREDENTIAL_UNSEEN_RSSI, -68};
  wifiCredentialOrder(fallbackOnly, 2, order);
  CHECK_EQ(order[0], 1);
  CHECK_EQ(order[1], 0);

  // When both sites overlap, use the stronger known AP.
  int16_t fallbackStronger[2] = {-82, -54};
  wifiCredentialOrder(fallbackStronger, 2, order);
  CHECK_EQ(order[0], 1);
  CHECK_EQ(order[1], 0);

  int16_t primaryStronger[2] = {-48, -71};
  wifiCredentialOrder(primaryStronger, 2, order);
  CHECK_EQ(order[0], 0);
  CHECK_EQ(order[1], 1);

  // Equal RSSI remains stable and therefore prefers the declared primary.
  int16_t tied[2] = {-60, -60};
  wifiCredentialOrder(tied, 2, order);
  CHECK_EQ(order[0], 0);
  CHECK_EQ(order[1], 1);

  return testReport("wifi_credential_plan");
}
