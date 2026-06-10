// ina_monitor -- Adafruit Metro ESP32-S3 as a standalone 4-channel power monitor for the
// Resonance bench. Reads 4x INA219 (DFRobot SEN0291) on the Metro's I2C (SDA=47/SCL=48,
// the STEMMA-QT / header bus) and streams V/I per channel so the host logs a board-under-
// test's current GROUND-TRUTH -- gauge-independent, continuous, even while the tested board
// deep-sleeps (the separate-monitor topology). See LOG / the "sizing campaign" TODO.
//
// Reads BUS voltage (reg 0x02) + RAW SHUNT voltage (reg 0x01) directly -- calibration-
// independent. Current = shunt_mV / R_shunt; R_shunt is set once by calibration (force a
// known current through IN+/IN-, read shunt_mv, R = shunt_mv / I_mA).
//
// Flash (hardware-CDC so serial is stable like the PowerFeathers):
//   arduino-cli compile -u -p <port> \
//     --fqbn esp32:esp32:adafruit_metro_esp32s3:USBMode=hwcdc,CDCOnBoot=cdc firmware/ina_monitor
//
// Output (one line per present channel per sample):
//   ina t=<ms> ch=0x40 bus_v=<V> shunt_mv=<mV> ma=<mA>

#include <Wire.h>

static const uint8_t INA_ADDR[4] = {0x40, 0x41, 0x44, 0x45};

// SEN0291 shunt resistance (ohms). The DFRobot SEN0291 uses a 10 mOhm (0.01) alloy
// shunt -- NOT the INA219 reference 0.1. Confirmed by datasheet ("10 mOhm" + "1 mA
// resolution" = INA219's 10 uV LSB / 0.01) and a live cross-check vs PowerFeather
// fuel-gauge telemetry (2026-06-09): the old 0.1 under-reported current ~10x. Raw
// shunt_mv is still logged, so old data is recoverable by x10.
#ifndef INA_RSHUNT_OHMS
#define INA_RSHUNT_OHMS 0.01f
#endif
#ifndef INA_HZ
#define INA_HZ 10 // samples/s per channel
#endif
// INA219 config (reg 0x00): BRNG=32V, PG=/1 (+-40mV; @ 0.01ohm = +-4A range, 1mA/LSB),
// BADC+SADC = 128-sample averaging, continuous. Low-noise for steady-state mA-class work
// (~7 Hz fresh data). For fast transients (e.g. sleep-wake spikes) drop the averaging.
#ifndef INA_CONFIG
#define INA_CONFIG 0x27FF
#endif

#define REG_CONFIG 0x00
#define REG_SHUNT 0x01
#define REG_BUS 0x02

static bool present[4] = {false, false, false, false};

static bool readReg(uint8_t addr, uint8_t reg, uint16_t &val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false; // repeated start
  if (Wire.requestFrom((int)addr, 2) != 2) return false;
  val = ((uint16_t)Wire.read() << 8) | Wire.read();
  return true;
}

static bool writeReg(uint8_t addr, uint8_t reg, uint16_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val >> 8);
  Wire.write(val & 0xFF);
  return Wire.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();
  Wire.setClock(400000);
  Serial.println();
  Serial.println("=== ina_monitor: 4-ch INA219 reader ===");
  Serial.printf("R_shunt=%.4f ohm (VERIFY by calibration), rate=%d Hz, config=0x%04X (PG/1, 128-avg)\n",
                INA_RSHUNT_OHMS, INA_HZ, INA_CONFIG);
  int n = 0;
  for (int i = 0; i < 4; i++) {
    uint16_t v;
    present[i] = readReg(INA_ADDR[i], REG_CONFIG, v);
    if (present[i]) { writeReg(INA_ADDR[i], REG_CONFIG, INA_CONFIG); n++; }
    Serial.printf("  0x%02X %s\n", INA_ADDR[i], present[i] ? "present (cfg set)" : "MISSING");
  }
  Serial.printf("%d/4 meters present. Streaming 'ina' lines @ %d Hz.\n", n, INA_HZ);
}

void loop() {
  uint32_t t = millis();
  for (int i = 0; i < 4; i++) {
    if (!present[i]) continue;
    uint16_t rs, rb;
    if (!(readReg(INA_ADDR[i], REG_SHUNT, rs) && readReg(INA_ADDR[i], REG_BUS, rb))) {
      Serial.printf("ina t=%lu ch=0x%02X ERR\n", (unsigned long)t, INA_ADDR[i]);
      continue;
    }
    float shunt_mv = (int16_t)rs * 0.01f;  // 10 uV LSB -> mV
    float bus_v = (rb >> 3) * 0.004f;      // 4 mV LSB (bits 15:3)
    float ma = shunt_mv / INA_RSHUNT_OHMS; // mV / ohm = mA
    Serial.printf("ina t=%lu ch=0x%02X bus_v=%.3f shunt_mv=%.3f ma=%.2f\n",
                  (unsigned long)t, INA_ADDR[i], bus_v, shunt_mv, ma);
  }
  delay(1000 / INA_HZ);
}
