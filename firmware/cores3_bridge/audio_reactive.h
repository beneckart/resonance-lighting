#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

struct AudioColor {
  uint8_t r, g, b, w;
};

static constexpr size_t AUDIO_DIRECT_ENTRIES_PER_FRAME = 18;
static constexpr size_t AUDIO_FFT_SIZE = 512;
static constexpr size_t AUDIO_SPECTRUM_ROWS = 24;
static constexpr uint16_t AUDIO_ANALYSIS_HZ = 25;
static constexpr uint16_t AUDIO_FIXTURE_HZ = 10;
static constexpr uint32_t AUDIO_ANALYSIS_PERIOD_MS = 1000 / AUDIO_ANALYSIS_HZ;
static constexpr uint32_t AUDIO_FIXTURE_PERIOD_MS = 1000 / AUDIO_FIXTURE_HZ;
static constexpr uint32_t AUDIO_CALIBRATION_MS = 2000;
static constexpr uint32_t AUDIO_CALIBRATION_MAX_GAP_MS = 200;
// Legacy users of AudioEnvelope/AudioSpectrum still get a bounded frame-count
// calibration. CoreS3 supplies the elapsed successful-capture window instead.
static constexpr uint16_t AUDIO_CALIBRATION_FRAMES = 50;
static constexpr float AUDIO_PI = 3.14159265358979323846f;

// Phase-locked periodic deadline. Dispatch advances from the prior deadline,
// never from a late `now`, so polling a 100 ms lane on a 40 ms grid alternates
// around 100 ms instead of silently degrading to 120 ms forever. Missed periods
// are skipped and counted; callers never issue catch-up bursts.
struct AudioPeriodicDeadline {
  uint32_t nextMs = 0;
  uint32_t dispatches = 0;
  uint32_t skippedPeriods = 0;
  uint32_t maxLatenessMs = 0;
  bool armed = false;

  void reset(uint32_t nowMs, uint32_t initialDelayMs = 0) {
    nextMs = nowMs + initialDelayMs;
    dispatches = 0;
    skippedPeriods = 0;
    maxLatenessMs = 0;
    armed = true;
  }

  bool take(uint32_t nowMs, uint32_t periodMs) {
    if (!periodMs) return false;
    if (!armed) reset(nowMs);
    if ((int32_t)(nowMs - nextMs) < 0) return false;

    uint32_t lateness = nowMs - nextMs;
    if (lateness > maxLatenessMs) maxLatenessMs = lateness;
    uint32_t elapsedPeriods = lateness / periodMs;
    skippedPeriods += elapsedPeriods;
    nextMs += (elapsedPeriods + 1) * periodMs;
    ++dispatches;
    return true;
  }
};

struct AudioCadenceStats {
  uint32_t count = 0;
  uint32_t firstMs = 0;
  uint32_t lastMs = 0;
  uint32_t minIntervalMs = UINT32_MAX;
  uint32_t maxIntervalMs = 0;

  void reset() {
    count = 0;
    firstMs = 0;
    lastMs = 0;
    minIntervalMs = UINT32_MAX;
    maxIntervalMs = 0;
  }

  void note(uint32_t nowMs) {
    if (!count) {
      firstMs = lastMs = nowMs;
      count = 1;
      return;
    }
    uint32_t interval = nowMs - lastMs;
    if (interval < minIntervalMs) minIntervalMs = interval;
    if (interval > maxIntervalMs) maxIntervalMs = interval;
    lastMs = nowMs;
    ++count;
  }

  uint32_t intervalMinMs() const {
    return count >= 2 ? minIntervalMs : 0;
  }

  uint32_t intervalMaxMs() const {
    return count >= 2 ? maxIntervalMs : 0;
  }

  // Achieved Hz * 1000 over the complete observation window.
  uint32_t rateMilliHz() const {
    if (count < 2) return 0;
    uint32_t elapsed = lastMs - firstMs;
    if (!elapsed) return 0;
    return (uint32_t)(((uint64_t)(count - 1) * 1000000ULL + elapsed / 2) /
                      elapsed);
  }
};

// Two seconds measured across successful capture completions. This preserves a
// two-second room observation if analysis cadence changes, while a failed input
// cannot become calibrated without at least two successful captures.
struct AudioCalibrationClock {
  uint32_t firstCaptureMs = 0;
  uint32_t lastCaptureMs = 0;
  uint32_t observations = 0;

  void reset() {
    firstCaptureMs = 0;
    lastCaptureMs = 0;
    observations = 0;
  }

  void noteCapture(uint32_t nowMs) {
    if (observations && nowMs - lastCaptureMs > AUDIO_CALIBRATION_MAX_GAP_MS) {
      firstCaptureMs = nowMs;
      lastCaptureMs = nowMs;
      observations = 1;
      return;
    }
    if (!observations) firstCaptureMs = nowMs;
    lastCaptureMs = nowMs;
    ++observations;
  }

  uint32_t elapsedMs() const {
    return observations >= 2 ? lastCaptureMs - firstCaptureMs : 0;
  }

  bool calibrated() const {
    return observations >= 2 && elapsedMs() >= AUDIO_CALIBRATION_MS;
  }

  bool captureFresh(uint32_t nowMs) const {
    return observations &&
           nowMs - lastCaptureMs <= AUDIO_CALIBRATION_MAX_GAP_MS;
  }

  bool calibratingCurrentCapture() const {
    return !calibrated() || elapsedMs() == AUDIO_CALIBRATION_MS;
  }

  uint16_t observationCount16() const {
    return observations > UINT16_MAX ? UINT16_MAX : (uint16_t)observations;
  }
};

struct AudioRuntimeTiming {
  AudioPeriodicDeadline analysisDeadline;
  AudioPeriodicDeadline publishDeadline;
  AudioPeriodicDeadline displayDeadline;
  AudioCadenceStats capture;
  AudioCadenceStats analysis;
  AudioCadenceStats publish;
  AudioCadenceStats display;
  uint32_t captureMaxUs = 0;
  uint32_t analysisMaxUs = 0;
  uint32_t publishMaxUs = 0;
  uint32_t displayMaxUs = 0;
  uint32_t loopMaxUs = 0;

  void reset(uint32_t nowMs) {
    analysisDeadline.reset(nowMs);
    publishDeadline.reset(nowMs, AUDIO_FIXTURE_PERIOD_MS);
    displayDeadline.reset(nowMs, AUDIO_ANALYSIS_PERIOD_MS);
    capture.reset();
    analysis.reset();
    publish.reset();
    display.reset();
    captureMaxUs = 0;
    analysisMaxUs = 0;
    publishMaxUs = 0;
    displayMaxUs = 0;
    loopMaxUs = 0;
  }

  static void noteMax(uint32_t value, uint32_t *maximum) {
    if (maximum && value > *maximum) *maximum = value;
  }
};

enum AudioVisualMode : uint8_t {
  AUDIO_MODE_CLASSIC = 0, // per-slot R/G/B driven by the broadband envelope
  AUDIO_MODE_EMBER,       // shared warm RGBW driven by the broadband envelope
  AUDIO_MODE_HUECYCLE,    // shared slow hue rotation, envelope brightness
  AUDIO_MODE_PULSE,       // broadband transient flash over a dim floor
  AUDIO_MODE_BAND_RGB,    // bass/mid/treble drive shared red/green/blue
  AUDIO_MODE_BAND_SPLIT,  // fixture thirds each follow one frequency band
  AUDIO_MODE_TIMBRE_HUE,  // spectral centroid selects hue, energy selects value
  AUDIO_MODE_COUNT,
};

enum AudioOutputGain : uint8_t {
  AUDIO_GAIN_1X = 0,
  AUDIO_GAIN_1_5X,
  AUDIO_GAIN_2X,
  AUDIO_GAIN_3X,
  AUDIO_GAIN_COUNT,
};

inline float audioClampUnit(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

inline float audioOutputGainMultiplier(AudioOutputGain gain) {
  switch (gain) {
  case AUDIO_GAIN_1_5X: return 1.5f;
  case AUDIO_GAIN_2X: return 2.0f;
  case AUDIO_GAIN_3X: return 3.0f;
  default: return 1.0f;
  }
}

inline AudioOutputGain audioNextOutputGain(AudioOutputGain gain) {
  return (AudioOutputGain)(((uint8_t)gain + 1) % AUDIO_GAIN_COUNT);
}

inline uint8_t audioGainChannel(uint8_t value, AudioOutputGain gain) {
  float scaled = (float)value * audioOutputGainMultiplier(gain);
  return scaled >= 255.0f ? 255 : (uint8_t)(scaled + 0.5f);
}

inline AudioColor audioApplyOutputGain(AudioColor color,
                                       AudioOutputGain gain) {
  color.r = audioGainChannel(color.r, gain);
  color.g = audioGainChannel(color.g, gain);
  color.b = audioGainChannel(color.b, gain);
  color.w = audioGainChannel(color.w, gain);
  return color;
}

inline size_t audioDirectFrameCount(size_t fixtureCount) {
  return fixtureCount
             ? (fixtureCount + AUDIO_DIRECT_ENTRIES_PER_FRAME - 1) /
                   AUDIO_DIRECT_ENTRIES_PER_FRAME
             : 0;
}

struct AudioEnvelope {
  float rms = 0.0f;
  float noise = 0.0f;
  float ceiling = 512.0f;
  float level = 0.0f;
  uint16_t calibrationFrames = 0;

  bool calibrated() const {
    return calibrationFrames >= AUDIO_CALIBRATION_FRAMES;
  }

  float update(const int16_t *samples, size_t count, bool calibrating,
               uint16_t calibrationObservation) {
    if (!samples || !count) return level;

    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) sum += samples[i];
    float mean = (float)sum / (float)count;

    double sumSquares = 0.0;
    for (size_t i = 0; i < count; ++i) {
      float centered = (float)samples[i] - mean;
      sumSquares += (double)centered * centered;
    }
    rms = sqrtf((float)(sumSquares / (double)count));

    if (calibrationObservation > calibrationFrames)
      calibrationFrames = calibrationObservation;
    if (calibrating) {
      uint16_t observation = calibrationObservation ? calibrationObservation : 1;
      noise += (rms - noise) / (float)observation;
      ceiling = noise + 512.0f;
      level = 0.0f;
      return level;
    }

    // Follow a quiet room slowly, but never let a beat drag the floor upward.
    if (rms < noise * 1.5f + 64.0f) noise += (rms - noise) * 0.01f;
    float minimumCeiling = noise + 512.0f;
    ceiling *= 0.985f;
    if (ceiling < minimumCeiling) ceiling = minimumCeiling;
    if (rms > ceiling) ceiling = rms;

    float normalized = (rms - noise) / (ceiling - noise);
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    normalized = sqrtf(normalized); // useful response before clipping

    // Fast attack, slower release: beats remain crisp without display chatter.
    float blend = normalized > level ? 0.65f : 0.15f;
    level += (normalized - level) * blend;
    if (level < 0.002f) level = 0.0f;
    return level;
  }

  float update(const int16_t *samples, size_t count) {
    bool calibrating = !calibrated();
    uint16_t observation = calibrating
                               ? (uint16_t)(calibrationFrames + 1)
                               : calibrationFrames;
    return update(samples, count, calibrating, observation);
  }
};

struct AudioBandEnvelope {
  float raw = 0.0f;
  float floor = 0.0f;
  float ceiling = 128.0f;
  float level = 0.0f;

  void reset() {
    raw = 0.0f;
    floor = 0.0f;
    ceiling = 128.0f;
    level = 0.0f;
  }

  float update(float value, bool calibrating, uint16_t calibrationFrame) {
    raw = value > 0.0f ? value : 0.0f;
    if (calibrating) {
      floor += (raw - floor) / (float)calibrationFrame;
      ceiling = floor + 128.0f;
      level = 0.0f;
      return level;
    }

    // Each band carries its own slow noise floor and peak memory. Independent
    // AGC keeps a quiet treble band usable beside a loud bass line.
    if (raw < floor * 1.5f + 32.0f) floor += (raw - floor) * 0.01f;
    float minimumCeiling = floor + 128.0f;
    ceiling *= 0.99f;
    if (ceiling < minimumCeiling) ceiling = minimumCeiling;
    if (raw > ceiling) ceiling = raw;

    float normalized = audioClampUnit((raw - floor) / (ceiling - floor));
    normalized = sqrtf(normalized);
    float blend = normalized > level ? 0.60f : 0.12f;
    level += (normalized - level) * blend;
    if (level < 0.002f) level = 0.0f;
    return level;
  }

};

struct AudioSpectrum {
  AudioBandEnvelope bass;
  AudioBandEnvelope mid;
  AudioBandEnvelope treble;
  float energy = 0.0f;
  float centroid = 0.0f; // 0 = low end, 1 = 8 kHz
  uint8_t rows[AUDIO_SPECTRUM_ROWS] = {};
  uint16_t calibrationFrames = 0;
  uint32_t analysisFrames = 0;

  // Fixed storage avoids heap churn in the real-time path. About 8 KB total.
  float real[AUDIO_FFT_SIZE] = {};
  float imag[AUDIO_FFT_SIZE] = {};
  float magnitude[AUDIO_FFT_SIZE / 2] = {};
  float window[AUDIO_FFT_SIZE] = {};
  uint16_t rowFirst[AUDIO_SPECTRUM_ROWS] = {};
  uint16_t rowLast[AUDIO_SPECTRUM_ROWS] = {};
  uint32_t rowSampleRate = 0;
  bool windowReady = false;

  bool calibrated() const {
    return calibrationFrames >= AUDIO_CALIBRATION_FRAMES;
  }

  void reset() {
    bass.reset();
    mid.reset();
    treble.reset();
    energy = 0.0f;
    centroid = 0.0f;
    memset(rows, 0, sizeof(rows));
    calibrationFrames = 0;
    analysisFrames = 0;
    memset(real, 0, sizeof(real));
    memset(imag, 0, sizeof(imag));
    memset(magnitude, 0, sizeof(magnitude));
    rowSampleRate = 0;
    // The Hann coefficients are source-independent and can survive an Input
    // handoff; avoid 512 fresh cosine calls after every recalibration.
  }

  bool update(const int16_t *samples, size_t count, uint32_t sampleRate,
              bool calibrating, uint16_t calibrationObservation) {
    if (!samples || count != AUDIO_FFT_SIZE || sampleRate < 1000) return false;

    if (!windowReady) {
      for (size_t i = 0; i < AUDIO_FFT_SIZE; ++i) {
        float phase = (float)i / (float)(AUDIO_FFT_SIZE - 1);
        window[i] = 0.5f - 0.5f * cosf(2.0f * AUDIO_PI * phase);
      }
      windowReady = true;
    }

    int64_t sum = 0;
    for (size_t i = 0; i < AUDIO_FFT_SIZE; ++i) sum += samples[i];
    float mean = (float)sum / (float)AUDIO_FFT_SIZE;
    for (size_t i = 0; i < AUDIO_FFT_SIZE; ++i) {
      real[i] = ((float)samples[i] - mean) * window[i];
      imag[i] = 0.0f;
    }

    // In-place radix-2 FFT with one twiddle calculation per stage.
    for (size_t i = 1, j = 0; i < AUDIO_FFT_SIZE; ++i) {
      size_t bit = AUDIO_FFT_SIZE >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) {
        float tmp = real[i]; real[i] = real[j]; real[j] = tmp;
        tmp = imag[i]; imag[i] = imag[j]; imag[j] = tmp;
      }
    }
    for (size_t length = 2; length <= AUDIO_FFT_SIZE; length <<= 1) {
      float angle = -2.0f * AUDIO_PI / (float)length;
      float stepR = cosf(angle);
      float stepI = sinf(angle);
      for (size_t start = 0; start < AUDIO_FFT_SIZE; start += length) {
        float wr = 1.0f;
        float wi = 0.0f;
        size_t half = length >> 1;
        for (size_t j = 0; j < half; ++j) {
          size_t even = start + j;
          size_t odd = even + half;
          float tr = wr * real[odd] - wi * imag[odd];
          float ti = wr * imag[odd] + wi * real[odd];
          real[odd] = real[even] - tr;
          imag[odd] = imag[even] - ti;
          real[even] += tr;
          imag[even] += ti;
          float nextWr = wr * stepR - wi * stepI;
          wi = wr * stepI + wi * stepR;
          wr = nextWr;
        }
      }
    }

    // Hann coherent gain is 0.5, so 4/N reconstructs a sine's amplitude.
    magnitude[0] = 0.0f;
    for (size_t bin = 1; bin < AUDIO_FFT_SIZE / 2; ++bin) {
      magnitude[bin] =
          sqrtf(real[bin] * real[bin] + imag[bin] * imag[bin]) *
          (4.0f / (float)AUDIO_FFT_SIZE);
    }

    double bassSq = 0.0, midSq = 0.0, trebleSq = 0.0;
    size_t bassCount = 0, midCount = 0, trebleCount = 0;
    double weightedHz = 0.0, weight = 0.0;
    float usefulHigh = sampleRate / 2.0f;
    if (usefulHigh > 8000.0f) usefulHigh = 8000.0f;
    for (size_t bin = 1; bin < AUDIO_FFT_SIZE / 2; ++bin) {
      float hz = (float)bin * (float)sampleRate / (float)AUDIO_FFT_SIZE;
      float mag = magnitude[bin];
      if (hz >= 60.0f && hz < 250.0f) {
        bassSq += (double)mag * mag;
        ++bassCount;
      } else if (hz >= 250.0f && hz < 2000.0f) {
        midSq += (double)mag * mag;
        ++midCount;
      } else if (hz >= 2000.0f && hz <= usefulHigh) {
        trebleSq += (double)mag * mag;
        ++trebleCount;
      }
      if (hz >= 60.0f && hz <= usefulHigh) {
        weightedHz += (double)hz * mag;
        weight += mag;
      }
    }

    float bassRaw = bassCount ? sqrtf((float)(bassSq / bassCount)) : 0.0f;
    float midRaw = midCount ? sqrtf((float)(midSq / midCount)) : 0.0f;
    float trebleRaw = trebleCount ? sqrtf((float)(trebleSq / trebleCount)) : 0.0f;
    if (calibrationObservation > calibrationFrames)
      calibrationFrames = calibrationObservation;
    uint16_t frame = calibrationObservation ? calibrationObservation : 1;
    bass.update(bassRaw, calibrating, frame);
    mid.update(midRaw, calibrating, frame);
    treble.update(trebleRaw, calibrating, frame);
    energy = (bass.level + mid.level + treble.level) / 3.0f;
    centroid = weight > 0.0
                   ? audioClampUnit(((float)(weightedHz / weight) - 60.0f) /
                                    (usefulHigh - 60.0f))
                   : 0.0f;

    // Log-frequency display rows: 60 Hz at row 0 through 8 kHz (or Nyquist).
    if (rowSampleRate != sampleRate) {
      float ratio = usefulHigh > 60.0f ? usefulHigh / 60.0f : 1.0f;
      for (size_t row = 0; row < AUDIO_SPECTRUM_ROWS; ++row) {
        float low = 60.0f * powf(ratio, (float)row / AUDIO_SPECTRUM_ROWS);
        float high = 60.0f * powf(ratio, (float)(row + 1) /
                                             AUDIO_SPECTRUM_ROWS);
        size_t first = (size_t)ceilf(low * AUDIO_FFT_SIZE / sampleRate);
        size_t last = (size_t)ceilf(high * AUDIO_FFT_SIZE / sampleRate);
        if (first < 1) first = 1;
        if (last <= first) last = first + 1;
        if (last > AUDIO_FFT_SIZE / 2) last = AUDIO_FFT_SIZE / 2;
        rowFirst[row] = (uint16_t)first;
        rowLast[row] = (uint16_t)last;
      }
      rowSampleRate = sampleRate;
    }
    float logDenominator = log1pf(32768.0f);
    for (size_t row = 0; row < AUDIO_SPECTRUM_ROWS; ++row) {
      float peak = 0.0f;
      for (size_t bin = rowFirst[row]; bin < rowLast[row]; ++bin)
        if (magnitude[bin] > peak) peak = magnitude[bin];
      float visible = audioClampUnit(log1pf(peak) / logDenominator);
      rows[row] = (uint8_t)(visible * 255.0f + 0.5f);
    }
    ++analysisFrames;
    return true;
  }

  bool update(const int16_t *samples, size_t count, uint32_t sampleRate) {
    bool calibrating = !calibrated();
    uint16_t observation = calibrating
                               ? (uint16_t)(calibrationFrames + 1)
                               : calibrationFrames;
    return update(samples, count, sampleRate, calibrating, observation);
  }
};

inline void audioStereoToMono(const int16_t *stereo, int16_t *mono,
                              size_t frames) {
  if (!stereo || !mono) return;
  for (size_t i = 0; i < frames; ++i) {
    int32_t mixed = (int32_t)stereo[i * 2] + stereo[i * 2 + 1];
    mono[i] = (int16_t)(mixed / 2);
  }
}

inline AudioColor audioColorForSlot(uint8_t slot, float level) {
  level = audioClampUnit(level);
  uint8_t v = (uint8_t)(level * 255.0f + 0.5f);
  switch (slot % 3) {
  case 0: return {v, (uint8_t)(v / 4), 0, 0};
  case 1: return {0, v, (uint8_t)(v / 8), 0};
  default: return {(uint8_t)(v / 8), (uint8_t)(v / 3), v, 0};
  }
}

inline AudioColor audioBandRgbColor(float bass, float mid, float treble) {
  bass = audioClampUnit(bass);
  mid = audioClampUnit(mid);
  treble = audioClampUnit(treble);
  return {(uint8_t)(bass * 255.0f + 0.5f),
          (uint8_t)(mid * 255.0f + 0.5f),
          (uint8_t)(treble * 255.0f + 0.5f), 0};
}

inline AudioColor audioBandSplitColor(uint8_t slot, float bass, float mid,
                                      float treble) {
  switch (slot % 3) {
  case 0: return audioBandRgbColor(bass, 0.0f, 0.0f);
  case 1: return audioBandRgbColor(0.0f, mid, 0.0f);
  default: return audioBandRgbColor(0.0f, 0.0f, treble);
  }
}
