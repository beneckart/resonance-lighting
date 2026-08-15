#include "loads.h"

#include <Arduino.h>

#include "boot_park.h"
#include "led_driver.h"
#include "solenoid.h"

void allLoadsOff(const char *why) {
  solenoidStop(why);
  ledRailOff(); // all-off frame -> data LOW -> rail cut, safe when already off
  digitalWrite(RES_STATUS_LED_PIN, LOW);
  (void)why;
}
