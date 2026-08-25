// Magic Wand NeoHex commissioning test.
//
// Hardware:
//   PowerFeather V2 A0 / GPIO10 -> 74AHCT125 -> M5Stack NeoHex data IN
//   740 WS2812-compatible RGB pixels (twenty NeoHex boards), GRB order, 800 kHz
//
// Normal behavior:
//   The data line starts LOW, then the preferred 0.4-second upward-ring pattern
//   starts automatically. Send 'o' to blank the strip and stop the pattern.

#include "esp32-hal-rmt.h"
#include <Adafruit_BMP5xx.h>
#include <Adafruit_MSA301.h>
#include <PowerFeather.h>
#include <Wire.h>
#include "../powerfeather_solar_guard.h"

using namespace PowerFeather;

#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 so the SDK targets the V2 charger/gauge."
#endif

namespace {

constexpr uint8_t kDataPin = 10;  // PowerFeather A0 / GPIO10
constexpr uint8_t kBoardCount = 20;
constexpr uint8_t kPixelsPerBoard = 37;
constexpr uint16_t kPixelCount = kBoardCount * kPixelsPerBoard;
constexpr uint8_t kDimLevel = 8;  // About 3% of an 8-bit channel
constexpr uint8_t kRainbowLevel = 6;  // About 2.4%, with constant low RGB sum
constexpr uint16_t kRainbowFrameMs = 80;
constexpr uint16_t kUpwardRingStepMs = 400;
constexpr size_t kBitsPerPixel = 24;
constexpr size_t kSymbolCount = kPixelCount * kBitsPerPixel;

// Gotion 33140 LiFePO4 cell. The V2 MAX17260 accepts capacities through
// 16,383 mAh. A 500 mA charge cap is deliberately gentle and USB-friendly.
constexpr uint16_t kBatteryCapacityMah = 15000;
constexpr float kChargeLimitMa = 500.0f;
constexpr float kSupplyMaintainV = 4.6f;
constexpr float kBatteryPresentMinV = 2.0f;

struct Pixel {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

Pixel pixels[kPixelCount]{};
rmt_data_t symbols[kSymbolCount]{};
bool rmtReady = false;

enum class TestMode : uint8_t {
  Idle,
  FullChain,
  BoardLedOne,
  RainbowSpin,
  UpwardRedRing,
};

TestMode testMode = TestMode::Idle;
uint32_t lastStepMs = 0;
uint32_t stepIntervalMs = 250;
uint16_t stepIndex = 0;

bool powerFeatherReady = false;
bool chargingEnabled = false;
uint32_t lastPowerServiceMs = 0;

Adafruit_MSA311 msa311;
Adafruit_BMP5xx bmp581;
bool msa311Present = false;
bool msa311ReadOk = false;
bool bmp581Present = false;
bool bmp581ReadOk = false;
float accelXG = NAN;
float accelYG = NAN;
float accelZG = NAN;
float gravityXG = NAN;
float gravityYG = NAN;
float gravityZG = NAN;
float movementG = 0.0f;
float pressureHpa = NAN;
float filteredPressureHpa = NAN;
float pressureZeroHpa = NAN;
float relativeAltitudeM = 0.0f;
uint32_t nextMsaReadMs = 0;
uint32_t nextBmpReadMs = 0;

void printPowerState() {
  if (!powerFeatherReady) {
    Serial.println("powerfeather=FAILED charger=UNCONFIGURED do_not_attach_cell_with_USB");
    return;
  }

  float batteryV = 0.0f;
  float batteryMa = 0.0f;
  float supplyV = 0.0f;
  float supplyMa = 0.0f;
  bool supplyGood = false;
  const Result bvResult = Board.getBatteryVoltage(batteryV);
  const Result biResult = Board.getBatteryCurrent(batteryMa);
  const Result svResult = Board.getSupplyVoltage(supplyV);
  const Result siResult = Board.getSupplyCurrent(supplyMa);
  const Result sgResult = Board.checkSupplyGood(supplyGood);

  Serial.printf("powerfeather=ready chemistry=LFP capacity=%u_mAh charge_limit=%.0f_mA "
                "charger=%s battery_v=%s%.3f battery_ma=%s%.1f "
                "supply_v=%s%.3f supply_ma=%s%.1f supply_good=%s\n",
                (unsigned)kBatteryCapacityMah,
                (double)kChargeLimitMa,
                chargingEnabled ? "enabled" : "disabled",
                bvResult == Result::Ok ? "" : "ERR/", (double)batteryV,
                biResult == Result::Ok ? "" : "ERR/", (double)batteryMa,
                svResult == Result::Ok ? "" : "ERR/", (double)supplyV,
                siResult == Result::Ok ? "" : "ERR/", (double)supplyMa,
                sgResult == Result::Ok ? (supplyGood ? "yes" : "no") : "ERR");
}
void initPowerFeather() {
  Result result = Result::Failure;
  for (int attempt = 0; attempt < 4 && result != Result::Ok; ++attempt) {
    result = Board.init(kBatteryCapacityMah, Mainboard::BatteryType::Generic_LFP);
    if (result != Result::Ok) {
      delay(250);
    }
  }

  if (result != Result::Ok) {
    Serial.printf("WARNING: PowerFeather init failed (%d); charger unconfigured. "
                  "Do not attach a cell while USB is connected.\n",
                  (int)result);
    return;
  }

  powerFeatherReady = true;
  const Result maintainResult = Board.setSupplyMaintainVoltage(kSupplyMaintainV);
  const Result currentResult = Board.setBatteryChargingMaxCurrent(kChargeLimitMa);

  // Board.init() starts with charging disabled. Only enable it when a cell was
  // already present at boot. This keeps a no-battery USB flash session stable.
  float batteryV = 0.0f;
  const Result batteryResult = Board.getBatteryVoltage(batteryV);
  chargingEnabled = batteryResult == Result::Ok && batteryV >= kBatteryPresentMinV;
  const Result chargeResult = Board.enableBatteryCharging(chargingEnabled);
  const bool guardOk = pfSolarGuardInit("magic_wand", kSupplyMaintainV,
                                        chargingEnabled);

  Serial.printf("PowerFeather init: LFP %u mAh, maintain=%d, current=%d, "
                "battery=%s%.3f V, charger=%s (%d), guard=%s\n",
                (unsigned)kBatteryCapacityMah,
                (int)maintainResult,
                (int)currentResult,
                batteryResult == Result::Ok ? "" : "ERR/", (double)batteryV,
                chargingEnabled ? "enabled" : "disabled",
                (int)chargeResult,
                guardOk ? "ok" : "ERR");
}

void servicePowerFeather() {
  if (!powerFeatherReady) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastPowerServiceMs < 2000) {
    return;
  }
  lastPowerServiceMs = now;

  float supplyV = 0.0f;
  float supplyMa = 0.0f;
  bool supplyGood = false;
  if (Board.getSupplyVoltage(supplyV) == Result::Ok &&
      Board.getSupplyCurrent(supplyMa) == Result::Ok &&
      Board.checkSupplyGood(supplyGood) == Result::Ok) {
    pfSolarGuardTick("magic_wand", supplyV, supplyMa, supplyGood,
                     kSupplyMaintainV, chargingEnabled);
  }
}

void capturePressureZero() {
  if (!bmp581ReadOk || !isfinite(filteredPressureHpa) ||
      filteredPressureHpa <= 0.0f) {
    Serial.println("BMP581 zero skipped: no valid pressure reading");
    return;
  }
  pressureZeroHpa = filteredPressureHpa;
  relativeAltitudeM = 0.0f;
  Serial.printf("BMP581 elevation zero: %.3f hPa\n", (double)pressureZeroHpa);
}

void initSensors() {
  if (!powerFeatherReady) {
    Serial.println("Sensors not probed: PowerFeather bus initialization failed");
    return;
  }

  // MSA311 and BMP581 share Wire1 with the charger and gauge. ADR 0028 makes
  // this 100 kHz ceiling mandatory; a faster clock can open the battery path.
  Board.enableVSQT(true);
  delay(150);
  Wire1.setClock(100000);

  msa311Present = msa311.begin(MSA311_I2CADDR_DEFAULT, &Wire1);
  if (msa311Present) {
    msa311.setRange(MSA301_RANGE_4_G);
    msa311.setDataRate(MSA301_DATARATE_125_HZ);
    msa311.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
    msa311.setPowerMode(MSA301_NORMALMODE);
  }

  Wire1.setClock(100000);
  bmp581Present = bmp581.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire1);
  if (bmp581Present) {
    bmp581.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
    bmp581.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
    bmp581.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    bmp581.setOutputDataRate(BMP5XX_ODR_10_HZ);
    bmp581.setPowerMode(BMP5XX_POWERMODE_NORMAL);
    bmp581.enablePressure(true);
  }
  Wire1.setClock(100000);

  Serial.printf("Sensors on Wire1 GPIO47/48 @100kHz: MSA311(0x62)=%s "
                "BMP581(0x47)=%s\n",
                msa311Present ? "FOUND" : "NOT FOUND",
                bmp581Present ? "FOUND" : "NOT FOUND");
}

void serviceSensors() {
  const uint32_t now = millis();

  if (msa311Present && now >= nextMsaReadMs) {
    nextMsaReadMs = now + 40;
    Wire1.setClock(100000);
    msa311.read();
    accelXG = msa311.x_g;
    accelYG = msa311.y_g;
    accelZG = msa311.z_g;
    const float magnitude = sqrtf(accelXG * accelXG + accelYG * accelYG +
                                  accelZG * accelZG);
    msa311ReadOk = isfinite(magnitude) && magnitude > 0.05f;
    if (msa311ReadOk) {
      if (!isfinite(gravityXG)) {
        gravityXG = accelXG;
        gravityYG = accelYG;
        gravityZG = accelZG;
      } else {
        constexpr float kGravityAlpha = 0.08f;
        gravityXG += kGravityAlpha * (accelXG - gravityXG);
        gravityYG += kGravityAlpha * (accelYG - gravityYG);
        gravityZG += kGravityAlpha * (accelZG - gravityZG);
      }
      const float dx = accelXG - gravityXG;
      const float dy = accelYG - gravityYG;
      const float dz = accelZG - gravityZG;
      const float instantMovement = sqrtf(dx * dx + dy * dy + dz * dz);
      const float alpha = instantMovement > movementG ? 0.45f : 0.08f;
      movementG += alpha * (instantMovement - movementG);
    }
  }

  if (bmp581Present && now >= nextBmpReadMs) {
    nextBmpReadMs = now + 200;
    Wire1.setClock(100000);
    bmp581ReadOk = bmp581.performReading();
    if (bmp581ReadOk) {
      pressureHpa = bmp581.pressure;
      if (!isfinite(filteredPressureHpa)) {
        filteredPressureHpa = pressureHpa;
      } else {
        filteredPressureHpa += 0.25f * (pressureHpa - filteredPressureHpa);
      }
      if (!isfinite(pressureZeroHpa)) {
        capturePressureZero();
      }
      if (isfinite(pressureZeroHpa) && pressureZeroHpa > 0.0f) {
        const float ratio = filteredPressureHpa / pressureZeroHpa;
        relativeAltitudeM =
            44330.0f * (1.0f - powf(ratio, 0.19029495f));
      }
    }
    Wire1.setClock(100000);
  }
}

void printSensorState() {
  Serial.printf("sensors msa311=%s read=%s xyz_g=[%.3f %.3f %.3f] movement_g=%.3f "
                "bmp581=%s read=%s pressure_hpa=%.3f relative_altitude_m=%.2f\n",
                msa311Present ? "found" : "missing",
                msa311ReadOk ? "ok" : "no",
                (double)accelXG, (double)accelYG, (double)accelZG,
                (double)movementG,
                bmp581Present ? "found" : "missing",
                bmp581ReadOk ? "ok" : "no",
                (double)filteredPressureHpa, (double)relativeAltitudeM);
}

void clearPixels() {
  for (Pixel &pixel : pixels) {
    pixel = {0, 0, 0};
  }
}

void encodeByte(uint8_t value, size_t &symbolIndex) {
  for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
    rmt_data_t &symbol = symbols[symbolIndex++];
    symbol.level0 = 1;
    symbol.level1 = 0;
    if ((value & mask) != 0) {
      symbol.duration0 = 8;  // WS2812 logical 1: 0.8 us HIGH
      symbol.duration1 = 4;  //                         0.4 us LOW
    } else {
      symbol.duration0 = 4;  // WS2812 logical 0: 0.4 us HIGH
      symbol.duration1 = 8;  //                         0.8 us LOW
    }
  }
}

bool showPixels() {
  if (!rmtReady) {
    return false;
  }

  size_t symbolIndex = 0;
  for (const Pixel &pixel : pixels) {
    // M5Stack NeoHex uses WS2812 GRB byte order.
    encodeByte(pixel.green, symbolIndex);
    encodeByte(pixel.red, symbolIndex);
    encodeByte(pixel.blue, symbolIndex);
  }

  const bool ok = rmtWrite(kDataPin, symbols, kSymbolCount, RMT_WAIT_FOR_EVER);
  delayMicroseconds(80);  // WS2812 reset/latch interval
  return ok;
}

void blankPixels() {
  clearPixels();
  showPixels();
}

void fillDim(uint8_t red, uint8_t green, uint8_t blue) {
  for (Pixel &pixel : pixels) {
    pixel = {red, green, blue};
  }
  showPixels();
}

void renderFullChainStep() {
  // Begin with three dim full-board color checks, then chase one dim white
  // pixel through every physical LED.  The deliberately small channel values
  // keep the test well below a full-white load.
  if (stepIndex == 0) {
    fillDim(kDimLevel, 0, 0);
  } else if (stepIndex == 1) {
    fillDim(0, kDimLevel, 0);
  } else if (stepIndex == 2) {
    fillDim(0, 0, kDimLevel);
  } else {
    clearPixels();
    pixels[stepIndex - 3] = {kDimLevel, kDimLevel, kDimLevel};
    showPixels();
  }

  ++stepIndex;
  if (stepIndex >= kPixelCount + 3) {
    stepIndex = 0;
  }
  stepIntervalMs = 250;
}

void renderBoardLedOneStep() {
  clearPixels();

  if (stepIndex < kBoardCount) {
    // LED 1 is the first pixel received by each 37-pixel NeoHex.
    const uint16_t pixelIndex = stepIndex * kPixelsPerBoard;
    pixels[pixelIndex] = {kDimLevel, kDimLevel, kDimLevel};
    showPixels();
    Serial.printf("LED1: Hex %u, global pixel %u\n",
                  (unsigned)(stepIndex + 1), (unsigned)(pixelIndex + 1));
    stepIntervalMs = 750;
  } else if (stepIndex == kBoardCount) {
    // End marker: illuminate LED 1 on all twenty boards together in green.
    for (uint8_t board = 0; board < kBoardCount; ++board) {
      pixels[board * kPixelsPerBoard] = {0, kDimLevel, 0};
    }
    showPixels();
    Serial.println("LED1: all 20 Hex boards together");
    stepIntervalMs = 2000;
  } else {
    showPixels();
    Serial.println("LED1: cycle blank");
    stepIntervalMs = 1000;
  }

  ++stepIndex;
  if (stepIndex >= kBoardCount + 2) {
    stepIndex = 0;
  }
}

Pixel rainbowColor(uint8_t hue) {
  // Three linear 85-step segments. The RGB channel sum stays at
  // kRainbowLevel, limiting power while keeping every pixel illuminated.
  if (hue < 85) {
    const uint8_t rise = static_cast<uint8_t>(
        (static_cast<uint16_t>(hue) * kRainbowLevel) / 85);
    return {static_cast<uint8_t>(kRainbowLevel - rise), rise, 0};
  }
  if (hue < 170) {
    const uint8_t position = static_cast<uint8_t>(hue - 85);
    const uint8_t rise = static_cast<uint8_t>(
        (static_cast<uint16_t>(position) * kRainbowLevel) / 85);
    return {0, static_cast<uint8_t>(kRainbowLevel - rise), rise};
  }

  const uint8_t position = static_cast<uint8_t>(hue - 170);
  const uint8_t rise = static_cast<uint8_t>(
      (static_cast<uint16_t>(position) * kRainbowLevel) / 85);
  return {rise, 0, static_cast<uint8_t>(kRainbowLevel - rise)};
}

void renderRainbowSpinFrame() {
  // The user-numbered geometry is four rings of five Hex boards:
  // 1-5, 6-10, 11-15, and 16-20. The five slot colors rotate around
  // each ring. A small per-ring phase makes the bands form a gentle spiral.
  const uint8_t animationPhase = static_cast<uint8_t>(stepIndex * 2);
  for (uint8_t board = 0; board < kBoardCount; ++board) {
    const uint8_t ring = board / 5;
    const uint8_t slot = board % 5;
    const uint8_t hue = static_cast<uint8_t>(
        animationPhase + static_cast<uint8_t>(slot * 51) +
        static_cast<uint8_t>(ring * 9));
    const Pixel color = rainbowColor(hue);
    const uint16_t firstPixel = board * kPixelsPerBoard;
    for (uint8_t pixel = 0; pixel < kPixelsPerBoard; ++pixel) {
      pixels[firstPixel + pixel] = color;
    }
  }
  showPixels();
  ++stepIndex;
  stepIntervalMs = kRainbowFrameMs;
}

void renderUpwardRedRingStep() {
  // Each geometric row contains five boards. Red is the moving ring; the
  // row-specific low-light colors make the direction of travel obvious.
  constexpr Pixel kRed = {6, 0, 0};
  constexpr Pixel kRowColors[4] = {
      {4, 2, 0},  // Row 1 background: orange
      {3, 3, 0},  // Row 2 background: yellow
      {0, 6, 0},  // Row 3 background: green
      {0, 0, 6},  // Row 4 background: blue
  };

  const uint8_t activeRow = static_cast<uint8_t>(stepIndex % 4);
  for (uint8_t board = 0; board < kBoardCount; ++board) {
    const uint8_t row = board / 5;
    const Pixel color = row == activeRow ? kRed : kRowColors[row];
    const uint16_t firstPixel = board * kPixelsPerBoard;
    for (uint8_t pixel = 0; pixel < kPixelsPerBoard; ++pixel) {
      pixels[firstPixel + pixel] = color;
    }
  }
  showPixels();
  ++stepIndex;
  stepIntervalMs = kUpwardRingStepMs;
}

const char *testModeName() {
  switch (testMode) {
    case TestMode::FullChain:
      return "full-chain";
    case TestMode::BoardLedOne:
      return "board-led1";
    case TestMode::RainbowSpin:
      return "rainbow-spin";
    case TestMode::UpwardRedRing:
      return "upward-red-ring";
    default:
      return "idle";
  }
}

void printHelp() {
  Serial.println("Magic Wand 740-pixel / 20-NeoHex test ready");
  Serial.println("u = move a red ring upward through the four colored rows");
  Serial.println("r = low-brightness all-pixel rainbow globe spin");
  Serial.println("n = light LED 1 on Hex 1 through Hex 20 in order");
  Serial.println("g = start dim RGB + 740-pixel full-chain chase");
  Serial.println("o = all pixels off and stop");
  Serial.println("h = stop and hold GPIO10 HIGH for meter tracing");
  Serial.println("l = stop and hold GPIO10 LOW for meter tracing");
  Serial.println("s = print LED and PowerFeather state");
  Serial.println("z = re-zero BMP581 relative elevation at the current height");
}

}  // namespace

void setup() {
  pinMode(kDataPin, OUTPUT);
  digitalWrite(kDataPin, LOW);

  Serial.begin(115200);
  delay(250);
  initPowerFeather();
  initSensors();
  clearPixels();
  // Reserve all four ESP32-S3 TX memory blocks. A single block worked for one
  // NeoHex but under-ran while streaming the longer two-board frame.
  rmtReady = rmtInit(kDataPin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_4, 10000000);
  rmtSetEOT(kDataPin, LOW);
  // Do not call show() here: external NeoHex power may still be off. loop()
  // continuously sends the default pattern, so the LEDs synchronize as soon
  // as the Pololu 5 V rail is switched on.

  testMode = TestMode::UpwardRedRing;
  stepIndex = 0;
  lastStepMs = 0;
  stepIntervalMs = kUpwardRingStepMs;

  delay(500);
  printHelp();
  Serial.printf("RMT=%s\n", rmtReady ? "ready" : "FAILED");
  Serial.println("AUTO-STARTED: upward red ring through rows 1-4");
}

void loop() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == 'g' || command == 'G') {
      testMode = TestMode::FullChain;
      stepIndex = 0;
      lastStepMs = 0;
      stepIntervalMs = 250;
      Serial.println("STARTED: dim 740-pixel full-chain test");
    } else if (command == 'n' || command == 'N') {
      testMode = TestMode::BoardLedOne;
      stepIndex = 0;
      lastStepMs = 0;
      stepIntervalMs = 750;
      Serial.println("STARTED: Hex 1-20 LED-1 orientation test");
    } else if (command == 'r' || command == 'R') {
      testMode = TestMode::RainbowSpin;
      stepIndex = 0;
      lastStepMs = 0;
      stepIntervalMs = kRainbowFrameMs;
      Serial.println("STARTED: low-brightness 20-Hex rainbow globe spin");
    } else if (command == 'u' || command == 'U') {
      testMode = TestMode::UpwardRedRing;
      stepIndex = 0;
      lastStepMs = 0;
      stepIntervalMs = kUpwardRingStepMs;
      Serial.println("STARTED: upward red ring through rows 1-4");
    } else if (command == 'o' || command == 'O') {
      testMode = TestMode::Idle;
      blankPixels();
      Serial.println("STOPPED: pixels blanked");
    } else if (command == 'h' || command == 'H') {
      testMode = TestMode::Idle;
      pinMode(kDataPin, OUTPUT);
      digitalWrite(kDataPin, HIGH);
      Serial.println("TRACE: GPIO10 held HIGH");
    } else if (command == 'l' || command == 'L') {
      testMode = TestMode::Idle;
      pinMode(kDataPin, OUTPUT);
      digitalWrite(kDataPin, LOW);
      Serial.println("TRACE: GPIO10 held LOW");
    } else if (command == 's' || command == 'S') {
      Serial.printf("state=%s step=%u pixels=%u boards=%u pin=%u dim=%u rmt=%s\n",
                    testModeName(),
                    stepIndex,
                    kPixelCount,
                    kBoardCount,
                    kDataPin,
                    kDimLevel,
                    rmtReady ? "ready" : "FAILED");
      printPowerState();
      printSensorState();
    } else if (command == 'z' || command == 'Z') {
      capturePressureZero();
    }
  }

  servicePowerFeather();
  serviceSensors();

  if (testMode == TestMode::Idle) {
    return;
  }

  const uint32_t now = millis();
  if (lastStepMs == 0 || now - lastStepMs >= stepIntervalMs) {
    lastStepMs = now;
    if (testMode == TestMode::BoardLedOne) {
      renderBoardLedOneStep();
    } else if (testMode == TestMode::RainbowSpin) {
      renderRainbowSpinFrame();
    } else if (testMode == TestMode::UpwardRedRing) {
      renderUpwardRedRingStep();
    } else {
      renderFullChainStep();
    }
  }
}
