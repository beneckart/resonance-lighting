// ina_monitor -- standalone 4-channel power monitor for the Resonance bench. Reads up to
// 4x INA219 (DFRobot SEN0291) on the default I2C / STEMMA-QT bus and streams V/I per
// channel so the host logs a board-under-test's current GROUND-TRUTH -- gauge-independent,
// continuous, even while the tested board deep-sleeps (the separate-monitor topology).
// See LOG / the "sizing campaign" TODO.
//
// Targets (sketch is portable; Wire.begin() default pins hit the QT port on both):
//   - Adafruit Metro ESP32-S3 (SDA=47/SCL=48) -- original monitor, now on clacker duty.
//   - Adafruit KB2040 / RP2040 (SDA=GPIO12/SCL=GPIO13, arduino-pico core) -- since
//     2026-07-02 for the TPS63802 4.2 V boost bench.
//
// Reads BUS voltage (reg 0x02) + RAW SHUNT voltage (reg 0x01) directly -- calibration-
// independent. Current = shunt_mV / R_shunt; R_shunt is set once by calibration (force a
// known current through IN+/IN-, read shunt_mv, R = shunt_mv / I_mA).
//
// Flash, Metro (hardware-CDC so serial is stable like the PowerFeathers):
//   arduino-cli compile -u -p <port> \
//     --fqbn esp32:esp32:adafruit_metro_esp32s3:USBMode=hwcdc,CDCOnBoot=cdc firmware/ina_monitor
// Flash, KB2040 (UF2 drop -- hold BOOTSEL, tap RESET, release BOOTSEL -> RPI-RP2 drive):
//   arduino-cli compile --fqbn rp2040:rp2040:adafruit_kb2040 \
//     --build-path /tmp/ina_monitor_kb2040_build firmware/ina_monitor
//   cp /tmp/ina_monitor_kb2040_build/ina_monitor.ino.uf2 /media/$USER/RPI-RP2/ && sync
//
// Optional lux sensor on the same QT chain (photopic ground truth for the 4.2 V boost
// bench -- lumens-weighted, unlike the PAR meter's flat quantum response): TSL2591
// (0x29) and/or VEML7700 (0x10) are auto-detected at boot, on 'r', and by a 5 s
// background re-probe, so hot-plugging one mid-session just works. Fixed low-gain /
// 100 ms config for LED-bench light levels; sat=1 flags saturation -> move the sensor
// back rather than changing gain, so A/B ratios stay comparable.
//
// Output (one line per present channel per sample):
//   ina t=<ms> ch=0x40 bus_v=<V> shunt_mv=<mV> ma=<mA>
//   lux t=<ms> sensor=veml7700 raw=<u16> lux=<lx> sat=<0|1>
//   lux t=<ms> sensor=tsl2591 ch0=<u16> ch1=<u16> lux=<lx> sat=<0|1>
//
// Serial commands: 's' = full I2C bus scan (0x08-0x77) -- e.g. plug a known-good
// STEMMA-QT device into the QT port to verify the port reaches the same bus as the
// SDA/SCL headers (it does by schematic: QT = GPIO47/48); 'r' = re-probe INA channels
// and lux sensors.

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

// Lux sensors. VEML7700: 16-bit little-endian registers; gain 1/8 + IT 100 ms ->
// 0.4608 lux/count, ~30 klx full scale (linear-enough regime; apply the datasheet
// high-lux polynomial host-side if absolute lux above ~1 klx matters -- A/B ratios
// don't need it). TSL2591: command bit 0xA0 | reg; gain 1x + ATIME 100 ms; lux from
// the Adafruit cpl formula on CH0 (full) / CH1 (IR).
#define VEML_ADDR 0x10
#define VEML_REG_CONF 0x00
#define VEML_REG_ALS 0x04
#define VEML_CONF 0x1000 // gain 1/8, IT 100 ms, no persistence/interrupt, powered on
#define VEML_LUX_PER_CT 0.4608f
#define TSL_ADDR 0x29
#define TSL_CMD 0xA0
#define TSL_REG_ENABLE 0x00
#define TSL_REG_CONTROL 0x01
#define TSL_REG_ID 0x12
#define TSL_REG_C0L 0x14
#define TSL_REG_C1L 0x16
#define TSL_ENABLE_ON 0x03  // PON | AEN
#define TSL_CTRL_1X_100MS 0x00
#define TSL_CPL (100.0f * 1.0f / 408.0f) // atime_ms * gain / 408

static bool present[4] = {false, false, false, false};
static bool vemlPresent = false, tslPresent = false;
static uint32_t lastLuxProbe = 0;

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

// VEML7700 registers are little-endian (INA219's are big-endian, hence the pair).
static bool readReg16LE(uint8_t addr, uint8_t reg, uint16_t &val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 2) != 2) return false;
  uint16_t lo = Wire.read(), hi = Wire.read();
  val = lo | (hi << 8);
  return true;
}

static bool writeReg16LE(uint8_t addr, uint8_t reg, uint16_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val & 0xFF);
  Wire.write(val >> 8);
  return Wire.endTransmission() == 0;
}

static bool tslWrite8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TSL_ADDR);
  Wire.write(TSL_CMD | reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool tslRead8(uint8_t reg, uint8_t &val) {
  Wire.beginTransmission(TSL_ADDR);
  Wire.write(TSL_CMD | reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)TSL_ADDR, 1) != 1) return false;
  val = Wire.read();
  return true;
}

static bool tslRead16(uint8_t reg, uint16_t &val) {
  Wire.beginTransmission(TSL_ADDR);
  Wire.write(TSL_CMD | reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)TSL_ADDR, 2) != 2) return false;
  uint16_t lo = Wire.read(), hi = Wire.read();
  val = lo | (hi << 8);
  return true;
}

static void probeLux(bool verbose) {
  bool was;
  was = vemlPresent;
  vemlPresent = writeReg16LE(VEML_ADDR, VEML_REG_CONF, VEML_CONF);
  if (verbose || vemlPresent != was)
    Serial.printf("  0x%02X VEML7700 %s\n", VEML_ADDR,
                  vemlPresent ? "present (gain 1/8, IT 100 ms)" : "MISSING");
  was = tslPresent;
  uint8_t id = 0;
  tslPresent = tslRead8(TSL_REG_ID, id) && id == 0x50 &&
               tslWrite8(TSL_REG_ENABLE, TSL_ENABLE_ON) &&
               tslWrite8(TSL_REG_CONTROL, TSL_CTRL_1X_100MS);
  if (verbose || tslPresent != was)
    Serial.printf("  0x%02X TSL2591 %s\n", TSL_ADDR,
                  tslPresent ? "present (gain 1x, 100 ms)" : "MISSING");
}

static void emitLux(uint32_t t) {
  if (vemlPresent) {
    uint16_t raw;
    if (readReg16LE(VEML_ADDR, VEML_REG_ALS, raw)) {
      Serial.printf("lux t=%lu sensor=veml7700 raw=%u lux=%.1f sat=%d\n",
                    (unsigned long)t, raw, raw * VEML_LUX_PER_CT, raw >= 65000 ? 1 : 0);
    } else {
      vemlPresent = false;
      Serial.printf("lux t=%lu sensor=veml7700 ERR (unplugged?)\n", (unsigned long)t);
    }
  }
  if (tslPresent) {
    uint16_t c0, c1;
    if (tslRead16(TSL_REG_C0L, c0) && tslRead16(TSL_REG_C1L, c1)) {
      // Adafruit formula; guard div-by-zero. 100 ms ATIME saturates CH0 at 36863.
      float lux = 0.0f;
      if (c0 > 0 && c0 > c1)
        lux = ((float)c0 - c1) * (1.0f - (float)c1 / c0) / TSL_CPL;
      Serial.printf("lux t=%lu sensor=tsl2591 ch0=%u ch1=%u lux=%.1f sat=%d\n",
                    (unsigned long)t, c0, c1, lux, c0 >= 36863 ? 1 : 0);
    } else {
      tslPresent = false;
      Serial.printf("lux t=%lu sensor=tsl2591 ERR (unplugged?)\n", (unsigned long)t);
    }
  }
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
  probeInas();
  probeLux(true);
  Serial.printf("Streaming 'ina'/'lux' lines @ %d Hz. Commands: 's' i2c scan, 'r' re-probe.\n",
                INA_HZ);
}

static void scanBus() {
  Serial.println("i2c scan 0x08-0x77:");
  int n = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      const char *known = (a == 0x40 || a == 0x41 || a == 0x44 || a == 0x45) ? " (INA219)"
                          : (a == 0x29) ? " (TSL2591?)" : (a == 0x30) ? " (IS31?)"
                          : (a == 0x36) ? " (Metro onboard MAX17048)"
                          : (a == 0x49 || a == 0x60) ? " (seesaw?)" : "";
      Serial.printf("  found 0x%02X%s\n", a, known);
      n++;
    }
  }
  Serial.printf("scan done, %d device(s).\n", n);
}

static void probeInas() {
  int n = 0;
  for (int i = 0; i < 4; i++) {
    uint16_t v;
    present[i] = readReg(INA_ADDR[i], REG_CONFIG, v);
    if (present[i]) { writeReg(INA_ADDR[i], REG_CONFIG, INA_CONFIG); n++; }
    Serial.printf("  0x%02X %s\n", INA_ADDR[i], present[i] ? "present (cfg set)" : "MISSING");
  }
  Serial.printf("%d/4 meters present.\n", n);
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's') scanBus();
    else if (c == 'r') { probeInas(); probeLux(true); }
  }
  uint32_t t = millis();
  // Hot-plug: quietly re-probe missing lux sensors every 5 s (prints only on change).
  if ((!vemlPresent || !tslPresent) && t - lastLuxProbe >= 5000) {
    lastLuxProbe = t;
    probeLux(false);
  }
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
  emitLux(t);
  delay(1000 / INA_HZ);
}
