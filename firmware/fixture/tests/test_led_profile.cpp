#include "test_util.h"

#include "../src/core/led_profile.h"

int main() {
  FixtureLedProfile downlight = fixtureLedProfile(FIXTURE_DOWNLIGHT);
  CHECK_EQ(downlight.pixelCount, 1u);
  CHECK(downlight.rgbw);

  FixtureLedProfile perimeter = fixtureLedProfile(FIXTURE_PERIMETER);
  CHECK_EQ(perimeter.pixelCount, (uint8_t)FRAME_MAX_PIXELS);
  CHECK(!perimeter.rgbw);

  FixtureLedProfile uplight = fixtureLedProfile(FIXTURE_UPLIGHT);
  CHECK_EQ(uplight.pixelCount, 1u);
  CHECK(!uplight.rgbw);

  FixtureLedProfile chandelier = fixtureLedProfile(FIXTURE_CHANDELIER);
  CHECK_EQ(chandelier.pixelCount, 1u);
  CHECK(chandelier.rgbw);

  FixtureLedProfile unknown = fixtureLedProfile(FIXTURE_UNKNOWN);
  CHECK_EQ(unknown.pixelCount, 1u);
  CHECK(unknown.rgbw);

  return testReport("led_profile");
}
