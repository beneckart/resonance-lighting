#include "boot_park.h"

#include <Arduino.h>
#include "driver/rtc_io.h"

void bootParkRailLow() {
  rtc_gpio_hold_dis(GPIO_NUM_4);
  rtc_gpio_init(GPIO_NUM_4);
  rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_OUTPUT);
  rtc_gpio_set_level(GPIO_NUM_4, 0);
  rtc_gpio_hold_en(GPIO_NUM_4);
}

void bootParkAll() {
  // Solenoid gate first: VDC is always live and only the pulldown resistor is
  // holding the MOSFET off until this line runs.
  pinMode(RES_SOLENOID_PIN, OUTPUT);
  digitalWrite(RES_SOLENOID_PIN, LOW);
  // Pixel data low so a floating line can't clock garbage into latched pixels
  // the moment the rail comes up.
  pinMode(RES_LED_DATA_PIN, OUTPUT);
  digitalWrite(RES_LED_DATA_PIN, LOW);
  bootParkRailLow();
}
