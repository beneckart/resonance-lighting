#include "../../fixture/tests/test_util.h"

#include "../app_model.h"

int main() {
  CHECK_EQ(cores3BatteryBand(false, 3300), CORES3_BATTERY_OFF_AIR);
  CHECK_EQ(cores3BatteryBand(true, 0), CORES3_BATTERY_UNKNOWN);
  CHECK_EQ(cores3BatteryBand(true, 3201), CORES3_BATTERY_GOOD);
  CHECK_EQ(cores3BatteryBand(true, 3200), CORES3_BATTERY_NEAR_LOW);
  CHECK_EQ(cores3BatteryBand(true, 3101), CORES3_BATTERY_NEAR_LOW);
  CHECK_EQ(cores3BatteryBand(true, 3100), CORES3_BATTERY_LOW);

  CHECK_EQ(cores3PageCount(0), 1u);
  CHECK_EQ(cores3PageCount(24), 1u);
  CHECK_EQ(cores3PageCount(25), 2u);
  CHECK_EQ(cores3PageCount(192), 8u);
  CHECK_EQ(cores3ClampPage(9, 25), 1u);
  CHECK_EQ(cores3PageStart(1, 25), 24u);
  CHECK_EQ(cores3PageStart(9, 25), 24u);

  CHECK_EQ(cores3NextAudioInput(CORES3_AUDIO_INPUT_AMBIENT, true),
           CORES3_AUDIO_INPUT_AUX);
  CHECK_EQ(cores3NextAudioInput(CORES3_AUDIO_INPUT_AUX, true),
           CORES3_AUDIO_INPUT_AMBIENT);
  CHECK_EQ(cores3NextAudioInput(CORES3_AUDIO_INPUT_AUX, false),
           CORES3_AUDIO_INPUT_AMBIENT);

  CHECK_EQ(cores3IsFixtureFirmware("fx-260826-024e508-p"), true);
  CHECK_EQ(cores3IsFixtureFirmware("fixture-0.1"), true);
  CHECK_EQ(cores3IsFixtureFirmware("dev-local"), true);
  CHECK_EQ(cores3IsFixtureFirmware("net-bench-2026-08-19.1"), false);
  CHECK_EQ(cores3IsFixtureFirmware("cores3-os-0.1.2-dev"), false);
  CHECK_EQ(cores3IsFixtureFirmware("tdeck-dev-local"), false);
  CHECK_EQ(cores3IsFixtureFirmware(""), false);
  CHECK_EQ(cores3IsFixtureFirmware(nullptr), false);
  CHECK_EQ(cores3AudioPeerEligible(false, ""), true);
  CHECK_EQ(cores3AudioPeerEligible(true, "fx-260826-024e508-p"), true);
  CHECK_EQ(cores3AudioPeerEligible(true, "net-bench-2026-08-19.1"), false);

  return testReport("cores3_app_model");
}
