#include "store.h"

#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

static Preferences gPrefs;
static Settings gSettings;
static ActionAuditLog gActionAudit;
static portMUX_TYPE gActionAuditMux = portMUX_INITIALIZER_UNLOCKED;
static const char *kActionAuditKey = "act_audit";

static void loadStr(const char *key, char *dst, size_t cap, const char *dflt) {
  String v = gPrefs.getString(key, dflt);
  strlcpy(dst, v.c_str(), cap);
}

void storeBegin() {
  gPrefs.begin("tdeck", false);
  loadStr("ssid", gSettings.ssid, sizeof(gSettings.ssid), "");
  loadStr("psk", gSettings.psk, sizeof(gSettings.psk), "");
  loadStr("key", gSettings.apiKey, sizeof(gSettings.apiKey), "");
  loadStr("model", gSettings.model, sizeof(gSettings.model), "claude-sonnet-5");
  gSettings.channel = (uint8_t)gPrefs.getUChar("ch", 11);
  if (gSettings.channel < 1 || gSettings.channel > 13) gSettings.channel = 11;
  gSettings.backlight = (uint8_t)gPrefs.getUChar("bl", 200);

  bool loaded = gPrefs.getBytesLength(kActionAuditKey) == sizeof(gActionAudit) &&
                gPrefs.getBytes(kActionAuditKey, &gActionAudit,
                                sizeof(gActionAudit)) == sizeof(gActionAudit) &&
                actionAuditValid(gActionAudit);
  if (!loaded) actionAuditInit(gActionAudit);
  ActionAuditRecord latest;
  if (actionAuditNewest(gActionAudit, latest))
    Serial.printf("action-audit: restored %u entries; last=%s value=%lu seq=%lu "
                  "utc=%lu flags=%02X\n",
                  gActionAudit.count, actionAuditName(latest.action),
                  (unsigned long)latest.value, (unsigned long)latest.mesh_seq,
                  (unsigned long)latest.utc_s, latest.flags);
  else
    Serial.println("action-audit: empty");
}

Settings &settings() { return gSettings; }

void storeSave() {
  gPrefs.putString("ssid", gSettings.ssid);
  gPrefs.putString("psk", gSettings.psk);
  gPrefs.putString("key", gSettings.apiKey);
  gPrefs.putString("model", gSettings.model);
  gPrefs.putUChar("ch", gSettings.channel);
  gPrefs.putUChar("bl", gSettings.backlight);
}

bool storeHasWifi() { return gSettings.ssid[0] != 0; }
bool storeHasApiKey() { return gSettings.apiKey[0] != 0; }

bool storeRecordAction(uint8_t action, uint32_t value, uint32_t meshSeq,
                       uint32_t bridgeUptimeMs, const uint8_t targetId[3],
                       bool utcValid, uint32_t utcS) {
  ActionAuditLog next;
  portENTER_CRITICAL(&gActionAuditMux);
  next = gActionAudit;
  portEXIT_CRITICAL(&gActionAuditMux);
  uint8_t flags = utcValid ? ACTION_AUDIT_UTC_VALID : 0;
  if (!actionAuditAppend(next, action, value, utcS, bridgeUptimeMs, meshSeq,
                         targetId, flags))
    return false;
  Preferences auditPrefs;
  if (!auditPrefs.begin("tdeck", false)) return false;
  bool persisted =
      auditPrefs.putBytes(kActionAuditKey, &next, sizeof(next)) == sizeof(next);
  auditPrefs.end();
  if (!persisted) return false;
  portENTER_CRITICAL(&gActionAuditMux);
  gActionAudit = next;
  portEXIT_CRITICAL(&gActionAuditMux);
  return true;
}

bool storeLatestAction(ActionAuditRecord &out) {
  return storeActionAtNewestOffset(0, out);
}

bool storeActionAtNewestOffset(uint8_t offset, ActionAuditRecord &out) {
  ActionAuditLog snapshot;
  portENTER_CRITICAL(&gActionAuditMux);
  snapshot = gActionAudit;
  portEXIT_CRITICAL(&gActionAuditMux);
  return actionAuditNewestOffset(snapshot, offset, out);
}

uint8_t storeActionCount() {
  portENTER_CRITICAL(&gActionAuditMux);
  uint8_t count = gActionAudit.count;
  portEXIT_CRITICAL(&gActionAuditMux);
  return count;
}

void storePrintActions() {
  uint8_t count = storeActionCount();
  Serial.printf("action-audit: %u retained (newest first)\n", count);
  for (uint8_t i = 0; i < count; ++i) {
    ActionAuditRecord record;
    if (!storeActionAtNewestOffset(i, record)) continue;
    Serial.printf(
        " action[%u] %s value=%lu seq=%lu bridge_up=%lu utc=%lu flags=%02X "
        "target=%02X%02X%02X\n",
        i, actionAuditName(record.action), (unsigned long)record.value,
        (unsigned long)record.mesh_seq,
        (unsigned long)record.bridge_uptime_ms,
        (unsigned long)record.utc_s, record.flags, record.target_id[0],
        record.target_id[1], record.target_id[2]);
  }
}
