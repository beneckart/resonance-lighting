#include "protect_retry.h"

static uint32_t checksum(const ProtectRetryRecord &r) {
  uint32_t h = 2166136261UL;
  const uint32_t fields[] = {r.version, r.generation, r.attempts, r.pending, r.last_reason,
      r.success_generation, r.success_min_mv, (uint16_t)r.success_min_ma,
      r.success_external_seen, r.reserved[0], r.reserved[1], r.reserved[2]};
  for (uint32_t v : fields) {
    for (unsigned i = 0; i < 4; ++i) { h = (h ^ (v & 255u)) * 16777619UL; v >>= 8; }
  }
  return h;
}
void protectRetrySeal(ProtectRetryRecord &r) { r.checksum = checksum(r); }
ProtectRetryRecord protectRetryEmpty() {
  ProtectRetryRecord r = {}; r.version = 1; protectRetrySeal(r); return r;
}
bool protectRetryValid(const ProtectRetryRecord &r) {
  return r.version == 1 && r.attempts <= 4 && r.pending <= 2 &&
         r.last_reason <= PR_RAIL_FAILURE && r.success_external_seen <= 1 &&
         !r.reserved[0] && !r.reserved[1] && !r.reserved[2] && r.checksum == checksum(r);
}
uint32_t protectRetryCooldownS(uint16_t attempts) {
  const uint32_t delays[] = {0, 300, 900, 3600, 21600};
  return delays[attempts > 4 ? 4 : attempts];
}
bool protectRetryBoot(ProtectRetryRecord &r, bool softwareReset, bool storedProtect) {
  bool trial = r.pending == 1 && softwareReset && storedProtect;
  if (r.pending && !trial) r.last_reason = PR_RESET;
  r.pending = trial ? 2 : 0;
  protectRetrySeal(r);
  return trial;
}
void protectRetrySpend(ProtectRetryRecord &r) {
  if (r.attempts < 4) ++r.attempts;
  ++r.generation;
  r.pending = 1;
  protectRetrySeal(r);
}
void protectRetryInit(ProtectRetryState &s, uint32_t now, bool trial,
                      uint32_t cooldownS) {
  s = {};
  s.phase = trial ? PR_SENSORS : PR_PARKED;
  s.remaining_ms = cooldownS * 1000UL;
  s.last_ms = s.started_ms = s.phase_ms = now;
}
uint8_t protectRetryBrightness(uint8_t phase) {
  return phase == PR_LED_LOW ? 32 : phase == PR_LED_DIM ? 128 : 0;
}
ProtectRetryAction protectRetryTick(ProtectRetryState &s,
                                    const ProtectRetryInput &in,
                                    uint16_t entryMv, uint16_t trialMv) {
  uint32_t dt = in.now_ms - s.last_ms;
  s.last_ms = in.now_ms;
  if (s.phase >= PR_START) return PR_WAIT;
  bool trial = s.phase != PR_PARKED;
  if (!trial && s.remaining_ms) {
    s.remaining_ms = dt >= s.remaining_ms ? 0 : s.remaining_ms - dt;
    s.started_ms = in.now_ms;
    s.held_ms = 0; s.previous_good = false; s.reason = PR_COOLDOWN;
    if (s.remaining_ms) return PR_SLEEP;
  }
  bool data = in.battery_valid && in.corroborated && in.charger_valid;
  bool healthy = data && !in.charger_fault && in.mv >= (trial ? trialMv : entryMv);
  s.reason = !data ? PR_BAD_DATA : in.charger_fault ? PR_CHARGER_FAULT :
             !healthy ? PR_LOW_VOLTAGE : PR_OK;
  // Allow the known post-wake conversion window, not an unbounded data wait.
  if (in.now_ms < 15000 && !data) return PR_WAIT;
  if (trial && !healthy) return PR_FAIL;
  if (!trial && !healthy) { s.held_ms = 0; s.previous_good = false; return PR_SLEEP; }
  if (in.now_ms - s.started_ms >= (trial ? 180000UL : 90000UL)) {
    s.reason = trial ? PR_TRIAL_TIMEOUT : PR_ASSESS_TIMEOUT;
    return trial ? PR_FAIL : PR_SLEEP;
  }
  bool good = healthy && (!trial || (in.sensor_rail_ok &&
                     (s.phase == PR_SENSORS || in.led_on)));
  if (trial && !in.sensor_rail_ok && in.now_ms - s.phase_ms >= 20000UL) {
    s.reason = PR_SENSOR_TIMEOUT; return PR_FAIL;
  }
  // A scheduler gap is not evidence of health. The LED ramp is <=3.3 seconds.
  if (good && s.previous_good && dt <= 6000UL) s.held_ms += dt;
  else s.held_ms = 0;
  s.previous_good = good;
  if (s.held_ms < 30000UL) return PR_WAIT;
  s.held_ms = 0; s.previous_good = false; s.phase_ms = in.now_ms;
  if (!trial) { s.phase = PR_START; return PR_BEGIN; }
  if (s.phase == PR_LED_DIM) { s.phase = PR_DONE; return PR_PASS; }
  ++s.phase;
  return PR_WAIT;
}
