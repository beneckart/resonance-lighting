#include "contagion_fanout.h"

#include <Arduino.h>
#include <string.h>

#include "../core/contagion_fanout_model.h"
#include "../core/knock_plan.h"
#include "census_svc.h"
#include "mesh_tx.h"
#include "fixture/src/core/fixture_context.h"
#include "fixture/src/core/packet.h"

namespace {

static constexpr uint32_t ROLL_STEP_MS = 80;
static portMUX_TYPE gLock = portMUX_INITIALIZER_UNLOCKED;
static ContagionFanoutGate gGate = {};
static uint8_t gQueue[CENSUS_MAX_TRACKED][3] = {};
static size_t gQueueLen = 0;
static size_t gQueueNext = 0;
static uint16_t gPulseMs = 40;
static uint32_t gNextSendMs = 0;
static uint32_t gUntilMs = 0;
static uint32_t gConfigSerial = 0;
static bool gPending = false;

static void disableLocked() {
  contagionFanoutGateInit(gGate);
  gQueueLen = 0;
  gQueueNext = 0;
  gNextSendMs = 0;
  gUntilMs = 0;
  gPending = false;
  ++gConfigSerial;
}
} // namespace

void contagionFanoutConfigure(const uint8_t source[3], uint32_t leaseMs,
                              uint16_t pulseMs) {
  if (!source || leaseMs == 0) {
    contagionFanoutDisable();
    return;
  }
  taskENTER_CRITICAL(&gLock);
  contagionFanoutGateConfigure(gGate, source, true);
  gQueueLen = 0;
  gQueueNext = 0;
  gPulseMs = pulseMs < 5 ? 5 : (pulseMs > 300 ? 300 : pulseMs);
  gNextSendMs = 0;
  gUntilMs = millis() + leaseMs;
  gPending = false;
  ++gConfigSerial;
  taskEXIT_CRITICAL(&gLock);
  Serial.printf("contagion legacy fanout armed: source=%02X%02X%02X pulse=%ums\n",
                source[0], source[1], source[2], (unsigned)gPulseMs);
}

void contagionFanoutDisable() {
  taskENTER_CRITICAL(&gLock);
  disableLocked();
  taskEXIT_CRITICAL(&gLock);
  Serial.println("contagion legacy fanout off");
}

void contagionFanoutObserve(const RxItem &item) {
  if (item.len < sizeof(NbChoreoState)) return;
  const NbHeader *header = (const NbHeader *)item.data;
  if (header->type != NB_CHOREO_STATE) return;
  const NbChoreoState *state = (const NbChoreoState *)item.data;
  taskENTER_CRITICAL(&gLock);
  if (contagionFanoutGateObserve(gGate, header->src_id, state->program_id,
                                 state->state))
    gPending = true;
  taskEXIT_CRITICAL(&gLock);
}

void contagionFanoutTick(uint32_t nowMs) {
  bool buildQueue = false;
  bool sendOne = false;
  bool expired = false;
  uint32_t configSerial = 0;
  uint8_t target[3] = {};
  uint16_t pulseMs = 40;

  taskENTER_CRITICAL(&gLock);
  if (gGate.enabled && gUntilMs && (int32_t)(nowMs - gUntilMs) >= 0) {
    disableLocked();
    expired = true;
  } else if (gPending && gQueueNext >= gQueueLen) {
    gPending = false;
    buildQueue = true;
    configSerial = gConfigSerial;
  } else if (gQueueNext < gQueueLen &&
             (gNextSendMs == 0 || (int32_t)(nowMs - gNextSendMs) >= 0)) {
    memcpy(target, gQueue[gQueueNext++], sizeof(target));
    pulseMs = gPulseMs;
    gNextSendMs = nowMs + ROLL_STEP_MS;
    sendOne = true;
  }
  taskEXIT_CRITICAL(&gLock);

  if (expired) {
    Serial.println("contagion legacy fanout expired");
    return;
  }

  if (buildQueue) {
    static CensusView rows[CENSUS_MAX_TRACKED];
    static uint8_t planned[CENSUS_MAX_TRACKED][3];
    size_t rowCount = censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, nowMs);
    size_t plannedCount = knockPlanFreshClass(
        rows, rowCount, censusFreshMsSafe(), FIXTURE_DOWNLIGHT, planned,
        CENSUS_MAX_TRACKED);
    taskENTER_CRITICAL(&gLock);
    if (gGate.enabled && gConfigSerial == configSerial) {
      memcpy(gQueue, planned, plannedCount * sizeof(gQueue[0]));
      gQueueLen = plannedCount;
      gQueueNext = 0;
      gNextSendMs = 0;
    }
    taskEXIT_CRITICAL(&gLock);
    Serial.printf("contagion legacy fanout: queued %u fresh downlights\n",
                  (unsigned)plannedCount);
    return;
  }

  if (sendOne) {
    meshStrike(target, pulseMs);
    taskENTER_CRITICAL(&gLock);
    bool finished = gQueueNext >= gQueueLen;
    size_t sent = gQueueNext;
    taskEXIT_CRITICAL(&gLock);
    if (finished)
      Serial.printf("contagion legacy fanout: sent %u targeted strikes\n",
                    (unsigned)sent);
  }
}
