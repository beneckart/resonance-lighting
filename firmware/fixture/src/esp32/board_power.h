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

// BQ25628E precharge limit. The 30 mA POR value stranded deeply discharged
// production LFPs near 2.8 V despite valid solar input. Ben selected 300 mA
// for the supervised recovery rollout on 2026-08-16. Trickle charge below
// 2.25 V, input DPM, thermal protection, and the hardware transition to fast
// charge remain charger-owned and unchanged.
#ifndef RES_PF_PRECHARGE_MA
#define RES_PF_PRECHARGE_MA 300
#endif
#if RES_PF_PRECHARGE_MA < 10 || RES_PF_PRECHARGE_MA > 310 || \
    (RES_PF_PRECHARGE_MA % 10) != 0
#error "RES_PF_PRECHARGE_MA must be 10..310 mA in 10 mA steps"
#endif

void boardPowerInit();     // load NVS config + Board.init retries + charger policy
void boardPowerTick();     // call from loop; internally rate-limited to 1 Hz
void readBatteryNow();     // synchronous refresh (maintenance preflight)

bool pfIsReady();
bool chargingEnabled();
bool batteryPresent(); // plausible connected LFP cell, not a floating BAT read
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
bool prechargeConfigured(); // target register value read back successfully
uint16_t prechargeTargetMa();

// BQ25628E snapshot (0xFFFF/0xFF = unknown).
struct BqSnapshot {
  uint16_t vindpm_mv, ichg_ma, vreg_mv, precharge_ma;
  uint8_t reg10, reg16, reg18, stat0, stat1, fault0, flag0, flag1, fault_flag0, part;
};
const BqSnapshot &bqSnapshot();

// Rails. enable3V3 verifies the RTC pad actually changed (the SDK setter can
// silently no-op against a held pad); returns the pad's final state.
bool railEnable3V3(bool on);
// Retry the SDK's try-lock setter and verify the GPIO14 RTC pad level.
// Return true only when the requested physical state is observed.
bool railEnableVSQT(bool on);
// A bounded off/on reset for the full shared STEMMA sensor domain.
bool railCycleVSQT(uint16_t offMs = 100, uint16_t settleMs = 150);

// Persist + apply runtime config (radio/serial C and G commands).
bool applyCapacityAndReboot(uint16_t mah);
bool applyChargeMa(uint16_t ma);
bool applyMaintainV10(uint8_t v10);

// Rails-off timed deep sleep (~0.5%/h vs ~1.7%/h with rails left on).
void enterTimedDeepSleep(uint16_t seconds, const char *why);
