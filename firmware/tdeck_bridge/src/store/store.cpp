#include "store.h"

#include <Preferences.h>
#include <string.h>

static Preferences gPrefs;
static Settings gSettings;

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
