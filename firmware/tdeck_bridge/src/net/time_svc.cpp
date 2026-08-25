#include "time_svc.h"

#include <Arduino.h>
#include <esp_random.h>

#include "../hal/hal_board.h"
#include "mesh_tx.h"
#include "nb_emit.h"

static uint16_t gBootId = 0;
static uint32_t gNextPublishMs = 0;

void timeSvcBegin() {
  gBootId = (uint16_t)esp_random();
  if (!gBootId) gBootId = 1;
  gNextPublishMs = 0;
}

void timeSvcTick() {
  uint32_t now = millis();
  if ((int32_t)(now - gNextPublishMs) < 0) return;
  gNextPublishMs = now + 2000; // sleeping peers hear several per 8/15 s window
  GpsUtcObservation obs = halGpsUtc();
  if (!obs.valid) return;
  uint32_t ageMs = now - obs.receivedMs;
  if (ageMs > 10000) return;
  uint32_t uncertainty = 250U + ageMs;
  if (uncertainty > 65535U) uncertainty = 65535U;
  if (meshTimeQuality(obs.utcS, obs.subMs, (uint16_t)(ageMs / 1000U),
                      (uint16_t)uncertainty, gBootId))
    nbEmitLocalGps(meshMyId(), obs.utcS, obs.subMs, ageMs,
                   (uint16_t)uncertainty, gBootId);
}
