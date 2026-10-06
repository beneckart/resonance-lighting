#include <stdio.h>

#include "core/wand_gesture.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static void arm(WandGesture &g, uint32_t startMs, float pressureHpa) {
  for (uint32_t t = 0; t <= WAND_STEADY_ARM_MS; t += 40)
    wandGestureSample(g, startMs + t, true, 0.01f, pressureHpa);
}

static bool completeLift(WandGesture &g, uint32_t startMs,
                         float basePressureHpa) {
  bool fired = false;
  fired |= wandGestureSample(g, startMs, true, 0.10f, basePressureHpa);
  // About 0.67 m at this pressure: decisively above the 0.45 m threshold.
  fired |= wandGestureSample(g, startMs + 200, true, 0.08f,
                             basePressureHpa - 0.08f);
  for (uint32_t t = 240; t <= 1200; t += 40)
    fired |= wandGestureSample(g, startMs + t, true, 0.01f,
                               basePressureHpa - 0.08f);
  return fired;
}

int main() {
  CHECK(wandPressureRiseM(1000.0f, 999.92f) > WAND_RISE_TRIGGER_M);

  WandGesture gesture;
  wandGestureInit(gesture);
  arm(gesture, 1000, 1000.0f);
  CHECK(gesture.state == WAND_GESTURE_ARMED);
  CHECK(completeLift(gesture, 3000, 1000.0f));
  CHECK(gesture.state == WAND_GESTURE_WAIT_RETURN);
  CHECK(gesture.triggerCount == 1);

  // Merely holding the wand high cannot retrigger.
  for (uint32_t t = 0; t < 3000; t += 40)
    CHECK(!wandGestureSample(gesture, 5000 + t, true, 0.01f, 999.92f));
  CHECK(gesture.triggerCount == 1);

  // Lowering and holding returns to the normal arming path.
  for (uint32_t t = 0; t < 200; t += 40)
    CHECK(!wandGestureSample(gesture, 8000 + t, true, 0.08f, 1000.0f));
  for (uint32_t t = 200; t <= 400; t += 40)
    CHECK(!wandGestureSample(gesture, 8000 + t, true, 0.01f, 1000.0f));
  CHECK(gesture.state == WAND_GESTURE_SEEK_STEADY);

  // A hard knock has motion but no corroborating height and is rejected.
  WandGesture knock;
  wandGestureInit(knock);
  arm(knock, 0, 1000.0f);
  CHECK(!wandGestureSample(knock, 1800, true, 1.2f, 1000.0f));
  CHECK(knock.state == WAND_GESTURE_SEEK_STEADY);
  CHECK(knock.triggerCount == 0);

  // A shock after qualifying the height also cancels the candidate. Remaining
  // still at that height must not resume the old lift and fire a fleet action.
  WandGesture highKnock;
  wandGestureInit(highKnock);
  arm(highKnock, 0, 1000.0f);
  CHECK(!wandGestureSample(highKnock, 1800, true, 0.10f, 1000.0f));
  CHECK(!wandGestureSample(highKnock, 2000, true, 0.01f, 999.92f));
  CHECK(highKnock.state == WAND_GESTURE_HIGH_HOLD);
  CHECK(!wandGestureSample(highKnock, 2200, true,
                           WAND_REJECT_SHOCK_G, 999.92f));
  CHECK(highKnock.state == WAND_GESTURE_SEEK_STEADY);
  for (uint32_t t = 2240; t <= 5000; t += 40)
    CHECK(!wandGestureSample(highKnock, t, true, 0.01f, 999.92f));
  CHECK(highKnock.triggerCount == 0);

  // Pressure drift without an accelerometer-observed lift is rejected.
  WandGesture drift;
  wandGestureInit(drift);
  arm(drift, 0, 1000.0f);
  for (uint32_t t = 1800; t < 5000; t += 40)
    CHECK(!wandGestureSample(drift, t, true, 0.01f, 999.90f));
  CHECK(drift.triggerCount == 0);

  // Motion without the corresponding barometric rise is also rejected.
  WandGesture wave;
  wandGestureInit(wave);
  arm(wave, 0, 1000.0f);
  CHECK(!wandGestureSample(wave, 1800, true, 0.10f, 1000.0f));
  for (uint32_t t = 1840; t < 7000; t += 40)
    CHECK(!wandGestureSample(wave, t, true, 0.01f, 1000.0f));
  CHECK(wave.triggerCount == 0);

  // Losing either sensor clears a half-complete gesture.
  WandGesture invalid;
  wandGestureInit(invalid);
  arm(invalid, 0, 1000.0f);
  CHECK(!wandGestureSample(invalid, 1800, true, 0.10f, 1000.0f));
  CHECK(!wandGestureSample(invalid, 2000, false, 0.01f, 999.92f));
  CHECK(invalid.state == WAND_GESTURE_SEEK_STEADY);

  printf("wand_gesture ok\n");
  return 0;
}
