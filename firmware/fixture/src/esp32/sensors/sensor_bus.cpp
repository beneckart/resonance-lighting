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

  Serial.printf("class probe: tmf8820=%d vl53l5cx=%d bmp581=%d msa311=%d\n",
                bits.tmf8820, bits.vl53l5cx, bits.bmp581, bits.msa311);
  return bits;
}
