#include "../../fixture/tests/test_util.h"

#include "../audio_reactive.h"

int main() {
  AudioPeriodicDeadline deadline;
  deadline.reset(1000);
  CHECK(deadline.take(1000, 40));
  CHECK(!deadline.take(1039, 40));
  CHECK(deadline.take(1040, 40));
  CHECK_EQ(deadline.dispatches, 2u);
  CHECK_EQ(deadline.skippedPeriods, 0u);

  // Regression for the former 8.3 Hz bug. Even if a 100 ms publisher is only
  // polled on 40 ms analysis boundaries, its phase remains 100 ms: the first
  // late dispatch is at 120, then the next deadline is 200 rather than 220.
  AudioPeriodicDeadline quantized;
  quantized.reset(0);
  CHECK(quantized.take(0, 100));
  CHECK(!quantized.take(40, 100));
  CHECK(!quantized.take(80, 100));
  CHECK(quantized.take(120, 100));
  CHECK_EQ(quantized.nextMs, 200u);
  CHECK(quantized.take(200, 100));
  CHECK_EQ(quantized.dispatches, 3u);
  CHECK_EQ(quantized.skippedPeriods, 0u);
  CHECK_EQ(quantized.maxLatenessMs, 20u);

  AudioPeriodicDeadline overrun;
  overrun.reset(0);
  CHECK(overrun.take(0, 100));
  CHECK(overrun.take(250, 100));
  CHECK_EQ(overrun.nextMs, 300u);
  CHECK_EQ(overrun.skippedPeriods, 1u);
  CHECK_EQ(overrun.maxLatenessMs, 150u);

  // Signed-delta deadline comparison remains valid across millis() wrap.
  AudioPeriodicDeadline wrapping;
  wrapping.reset(0xFFFFFFF0u, 20);
  CHECK(!wrapping.take(3u, 40));
  CHECK(wrapping.take(4u, 40));
  CHECK_EQ(wrapping.nextMs, 44u);

  AudioCadenceStats cadence;
  cadence.note(1000);
  cadence.note(1040);
  cadence.note(1080);
  CHECK_EQ(cadence.count, 3u);
  CHECK_EQ(cadence.intervalMinMs(), 40u);
  CHECK_EQ(cadence.intervalMaxMs(), 40u);
  CHECK_EQ(cadence.rateMilliHz(), 25000u);

  AudioCalibrationClock calibration;
  calibration.noteCapture(100);
  CHECK(!calibration.calibrated());
  CHECK(calibration.calibratingCurrentCapture());
  for (uint32_t now = 140; now <= 2060; now += 40)
    calibration.noteCapture(now);
  CHECK(!calibration.calibrated());
  calibration.noteCapture(2100);
  CHECK(calibration.calibrated());
  CHECK(calibration.calibratingCurrentCapture());
  calibration.noteCapture(2140);
  CHECK(calibration.calibrated());
  CHECK(!calibration.calibratingCurrentCapture());
  CHECK(calibration.captureFresh(2200));
  CHECK(!calibration.captureFresh(2400));
  CHECK_EQ(calibration.elapsedMs(), 2040u);
  CHECK_EQ(calibration.observationCount16(), 52u);

  AudioCalibrationClock interruptedCalibration;
  interruptedCalibration.noteCapture(100);
  interruptedCalibration.noteCapture(140);
  interruptedCalibration.noteCapture(1000);
  CHECK(!interruptedCalibration.calibrated());
  CHECK_EQ(interruptedCalibration.observations, 1u);
  CHECK_EQ(interruptedCalibration.elapsedMs(), 0u);

  AudioRuntimeTiming timing;
  timing.reset(500);
  CHECK(timing.analysisDeadline.take(500, AUDIO_ANALYSIS_PERIOD_MS));
  CHECK(!timing.publishDeadline.take(500, AUDIO_FIXTURE_PERIOD_MS));
  CHECK(timing.publishDeadline.take(600, AUDIO_FIXTURE_PERIOD_MS));
  CHECK(!timing.displayDeadline.take(539, AUDIO_ANALYSIS_PERIOD_MS));
  CHECK(timing.displayDeadline.take(540, AUDIO_ANALYSIS_PERIOD_MS));
  AudioRuntimeTiming::noteMax(123, &timing.captureMaxUs);
  AudioRuntimeTiming::noteMax(100, &timing.captureMaxUs);
  CHECK_EQ(timing.captureMaxUs, 123u);

  return testReport("audio_timing");
}
