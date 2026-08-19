#include "loads.h"

#include <Arduino.h>

#include "boot_guard_io.h"
#include "boot_park.h"
#include "led_driver.h"
#include "solenoid.h"

void allLoadsOff(const char *why) {
  solenoidStop(why);
  ledRailOff(); // all-off frame -> data LOW -> rail cut, safe when already off
  digitalWrite(RES_STATUS_LED_PIN, LOW);
  // Every load is now provably off: clear the ADR 0051 armed marker so a
  // reset from here (OTA upload, maintenance, deep sleep, recovery, bench
  // power ordering) never reads as a load collapse.
  bootGuardLoadDisarm(why);
  (void)why;
}
