// Class-dispatched sensor stack, all cooperative on the shared Wire1 bus at
// 100 kHz from loop context (ADR 0028; the 2026-07-29 blocking-driver lesson).
//   downlight: MSA311 + TMF8820 (down) + optional BMP581 environmental logger
//   perimeter: MSA311 + VL53L5CX (out)
//   uplight:   MSA311 + BMP581           chandelier: none
#pragma once

#include <stdint.h>

struct SensorSnapshot {
  // MSA311 accel chain
  bool msaPresent, msaOk;
  float tiltDeg;    // vs calibrated rest, spin-invariant
  float swayEnvG;   // sway envelope (g)
  // TMF8820 depth (downlight)
  bool tmfPresent, tmfOk;
  uint16_t tofDepthMm;       // closest confident scene target (80..2500 mm)
  float tofDepthFilteredMm;  // EMA 0.35
  uint16_t tofConfidence;
  uint16_t tofZoneMm[9];         // closest confident return per 3x3 channel
  uint16_t tofZoneConfidence[9];
  uint32_t tmfReads, tmfErrors, tmfRecoveries;
  uint8_t tmfDomainResets; // bounded full VSQT + driver rebuilds this boot
  // VL53L5CX plane fit (perimeter)
  bool vlPresent, vlOk;
  float vlTiltDeg;   // ground-plane tilt vs boot-captured rest plane
  uint8_t vlZones;   // zones kept in the last fit
  uint16_t vlClosestMm; // closest valid return (presence proxy)
  // BMP581 env (uplight)
  bool bmpPresent, bmpOk;
  float tempC;
  float pressureHpa; // EMA 0.25
};

void sensorsInit(uint8_t fixtureClass);
// Cooperative in steady state. Its one bounded domain recovery may synchronously
// reload TMF firmware after three consecutive failed measurement cycles.
void sensorsTick();
const SensorSnapshot &sensors();
