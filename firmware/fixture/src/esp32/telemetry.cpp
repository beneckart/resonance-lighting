#include "telemetry.h"

#include "esp_ota_ops.h"
#include "esp_system.h"

#include "../core/fixture_context.h"
#include "../core/version.h"
#include "behavior_glue.h"
#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "led_driver.h"
#include "maintenance.h"
#include "motion_trace.h"
#include "net_peer.h"
#include "nvs_store.h"
#include "sensors/sensor_bus.h"
#include "sensors/sensors.h"
#include "sentinel_trace.h"
#include "solenoid.h"
#include "sleep_audit_io.h"

// Deliberately TRIMMED vs net_bench (Ben, 2026-07-30: brevity over compat).
// Dropped: INA219 channels (instrument out of circuit), drawdown_* and mppt_*
// campaign blocks, TSL2591/SHT31 env, scan-AP. The 11 keys fleet_usb_bringup
// asserts stay byte-identical in name and type:
//   fixture_id fw pf_ready flash_bytes psram_bytes battery_type
//   battery_capacity_mah charge_limit_ma maintain_v battery_present
//   charging_enabled

// Wired to real state in later phases; declared here so the JSON shape is
// stable from the first flash.
uint8_t gTelemetryLifeState = 0;   // lifecycle enum (P5)
uint8_t gTelemetryPowerTier = 0;   // LedTier (P3)
uint8_t gTelemetryProgram = 0;     // choreo program id (P5)
uint8_t gTelemetryGuardStage = 0;  // boot-guard stage
bool gTelemetryGuardInterrupted = false;
uint8_t gTelemetryFixtureClass = FIXTURE_UNKNOWN; // probe+override decision
bool gTelemetryClassMismatch = false;
uint8_t gTelemetrySensorBits = 0;

static const char *otaStateName(esp_ota_img_states_t state) {
  switch (state) {
  case ESP_OTA_IMG_NEW: return "new";
  case ESP_OTA_IMG_PENDING_VERIFY: return "pending_verify";
  case ESP_OTA_IMG_VALID: return "valid";
  case ESP_OTA_IMG_INVALID: return "invalid";
  case ESP_OTA_IMG_ABORTED: return "aborted";
  default: return "undefined";
  }
}

static uint8_t effectiveClass() {
  if (gTelemetryFixtureClass != FIXTURE_UNKNOWN) return gTelemetryFixtureClass;
  if (gCfg.classOvr != FIXTURE_UNKNOWN) return gCfg.classOvr;
  return gCfg.classLast;
}

String telemetryBanner() {
  return String("fixture " RES_FIXTURE_VERSION "\n");
}

String telemetryJson() {
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t otaState = ESP_OTA_IMG_UNDEFINED;
  esp_ota_get_state_partition(running, &otaState);

  String j = "{";
  j += "\"board\":\"" RES_BOARD_NAME "\"";
  j += ",\"fw\":\"" RES_FIXTURE_VERSION "\"";
  j += ",\"fixture_id\":\"" + gShortId + "\"";
  j += ",\"role\":\"peer\"";
  j += ",\"mode\":";
  j += (int)maintMode();
  j += ",\"uptime_ms\":" + String((unsigned long)millis());
  j += ",\"heap_free\":" + String(ESP.getFreeHeap());
  j += ",\"flash_bytes\":" + String((unsigned long)ESP.getFlashChipSize());
  j += ",\"psram_bytes\":" + String((unsigned long)ESP.getPsramSize());
  j += ",\"reset_reason\":\"" + String(resetReasonName(esp_reset_reason())) + "\"";
  j += ",\"ota_partition\":\"";
  j += running ? running->label : "unknown";
  j += "\"";
  j += ",\"ota_address\":";
  j += running ? String((unsigned long)running->address) : "0";
  j += ",\"ota_state\":\"" + String(otaStateName(otaState)) + "\"";
  j += ",\"pf_ready\":";
  j += pfIsReady() ? "true" : "false";
  j += ",\"battery_present\":";
  j += batteryPresent() ? "true" : "false";
  j += ",\"charging_enabled\":";
  j += chargingEnabled() ? "true" : "false";
  j += ",\"maint_status\":";
  j += String(maintStatus());
  j += ",\"battery_type\":\"" + String(batteryTypeName()) + "\"";
  j += ",\"battery_capacity_mah\":" + String(gCfg.capMah);
  j += ",\"charge_limit_ma\":" + String(gCfg.chargeMa);
  j += ",\"precharge_target_ma\":" + String(prechargeTargetMa());
  j += ",\"precharge_configured\":";
  j += prechargeConfigured() ? "true" : "false";
  j += ",\"deep_recovery_build\":";
  j += deepRecoveryBuild() ? "true" : "false";
  j += ",\"deep_recovery_target_match\":";
  j += deepRecoveryTargetMatches() ? "true" : "false";
  j += ",\"deep_recovery_charge_active\":";
  j += deepRecoveryChargeActive() ? "true" : "false";
  j += ",\"low_vbat_recovery_state\":" + String(lowVbatRecoveryState());
  j += ",\"low_vbat_recovery_detect_mv\":" + String(lowVbatRecoveryDetectMv());
  j += ",\"maintain_v\":" + String(maintainVolts(), 1);
  if (pfIsReady()) {
    char b[24];
    snprintf(b, sizeof(b), "%.3f", batteryVolts());
    j += ",\"battery_v\":" + String(b);
    snprintf(b, sizeof(b), "%.1f", batteryMa());
    j += ",\"battery_ma\":" + String(b);
    snprintf(b, sizeof(b), "%.1f", batteryMaRaw());
    j += ",\"battery_ma_raw\":" + String(b);
    j += ",\"battery_current_divisor\":" + String(RES_GAUGE_CURRENT_DIVISOR, 2);
    if (batterySocPct() >= 0) j += ",\"soc_pct\":" + String(batterySocPct());
    snprintf(b, sizeof(b), "%.3f", supplyVolts());
    j += ",\"supply_v\":" + String(b);
    snprintf(b, sizeof(b), "%.1f", supplyMa());
    j += ",\"supply_ma\":" + String(b);
    j += ",\"supply_good\":";
    j += supplyGood() ? "true" : "false";
    const BqSnapshot &bq = bqSnapshot();
    j += ",\"bq_vindpm_mv\":" + String(bq.vindpm_mv);
    j += ",\"bq_ichg_ma\":" + String(bq.ichg_ma);
    j += ",\"bq_vreg_mv\":" + String(bq.vreg_mv);
    j += ",\"bq_precharge_ma\":" + String(bq.precharge_ma);
    j += ",\"bq_reg10\":" + String(bq.reg10);
    j += ",\"bq_reg16\":" + String(bq.reg16);
    j += ",\"bq_reg18\":" + String(bq.reg18);
    j += ",\"bq_stat0\":" + String(bq.stat0);
    j += ",\"bq_stat1\":" + String(bq.stat1);
    j += ",\"bq_fault0\":" + String(bq.fault0);
    j += ",\"bq_flag0\":" + String(bq.flag0);
    j += ",\"bq_flag1\":" + String(bq.flag1);
    j += ",\"bq_fault_flag0\":" + String(bq.fault_flag0);
    j += ",\"bq_part\":" + String(bq.part);
  }
  j += ",\"fixture_class\":\"" + String(fixtureClassName(effectiveClass())) + "\"";
  j += ",\"class_ovr\":" + String(gCfg.classOvr);
  j += ",\"class_mismatch\":";
  j += gTelemetryClassMismatch ? "true" : "false";
  j += ",\"sensor_bits\":" + String(gTelemetrySensorBits);
  j += ",\"sam_m8q_present\":";
  j += (gTelemetrySensorBits & 0x10) ? "true" : "false";
  j += ",\"ds3231_present\":";
  j += (gTelemetrySensorBits & 0x20) ? "true" : "false";
  uint32_t rtcUtcS = 0;
  bool rtcValid = (gTelemetrySensorBits & 0x20) &&
                  sensorBusReadRtcUtc(rtcUtcS);
  j += ",\"rtc_valid\":";
  j += rtcValid ? "true" : "false";
  if (rtcValid)
    j += ",\"rtc_utc_s\":" + String((unsigned long)rtcUtcS);
  j += ",\"profile\":\"";
  j += (gCfg.profile == PROFILE_DEV) ? "commission" : "field";
  j += "\"";
  j += ",\"commission_default\":\"";
  j += commissionDefaultName(gCfg.commissionDefault);
  j += "\"";
  j += ",\"channel\":" + String(gCfg.channel);
  j += ",\"espnow_up\":";
  j += espNowUp() ? "true" : "false";
  j += ",\"comms_init_attempts\":" + String((unsigned long)commsInitAttempts());
  j += ",\"comms_init_failures\":" + String((unsigned long)commsInitFailures());
  j += ",\"led_rail_on\":";
  j += ledRailIsOn() ? "true" : "false";
  j += ",\"smoke_render\":";
  j += gSmokeRender ? "true" : "false";
  j += ",\"bench_rail_forced_off\":";
  j += gBenchRailForcedOff ? "true" : "false";
  j += ",\"life_state\":" + String(gTelemetryLifeState);
  j += ",\"day_sleep_s\":" + String(behaviorDaySleepS());
  j += ",\"wake_listen_ms\":" +
       String((unsigned long)behaviorWakeListenMs());
  j += ",\"power_tier\":" + String(gTelemetryPowerTier);
  j += ",\"active_program\":" + String(gTelemetryProgram);
  j += ",\"direct_seen\":" + String((unsigned long)netPeerDirectSeen());
  j += ",\"direct_matched\":" + String((unsigned long)netPeerDirectMatched());
  j += ",\"guard_stage\":" + String(gTelemetryGuardStage);
  j += ",\"guard_interrupted\":";
  j += gTelemetryGuardInterrupted ? "true" : "false";
  SleepAuditRecord sleepRecord;
  bool hasSleepRecord = sleepAuditWakeRecord(sleepRecord);
  j += ",\"last_sleep_valid\":";
  j += hasSleepRecord ? "true" : "false";
  if (hasSleepRecord) {
    char source[7];
    snprintf(source, sizeof(source), "%02X%02X%02X", sleepRecord.source_id[0],
             sleepRecord.source_id[1], sleepRecord.source_id[2]);
    j += ",\"last_sleep_reason\":\"" + String(sleepCauseName(sleepRecord.cause)) + "\"";
    j += ",\"last_sleep_s\":" + String((unsigned long)sleepRecord.duration_s);
    j += ",\"last_sleep_batt_mv\":" + String(sleepRecord.batt_mv);
    j += ",\"last_sleep_profile\":" + String(sleepRecord.profile);
    j += ",\"last_sleep_life_state\":" + String(sleepRecord.life_state);
    j += ",\"last_sleep_power_tier\":" + String(sleepRecord.power_tier);
    j += ",\"last_sleep_source\":\"" + String(source) + "\"";
    j += ",\"last_sleep_source_seq\":" +
         String((unsigned long)sleepRecord.source_seq);
  }
  bool hasCommandRecord = sleepAuditCommandRecord(sleepRecord);
  j += ",\"last_operator_sleep_valid\":";
  j += hasCommandRecord ? "true" : "false";
  if (hasCommandRecord) {
    char source[7];
    snprintf(source, sizeof(source), "%02X%02X%02X", sleepRecord.source_id[0],
             sleepRecord.source_id[1], sleepRecord.source_id[2]);
    j += ",\"last_operator_sleep_reason\":\"" +
         String(sleepCauseName(sleepRecord.cause)) + "\"";
    j += ",\"last_operator_sleep_s\":" +
         String((unsigned long)sleepRecord.duration_s);
    j += ",\"last_operator_sleep_source\":\"" + String(source) + "\"";
    j += ",\"last_operator_sleep_source_seq\":" +
         String((unsigned long)sleepRecord.source_seq);
  }
  bool hasProtectRecord = sleepAuditProtectRecord(sleepRecord);
  j += ",\"last_protect_entry_valid\":";
  j += hasProtectRecord ? "true" : "false";
  if (hasProtectRecord) {
    j += ",\"last_protect_entry_batt_mv\":" + String(sleepRecord.batt_mv);
    j += ",\"last_protect_entry_profile\":" + String(sleepRecord.profile);
    j += ",\"last_protect_entry_life_state\":" + String(sleepRecord.life_state);
    j += ",\"last_protect_entry_fixture_uptime_ms\":" +
         String((unsigned long)sleepRecord.fixture_uptime_ms);
    ProtectAuditContext protectContext;
    bool hasProtectContext =
        sleepAuditGetProtectContext(sleepRecord, protectContext);
    j += ",\"last_protect_entry_context_valid\":";
    j += hasProtectContext ? "true" : "false";
    if (hasProtectContext) {
      j += ",\"last_protect_entry_origin\":\"" +
           String(protectOriginName(protectContext.origin)) + "\"";
      j += ",\"last_protect_entry_origin_code\":" +
           String(protectContext.origin);
      j += ",\"last_protect_entry_predecessor_stage\":" +
           String(protectContext.predecessor_stage);
      j += ",\"last_protect_entry_reset_reason\":" +
           String(protectContext.reset_reason);
      j += ",\"last_protect_entry_load_armed\":" +
           String(protectContext.load_armed);
      j += ",\"last_protect_entry_reset_streak\":" +
           String((unsigned long)protectContext.reset_streak);
    }
  }
  j += ",\"ota_pending_verify\":";
  j += (otaState == ESP_OTA_IMG_PENDING_VERIFY) ? "true" : "false";
  j += ",\"motion_trace_build\":";
  j += motionTraceBuild() ? "true" : "false";
  if (motionTraceBuild()) {
    char target[7];
    snprintf(target, sizeof(target), "%06lX",
             (unsigned long)motionTraceTargetId());
    j += ",\"motion_trace_target\":\"" + String(target) + "\"";
    j += ",\"motion_trace_target_match\":";
    j += motionTraceTargetMatches() ? "true" : "false";
    j += ",\"motion_trace_capacity\":" +
         String((unsigned long)motionTraceCapacity());
    j += ",\"motion_trace_count\":" +
         String((unsigned long)motionTraceCount());
    j += ",\"motion_trace_overwrites\":" +
         String((unsigned long)motionTraceOverwrites());
  }
  j += ",\"sentinel_trace_build\":";
  j += sentinelTraceBuild() ? "true" : "false";
  if (sentinelTraceBuild()) {
    char target[7];
    snprintf(target, sizeof(target), "%06lX",
             (unsigned long)sentinelTraceTargetId());
    j += ",\"sentinel_trace_target\":\"" + String(target) + "\"";
    j += ",\"sentinel_trace_target_match\":";
    j += sentinelTraceTargetMatches() ? "true" : "false";
    j += ",\"sentinel_trace_phase\":\"" +
         String(sentinelTracePhaseName(sentinelTracePhase())) + "\"";
    j += ",\"sentinel_trace_capacity\":" +
         String((unsigned long)sentinelTraceCapacity());
    j += ",\"sentinel_trace_count\":" +
         String((unsigned long)sentinelTraceCount());
    j += ",\"sentinel_trace_overwrites\":" +
         String((unsigned long)sentinelTraceOverwrites());
  }
  const SensorSnapshot &sn = sensors();
  j += ",\"msa311_present\":";
  j += sn.msaPresent ? "true" : "false";
  if (sn.msaPresent) {
    j += ",\"msa_read_ok\":";
    j += sn.msaOk ? "true" : "false";
    j += ",\"msa_reads\":" + String((unsigned long)sn.msaReads);
    j += ",\"tilt_deg\":" + String(sn.tiltDeg, 2);
    j += ",\"sway_env_g\":" + String(sn.swayEnvG, 4);
  }
  j += ",\"tmf8820_present\":";
  j += sn.tmfPresent ? "true" : "false";
  if (sn.tmfPresent) {
    j += ",\"tmf_read_ok\":";
    j += sn.tmfOk ? "true" : "false";
    j += ",\"tof_depth_mm\":" + String(sn.tofDepthMm);
    j += ",\"tof_depth_filtered_mm\":" + String(sn.tofDepthFilteredMm, 1);
    j += ",\"tof_confidence\":" + String(sn.tofConfidence);
    j += ",\"tmf_reads\":" + String((unsigned long)sn.tmfReads);
    j += ",\"tmf_errors\":" + String((unsigned long)sn.tmfErrors);
    j += ",\"tmf_recoveries\":" + String((unsigned long)sn.tmfRecoveries);
  }
  j += ",\"tmf_domain_resets\":" + String((unsigned)sn.tmfDomainResets);
  j += ",\"vl53l5cx_present\":";
  j += sn.vlPresent ? "true" : "false";
  if (sn.vlPresent) {
    j += ",\"vl_read_ok\":";
    j += sn.vlOk ? "true" : "false";
    j += ",\"vl_tilt_deg\":" + String(sn.vlTiltDeg, 2);
    j += ",\"vl_zones\":" + String(sn.vlZones);
    j += ",\"vl_closest_mm\":" + String(sn.vlClosestMm);
    j += ",\"vl_reads\":" + String((unsigned long)sn.vlReads);
    j += ",\"vl_near_zones\":" + String(sn.vlNearZones);
    j += ",\"vl_target_zones\":" + String(sn.vlTargetZones);
    j += ",\"vl_valid_zones\":" + String(sn.vlValidZones);
    j += ",\"vl_zone_nearest_mm\":[";
    for (uint8_t zone = 0; zone < 16; ++zone) {
      if (zone) j += ',';
      j += String(sn.vlZoneNearestMm[zone]);
    }
    j += ']';
  }
  j += ",\"bmp581_present\":";
  j += sn.bmpPresent ? "true" : "false";
  if (sn.bmpPresent) {
    j += ",\"bmp_read_ok\":";
    j += sn.bmpOk ? "true" : "false";
    j += ",\"bmp_temp_c\":" + String(sn.tempC, 2);
    j += ",\"bmp_pressure_hpa\":" + String(sn.pressureHpa, 2);
  }
  j += ",\"solenoid_enabled\":";
  j += gCfg.solEn ? "true" : "false";
  j += ",\"solenoid_gate_on\":";
  j += solenoidGateOn() ? "true" : "false";
  j += ",\"solenoid_strikes\":" + String((unsigned long)solenoidStrikes());
  j += ",\"solenoid_blocked\":" + String((unsigned long)solenoidBlocked());
  j += ",\"solenoid_failsafes\":" + String((unsigned long)solenoidFailsafes());
  j += ",\"solenoid_last_ms\":" + String(solenoidLastPulseMs());
  j += "}";
  return j;
}
