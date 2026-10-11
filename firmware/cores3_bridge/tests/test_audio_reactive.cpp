#include "../../fixture/tests/test_util.h"

#include "../audio_reactive.h"

static void fillTone(int16_t samples[AUDIO_FFT_SIZE], float hz,
                     uint32_t sampleRate, float amplitude) {
  for (size_t i = 0; i < AUDIO_FFT_SIZE; ++i) {
    samples[i] = (int16_t)(sinf(2.0f * AUDIO_PI * hz * (float)i /
                               (float)sampleRate) * amplitude);
  }
}

static void calibrateSpectrum(AudioSpectrum *spectrum,
                              const int16_t samples[AUDIO_FFT_SIZE],
                              uint32_t sampleRate) {
  spectrum->reset();
  for (uint16_t i = 0; i < AUDIO_CALIBRATION_FRAMES; ++i)
    CHECK(spectrum->update(samples, AUDIO_FFT_SIZE, sampleRate));
  CHECK(spectrum->calibrated());
}

int main() {
  int16_t quiet[64];
  int16_t loud[64];
  for (size_t i = 0; i < 64; ++i) {
    quiet[i] = (i & 1) ? 100 : -100;
    loud[i] = (i & 1) ? 5000 : -5000;
  }

  AudioEnvelope envelope;
  for (int i = 0; i < AUDIO_CALIBRATION_FRAMES; ++i)
    envelope.update(quiet, 64);
  CHECK(envelope.calibrated());
  CHECK_EQ((int)envelope.level, 0);
  CHECK(envelope.noise > 99.0f && envelope.noise < 101.0f);

  envelope.update(loud, 64);
  CHECK(envelope.level > 0.60f);
  float beat = envelope.level;
  envelope.update(quiet, 64);
  CHECK(envelope.level < beat);
  CHECK(envelope.level > 0.0f);

  // CoreS3 can keep calibration tied to elapsed successful-capture time even
  // when a future capture cadence produces more than 50 observations.
  AudioEnvelope timedEnvelope;
  timedEnvelope.update(quiet, 64, true, 1);
  timedEnvelope.update(quiet, 64, true, 70);
  CHECK_EQ((int)timedEnvelope.level, 0);
  CHECK(timedEnvelope.calibrated());
  timedEnvelope.update(loud, 64, false, 71);
  CHECK(timedEnvelope.level > 0.60f);

  AudioColor red = audioColorForSlot(0, 1.0f);
  AudioColor green = audioColorForSlot(1, 1.0f);
  AudioColor blue = audioColorForSlot(2, 1.0f);
  CHECK_EQ(red.r, 255);
  CHECK_EQ(red.g, 63);
  CHECK_EQ(green.g, 255);
  CHECK_EQ(blue.b, 255);
  CHECK_EQ(red.w, 0);

  CHECK(audioOutputGainMultiplier(AUDIO_GAIN_1X) == 1.0f);
  CHECK(audioOutputGainMultiplier(AUDIO_GAIN_1_5X) == 1.5f);
  CHECK(audioOutputGainMultiplier(AUDIO_GAIN_2X) == 2.0f);
  CHECK(audioOutputGainMultiplier(AUDIO_GAIN_3X) == 3.0f);
  CHECK_EQ(audioNextOutputGain(AUDIO_GAIN_1X), AUDIO_GAIN_1_5X);
  CHECK_EQ(audioNextOutputGain(AUDIO_GAIN_3X), AUDIO_GAIN_1X);
  AudioColor gained = audioApplyOutputGain({100, 150, 200, 75}, AUDIO_GAIN_2X);
  CHECK_EQ(gained.r, 200);
  CHECK_EQ(gained.g, 255);
  CHECK_EQ(gained.b, 255);
  CHECK_EQ(gained.w, 150);

  CHECK_EQ(audioDirectFrameCount(0), 0u);
  CHECK_EQ(audioDirectFrameCount(18), 1u);
  CHECK_EQ(audioDirectFrameCount(19), 2u);
  CHECK_EQ(audioDirectFrameCount(192), 11u);

  int16_t stereo[8] = {100, 300, -500, 100, 32767, 32767, -32768, -32768};
  int16_t mono[4] = {};
  audioStereoToMono(stereo, mono, 4);
  CHECK_EQ(mono[0], 200);
  CHECK_EQ(mono[1], -200);
  CHECK_EQ(mono[2], 32767);
  CHECK_EQ(mono[3], -32768);

  static int16_t silence[AUDIO_FFT_SIZE] = {};
  static int16_t tone[AUDIO_FFT_SIZE] = {};
  static AudioSpectrum spectrum;
  static AudioSpectrum calibratedBaseline;
  calibrateSpectrum(&spectrum, silence, 16000);
  calibratedBaseline = spectrum;
  fillTone(tone, 125.0f, 16000, 10000.0f);
  CHECK(spectrum.update(tone, AUDIO_FFT_SIZE, 16000));
  CHECK(spectrum.bass.level > spectrum.mid.level);
  CHECK(spectrum.bass.level > spectrum.treble.level);
  float bassCentroid = spectrum.centroid;

  spectrum = calibratedBaseline;
  fillTone(tone, 1000.0f, 16000, 10000.0f);
  CHECK(spectrum.update(tone, AUDIO_FFT_SIZE, 16000));
  CHECK(spectrum.mid.level > spectrum.bass.level);
  CHECK(spectrum.mid.level > spectrum.treble.level);
  float midCentroid = spectrum.centroid;

  spectrum = calibratedBaseline;
  fillTone(tone, 5000.0f, 16000, 10000.0f);
  CHECK(spectrum.update(tone, AUDIO_FFT_SIZE, 16000));
  CHECK(spectrum.treble.level > spectrum.bass.level);
  CHECK(spectrum.treble.level > spectrum.mid.level);
  CHECK(bassCentroid < midCentroid);
  CHECK(midCentroid < spectrum.centroid);

  spectrum.reset();
  spectrum.calibrationFrames = AUDIO_CALIBRATION_FRAMES;
  fillTone(tone, 44100.0f * 12.0f / AUDIO_FFT_SIZE, 44100, 10000.0f);
  CHECK(spectrum.update(tone, AUDIO_FFT_SIZE, 44100));
  CHECK(spectrum.mid.level > spectrum.bass.level);
  CHECK(spectrum.mid.level > spectrum.treble.level);
  CHECK_EQ(spectrum.rowSampleRate, 44100u);

  AudioColor bands = audioBandRgbColor(1.0f, 0.5f, 0.25f);
  CHECK_EQ(bands.r, 255);
  CHECK_EQ(bands.g, 128);
  CHECK_EQ(bands.b, 64);
  AudioColor splitBass = audioBandSplitColor(0, 0.75f, 0.5f, 0.25f);
  AudioColor splitMid = audioBandSplitColor(1, 0.75f, 0.5f, 0.25f);
  AudioColor splitTreble = audioBandSplitColor(2, 0.75f, 0.5f, 0.25f);
  CHECK(splitBass.r > 0 && splitBass.g == 0 && splitBass.b == 0);
  CHECK(splitMid.r == 0 && splitMid.g > 0 && splitMid.b == 0);
  CHECK(splitTreble.r == 0 && splitTreble.g == 0 && splitTreble.b > 0);

  return testReport("audio_reactive");
}
