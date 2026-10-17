#include "test_util.h"
#include "../src/core/protect_retry.h"
#include "../src/core/power_policy.h"
#include "../src/core/serial_mutation.h"
#include <string>

static ProtectRetryInput healthy(uint32_t now) {
  return {now, true, true, 3300, true, false, true, true};
}
static ProtectRetryAction run(ProtectRetryState &s, unsigned seconds) {
  ProtectRetryAction a = PR_WAIT;
  for (unsigned i = 0; i < seconds; ++i)
    a = protectRetryTick(s, healthy(s.last_ms + 1000), 3250, 3150);
  return a;
}
int main() {
  // Restart tickets are single-use; a reset during any trial cannot repeat it.
  auto r = protectRetryEmpty(); CHECK(protectRetryValid(r));
  protectRetrySpend(r); CHECK_EQ(r.attempts, 1); CHECK_EQ(r.pending, 1);
  auto wrongReset = r;
  CHECK(!protectRetryBoot(wrongReset, false, true)); CHECK_EQ(wrongReset.pending, 0);
  CHECK(protectRetryBoot(r, true, true)); CHECK_EQ(r.pending, 2);
  CHECK(!protectRetryBoot(r, true, true)); CHECK_EQ(r.pending, 0);
  CHECK_EQ(r.attempts, 1);
  for (int pending = 0; pending < 3; ++pending)
    for (bool sw : {false, true}) for (bool parked : {false, true}) {
      auto record = protectRetryEmpty(); record.attempts = 3; record.pending = pending;
      protectRetrySeal(record);
      CHECK(protectRetryBoot(record, sw, parked) == (pending == 1 && sw && parked));
      CHECK_EQ(record.attempts, 3); CHECK(protectRetryValid(record));
      if (pending && !(pending == 1 && sw && parked)) CHECK_EQ(record.last_reason, PR_RESET);
    }
  auto broken = r; broken.attempts++; CHECK(!protectRetryValid(broken));
  for (int i = 0; i < 20; ++i) protectRetrySpend(r);
  CHECK_EQ(r.attempts, 4); CHECK_EQ(protectRetryCooldownS(r.attempts), 21600u);
  CHECK_EQ(protectRetryCooldownS(1), 300u);
  CHECK_EQ(protectRetryCooldownS(2), 900u);
  CHECK_EQ(protectRetryCooldownS(3), 3600u);

  // No current sign, supply-good, SOC or charger-enabled input is required.
  // A sustained real battery permits a trial at night or with weak solar.
  ProtectRetryState s;
  protectRetryInit(s, 15000, false, 0);
  CHECK(run(s, 30) == PR_WAIT);
  CHECK(run(s, 1) == PR_BEGIN); CHECK_EQ(s.phase, PR_START);
  // It is only permission to attempt; the durable PROTECT has not been cleared.
  CHECK(run(s, 10) == PR_WAIT);

  protectRetryInit(s, 15000, true, 0);
  CHECK(run(s, 31) == PR_WAIT); CHECK_EQ(s.phase, PR_LED_LOW);
  CHECK_EQ(protectRetryBrightness(s.phase), 32);
  CHECK(run(s, 31) == PR_WAIT); CHECK_EQ(s.phase, PR_LED_DIM);
  CHECK_EQ(protectRetryBrightness(s.phase), 128);
  CHECK(run(s, 31) == PR_PASS); CHECK_EQ(s.phase, PR_DONE);

  // Any sag, lost cell corroboration, stale data or charger fault aborts.
  for (int fault = 0; fault < 5; ++fault) {
    protectRetryInit(s, 15000, true, 0);
    auto in = healthy(16000);
    if (fault == 0) in.mv = 3149;
    if (fault == 1) in.battery_valid = false;
    if (fault == 2) in.corroborated = false;
    if (fault == 3) in.charger_valid = false;
    if (fault == 4) in.charger_fault = true;
    CHECK(protectRetryTick(s, in, 3250, 3150) == PR_FAIL);
  }
  protectRetryInit(s, 15000, true, 0);
  auto in = healthy(35000); in.sensor_rail_ok = false;
  CHECK(protectRetryTick(s, in, 3250, 3150) == PR_FAIL);
  CHECK_EQ(s.reason, PR_SENSOR_TIMEOUT);
  protectRetryInit(s, 15000, true, 0);
  in = healthy(195000);
  CHECK(protectRetryTick(s, in, 3250, 3150) == PR_FAIL);
  CHECK_EQ(s.reason, PR_TRIAL_TIMEOUT);

  // Backoff cannot be bypassed by a healthy sample, and saturates rather than
  // permanently exhausting retries. Timer-sleep credit is supplied by glue.
  protectRetryInit(s, 15000, false, 300);
  CHECK(run(s, 299) == PR_SLEEP); CHECK_EQ(s.remaining_ms, 1000u);
  CHECK(run(s, 1) == PR_WAIT);
  CHECK(run(s, 30) == PR_BEGIN);
  protectRetryInit(s, 15000, false, 21600);
  in = healthy(15000 + 21600000);
  CHECK(protectRetryTick(s, in, 3250, 3150) == PR_WAIT);
  CHECK(run(s, 30) == PR_BEGIN);

  // Missing data before gauge warmup cannot start the qualification timer.
  protectRetryInit(s, 1000, false, 0);
  in = healthy(2000); in.battery_valid = false;
  CHECK(protectRetryTick(s, in, 3250, 3150) == PR_WAIT);
  in.now_ms = 16000; CHECK(protectRetryTick(s, in, 3250, 3150) == PR_SLEEP);
  CHECK(run(s, 31) == PR_BEGIN);
  // A large scheduler gap and millis wrap do not manufacture a completed hold.
  protectRetryInit(s, 15000, false, 0); run(s, 10);
  in = healthy(s.last_ms + 20000);
  CHECK(protectRetryTick(s, in, 3250, 3150) == PR_WAIT); CHECK_EQ(s.held_ms, 0u);
  protectRetryInit(s, 0xFFFFFF00u, false, 0);
  CHECK(run(s, 31) == PR_BEGIN);

  // Managed policy never performs the older automatic charge-only clear.
  auto config = powerConfigDefaults(); config.managed_recovery = true;
  PowerState power; powerStateInit(power, LedTier::PROTECT);
  PowerSample ps = {}; ps.batt_valid = ps.batt_corroborated = true;
  ps.batt_v = 3.6f; ps.batt_ma = 500; ps.supply_valid = ps.supply_good = true;
  ps.charger_valid = ps.charging_enabled = true;
  for (int i = 1; i <= 120; ++i) {
    ps.now_ms = i * 1000;
    auto b = powerPolicyTick(power, ps, config);
    CHECK(b.tier == LedTier::PROTECT); CHECK(!b.protect_released);
  }

  // Fault-injection commands retain the shared exact-target, complete-line gate.
  const uint8_t me[] = {0xF4, 0x03, 0x48};
  for (const std::string text : {"!XF40348:1\n", "!XF40348:2\n", "!XF40348:3\n",
                                "!XF40349:1\n", "!XF40348:0\n", "!XF40348:4\n",
                                "!XF40348:01\n", "!XF40348:1"}) {
    SerialMutationParser parser; SerialMutationCommand command = {};
    unsigned ready = 0;
    for (uint8_t c : text) if (parser.feed(c, me, command) == SERIAL_MUTATION_READY) {
      ++ready; CHECK_EQ(command.type, SERIAL_MUTATION_PROTECT_TEST);
    }
    bool expected = text == "!XF40348:1\n" || text == "!XF40348:2\n" || text == "!XF40348:3\n";
    CHECK_EQ(ready, expected ? 1u : 0u);
  }
  return testReport("protect_retry");
}
