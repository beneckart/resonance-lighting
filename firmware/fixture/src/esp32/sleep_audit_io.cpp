#include "sleep_audit_io.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_system.h"

#include "board_power.h"
#include "net_peer.h"
#include "nvs_store.h"

RTC_DATA_ATTR static SleepAuditRecord gRtcSleepAudit;
static SleepAuditRecord gWakeAudit;
static SleepAuditRecord gCommandAudit;
static SleepAuditRecord gProtectAudit;

static int16_t batteryMvForAudit() {
  float volts = batteryVolts();
  if (!isfinite(volts) || volts < 0.0f || volts > 32.0f) return INT16_MIN;
  return (int16_t)lroundf(volts * 1000.0f);
}

static SleepAuditRecord makeRecord(uint8_t cause, uint32_t durationS,
                                   const NbHeader *source) {
  return sleepAuditMake(cause, durationS, batteryMvForAudit(), gCfg.profile,
                        gNetLifeState, gNetPowerTier, millis(),
                        source ? source->src_id : nullptr,
                        source ? source->seq : 0,
                        source ? source->uptime_ms : 0);
}

static void printRecord(const char *label, const SleepAuditRecord &record) {
  if (!sleepAuditValid(record)) {
    Serial.printf("sleep-audit: %s=none\n", label);
    return;
  }
  Serial.printf(
      "sleep-audit: %s=%s duration=%lus batt=%dmV profile=%u life=%u tier=%u "
      "source=%02X%02X%02X seq=%lu source_up=%lu fixture_up=%lu\n",
      label, sleepCauseName(record.cause), (unsigned long)record.duration_s,
      record.batt_mv, record.profile, record.life_state, record.power_tier,
      record.source_id[0], record.source_id[1], record.source_id[2],
      (unsigned long)record.source_seq,
      (unsigned long)record.source_uptime_ms,
      (unsigned long)record.fixture_uptime_ms);
  ProtectAuditContext context;
  if (sleepAuditGetProtectContext(record, context)) {
    Serial.printf(
        "sleep-audit: %s protect-origin=%s predecessor=%u reset=%u "
        "load_armed=%u streak=%lu\n",
        label, protectOriginName(context.origin), context.predecessor_stage,
        context.reset_reason, context.load_armed,
        (unsigned long)context.reset_streak);
  }
}

void sleepAuditInit() {
  memset(&gWakeAudit, 0, sizeof(gWakeAudit));
  memset(&gCommandAudit, 0, sizeof(gCommandAudit));
  memset(&gProtectAudit, 0, sizeof(gProtectAudit));

  if (esp_reset_reason() == ESP_RST_DEEPSLEEP && sleepAuditValid(gRtcSleepAudit))
    gWakeAudit = gRtcSleepAudit;
  else
    memset(&gRtcSleepAudit, 0, sizeof(gRtcSleepAudit));

  if (!nvsReadSleepCommand(gCommandAudit))
    Serial.println("sleep-audit: command NVS read FAILED");
  if (!sleepAuditValid(gCommandAudit))
    memset(&gCommandAudit, 0, sizeof(gCommandAudit));

  if (!nvsReadProtectEntry(gProtectAudit))
    Serial.println("sleep-audit: protect NVS read FAILED");
  if (!sleepAuditValid(gProtectAudit))
    memset(&gProtectAudit, 0, sizeof(gProtectAudit));
  else {
    // Preserve pre-ADR-0068 entry voltage/profile/uptime while making the
    // absent reset provenance explicit. This is a one-time same-size NVS
    // migration; it never invents a cause for an old latch.
    ProtectAuditContext context;
    if (!sleepAuditGetProtectContext(gProtectAudit, context)) {
      context.origin = PROTECT_ORIGIN_LEGACY;
      context.predecessor_stage = 0xFF;
      context.reset_reason = 0xFF;
      if (sleepAuditSetProtectContext(gProtectAudit, context)) {
        if (nvsWriteProtectEntry(gProtectAudit))
          Serial.println("sleep-audit: annotated legacy PROTECT provenance");
        else
          Serial.println("sleep-audit: legacy PROTECT annotation persist FAILED");
      }
    }
  }

  printRecord("wake", gWakeAudit);
  printRecord("last-command", gCommandAudit);
  printRecord("last-protect", gProtectAudit);
}

bool sleepAuditBeforeSleep(uint8_t cause, uint32_t durationS,
                           const NbHeader *source) {
  SleepAuditRecord record = makeRecord(cause, durationS, source);
  if (!sleepAuditValid(record)) return false;

  if (sleepCauseIsOperator(cause)) {
    if (!nvsWriteSleepCommand(record)) {
      Serial.println("sleep-audit: command persist FAILED; refusing sleep");
      return false;
    }
    gCommandAudit = record;
  }

  // This write is to RTC slow memory, not flash. It is intentionally refreshed
  // on every automatic day/protection sleep so the next wake names its cause.
  gRtcSleepAudit = record;
  return true;
}

bool sleepAuditRecordProtectEntry(uint32_t durationS,
                                  const ProtectAuditContext *context) {
  SleepAuditRecord record =
      makeRecord(SLEEP_CAUSE_POWER_PROTECT, durationS, nullptr);
  ProtectAuditContext fallback = {};
  if (!context) {
    fallback.origin = PROTECT_ORIGIN_LEGACY;
    fallback.predecessor_stage = 0xFF;
    fallback.reset_reason = 0xFF;
    context = &fallback;
  }
  if (!sleepAuditSetProtectContext(record, *context)) {
    Serial.println("sleep-audit: protect context encode FAILED");
    return false;
  }
  if (!nvsWriteProtectEntry(record)) {
    Serial.println("sleep-audit: protect entry persist FAILED");
    return false;
  }
  gProtectAudit = record;
  return true;
}

bool sleepAuditWakeRecord(SleepAuditRecord &out) {
  out = gWakeAudit;
  return sleepAuditValid(out);
}

bool sleepAuditCommandRecord(SleepAuditRecord &out) {
  out = gCommandAudit;
  return sleepAuditValid(out);
}

bool sleepAuditProtectRecord(SleepAuditRecord &out) {
  out = gProtectAudit;
  return sleepAuditValid(out);
}

bool sleepAuditHasProtectRecord() { return sleepAuditValid(gProtectAudit); }
