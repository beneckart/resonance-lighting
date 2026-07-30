#include "serial_cli.h"

#include <Arduino.h>

#include "../core/fixture_context.h"
#include "../core/packet.h"
#include "behavior_glue.h"
#include "board_power.h"
#include "espnow_link.h"
#include "identity.h"
#include "led_driver.h"
#include "maintenance.h"
#include "nvs_store.h"
#include "solenoid.h"
#include "telemetry.h"

#define RES_REMOTE_SLEEP_S 21600 // 6 h: wait for sun without losing recoverability

static int hexNibble(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static int readSerialUint(uint32_t windowMs, uint32_t maxValue) {
  int value = -1;
  uint32_t deadline = millis() + windowMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int p = Serial.peek();
    if (p < '0' || p > '9') break;
    Serial.read();
    value = (value < 0 ? 0 : value * 10) + (p - '0');
    if ((uint32_t)value > maxValue) return -1;
    deadline = millis() + 20;
  }
  return value;
}

static size_t readSerialArg(char *out, size_t outLen, uint32_t windowMs) {
  size_t n = 0;
  bool saw = false;
  uint32_t deadline = millis() + windowMs;
  while ((int32_t)(deadline - millis()) > 0) {
    if (!Serial.available()) { delay(1); continue; }
    int p = Serial.peek();
    if (p == '\r' || p == '\n' || p == ' ' || p == '\t') {
      Serial.read();
      if (saw) break;
      continue;
    }
    Serial.read();
    saw = true;
    if (n + 1 < outLen) out[n++] = (char)p;
    deadline = millis() + 20;
  }
  if (outLen > 0) out[n] = 0;
  return n;
}

static bool parseHexIdText(const char *s, uint8_t out[3]) {
  if (strlen(s) != 6) return false;
  uint8_t nib[6];
  for (int i = 0; i < 6; i++) {
    int h = hexNibble(s[i]);
    if (h < 0) return false;
    nib[i] = (uint8_t)h;
  }
  out[0] = (nib[0] << 4) | nib[1];
  out[1] = (nib[2] << 4) | nib[3];
  out[2] = (nib[4] << 4) | nib[5];
  return true;
}

static bool parseUint16Text(const char *s, uint16_t minValue, uint16_t maxValue, uint16_t *out) {
  if (!s || !*s) return false;
  uint32_t value = 0;
  for (const char *p = s; *p; p++) {
    if (*p < '0' || *p > '9') return false;
    value = value * 10 + (uint32_t)(*p - '0');
    if (value > maxValue) return false;
  }
  if (value < minValue) return false;
  *out = (uint16_t)value;
  return true;
}

static bool parseTargetU16Arg(const char *arg, uint8_t target[3], bool *haveTarget,
                              uint16_t minValue, uint16_t maxValue, uint16_t *value) {
  *haveTarget = false;
  const char *sep = strchr(arg, ':');
  if (!sep) sep = strchr(arg, ',');
  if (!sep) sep = strchr(arg, '=');
  if (!sep) return parseUint16Text(arg, minValue, maxValue, value);

  size_t idLen = (size_t)(sep - arg);
  if (idLen != 6) return false;
  char id[7];
  memcpy(id, arg, 6);
  id[6] = 0;
  if (!parseHexIdText(id, target)) return false;
  if (!parseUint16Text(sep + 1, minValue, maxValue, value)) return false;
  *haveTarget = true;
  return true;
}

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
  case 't':
    Serial.println(telemetryJson());
    break;
  case 'u':
    // Commissioning path: a USB-connected peer proves shared-WiFi OTA without
    // a separately flashed ESP-NOW bridge. Fleet maintenance still arrives via
    // the bridge's sustained U command over ESP-NOW.
    if (maintMode() == MODE_COMMS) {
      Serial.println("local USB ENTER_MAINT");
      enterMaintenance();
    }
    break;
  case 'c':
    if (maintMode() == MODE_MAINT) enterComms();
    break;
  case 'C': {
    char arg[24];
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = false;
    uint16_t mah = 0;
    if (readSerialArg(arg, sizeof(arg), 100) == 0) {
      Serial.printf("config: cap=%u mAh charge=%u mA\n", gCfg.capMah, gCfg.chargeMa);
      break;
    }
    if (!parseTargetU16Arg(arg, target, &haveTarget, RES_CAPACITY_MIN_MAH,
                           RES_CAPACITY_MAX_MAH, &mah)) {
      Serial.printf("SET_CAPACITY rejected: use C<mah> or C<id>:<mah> (range %u..%u mAh)\n",
                    (unsigned)RES_CAPACITY_MIN_MAH, (unsigned)RES_CAPACITY_MAX_MAH);
      break;
    }
    if (haveTarget && !nbTargetMatches(target, gMyId)) break;
    applyCapacityAndReboot(mah);
    break;
  }
  case 'G': {
    char arg[24];
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = false;
    uint16_t ma = 0;
    if (readSerialArg(arg, sizeof(arg), 100) == 0) {
      Serial.printf("config: cap=%u mAh charge=%u mA\n", gCfg.capMah, gCfg.chargeMa);
      break;
    }
    if (!parseTargetU16Arg(arg, target, &haveTarget, RES_CHARGE_MIN_MA,
                           RES_CHARGE_MAX_MA, &ma)) {
      Serial.printf("SET_CHARGE_MA rejected: use G<mA> or G<id>:<mA> (range %u..%u mA)\n",
                    (unsigned)RES_CHARGE_MIN_MA, (unsigned)RES_CHARGE_MAX_MA);
      break;
    }
    if (haveTarget && !nbTargetMatches(target, gMyId)) break;
    applyChargeMa(ma);
    break;
  }
  case 'K': {
    char arg[24];
    uint8_t target[3] = {0, 0, 0};
    bool haveTarget = false;
    uint16_t pulseMs = RES_SOLENOID_DEFAULT_MS;
    if (readSerialArg(arg, sizeof(arg), 100) == 0 ||
        !parseTargetU16Arg(arg, target, &haveTarget, RES_SOLENOID_MIN_MS,
                           RES_SOLENOID_MAX_MS, &pulseMs) ||
        !haveTarget) {
      Serial.printf("SOLENOID rejected: use K<id>:<ms> (range %u..%u ms)\n",
                    (unsigned)RES_SOLENOID_MIN_MS, (unsigned)RES_SOLENOID_MAX_MS);
      break;
    }
    if (memcmp(target, gMyId, 3) == 0) solenoidStrike(pulseMs, "serial");
    break;
  }
  case 'S': {
    int seconds = readSerialUint(80, 65535);
    uint16_t sleepS = (uint16_t)(seconds < 0 ? RES_REMOTE_SLEEP_S : seconds);
    if (sleepS == 0) {
      Serial.println("SLEEP rejected (seconds must be 1..65535)");
      break;
    }
    enterTimedDeepSleep(sleepS, "serial");
    break;
  }
  case 'O': {
    int cls = readSerialUint(80, 4);
    if (cls < 0) {
      Serial.printf("class_ovr=%u (%s) class_last=%u (%s)\n", gCfg.classOvr,
                    fixtureClassName(gCfg.classOvr), gCfg.classLast,
                    fixtureClassName(gCfg.classLast));
      break;
    }
    if (nvsPersistClassOvr((uint8_t)cls))
      Serial.printf("class override -> %u (%s); reboot to re-profile LEDs\n",
                    (unsigned)cls, fixtureClassName((uint8_t)cls));
    break;
  }
  case 'N': { // demo lifecycle override: N1 force night, N0 force day, N2 auto
    int v = readSerialUint(80, 2);
    if (v < 0) {
      Serial.printf("force_night=%d life_state=%u\n", behaviorForcedNight(),
                    behaviorLifeState());
      break;
    }
    behaviorForceNight(v == 2 ? -1 : (int8_t)v);
    Serial.printf("force_night -> %s\n", v == 2 ? "auto" : (v ? "night" : "day"));
    break;
  }
  case 'L': { // bench smoke render: L1 on, L0 off, bare L reports
    int on = readSerialUint(80, 1);
    if (on < 0) {
      Serial.printf("smoke=%d rail=%d pixels=%u\n", gSmokeRender ? 1 : 0,
                    ledRailIsOn() ? 1 : 0, (unsigned)ledPixelCount());
      break;
    }
    gSmokeRender = (on == 1);
    Serial.printf("smoke render -> %d\n", on);
    break;
  }
  case 'F': {
    int p = readSerialUint(80, 1);
    if (p < 0) {
      Serial.printf("profile=%s\n", gCfg.profile == PROFILE_DEV ? "dev" : "prod");
      break;
    }
    if (nvsPersistProfile((uint8_t)p))
      Serial.printf("profile -> %s\n", p == PROFILE_DEV ? "dev" : "prod");
    break;
  }
  case 'r':
    Serial.printf("role=peer mode=%d ch=%d cap=%u charge=%umA class=%s profile=%s "
                  "txok=%lu txfail=%lu\n",
                  (int)maintMode(), gCfg.channel, gCfg.capMah, gCfg.chargeMa,
                  fixtureClassName(gCfg.classOvr ? gCfg.classOvr : gCfg.classLast),
                  gCfg.profile == PROFILE_DEV ? "dev" : "prod",
                  (unsigned long)espNowSendOk(), (unsigned long)espNowSendFail());
    break;
  default:
    break;
  }
}
