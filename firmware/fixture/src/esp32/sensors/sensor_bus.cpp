#include "sensor_bus.h"

#include <Arduino.h>
#include <Wire.h>

static bool ack(uint8_t addr) {
  Wire1.beginTransmission(addr);
  return Wire1.endTransmission() == 0;
}

static bool readReg8(uint8_t addr, uint8_t reg, uint8_t &val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom((int)addr, 1) != 1) return false;
  val = (uint8_t)Wire1.read();
  return true;
}

ProbeBits sensorBusProbe() {
  Wire1.setClock(100000); // ADR 0028: reassert, never raise
  ProbeBits bits = {};

  // TMF8820 @0x41 must be ID-verified: bench survivors may still carry an
  // INA219 at 0x41 which would otherwise classify a board as a downlight.
  // TMF882x ID register 0xE3 reads 0x08.
  if (ack(0x41)) {
    uint8_t id = 0;
    bits.tmf8820 = readReg8(0x41, 0xE3, id) && id == 0x08;
  }

  // VL53L5CX @0x29: plain ACK is sufficient -- TSL2591 (the old 0x29-adjacent
  // worry) is not used in production.
  bits.vl53l5cx = ack(0x29);

  // BMP581 @0x47 (alt address, per led_studio/net_bench): CHIP_ID 0x01 == 0x50.
  if (ack(0x47)) {
    uint8_t id = 0;
    bits.bmp581 = readReg8(0x47, 0x01, id) && id == 0x50;
  }

  // MSA311 @0x26: PART_ID 0x01 == 0x13. Logged only (not a discriminator).
  if (ack(0x26)) {
    uint8_t id = 0;
    bits.msa311 = readReg8(0x26, 0x01, id) && id == 0x13;
  }

  // Sparse time anchors do not determine fixture class. Their production
  // addresses are unique on the current STEMMA complement, so a read-only ACK
  // is sufficient for inventory. Time/fix validity is a later qualification.
  bits.samM8q = ack(0x42);
  bits.ds3231 = ack(0x68);

  Serial.printf("class probe: tmf8820=%d vl53l5cx=%d bmp581=%d msa311=%d "
                "sam_m8q=%d ds3231=%d\n",
                bits.tmf8820, bits.vl53l5cx, bits.bmp581, bits.msa311,
                bits.samM8q, bits.ds3231);
  return bits;
}

static int bcd(uint8_t value) {
  uint8_t lo = value & 0x0F;
  uint8_t hi = (value >> 4) & 0x0F;
  return lo <= 9 && hi <= 9 ? hi * 10 + lo : -1;
}

static int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? (unsigned)-3 : 9)) + 2) / 5 +
                       day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static int daysInMonth(int year, int month) {
  static const uint8_t kDays[] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
  if (month == 2 && (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0))
    return 29;
  return month >= 1 && month <= 12 ? kDays[month - 1] : 0;
}

bool sensorBusReadRtcUtc(uint32_t &utcS) {
  Wire1.setClock(100000); // ADR 0028: reassert, never raise
  uint8_t status = 0;
  if (!readReg8(0x68, 0x0F, status) || (status & 0x80)) return false;
  Wire1.beginTransmission(0x68);
  Wire1.write((uint8_t)0x00);
  if (Wire1.endTransmission(false) != 0) return false;
  if (Wire1.requestFrom(0x68, 7) != 7) return false;
  uint8_t reg[7];
  for (uint8_t &v : reg) v = (uint8_t)Wire1.read();

  int second = bcd(reg[0] & 0x7F);
  int minute = bcd(reg[1] & 0x7F);
  int hour;
  if (reg[2] & 0x40) {
    hour = bcd(reg[2] & 0x1F);
    if (hour < 1 || hour > 12) return false;
    hour %= 12;
    if (reg[2] & 0x20) hour += 12;
  } else {
    hour = bcd(reg[2] & 0x3F);
  }
  int day = bcd(reg[4] & 0x3F);
  int month = bcd(reg[5] & 0x1F);
  int yearPart = bcd(reg[6]);
  if (second < 0 || second > 59 || minute < 0 || minute > 59 || hour < 0 ||
      hour > 23 || month < 1 || month > 12 || yearPart < 0)
    return false;
  int year = 2000 + yearPart + ((reg[5] & 0x80) ? 100 : 0);
  if (year < 2025 || year >= 2036 || day < 1 || day > daysInMonth(year, month))
    return false;
  int64_t utc = daysFromCivil(year, (unsigned)month, (unsigned)day) * 86400LL +
                hour * 3600 + minute * 60 + second;
  if (utc < 0 || utc > UINT32_MAX) return false;
  utcS = (uint32_t)utc;
  return true;
}
