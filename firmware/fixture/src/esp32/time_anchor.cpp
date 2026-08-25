#include "time_anchor.h"

#include <Arduino.h>
#include <esp_random.h>

#include "behavior_glue.h"
#include "espnow_link.h"
#include "identity.h"
#include "sensors/sensor_bus.h"

static bool gHaveRtc = false;
static uint16_t gBootId = 0;
static uint32_t gNextReadMs = 0;
static bool gLoggedInvalid = false;

void timeAnchorInit(bool haveDs3231) {
  gHaveRtc = haveDs3231;
  gBootId = (uint16_t)esp_random();
  if (!gBootId) gBootId = 1;
  gNextReadMs = millis() + 500;
  gLoggedInvalid = false;
}

void timeAnchorTick() {
  if (!gHaveRtc) return;
  uint32_t now = millis();
  if ((int32_t)(now - gNextReadMs) < 0) return;
  gNextReadMs = now + 10000UL;
  uint32_t utcS = 0;
  if (!sensorBusReadRtcUtc(utcS)) {
    if (!gLoggedInvalid) {
      Serial.println("time-anchor: DS3231 invalid/OSF; refusing UTC");
      gLoggedInvalid = true;
    }
    return;
  }
  gLoggedInvalid = false;
  NbTimeQuality q = {};
  fillHeader(&q.h, NB_TIME_QUALITY);
  q.utc_s = utcS;
  q.source = NB_TIME_RTC;
  q.uncert_ms = 2000;
  q.boot_id = gBootId;
  q.flags = NB_TIME_FLAG_VALID | NB_TIME_FLAG_DATE_VALID;
  // ESP-NOW does not promise a local broadcast echo; feed our own selector.
  behaviorOnTimeQuality(q, gMyId);
  espNowSendRaw(&q, sizeof(q));
}
