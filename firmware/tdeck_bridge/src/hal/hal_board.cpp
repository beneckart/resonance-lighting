#include "hal_board.h"

#include <Arduino.h>
#include <Wire.h>

#include "../core/nmea_time.h"
#include "pins_tdeck.h"

static ProbeReport gProbe = {};
static char gGpsSummary[64] = "gps: not probed";
static bool gGpsStreamOn = false;
static GpsUtcObservation gGpsUtc = {};

void halBoardPowerOn() {
  pinMode(TDECK_PIN_POWERON, OUTPUT);
  digitalWrite(TDECK_PIN_POWERON, HIGH);
  delay(500);  // keyboard aux MCU boot + rail settle
}

uint16_t halBatteryMv() {
  uint32_t mv = analogReadMilliVolts(TDECK_PIN_BAT_ADC);
  return (uint16_t)(mv * 2);  // 2:1 divider
}

static bool i2cAck(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

// Try one GPS UART configuration for windowMs; true if NMEA '$G' seen.
static bool gpsTry(uint32_t baud, int rxPin, int txPin, uint32_t windowMs) {
  Serial1.end();
  Serial1.begin(baud, SERIAL_8N1, rxPin, txPin);
  uint32_t t0 = millis();
  int state = 0;
  while (millis() - t0 < windowMs) {
    while (Serial1.available()) {
      char ch = (char)Serial1.read();
      if (state == 0 && ch == '$') state = 1;
      else if (state == 1) return ch == 'G';
      else state = 0;
    }
    delay(5);
  }
  return false;
}

void halProbeRun(ProbeReport *out) {
  gProbe.psram = psramFound();
  gProbe.psramBytes = ESP.getPsramSize();
  gProbe.keyboard = i2cAck(TDECK_I2C_ADDR_KEYBOARD);
  gProbe.touch = i2cAck(TDECK_I2C_ADDR_GT911_A) || i2cAck(TDECK_I2C_ADDR_GT911_B);
  gProbe.es7210Addr = 0;
  for (uint8_t a = 0x40; a <= 0x43; ++a) {
    if (i2cAck(a)) { gProbe.es7210Addr = a; break; }
  }

  gProbe.gpsNmea = false;
  gProbe.gpsBaud = 0;
  gProbe.gpsSwapped = false;
  const uint32_t bauds[] = {38400, 9600};
  for (uint32_t baud : bauds) {
    if (gpsTry(baud, TDECK_PIN_GPS_RX, TDECK_PIN_GPS_TX, 1200)) {
      gProbe.gpsNmea = true;
      gProbe.gpsBaud = baud;
      break;
    }
    if (gpsTry(baud, TDECK_PIN_GPS_TX, TDECK_PIN_GPS_RX, 1200)) {
      gProbe.gpsNmea = true;
      gProbe.gpsBaud = baud;
      gProbe.gpsSwapped = true;
      break;
    }
  }
  if (gProbe.gpsNmea) {
    // Leave Serial1 open on the working configuration for halGpsTick().
    int rx = gProbe.gpsSwapped ? TDECK_PIN_GPS_TX : TDECK_PIN_GPS_RX;
    int tx = gProbe.gpsSwapped ? TDECK_PIN_GPS_RX : TDECK_PIN_GPS_TX;
    Serial1.end();
    Serial1.begin(gProbe.gpsBaud, SERIAL_8N1, rx, tx);
    gGpsStreamOn = true;
    snprintf(gGpsSummary, sizeof(gGpsSummary), "gps: nmea@%lu no fix",
             (unsigned long)gProbe.gpsBaud);
  } else {
    Serial1.end();
    snprintf(gGpsSummary, sizeof(gGpsSummary), "gps: no nmea");
  }
  if (out) *out = gProbe;
}

const ProbeReport &halProbeLast() { return gProbe; }

static void gpsProcessLine(char *line) {
  if (strncmp(line, "$G", 2) != 0) return;
  NmeaUtcFix fix;
  if (nmeaParseRmcUtc(line, fix)) {
    gGpsUtc.valid = true;
    gGpsUtc.utcS = fix.utcS;
    gGpsUtc.subMs = fix.subMs;
    gGpsUtc.receivedMs = millis();
  }
  if (strstr(line, "GGA") == nullptr) return;
  // $..GGA,time,lat,NS,lon,EW,quality,numSats,...
  int commas = 0, quality = -1, sats = -1;
  for (char *p = line; *p; ++p) {
    if (*p != ',') continue;
    ++commas;
    if (commas == 6) quality = atoi(p + 1);
    if (commas == 7) { sats = atoi(p + 1); break; }
  }
  if (quality >= 0) {
    snprintf(gGpsSummary, sizeof(gGpsSummary), "gps: nmea@%lu %s sats=%d utc=%s",
             (unsigned long)gProbe.gpsBaud,
             quality > 0 ? "FIX" : "no fix", sats < 0 ? 0 : sats,
             gGpsUtc.valid ? "yes" : "no");
  }
}

void halGpsTick() {
  if (!gGpsStreamOn) return;
  static char line[100];
  static size_t n = 0;
  while (Serial1.available()) {
    char ch = (char)Serial1.read();
    if (ch == '\n' || n >= sizeof(line) - 1) {
      line[n] = 0;
      n = 0;
      gpsProcessLine(line);
    } else if (ch != '\r') {
      line[n++] = ch;
    }
  }
}

const char *halGpsSummary() { return gGpsSummary; }
GpsUtcObservation halGpsUtc() { return gGpsUtc; }
