#pragma once

#include <Arduino.h>
#include <PowerFeather.h>
#include <Wire.h>
#include <math.h>

// Resonance PowerFeather solar-input guard.
//
// Bench root cause, 2026-06: a bright-sun panel connect can leave the BQ25628E
// sitting at panel Voc with supply_good=false and zero input current until VBUS
// is fully removed. Standard practice for any Resonance firmware that enables
// PowerFeather solar/battery charging:
//   1. force BQ REG0x17[0] VBUS_OVP=1 at boot (wide input OVP);
//   2. if a panel is present but not accepted, toggle EN_HIZ to re-run input
//      qualification without requiring a physical unplug.

#ifndef PF_SOLAR_GUARD_BQ_ADDR
#define PF_SOLAR_GUARD_BQ_ADDR 0x6A
#endif

#define PF_SOLAR_GUARD_REG_CHG_CTRL0 0x16
#define PF_SOLAR_GUARD_REG_CHG_CTRL1 0x17
#define PF_SOLAR_GUARD_EN_HIZ_BIT 4
#define PF_SOLAR_GUARD_VBUS_OVP_BIT 0

#ifndef PF_SOLAR_GUARD_SUSPECT_V
#define PF_SOLAR_GUARD_SUSPECT_V 5.6f
#endif
#ifndef PF_SOLAR_GUARD_ZERO_MA
#define PF_SOLAR_GUARD_ZERO_MA 15.0f
#endif
#ifndef PF_SOLAR_GUARD_HOLD_MS
#define PF_SOLAR_GUARD_HOLD_MS 8000UL
#endif
#ifndef PF_SOLAR_GUARD_COOLDOWN_MS
#define PF_SOLAR_GUARD_COOLDOWN_MS 60000UL
#endif

struct PfSolarGuardState {
  uint32_t suspectSinceMs = 0;
  uint32_t lastKickMs = 0;
  uint16_t kickCount = 0;
};

static PfSolarGuardState gPfSolarGuard;

static bool pfSolarGuardRead8(uint8_t reg, uint8_t &val) {
  Wire1.beginTransmission(PF_SOLAR_GUARD_BQ_ADDR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom((int)PF_SOLAR_GUARD_BQ_ADDR, 1) != 1) return false;
  val = (uint8_t)Wire1.read();
  return true;
}

static bool pfSolarGuardWrite8(uint8_t reg, uint8_t val) {
  Wire1.beginTransmission(PF_SOLAR_GUARD_BQ_ADDR);
  Wire1.write(reg);
  Wire1.write(val);
  return Wire1.endTransmission() == 0;
}

static bool pfSolarGuardUpdate8(uint8_t reg, uint8_t mask, bool set) {
  uint8_t val = 0;
  if (!pfSolarGuardRead8(reg, val)) return false;
  uint8_t next = set ? (uint8_t)(val | mask) : (uint8_t)(val & ~mask);
  if (next == val) return true;
  return pfSolarGuardWrite8(reg, next);
}

static bool pfSolarGuardForceWideOvp(const char *tag) {
  uint8_t before = 0;
  if (!pfSolarGuardRead8(PF_SOLAR_GUARD_REG_CHG_CTRL1, before)) {
    Serial.printf("%s solar_guard: read REG0x17 failed\n", tag);
    return false;
  }
  uint8_t wanted = before | (1u << PF_SOLAR_GUARD_VBUS_OVP_BIT);
  bool wrote = (wanted == before) || pfSolarGuardWrite8(PF_SOLAR_GUARD_REG_CHG_CTRL1, wanted);
  uint8_t after = 0;
  bool readback = pfSolarGuardRead8(PF_SOLAR_GUARD_REG_CHG_CTRL1, after);
  bool ok = wrote && readback && ((after & (1u << PF_SOLAR_GUARD_VBUS_OVP_BIT)) != 0);
  Serial.printf("%s solar_guard: REG0x17 0x%02X -> 0x%02X VBUS_OVP=%s\n",
                tag, before, readback ? after : 0xFF, ok ? "wide" : "ERR");
  return ok;
}

static bool pfSolarGuardSetHiz(bool enable) {
  return pfSolarGuardUpdate8(PF_SOLAR_GUARD_REG_CHG_CTRL0,
                             (uint8_t)(1u << PF_SOLAR_GUARD_EN_HIZ_BIT),
                             enable);
}

static void pfSolarGuardReapply(float maintainV, bool chargingEnabled) {
  if (maintainV > 0.0f) PowerFeather::Board.setSupplyMaintainVoltage(maintainV);
  PowerFeather::Board.enableBatteryCharging(chargingEnabled);
}

static bool pfSolarGuardKick(const char *tag, float maintainV, bool chargingEnabled) {
  Serial.printf("%s solar_guard: HIZ requal kick #%u (maintain=%.2fV)\n",
                tag, (unsigned)(gPfSolarGuard.kickCount + 1), maintainV);
  bool ok = pfSolarGuardSetHiz(true);
  delay(250);
  ok = pfSolarGuardSetHiz(false) && ok;
  delay(100);
  pfSolarGuardReapply(maintainV, chargingEnabled);
  gPfSolarGuard.kickCount++;
  Serial.printf("%s solar_guard: kick %s\n", tag, ok ? "ok" : "I2C_ERR");
  return ok;
}

static bool pfSolarGuardInit(const char *tag, float maintainV, bool chargingEnabled) {
  bool ok = pfSolarGuardForceWideOvp(tag);
  ok = pfSolarGuardSetHiz(false) && ok;
  pfSolarGuardReapply(maintainV, chargingEnabled);
  return ok;
}

static void pfSolarGuardTick(const char *tag, float supplyV, float supplyMa,
                             bool supplyGood, float maintainV, bool chargingEnabled) {
  if (!chargingEnabled) {
    gPfSolarGuard.suspectSinceMs = 0;
    return;
  }

  uint32_t now = millis();
  bool suspect = (supplyV >= PF_SOLAR_GUARD_SUSPECT_V) && !supplyGood &&
                 (fabsf(supplyMa) <= PF_SOLAR_GUARD_ZERO_MA);
  if (!suspect) {
    gPfSolarGuard.suspectSinceMs = 0;
    return;
  }

  if (!gPfSolarGuard.suspectSinceMs) {
    gPfSolarGuard.suspectSinceMs = now;
    Serial.printf("%s solar_guard: suspect input latch sv=%.2fV sma=%.1fmA sgood=0\n",
                  tag, supplyV, supplyMa);
    return;
  }

  if ((now - gPfSolarGuard.suspectSinceMs) < PF_SOLAR_GUARD_HOLD_MS) return;
  if (gPfSolarGuard.lastKickMs &&
      (now - gPfSolarGuard.lastKickMs) < PF_SOLAR_GUARD_COOLDOWN_MS) {
    return;
  }

  gPfSolarGuard.lastKickMs = now;
  gPfSolarGuard.suspectSinceMs = 0;
  pfSolarGuardKick(tag, maintainV, chargingEnabled);
}
