// NVS-backed runtime configuration, namespace "resfx".
//
// Everything that used to be a net_bench build flag and can safely be runtime
// data lives here, so one inspected artifact serves the whole fleet (ADR 0009)
// and fleet_usb_bringup's "same sha256 everywhere" premise holds.
//
// First boot migrates cap_mah/chg_ma from the old "netbench" namespace (the 26
// commissioned units carry per-unit calibration there) and carries a parked
// fc_led_stage: an OTA'd production image must not un-park a protected unit.
// It also migrates the historical absent/channel-6 radio state to the compiled
// fleet default (channel 11 in production), and migrates the historical
// disarmed solenoid posture to the universal enabled capability once.
#pragma once

#include <stdint.h>
#include "../core/fixture_context.h"
#include "../core/sleep_audit.h"

struct FixtureConfig {
  uint16_t capMah;      // gauge DesignCap (C command)
  uint16_t chargeMa;    // charger current cap (G command)
  uint8_t classOvr;     // FixtureClass; 0 = auto (probe)
  uint8_t classLast;    // last accepted probe result (downgrade fallback)
  uint8_t profile;      // FixtureProfile (commission/field; values stay 0/1)
  uint8_t commissionDefault; // CommissionDefaultMode; ignored in field profile
  uint8_t battTier;     // 0 = 32700 6Ah thresholds, 1 = 33140 15Ah (pending qual)
  uint8_t solEn;        // solenoid armed (replaces -DNB_SOLENOID_D7)
  uint8_t maintV10;     // VINDPM/maintain voltage x10 (46 = 4.6 V)
  uint8_t channel;      // ESP-NOW/WiFi channel (must match the maintenance AP)
  uint16_t nightMaxMin; // bounded-night force-exit (13-15 h artifact fix)
  uint16_t dimMv, offMv, slpMv; // per-unit ADR-0023 overrides; 0 = tier default
};

extern FixtureConfig gCfg;

// Range clamps shared with the serial/radio setters (donor values).
#define RES_CAPACITY_MIN_MAH 100
#define RES_CAPACITY_MAX_MAH 30000
#define RES_CHARGE_MIN_MA 40
#define RES_CHARGE_MAX_MA 2000
#define RES_CHARGE_DEFAULT_MA 2000
#define RES_CHARGE_POLICY_VERSION 1
#define RES_MAINTAIN_MIN_V10 46  // PowerFeather SDK clamps below 4.6 V
#define RES_MAINTAIN_MAX_V10 168

void nvsLoadConfig(); // idempotent; runs the netbench migration once

// Individual persists (update gCfg AND NVS). Return false on NVS write failure.
bool nvsPersistCapacity(uint16_t mah);
bool nvsPersistChargeMa(uint16_t ma);
bool nvsPersistProfile(uint8_t profile);
bool nvsPersistCommissionDefault(uint8_t mode);
bool nvsPersistClassOvr(uint8_t cls);
bool nvsPersistClassLast(uint8_t cls);
bool nvsPersistSolEn(uint8_t en);
bool nvsPersistMaintV10(uint8_t v10);
bool nvsPersistChannel(uint8_t channel);

// Boot-guard stage (kept separate from FixtureConfig: it is read before
// Serial/Board init and written on safety-critical paths).
bool nvsReadStage(uint8_t &stage);  // false = NVS unreadable (caller fails safe)
bool nvsWriteStage(uint8_t stage);
// ADR 0051 load-armed marker: true only while a real load (LED rail, solenoid
// gate) is or was recently energized. Distinguishes load-induced resets from
// panel/battery/USB power-ordering resets in the boot guard.
bool nvsReadLoadArmed(bool &armed); // false = NVS unreadable
bool nvsWriteLoadArmed(bool armed);

// Reboot-loop breaker counter (ADR 0028 rule 4).
uint32_t nvsBumpBootCount();
void nvsClearBootCount();

// Sleep provenance. The command record is written only when a validated
// operator command is about to put the fixture to sleep. The protect record is
// written once on entry into PROTECT, never on each recurring timer sleep.
// A missing key is a successful read of an invalid/zero record.
bool nvsReadSleepCommand(SleepAuditRecord &record);
bool nvsWriteSleepCommand(const SleepAuditRecord &record);
bool nvsReadProtectEntry(SleepAuditRecord &record);
bool nvsWriteProtectEntry(const SleepAuditRecord &record);
