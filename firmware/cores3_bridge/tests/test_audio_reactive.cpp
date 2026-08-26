#include "../../fixture/tests/test_util.h"

#include "../audio_reactive.h"

int main() {
  int16_t quiet[64];
  int16_t loud[64];
  for (size_t i = 0; i < 64; ++i) {
    quiet[i] = (i & 1) ? 100 : -100;
    loud[i] = (i & 1) ? 5000 : -5000;
  }

  AudioEnvelope envelope;
  for (int i = 0; i < 20; ++i) envelope.update(quiet, 64);
  CHECK(envelope.calibrated());
  CHECK_EQ((int)envelope.level, 0);
  CHECK(envelope.noise > 99.0f && envelope.noise < 101.0f);

  envelope.update(loud, 64);
  CHECK(envelope.level > 0.60f);
  float beat = envelope.level;
  envelope.update(quiet, 64);
  CHECK(envelope.level < beat);
  CHECK(envelope.level > 0.0f);

  AudioColor red = audioColorForSlot(0, 1.0f);
  AudioColor green = audioColorForSlot(1, 1.0f);
  AudioColor blue = audioColorForSlot(2, 1.0f);
  CHECK_EQ(red.r, 255);
  CHECK_EQ(red.g, 63);
  CHECK_EQ(green.g, 255);
  CHECK_EQ(blue.b, 255);
  CHECK_EQ(red.w, 0);

  CHECK_EQ(audioDirectFrameCount(0), 0u);
  CHECK_EQ(audioDirectFrameCount(18), 1u);
  CHECK_EQ(audioDirectFrameCount(19), 2u);
  CHECK_EQ(audioDirectFrameCount(192), 11u);

  return testReport("audio_reactive");
}
