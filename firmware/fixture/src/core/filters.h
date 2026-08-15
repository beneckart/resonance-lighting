// Sensor math (pure/native): accel gravity/sway/tilt chain (sway_demo, ADR
// 0027) and the robust ground-plane least-squares fit for ToF tilt.
#pragma once

#include <stdint.h>

// Accel chain: gravity low-pass (alpha ~0.06 at 25 Hz sampling = ~0.4 s),
// sway = |sample - gravity| through a fast-attack/slow-decay envelope, tilt =
// angle between the gravity unit vector and the calibrated rest vector
// (spin-invariant -- no dependence on azimuth).
struct AccelFilter {
  float gx, gy, gz;       // gravity low-pass (g)
  bool seeded;
  float swayEnv;          // envelope (g)
  float restX, restY, restZ; // calibrated rest gravity unit vector
  bool restZeroed;
  float tiltDeg;
};

void accelFilterInit(AccelFilter &f);
// Feed one sample (g). alphaG ~0.06, attack 0.5, decay 0.96 (donor constants).
void accelFilterSample(AccelFilter &f, float ax, float ay, float az);
// Capture the current gravity direction as "rest" (auto-zero at boot).
bool accelFilterZero(AccelFilter &f);

// Plane fit z = a*x + b*y + c over kept points (least squares, donor verbatim).
// Returns false with fewer than minPoints kept or a degenerate determinant.
bool planeFitLS(const float *px, const float *py, const float *pz, const bool *keep,
                uint8_t n, uint8_t minPoints, float *a, float *b, float *c);

// Tilt between two plane normals derived from fits (deg). n = (-a, -b, 1).
float planeTiltDeg(float a, float b, float a0, float b0);
