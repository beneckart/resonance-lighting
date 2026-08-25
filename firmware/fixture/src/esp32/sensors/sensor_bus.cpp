#include "sensor_bus.h"

#include <Arduino.h>
#include <Wire.h>

#include "../../core/rtc_calendar.h"

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

static bool writeReg8(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  Wire1.write(val);
  return Wire1.endTransmission() == 0;
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
  if (second < 0 || minute < 0 || hour < 0 || day < 0 || month < 0 ||
      yearPart < 0)
    return false;
  int year = 2000 + yearPart + ((reg[5] & 0x80) ? 100 : 0);
  RtcCalendar calendar = {(uint16_t)year, (uint8_t)month, (uint8_t)day,
                          (uint8_t)hour, (uint8_t)minute, (uint8_t)second, 0};
  return rtcUtcFromCalendar(calendar, utcS);
}

static uint8_t toBcd(uint8_t value) {
  return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

bool sensorBusWriteRtcUtc(uint32_t utcS) {
  Wire1.setClock(100000); // ADR 0028: reassert, never raise
  RtcCalendar calendar = {};
  if (!rtcCalendarFromUtc(utcS, calendar)) return false;

  Wire1.beginTransmission(0x68);
  Wire1.write((uint8_t)0x00);
  Wire1.write(toBcd(calendar.second));
  Wire1.write(toBcd(calendar.minute));
  Wire1.write(toBcd(calendar.hour)); // explicit 24-hour mode
  Wire1.write(toBcd(calendar.weekday));
  Wire1.write(toBcd(calendar.day));
  Wire1.write(toBcd(calendar.month)); // 20xx, century bit clear
  Wire1.write(toBcd((uint8_t)(calendar.year - 2000U)));
  if (Wire1.endTransmission() != 0) return false;

  uint8_t status = 0;
  if (!readReg8(0x68, 0x0F, status)) return false;
  return writeReg8(0x68, 0x0F, (uint8_t)(status & ~0x80U));
}
