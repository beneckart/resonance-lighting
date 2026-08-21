#pragma once

#include <stdint.h>

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
