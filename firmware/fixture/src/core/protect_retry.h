// Bounded recovery from a durable PROTECT latch (ADR 0085).
// The journal survives resets. A retry is spent BEFORE sensor/LED power.
#pragma once
#include <stdint.h>

enum ProtectRetryPhase : uint8_t {
  PR_PARKED, PR_SENSORS, PR_LED_LOW, PR_LED_DIM, PR_START, PR_DONE
};
enum ProtectRetryAction : uint8_t { PR_WAIT, PR_SLEEP, PR_BEGIN, PR_FAIL, PR_PASS };
enum ProtectRetryReason : uint8_t {
  PR_OK, PR_COOLDOWN, PR_LOW_VOLTAGE, PR_BAD_DATA, PR_CHARGER_FAULT,
  PR_SENSOR_TIMEOUT, PR_TRIAL_TIMEOUT, PR_ASSESS_TIMEOUT, PR_RESET, PR_STORAGE,
  PR_RAIL_FAILURE
};
struct ProtectRetryRecord {
  uint32_t version;
  uint32_t generation;
  uint16_t attempts; // saturated; controls retry frequency, never permanent lockout
  uint8_t pending;   // 0=parked/completed, 1=one SW reboot ticket, 2=trial running
  uint8_t last_reason;
  uint32_t success_generation;
  uint16_t success_min_mv;
  int16_t success_min_ma;
  uint8_t success_external_seen;
  uint8_t reserved[3];
  uint32_t checksum;
};
static_assert(sizeof(ProtectRetryRecord) == 28, "retry journal layout");
ProtectRetryRecord protectRetryEmpty();
void protectRetrySeal(ProtectRetryRecord &r);
bool protectRetryValid(const ProtectRetryRecord &r);
uint32_t protectRetryCooldownS(uint16_t attempts);
// Caller must persist the returned record before honoring a true result.
bool protectRetryBoot(ProtectRetryRecord &r, bool softwareReset, bool storedProtect);
void protectRetrySpend(ProtectRetryRecord &r);

struct ProtectRetryInput {
  uint32_t now_ms;
  bool battery_valid; // fresh VBAT AND post-wake-valid IBAT; never cached zeros
  bool corroborated;
  uint16_t mv;        // raw terminal voltage, deliberately no current compensation
  bool charger_valid;
  bool charger_fault;
  bool sensor_rail_ok; // power-domain proof; missing ranging is reported separately
  bool led_on;
};
struct ProtectRetryState {
  uint8_t phase, reason;
  uint32_t remaining_ms, last_ms, started_ms, phase_ms, held_ms;
  bool previous_good;
};
void protectRetryInit(ProtectRetryState &s, uint32_t now, bool trial,
                      uint32_t cooldownS);
ProtectRetryAction protectRetryTick(ProtectRetryState &s,
                                    const ProtectRetryInput &in,
                                    uint16_t entryMv, uint16_t trialMv);
uint8_t protectRetryBrightness(uint8_t phase);
