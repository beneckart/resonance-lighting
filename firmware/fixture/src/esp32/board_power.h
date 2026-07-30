// PowerFeather V2 power stack: Board.init with rail re-park, deferred charge
// enable (bare-board safe), MAX17260 reads with the +8% current correction,
// BQ25628E raw-register snapshot, solar guard, rails, timed deep sleep.
//
// This is the ONLY translation unit that includes powerfeather_solar_guard.h
// (header-only with static state -- a second include would fork the state).
#pragma once

#include <stdint.h>

// MAX17260 reads +8% high; ADR 0023, replicated across 8 sessions.
#define RES_GAUGE_CURRENT_DIVISOR 1.08f

void boardPowerInit();     // load NVS config + Board.init retries + charger policy
void boardPowerTick();     // call from loop; internally rate-limited to 1 Hz
void readBatteryNow();     // synchronous refresh (maintenance preflight)

bool pfIsReady();
bool chargingEnabled();
float maintainVolts();
const char *batteryTypeName();

// Cached cell/supply readings (refreshed by boardPowerTick).
float batteryVolts();
float batteryMa();     // corrected (/1.08)
float batteryMaRaw();
int batterySocPct();   // -1 = no reading; ADVISORY ONLY, never a control gate
float supplyVolts();
float supplyMa();
bool supplyGood();

// BQ25628E snapshot (0xFFFF/0xFF = unknown).
struct BqSnapshot {
  uint16_t vindpm_mv, ichg_ma, vreg_mv;
  uint8_t reg16, reg18, stat0, stat1, fault0, flag0, flag1, fault_flag0, part;
};
const BqSnapshot &bqSnapshot();

// Rails. enable3V3 verifies the RTC pad actually changed (the SDK setter can
// silently no-op against a held pad); returns the pad's final state.
bool railEnable3V3(bool on);
void railEnableVSQT(bool on);

// Persist + apply runtime config (radio/serial C and G commands).
bool applyCapacityAndReboot(uint16_t mah);
bool applyChargeMa(uint16_t ma);
bool applyMaintainV10(uint8_t v10);

// Rails-off timed deep sleep (~0.5%/h vs ~1.7%/h with rails left on).
void enterTimedDeepSleep(uint16_t seconds, const char *why);
