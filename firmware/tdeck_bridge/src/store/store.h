#pragma once

#include <stdint.h>

#include "../core/action_audit.h"

// NVS-backed settings (namespace "tdeck"). No secret is compiled in or
// committed — provisioning is the serial CLI only (ADR 0037).
struct Settings {
  char ssid[33];
  char psk[65];
  char apiKey[128];
  char model[48];
  uint8_t channel;    // mesh channel; 11 is the commissioned fleet value
  uint8_t backlight;  // 0..255
};

void storeBegin();
Settings &settings();          // in-RAM copy, loaded at boot
void storeSave();              // persist the in-RAM copy
bool storeHasWifi();
bool storeHasApiKey();

// Four-entry NVS-backed ring of power/lifecycle operator actions. Commands that
// can make the fleet unavailable persist their exact mesh sequence before RF.
bool storeRecordAction(uint8_t action, uint32_t value, uint32_t meshSeq,
                       uint32_t bridgeUptimeMs, const uint8_t targetId[3],
                       bool utcValid, uint32_t utcS);
bool storeLatestAction(ActionAuditRecord &out);
bool storeActionAtNewestOffset(uint8_t offset, ActionAuditRecord &out);
uint8_t storeActionCount();
void storePrintActions();
