// Resonance Magic Wand Conductor: deliberate lift gesture -> bounded fleet wave.
//
// This is a dedicated image for Steve's F40344 PowerFeather and 20-board
// NeoHex wand. It shares the Atom Conductor's session/graph policy and the
// fleet's one packet contract, UTC selector, and BRC solar schedule.

#include <Arduino.h>
#include <Adafruit_BMP5xx.h>
#include <Adafruit_MSA301.h>
#include <PowerFeather.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <math.h>
#include <stddef.h>

#include "esp32-hal-rmt.h"
#include "../powerfeather_solar_guard.h"
#include "../fixture/src/core/filters.h"
#include "../fixture/src/core/fixture_context.h"
#include "../fixture/src/core/hex_geometry.h"
#include "../fixture/src/core/packet.h"
#include "../fixture/src/core/show_schedule.h"
#include "../fixture/src/core/time_consensus.h"
#include "../atom_conductor/src/core/action_gate.h"
#include "../atom_conductor/src/core/atom_session.h"
#include "../atom_conductor/src/core/fleet_graph.h"
#include "src/core/wand_gesture.h"
#include "src/core/wand_visual.h"

// Arduino copies sources under src/ to an isolated build tree. Include shared
// implementations at sketch level so this target uses exact canonical code
// rather than maintaining a second packet/session/schedule implementation.
#include "../fixture/src/core/filters.cpp"
#include "../fixture/src/core/hex_geometry.cpp"
#include "../fixture/src/core/show_schedule.cpp"
#include "../fixture/src/core/time_consensus.cpp"
#include "../atom_conductor/src/core/action_gate.cpp"
#include "../atom_conductor/src/core/atom_session.cpp"
#include "../atom_conductor/src/core/fleet_graph.cpp"

using namespace PowerFeather;

#define MAGIC_WAND_CONDUCTOR_VERSION "wand-cond-260903.1"

#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif
#ifndef RES_WAND_PULSE_MS
#define RES_WAND_PULSE_MS 40
#endif
#ifndef RES_WAND_TREE_COLOR_VALUE
#define RES_WAND_TREE_COLOR_VALUE 64
#endif
#ifndef RES_WAND_TREE_RGB_VALUE
#define RES_WAND_TREE_RGB_VALUE 21
#endif

#if RES_WAND_PULSE_MS < 5 || RES_WAND_PULSE_MS > 300
#error "RES_WAND_PULSE_MS must be 5..300"
#endif
#if RES_WAND_TREE_COLOR_VALUE < 1 || RES_WAND_TREE_COLOR_VALUE > 255
#error "RES_WAND_TREE_COLOR_VALUE must be 1..255"
#endif
#if RES_WAND_TREE_RGB_VALUE < 1 || RES_WAND_TREE_RGB_VALUE > 85
#error "RES_WAND_TREE_RGB_VALUE must be 1..85"
#endif

static constexpr uint8_t EXPECTED_MAC[6] =
    {0x68, 0xEE, 0x8F, 0xF4, 0x03, 0x44};
static constexpr uint8_t BROADCAST_MAC[6] =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uint8_t ALL_TARGETS[3] = {0, 0, 0};

static constexpr uint8_t DATA_PIN = 10;
static constexpr uint8_t STATUS_LED_PIN = 46;
static constexpr uint8_t BOARD_COUNT = 20;
static constexpr uint8_t PIXELS_PER_BOARD = 37;
static constexpr uint16_t PIXEL_COUNT = BOARD_COUNT * PIXELS_PER_BOARD;
static constexpr size_t BITS_PER_PIXEL = 24;
static constexpr size_t SYMBOL_COUNT = PIXEL_COUNT * BITS_PER_PIXEL;

static constexpr uint16_t BATTERY_CAPACITY_MAH = 15000;
static constexpr float CHARGE_LIMIT_MA = 500.0f;
static constexpr float SUPPLY_MAINTAIN_V = 4.6f;
static constexpr float BATTERY_PRESENT_MIN_V = 2.0f;
static constexpr float GAUGE_CURRENT_DIVISOR = 1.08f;

static constexpr uint32_t TARGET_FRESH_MS = 12000;
static constexpr uint16_t LOCATE_DURATION_S = 120;
static constexpr uint8_t LOCATE_PERIOD_DS = 200;
static constexpr uint16_t WAVE_LAYER_GAP_MS = 140;
static constexpr uint8_t COMMAND_SEND_COUNT = 6;
static constexpr uint16_t COMMAND_SEND_GAP_MS = 8;
static constexpr uint8_t FRAME_SEND_COUNT = 3;
static constexpr uint16_t FRAME_SEND_GAP_MS = 12;
static constexpr uint8_t RELIABLE_SEND_COUNT = 4;
static constexpr uint16_t RELIABLE_SEND_GAP_MS = 5;
static constexpr uint32_t HEARTBEAT_MS = 5000;
static constexpr size_t RX_QUEUE_DEPTH = 32;
static constexpr size_t RX_DATA_MAX = 250;

struct WandPixel {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct RxFrame {
  uint16_t len;
  int8_t rssi;
  uint32_t receivedMs;
  uint8_t data[RX_DATA_MAX];
};

static WandPixel pixels[PIXEL_COUNT] = {};
static rmt_data_t symbols[SYMBOL_COUNT] = {};
static bool rmtReady = false;
static uint32_t framesShown = 0;
static uint8_t idleRow = 0;
static uint32_t nextDisplayMs = 0;
static uint32_t displayOverrideUntilMs = 0;
static WandRgb displayOverrideColor = {0, 0, 0};
static bool displayOverrideAll = false;

static bool identityOk = false;
static bool powerFeatherReady = false;
static bool chargingEnabled = false;
static uint32_t nextPowerMs = 0;
static float batteryV = NAN;
static float batteryMa = NAN;
static float supplyV = NAN;
static float supplyMa = NAN;
static bool supplyGood = false;

static Adafruit_MSA311 msa311;
static Adafruit_BMP5xx bmp581;
static bool msaPresent = false;
static bool bmpPresent = false;
static uint32_t nextMsaMs = 0;
static uint32_t nextBmpMs = 0;
static uint32_t lastMsaOkMs = 0;
static uint32_t lastBmpOkMs = 0;
static float filteredPressureHpa = NAN;
static AccelFilter accelFilter;
static WandGesture gesture;
static WandGestureState lastLoggedGesture = WAND_GESTURE_SEEK_STEADY;

static AtomSession session;
static AtomFleet fleet;
static AtomWaveTarget waveTargets[ATOM_FLEET_MAX_PEERS];
static TimeConsensus timeConsensus;
static WandColorMode nightColor = WAND_COLOR_RED;

static QueueHandle_t rxQueue = nullptr;
static uint8_t myId[3] = {};
static uint32_t txSeq = 0;
static bool radioReady = false;
static uint32_t nextHeartbeatMs = 0;
static volatile uint32_t sendOk = 0;
static volatile uint32_t sendFail = 0;
static volatile uint32_t rxDropped = 0;

static void clearPixels() {
  for (WandPixel &pixel : pixels) pixel = {0, 0, 0};
}

static void encodeByte(uint8_t value, size_t &symbolIndex) {
  for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
    rmt_data_t &symbol = symbols[symbolIndex++];
    symbol.level0 = 1;
    symbol.level1 = 0;
    if (value & mask) {
      symbol.duration0 = 8;
      symbol.duration1 = 4;
    } else {
      symbol.duration0 = 4;
      symbol.duration1 = 8;
    }
  }
}

static bool showPixels() {
  if (!rmtReady) return false;
  size_t symbolIndex = 0;
  for (const WandPixel &pixel : pixels) {
    // M5Stack NeoHex uses WS2812 GRB byte order.
    encodeByte(pixel.green, symbolIndex);
    encodeByte(pixel.red, symbolIndex);
    encodeByte(pixel.blue, symbolIndex);
  }
  bool ok = rmtWrite(DATA_PIN, symbols, SYMBOL_COUNT, RMT_WAIT_FOR_EVER);
  delayMicroseconds(80);
  if (ok) ++framesShown;
  return ok;
}

static void fillPixels(WandRgb color) {
  for (WandPixel &pixel : pixels)
    pixel = {color.red, color.green, color.blue};
}

static void renderIdle() {
  constexpr WandPixel rowColors[4] = {
      {4, 2, 0}, {3, 3, 0}, {0, 6, 0}, {0, 0, 6}};
  constexpr WandPixel red = {6, 0, 0};
  for (uint8_t board = 0; board < BOARD_COUNT; ++board) {
    WandPixel color = board / 5 == idleRow ? red : rowColors[board / 5];
    uint16_t first = board * PIXELS_PER_BOARD;
    for (uint8_t p = 0; p < PIXELS_PER_BOARD; ++p)
      pixels[first + p] = color;
  }
  idleRow = (idleRow + 1) % 4;
  showPixels();
}

static void renderGather(uint32_t nowMs) {
  const HexGeometry &geo = hexGeometry();
  uint32_t elapsedMs = nowMs - session.startedMs;
  uint8_t loaded = wandGatherLoadedPixels(elapsedMs, ATOM_GATHER_MS);
  clearPixels();
  for (uint8_t board = 0; board < BOARD_COUNT; ++board) {
    uint16_t first = board * PIXELS_PER_BOARD;
    for (uint8_t pos = 0; pos < loaded; ++pos) {
      uint8_t local = geo.spiralOrder[pos];
      pixels[first + local] = {0, 1, 3};
    }

    // A bright head circles the frontier ring while the dim center-out spiral
    // records six-minute progress. Every physical Hex therefore visibly loads.
    uint8_t frontier = loaded < PIXELS_PER_BOARD ? loaded : PIXELS_PER_BOARD - 1;
    uint8_t ring = geo.ringOf[geo.spiralOrder[frontier]];
    uint8_t ringSize = geo.ringSize[ring];
    uint8_t head = (uint8_t)((nowMs / 120 + board) % ringSize);
    uint8_t local = geo.ringMembers[ring][head];
    pixels[first + local] = {0, 6, 8};
  }
  showPixels();
}

static void renderChimeReady() {
  const HexGeometry &geo = hexGeometry();
  clearPixels();
  for (uint8_t board = 0; board < BOARD_COUNT; ++board) {
    uint16_t first = board * PIXELS_PER_BOARD;
    for (uint8_t p = 0; p < PIXELS_PER_BOARD; ++p)
      if (geo.ringOf[p] <= 1) pixels[first + p] = {4, 2, 0};
  }
  showPixels();
}

static void renderUnknown() {
  const HexGeometry &geo = hexGeometry();
  clearPixels();
  for (uint8_t board = 0; board < BOARD_COUNT; ++board)
    pixels[board * PIXELS_PER_BOARD + geo.spiralOrder[0]] = {4, 0, 4};
  showPixels();
}

static void renderGestureCue(WandGestureState state) {
  const HexGeometry &geo = hexGeometry();
  clearPixels();
  uint8_t rings = state == WAND_GESTURE_ARMED
                      ? 0
                      : (state == WAND_GESTURE_LIFTING ? 1 : 2);
  for (uint8_t board = 0; board < BOARD_COUNT; ++board) {
    uint16_t first = board * PIXELS_PER_BOARD;
    for (uint8_t p = 0; p < PIXELS_PER_BOARD; ++p)
      if (geo.ringOf[p] <= rings) pixels[first + p] = {0, 4, 6};
  }
  showPixels();
}

static void setDisplayOverride(WandRgb color, uint32_t durationMs,
                               bool allPixels) {
  displayOverrideColor = color;
  displayOverrideUntilMs = millis() + durationMs;
  displayOverrideAll = allPixels;
  nextDisplayMs = 0;
}

static void renderOverride() {
  if (displayOverrideAll) {
    fillPixels(displayOverrideColor);
  } else {
    const HexGeometry &geo = hexGeometry();
    clearPixels();
    for (uint8_t board = 0; board < BOARD_COUNT; ++board)
      for (uint8_t p = 0; p < PIXELS_PER_BOARD; ++p)
        if (geo.ringOf[p] <= 1)
          pixels[board * PIXELS_PER_BOARD + p] =
              {displayOverrideColor.red, displayOverrideColor.green,
               displayOverrideColor.blue};
  }
  showPixels();
}

static void initPowerFeather() {
  Result result = Result::Failure;
  for (int attempt = 0; attempt < 4 && result != Result::Ok; ++attempt) {
    result = Board.init(BATTERY_CAPACITY_MAH,
                        Mainboard::BatteryType::Generic_LFP);
    if (result != Result::Ok) delay(250);
  }
  if (result != Result::Ok) {
    Serial.printf("power init FAILED (%d); charger remains unconfigured\n",
                  (int)result);
    return;
  }

  powerFeatherReady = true;
  Result maintainResult = Board.setSupplyMaintainVoltage(SUPPLY_MAINTAIN_V);
  Result currentResult = Board.setBatteryChargingMaxCurrent(CHARGE_LIMIT_MA);
  Result batteryResult = Board.getBatteryVoltage(batteryV);
  chargingEnabled = batteryResult == Result::Ok &&
                    batteryV >= BATTERY_PRESENT_MIN_V;
  Result chargeResult = Board.enableBatteryCharging(chargingEnabled);
  bool guardOk = pfSolarGuardInit("magic_wand_conductor",
                                  SUPPLY_MAINTAIN_V, chargingEnabled);
  Serial.printf("power LFP=%umAh maintain=%d current=%d batt=%s%.3fV "
                "charger=%s(%d) guard=%s\n",
                (unsigned)BATTERY_CAPACITY_MAH, (int)maintainResult,
                (int)currentResult,
                batteryResult == Result::Ok ? "" : "ERR/", (double)batteryV,
                chargingEnabled ? "on" : "off", (int)chargeResult,
                guardOk ? "ok" : "ERR");
}

static void powerTick() {
  if (!powerFeatherReady || (int32_t)(millis() - nextPowerMs) < 0) return;
  nextPowerMs = millis() + 2000;
  float value = NAN;
  if (Board.getBatteryVoltage(value) == Result::Ok) batteryV = value;
  if (Board.getBatteryCurrent(value) == Result::Ok)
    batteryMa = value / GAUGE_CURRENT_DIVISOR;
  if (Board.getSupplyVoltage(value) == Result::Ok) supplyV = value;
  if (Board.getSupplyCurrent(value) == Result::Ok) supplyMa = value;
  bool good = false;
  if (Board.checkSupplyGood(good) == Result::Ok) supplyGood = good;
  if (isfinite(supplyV) && isfinite(supplyMa))
    pfSolarGuardTick("magic_wand_conductor", supplyV, supplyMa, supplyGood,
                     SUPPLY_MAINTAIN_V, chargingEnabled);
}

static void initSensors() {
  if (!powerFeatherReady) return;
  Board.enableVSQT(true);
  delay(150);
  Wire1.setClock(100000);
  msaPresent = msa311.begin(MSA311_I2CADDR_DEFAULT, &Wire1);
  if (msaPresent) {
    msa311.setRange(MSA301_RANGE_4_G);
    msa311.setDataRate(MSA301_DATARATE_125_HZ);
    msa311.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
    msa311.setPowerMode(MSA301_NORMALMODE);
  }
  Wire1.setClock(100000);
  bmpPresent = bmp581.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire1);
  if (bmpPresent) {
    bmp581.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
    bmp581.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
    bmp581.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    bmp581.setOutputDataRate(BMP5XX_ODR_10_HZ);
    bmp581.setPowerMode(BMP5XX_POWERMODE_NORMAL);
    bmp581.enablePressure(true);
  }
  Wire1.setClock(100000);
  Serial.printf("sensors @100kHz MSA311=%s BMP581=%s\n",
                msaPresent ? "FOUND" : "MISSING",
                bmpPresent ? "FOUND" : "MISSING");
}

static bool sensorTick() {
  uint32_t now = millis();
  bool newMsaSample = false;
  if (msaPresent && (int32_t)(now - nextMsaMs) >= 0) {
    nextMsaMs = now + 40;
    Wire1.setClock(100000);
    msa311.read();
    float ax = msa311.x_g;
    float ay = msa311.y_g;
    float az = msa311.z_g;
    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    if (isfinite(magnitude) && magnitude > 0.05f) {
      accelFilterSample(accelFilter, ax, ay, az);
      lastMsaOkMs = now;
      newMsaSample = true;
    }
  }

  if (bmpPresent && (int32_t)(now - nextBmpMs) >= 0) {
    nextBmpMs = now + 100;
    Wire1.setClock(100000);
    if (bmp581.performReading() && isfinite(bmp581.pressure) &&
        bmp581.pressure >= 300.0f && bmp581.pressure <= 1200.0f) {
      if (!isfinite(filteredPressureHpa))
        filteredPressureHpa = bmp581.pressure;
      else
        filteredPressureHpa +=
            0.25f * (bmp581.pressure - filteredPressureHpa);
      lastBmpOkMs = now;
    }
    Wire1.setClock(100000);
  }
  return newMsaSample;
}

static void fillHeader(NbHeader *header, uint8_t type) {
  header->ver = NB_PROTO_VER;
  header->type = type;
  memcpy(header->src_id, myId, sizeof(myId));
  header->seq = txSeq++;
  header->uptime_ms = millis();
}

static void onEspNowSend(const esp_now_send_info_t *,
                         esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS)
    ++sendOk;
  else
    ++sendFail;
}

static void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data,
                         int len) {
  if (!rxQueue || !data || len < (int)sizeof(NbHeader) ||
      len > (int)RX_DATA_MAX)
    return;
  const NbHeader *header = (const NbHeader *)data;
  if (header->ver != NB_PROTO_VER ||
      (header->type != NB_HEARTBEAT &&
       header->type != NB_NEIGHBOR_REPORT &&
       header->type != NB_TIME_QUALITY))
    return;
  RxFrame frame = {};
  frame.len = (uint16_t)len;
  frame.rssi = info && info->rx_ctrl ? info->rx_ctrl->rssi : -127;
  frame.receivedMs = millis();
  memcpy(frame.data, data, len);
  if (xQueueSend(rxQueue, &frame, 0) != pdTRUE) ++rxDropped;
}

static bool setupEspNow() {
  rxQueue = xQueueCreate(RX_QUEUE_DEPTH, sizeof(RxFrame));
  if (!rxQueue) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(false, false);
  if (esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK)
    return false;
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
  peer.channel = NB_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

static void sendRepeated(const void *packet, size_t len, uint8_t count,
                         uint16_t gapMs) {
  if (!identityOk || !radioReady) return;
  for (uint8_t i = 0; i < count; ++i) {
    if (esp_now_send(BROADCAST_MAC, (const uint8_t *)packet, len) != ESP_OK)
      ++sendFail;
    if (i + 1 < count) delay(gapMs);
  }
}

static void sendLifecycle(uint8_t mode, uint8_t count) {
  NbForceLifecycle packet = {};
  fillHeader(&packet.h, NB_FORCE_LIFECYCLE);
  memcpy(packet.target_id, ALL_TARGETS, sizeof(packet.target_id));
  packet.mode = mode;
  sendRepeated(&packet, sizeof(packet), count, RELIABLE_SEND_GAP_MS);
  Serial.printf("lifecycle %s seq=%lu copies=%u\n",
                mode == 0 ? "wake" : (mode == 2 ? "auto" : "night"),
                (unsigned long)packet.h.seq, (unsigned)count);
}

static void sendLocate(uint16_t durationS) {
  NbLocateControl packet = {};
  fillHeader(&packet.h, NB_LOCATE_CONTROL);
  memcpy(packet.target_id, ALL_TARGETS, sizeof(packet.target_id));
  packet.duration_s = durationS;
  packet.period_ds = durationS ? LOCATE_PERIOD_DS : 0;
  sendRepeated(&packet, sizeof(packet), RELIABLE_SEND_COUNT,
               RELIABLE_SEND_GAP_MS);
}

static void processRxQueue() {
  if (!rxQueue) return;
  RxFrame frame;
  while (xQueueReceive(rxQueue, &frame, 0) == pdTRUE) {
    const NbHeader *header = (const NbHeader *)frame.data;
    if (header->type == NB_TIME_QUALITY) {
      if (frame.len >= sizeof(NbTimeQuality))
        timeConsensusObserve(timeConsensus,
                             *(const NbTimeQuality *)frame.data,
                             header->src_id, frame.receivedMs);
      continue;
    }
    if (header->type == NB_HEARTBEAT) {
      if (frame.len < offsetof(NbHeartbeat, mode) + sizeof(uint8_t)) continue;
      const NbHeartbeat *heartbeat = (const NbHeartbeat *)frame.data;
      bool hasClass = NB_HAS_HB_FIELD(frame.len, fixture_class);
      uint8_t fixtureClass = hasClass ? heartbeat->fixture_class : 0;
      atomFleetObserveHeartbeat(fleet, header->src_id, frame.rssi,
                                frame.receivedMs, hasClass, fixtureClass);
      continue;
    }
    if (header->type == NB_NEIGHBOR_REPORT) {
      size_t prefix = offsetof(NbNeighborReport, entries);
      if (frame.len < prefix) continue;
      const NbNeighborReport *report =
          (const NbNeighborReport *)frame.data;
      uint8_t available =
          (uint8_t)((frame.len - prefix) / sizeof(NbNeighborEntry));
      uint8_t count = report->count < available ? report->count : available;
      if (count > NB_NEIGHBOR_REPORT_MAX) count = NB_NEIGHBOR_REPORT_MAX;
      for (uint8_t i = 0; i < count; ++i)
        atomFleetObserveEdge(fleet, header->src_id, report->entries[i].id,
                             report->entries[i].med_dbm);
    }
  }
}

static void sendHeartbeat() {
  NbHeartbeat heartbeat = {};
  fillHeader(&heartbeat.h, NB_HEARTBEAT);
  heartbeat.batt_mv = isfinite(batteryV) ? (int16_t)(batteryV * 1000.0f) : 0;
  heartbeat.batt_ma = isfinite(batteryMa) ? (int16_t)batteryMa : 0;
  heartbeat.soc_pct = 255;
  if (powerFeatherReady) {
    uint8_t soc = 0;
    if (Board.getBatteryCharge(soc) == Result::Ok) heartbeat.soc_pct = soc;
  }
  heartbeat.reset_reason = (uint8_t)esp_reset_reason();
  heartbeat.mode = (uint8_t)session.phase;
  heartbeat.supply_mv = isfinite(supplyV) ? (int16_t)(supplyV * 1000.0f) : 0;
  heartbeat.supply_ma = isfinite(supplyMa) ? (int16_t)supplyMa : 0;
  heartbeat.supply_good = supplyGood ? 1 : 0;
  heartbeat.lux_x10 = 0xFFFFFFFFUL;
  heartbeat.ptemp_cx10 = INT16_MIN;
  heartbeat.prh_pct = 255;
  heartbeat.btemp_cx10 = INT16_MIN;
  heartbeat.ina_pv_mv = heartbeat.ina_pa_ma = INT16_MIN;
  heartbeat.ina_bv_mv = heartbeat.ina_ba_ma = INT16_MIN;
  heartbeat.cfg_cap_mah = BATTERY_CAPACITY_MAH;
  heartbeat.cfg_charge_ma = (uint16_t)CHARGE_LIMIT_MA;
  heartbeat.bq_vindpm_mv = heartbeat.bq_ichg_ma = 0xFFFF;
  heartbeat.bq_vreg_mv = 0xFFFF;
  heartbeat.bq_reg16 = heartbeat.bq_reg18 = 0xFF;
  heartbeat.bq_stat0 = heartbeat.bq_stat1 = 0xFF;
  heartbeat.bq_fault0 = heartbeat.bq_flag0 = 0xFF;
  heartbeat.bq_flag1 = heartbeat.bq_fault_flag0 = 0xFF;
  heartbeat.bq_part = 0xFF;
  strncpy(heartbeat.fw_rev, MAGIC_WAND_CONDUCTOR_VERSION,
          sizeof(heartbeat.fw_rev) - 1);
  // Stop before fixture_class: the external LED rail/switch has no feedback
  // into the PowerFeather, so this image refuses to publish invented LED truth.
  sendRepeated(&heartbeat, offsetof(NbHeartbeat, fixture_class), 1, 0);
}

static void sendStrike(const uint8_t target[3]) {
  NbTargetU16 packet = {};
  fillHeader(&packet.h, NB_TARGET_SOLENOID);
  memcpy(packet.target_id, target, sizeof(packet.target_id));
  packet.value = RES_WAND_PULSE_MS;
  sendRepeated(&packet, sizeof(packet), COMMAND_SEND_COUNT,
               COMMAND_SEND_GAP_MS);
}

static void sendColorLayer(const AtomWaveTarget *targets, size_t count,
                           WandRgb color) {
  size_t offset = 0;
  while (offset < count) {
    uint8_t chunk = (uint8_t)(count - offset);
    if (chunk > NB_DIRECT_MAX_ENTRIES) chunk = NB_DIRECT_MAX_ENTRIES;
    NbDirectFrame packet = {};
    fillHeader(&packet.h, NB_DIRECT_FRAME);
    packet.flags = 0x03; // 10 s micro-lease + hard cut
    packet.count = chunk;
    for (uint8_t i = 0; i < chunk; ++i) {
      memcpy(packet.entries[i].id, targets[offset + i].id, 3);
      packet.entries[i].r = color.red;
      packet.entries[i].g = color.green;
      packet.entries[i].b = color.blue;
      packet.entries[i].w = 0;
    }
    size_t wireLen = offsetof(NbDirectFrame, entries) +
                     (size_t)chunk * sizeof(NbDirectEntry);
    sendRepeated(&packet, wireLen, FRAME_SEND_COUNT, FRAME_SEND_GAP_MS);
    offset += chunk;
  }
}

static AtomActionFamily currentActionFamily(uint32_t nowMs,
                                            TimeEstimate *estimateOut) {
  TimeEstimate estimate = timeConsensusEstimate(timeConsensus, nowMs);
  if (estimateOut) *estimateOut = estimate;
  bool civilNight = estimate.valid && showScheduleAt(estimate.utcS).civilNight;
  return atomActionFamily(estimate.valid, civilNight);
}

static void finishSession() {
  atomSessionInit(session);
  Serial.println("session idle; radio remains available for the next gesture");
  setDisplayOverride({0, 3, 0}, 700, false);
}

static void handleSessionActions(uint16_t actions) {
  if (!actions) return;
  if (actions & ATOM_ACTION_STOP_LOCATE) sendLocate(0);
  if (actions & ATOM_ACTION_SEND_WAKE) {
    uint8_t copies = session.phase == ATOM_SESSION_READY
                         ? RELIABLE_SEND_COUNT
                         : 1;
    sendLifecycle(0, copies);
  }
  if (actions & ATOM_ACTION_START_LOCATE) sendLocate(LOCATE_DURATION_S);
  if (actions & ATOM_ACTION_SEND_AUTO)
    sendLifecycle(2, RELIABLE_SEND_COUNT);
  if (actions & ATOM_ACTION_BECAME_READY) {
    size_t fresh = atomFleetFreshCount(fleet, millis(), TARGET_FRESH_MS,
                                       FIXTURE_DOWNLIGHT);
    Serial.printf("READY peers=%u fresh_canopies=%u edges=%u drops=%lu\n",
                  (unsigned)fleet.count, (unsigned)fresh,
                  (unsigned)atomFleetDirectedEdgeCount(fleet),
                  (unsigned long)rxDropped);
    setDisplayOverride({0, 6, 3}, 900, false);
  }
  // The wand has no controller-power switch or wake input. End cleanup by
  // returning to idle, not deep sleep; a later deliberate gesture can restart.
  if (actions & ATOM_ACTION_SLEEP) finishSession();
}

static void startSession() {
  if (!identityOk || !radioReady) {
    setDisplayOverride({6, 0, 0}, 900, false);
    Serial.println("session refused: identity/radio not ready");
    return;
  }
  atomFleetInit(fleet);
  atomSessionStart(session, millis());
  Serial.println("session start: six-minute Wake + per-Hex spiral gather");
  setDisplayOverride({0, 5, 6}, 500, false);
}

static void playEffect() {
  uint32_t now = millis();
  atomSessionNoteInput(session, now);
  processRxQueue();
  TimeEstimate wall = {};
  AtomActionFamily family = currentActionFamily(now, &wall);
  if (family == ATOM_ACTION_UNKNOWN) {
    Serial.println("effect refused: no trustworthy fleet UTC");
    setDisplayOverride({6, 0, 0}, 900, false);
    return;
  }

  size_t count = atomFleetPlanWave(fleet, now, TARGET_FRESH_MS,
                                   FIXTURE_DOWNLIGHT, waveTargets,
                                   ATOM_FLEET_MAX_PEERS);
  if (!count) {
    Serial.println("effect refused: no fresh classified canopies");
    setDisplayOverride({6, 0, 0}, 900, false);
    return;
  }

  WandColorMode playedColor = nightColor;
  WandRgb treeColor = wandTreeColor(playedColor,
                                     RES_WAND_TREE_COLOR_VALUE,
                                     RES_WAND_TREE_RGB_VALUE);
  Serial.printf("effect=%s utc=%lu source=%u votes=%u targets=%u "
                "first=%02X%02X%02X rssi=%d edges=%u\n",
                family == ATOM_ACTION_CHIME ? "chime" :
                                               wandColorName(playedColor),
                (unsigned long)wall.utcS, (unsigned)wall.source,
                (unsigned)wall.votes, (unsigned)count,
                waveTargets[0].id[0], waveTargets[0].id[1],
                waveTargets[0].id[2], (int)waveTargets[0].atomRssi,
                (unsigned)atomFleetDirectedEdgeCount(fleet));

  size_t begin = 0;
  while (begin < count) {
    uint8_t layer = waveTargets[begin].layer;
    size_t end = begin + 1;
    while (end < count && waveTargets[end].layer == layer) ++end;
    if (family == ATOM_ACTION_CHIME) {
      for (size_t i = begin; i < end; ++i) sendStrike(waveTargets[i].id);
    } else {
      sendColorLayer(&waveTargets[begin], end - begin, treeColor);
    }
    processRxQueue();
    if (end < count) delay(WAVE_LAYER_GAP_MS);
    begin = end;
  }

  if (family == ATOM_ACTION_CHIME) {
    setDisplayOverride({6, 3, 0}, 1000, false);
  } else {
    setDisplayOverride(wandLocalColor(playedColor), 1200, true);
    nightColor = wandNextColor(nightColor);
  }
}

static void handleAcceptedGesture() {
  uint32_t now = millis();
  Serial.printf("gesture accepted #%lu session=%u rise=%.2fm peak=%.3fg\n",
                (unsigned long)gesture.triggerCount, (unsigned)session.phase,
                (double)gesture.maxRiseM, (double)gesture.peakMotionG);
  switch (session.phase) {
  case ATOM_SESSION_IDLE:
    startSession();
    break;
  case ATOM_SESSION_GATHER:
    atomSessionNoteInput(session, now);
    Serial.printf("still gathering: %lus remaining\n",
                  (unsigned long)((session.gatherUntilMs - now + 999) / 1000));
    setDisplayOverride({4, 2, 0}, 700, false);
    break;
  case ATOM_SESSION_READY:
    playEffect();
    break;
  case ATOM_SESSION_CLEANUP:
  case ATOM_SESSION_SLEEP_DUE:
    Serial.println("gesture ignored during Auto cleanup");
    break;
  }
}

static void gestureTick() {
  uint32_t now = millis();
  bool valid = lastMsaOkMs && lastBmpOkMs &&
               (uint32_t)(now - lastMsaOkMs) <= 300 &&
               (uint32_t)(now - lastBmpOkMs) <= 300 &&
               isfinite(filteredPressureHpa);
  bool accepted = wandGestureSample(gesture, now, valid,
                                    accelFilter.swayEnv,
                                    filteredPressureHpa);
  if (gesture.state != lastLoggedGesture) {
    Serial.printf("gesture %s pressure=%.3f sway=%.3f rise=%.2f\n",
                  wandGestureStateName(gesture.state),
                  (double)filteredPressureHpa,
                  (double)accelFilter.swayEnv,
                  (double)wandPressureRiseM(gesture.baselinePressureHpa,
                                            filteredPressureHpa));
    lastLoggedGesture = gesture.state;
    nextDisplayMs = 0;
  }
  if (accepted) handleAcceptedGesture();
}

static void displayTick() {
  uint32_t now = millis();
  if ((int32_t)(now - nextDisplayMs) < 0) return;

  if ((int32_t)(displayOverrideUntilMs - now) > 0) {
    renderOverride();
    nextDisplayMs = now + 250;
    return;
  }

  if (gesture.state == WAND_GESTURE_ARMED ||
      gesture.state == WAND_GESTURE_LIFTING ||
      gesture.state == WAND_GESTURE_HIGH_HOLD) {
    renderGestureCue(gesture.state);
    nextDisplayMs = now + 200;
    return;
  }

  switch (session.phase) {
  case ATOM_SESSION_IDLE:
    renderIdle();
    nextDisplayMs = now + 400;
    break;
  case ATOM_SESSION_GATHER:
    renderGather(now);
    nextDisplayMs = now + 120;
    break;
  case ATOM_SESSION_READY:
    switch (currentActionFamily(now, nullptr)) {
    case ATOM_ACTION_CHIME:
      renderChimeReady();
      break;
    case ATOM_ACTION_COLOR:
      fillPixels(wandLocalColor(nightColor));
      showPixels();
      break;
    default:
      renderUnknown();
      break;
    }
    nextDisplayMs = now + 1000;
    break;
  case ATOM_SESSION_CLEANUP:
  case ATOM_SESSION_SLEEP_DUE:
    fillPixels({3, 0, 3});
    showPixels();
    nextDisplayMs = now + 500;
    break;
  }
}

static void identityFaultTick() {
  static uint32_t nextMs = 0;
  static bool on = false;
  uint32_t now = millis();
  if ((int32_t)(now - nextMs) < 0) return;
  nextMs = now + 150;
  on = !on;
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  Serial.begin(115200);
  delay(300);

  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  memcpy(myId, mac + 3, sizeof(myId));
  identityOk = memcmp(mac, EXPECTED_MAC, sizeof(EXPECTED_MAC)) == 0;
  Serial.println();
  Serial.println("=== Resonance " MAGIC_WAND_CONDUCTOR_VERSION " ===");
  Serial.printf("mac=%02X:%02X:%02X:%02X:%02X:%02X expected=F40344 "
                "identity=%s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                identityOk ? "ok" : "REFUSED");
  if (!identityOk) {
    Serial.println("wrong board: no power configuration, LED frames, or fleet TX");
    return;
  }

  atomSessionInit(session);
  atomFleetInit(fleet);
  timeConsensusInit(timeConsensus);
  wandGestureInit(gesture);
  accelFilterInit(accelFilter);

  initPowerFeather();
  initSensors();
  rmtReady = rmtInit(DATA_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_4, 10000000);
  rmtSetEOT(DATA_PIN, LOW);
  radioReady = setupEspNow();
  nextHeartbeatMs = millis();
  Serial.printf("radio=%s channel=%u RMT=%s pixels=%u\n",
                radioReady ? "ready" : "FAILED", NB_CHANNEL,
                rmtReady ? "ready" : "FAILED", (unsigned)PIXEL_COUNT);
  Serial.println("gesture: hold still 1.6s -> raise >=0.45m -> hold high 0.8s");
  Serial.println("first gesture=gather; ready gestures=day chime / night RGB cycle");
  Serial.println("LED switch state is unknowable: external load bypasses gauge");
  setDisplayOverride({0, 5, 3}, 800, false);
}

void loop() {
  if (!identityOk) {
    identityFaultTick();
    delay(2);
    return;
  }

  processRxQueue();
  powerTick();
  if (sensorTick()) gestureTick();
  if (session.phase != ATOM_SESSION_IDLE)
    handleSessionActions(atomSessionTick(session, millis()));
  uint32_t now = millis();
  if (radioReady && (int32_t)(now - nextHeartbeatMs) >= 0) {
    nextHeartbeatMs = now + HEARTBEAT_MS;
    sendHeartbeat();
  }
  displayTick();
  delay(2);
}
