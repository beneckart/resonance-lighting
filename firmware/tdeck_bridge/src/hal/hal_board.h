#pragma once

#include <stdint.h>

// Board power rail, battery, and the M0 presence probes (mic ADC, GPS).

void halBoardPowerOn();  // TDECK_PIN_POWERON high + settle delay; call FIRST

uint16_t halBatteryMv();  // calibrated ADC read behind the 2:1 divider

struct ProbeReport {
  bool psram;
  uint32_t psramBytes;
  bool keyboard;
  bool touch;
  uint8_t es7210Addr;  // 0 = not found (candidates 0x40..0x43)
  bool gpsNmea;        // saw '$G' NMEA traffic during the probe window
  uint32_t gpsBaud;    // baud that produced NMEA (0 if none)
  bool gpsSwapped;     // true if RX/TX had to be swapped vs pins_tdeck.h naming
};
void halProbeRun(ProbeReport *out);       // one-shot, ~5 s worst case (GPS)
const ProbeReport &halProbeLast();

// GPS pass-through used by the status page after a successful probe: drains
// pending NMEA bytes and returns the latest fix summary line ("no fix" until
// GGA/RMC report one).
const char *halGpsSummary();
void halGpsTick();

struct GpsUtcObservation {
  bool valid;
  uint32_t utcS;       // UTC at the RMC observation
  uint16_t subMs;
  uint32_t receivedMs; // local millis() when that RMC line completed
};
GpsUtcObservation halGpsUtc();
