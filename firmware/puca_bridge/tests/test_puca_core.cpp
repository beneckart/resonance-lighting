#include "../../fixture/tests/test_util.h"

#include "../puca_core.h"

int main() {
  CHECK_EQ(pucaIsFixtureFirmware("fx-260826-024e508-p"), true);
  CHECK_EQ(pucaIsFixtureFirmware("fixture-0.1"), true);
  CHECK_EQ(pucaIsFixtureFirmware("dev-local"), true);
  CHECK_EQ(pucaIsFixtureFirmware("net-bench-2026-08-19.1"), false);
  CHECK_EQ(pucaIsFixtureFirmware("cores3-os-0.1.2-dev"), false);
  CHECK_EQ(pucaIsFixtureFirmware("puca-bridge-0.4.0-dev"), false);
  CHECK_EQ(pucaIsFixtureFirmware(""), false);
  CHECK_EQ(pucaIsFixtureFirmware(nullptr), false);
  CHECK_EQ(pucaAudioPeerEligible(false, ""), true);
  CHECK_EQ(pucaAudioPeerEligible(true, "fx-260826-024e508-p"), true);
  CHECK_EQ(pucaAudioPeerEligible(true, "net-bench-2026-08-19.1"), false);

  const uint8_t pucaId[3] = {0xA4, 0xEB, 0x10};
  const uint8_t exactTarget[3] = {0xA4, 0xEB, 0x10};
  const uint8_t otherTarget[3] = {0xA4, 0xEB, 0x11};
  const uint8_t fleetTarget[3] = {0, 0, 0};
  CHECK(pucaMaintenanceTargetMatches(exactTarget, pucaId));
  CHECK(!pucaMaintenanceTargetMatches(otherTarget, pucaId));
  CHECK(!pucaMaintenanceTargetMatches(fleetTarget, pucaId));
  CHECK(!pucaMaintenanceTargetMatches(nullptr, pucaId));

  CHECK_EQ(pucaNextLiveMode(MODE_CLASSIC), MODE_HEARTBEAT);
  CHECK_EQ(pucaNextLiveMode(MODE_HEARTBEAT), MODE_EMBER);
  CHECK_EQ(pucaNextLiveMode(MODE_EMBER), MODE_HUE);
  CHECK_EQ(pucaNextLiveMode(MODE_HUE), MODE_CLASSIC);
  CHECK_EQ(pucaNextLiveMode(MODE_OFF), MODE_CLASSIC);
  CHECK_EQ(pucaModeStatusCode(MODE_CLASSIC), 1);
  CHECK_EQ(pucaModeStatusCode(MODE_HEARTBEAT), 2);
  CHECK_EQ(pucaModeStatusCode(MODE_EMBER), 3);
  CHECK_EQ(pucaModeStatusCode(MODE_HUE), 4);
  CHECK_EQ(pucaModeStatusCode(MODE_OFF), 5);
  CHECK(pucaPublisherShouldArmAtBoot(true, true, true));
  CHECK(!pucaPublisherShouldArmAtBoot(false, true, true));
  CHECK(!pucaPublisherShouldArmAtBoot(true, false, true));
  CHECK(!pucaPublisherShouldArmAtBoot(true, true, false));

  CHECK_EQ(pucaChunkCount(0, 18), 0u);
  CHECK_EQ(pucaChunkCount(18, 18), 1u);
  CHECK_EQ(pucaChunkCount(19, 18), 2u);
  CHECK_EQ(pucaChunkCount(130, 18), 8u);
  CHECK_EQ(pucaChunkCount(192, 18), 11u);
  CHECK_EQ(pucaChunkCount(10, 0), 0u);
  CHECK_EQ(pucaChunkSize(130, 0, 18), 18u);
  CHECK_EQ(pucaChunkSize(130, 126, 18), 4u);
  CHECK_EQ(pucaChunkSize(130, 130, 18), 0u);

  int16_t stereo[] = {1000, -1000, 32767, 32767, -32768, -32768,
                      3000, 1000, 123};
  PucaPcmStats stats = pucaPcmStats(stereo, 9);
  CHECK_EQ(stats.peak, 32768);
  CHECK_EQ(stats.clipped, 4u);

  size_t monoCount = pucaStereoToMono(stereo, 9);
  CHECK_EQ(monoCount, 4u);
  CHECK_EQ(stereo[0], 0);
  CHECK_EQ(stereo[1], 32767);
  CHECK_EQ(stereo[2], -32768);
  CHECK_EQ(stereo[3], 2000);
  CHECK_EQ(pucaStereoToMono(nullptr, 4), 0u);
  CHECK_EQ(pucaPcmStats(nullptr, 4).peak, 0);

  PucaPeakFollower heartbeat;
  float firstPulse = heartbeat.update(32768, 1.0f);
  CHECK(firstPulse > 0.89f && firstPulse < 0.91f);
  float release = heartbeat.update(0, 1.0f);
  CHECK(release > 0.49f && release < 0.50f);
  heartbeat.reset();
  CHECK_EQ(heartbeat.level, 0.0f);
  float amplifiedPulse = heartbeat.update(8192, 4.0f);
  CHECK(amplifiedPulse > 0.89f && amplifiedPulse < 0.91f);

  PucaRgbw darkHeart = pucaHeartbeatColor(0.0f, 1.0f);
  CHECK_EQ(darkHeart.r, 0);
  CHECK_EQ(darkHeart.g, 0);
  PucaRgbw halfHeart = pucaHeartbeatColor(1.0f, 0.5f);
  CHECK_EQ(halfHeart.r, 128);
  CHECK_EQ(halfHeart.g, 5);
  CHECK_EQ(halfHeart.b, 0);
  CHECK_EQ(halfHeart.w, 0);
  CHECK_EQ(pucaHeartbeatColor(2.0f, 2.0f).r, 255);

  PucaTouchGesture touch;
  CHECK_EQ(touch.update(100, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2499, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2500, true), PUCA_TOUCH_NONE); // high baseline suppressed
  CHECK_EQ(touch.update(2600, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2610, false), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2641, false), PUCA_TOUCH_NONE); // baseline release ignored
  CHECK_EQ(touch.update(2700, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2730, true), PUCA_TOUCH_NONE);  // debounced press
  CHECK_EQ(touch.update(2800, false), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(2830, false), PUCA_TOUCH_SHORT);

  CHECK_EQ(touch.update(3000, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(3030, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(4529, true), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(4530, true), PUCA_TOUCH_LONG);
  CHECK_EQ(touch.update(4700, false), PUCA_TOUCH_NONE);
  CHECK_EQ(touch.update(4730, false), PUCA_TOUCH_NONE); // long never also short

  PucaBootHoldDetector bootHold;
  CHECK(!bootHold.update(0, true));
  CHECK(!bootHold.update(800, true));
  CHECK(!bootHold.update(900, false));
  CHECK(!bootHold.update(1000, true));
  CHECK(!bootHold.update(2199, true));
  CHECK(bootHold.update(2200, true));
  CHECK(bootHold.update(2300, false)); // completion latches

  PucaSetupWindow setupWindow;
  setupWindow.enter(1000, 20000);
  CHECK(setupWindow.unlocked);
  CHECK(!setupWindow.update(20999));
  setupWindow.activity(15000);
  CHECK(!setupWindow.update(34999));
  CHECK(setupWindow.update(35000));
  CHECK(!setupWindow.unlocked);
  setupWindow.enter(40000);
  setupWindow.lock();
  CHECK(!setupWindow.update(70000));

  PucaLedPattern led;
  led.startStatus(1000, true, 1); // line + HEARTBEAT
  CHECK(led.level(1000));
  CHECK(led.level(1499));
  CHECK(!led.level(1500));
  CHECK(!led.level(2099));
  CHECK(led.level(2100));
  CHECK(!led.level(2250));
  CHECK(!led.level(2430));
  CHECK(!led.active);

  led.startStatus(3000, false, 4); // mic + HUE fits the fixed pattern
  CHECK(led.level(3000));
  CHECK(!led.level(3500));
  CHECK(led.level(3680));
  CHECK(!led.level(4180));
  CHECK(!led.level(4779));
  CHECK(led.level(4780));
  CHECK(led.level(7000, true));
  CHECK(!led.active);

  return testReport("puca_core");
}
