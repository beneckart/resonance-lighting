#include "watchdog.h"

#include <Arduino.h>
#include "esp_task_wdt.h"

static bool gArmed = false;

void watchdogInit() {
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = (uint32_t)RES_WDT_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_err_t e = esp_task_wdt_init(&cfg);
  if (e == ESP_ERR_INVALID_STATE) e = esp_task_wdt_reconfigure(&cfg); // core already inited it
  gArmed = (e == ESP_OK) && (esp_task_wdt_add(NULL) == ESP_OK);
  Serial.printf("watchdog: %ds, panic+reset on hang%s\n", RES_WDT_S,
                gArmed ? "" : " (ARM FAILED)");
}

bool watchdogArmed() { return gArmed; }

void watchdogService() {
  if (gArmed) esp_task_wdt_reset();
}
