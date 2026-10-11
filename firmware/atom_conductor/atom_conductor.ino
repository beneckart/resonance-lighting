// Resonance Atom Conductor: one-button, bounded fleet interaction.
//
// A button wake opens a six-minute fleet gather, reconstructs an RSSI-neighbor
// graph from existing fixture reports, and then emits centrally sequenced
// canopy strike or RGBW waves. It never associates to WiFi and exposes no OTA,
// persistence, maintenance, sleep-fleet, or serial command surface.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../fixture/src/core/fixture_context.h"
#include "../fixture/src/core/packet.h"
#include "../fixture/src/core/show_schedule.h"
#include "../fixture/src/core/time_consensus.h"
#include "src/core/action_gate.h"
#include "src/core/atom_session.h"
#include "src/core/fleet_graph.h"

// Arduino copies sources below src/ into an isolated build tree. Include these
// canonical implementations at the sketch level so the Atom reuses the exact
// fleet schedule and time selector without maintaining forked copies.
#include "../fixture/src/core/show_schedule.cpp"
#include "../fixture/src/core/time_consensus.cpp"

#define ATOM_CONDUCTOR_VERSION "atom-conductor-2026-09-03.1"

#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif
#ifndef RES_ATOM_PULSE_MS
#define RES_ATOM_PULSE_MS 25
#endif
#ifndef RES_ATOM_COLOR_VALUE
#define RES_ATOM_COLOR_VALUE 64
#endif

#if RES_ATOM_PULSE_MS < 5 || RES_ATOM_PULSE_MS > 300
#error "RES_ATOM_PULSE_MS must be 5..300"
#endif
#if RES_ATOM_COLOR_VALUE < 1 || RES_ATOM_COLOR_VALUE > 255
#error "RES_ATOM_COLOR_VALUE must be 1..255"
#endif

static constexpr uint8_t BUTTON_PIN = 39; // Atom Matrix face, active LOW
static constexpr uint8_t PIXEL_PIN = 27;
static constexpr uint8_t PIXEL_COUNT = 25;
static constexpr uint8_t CENTER_PIXEL = 12;
static constexpr uint32_t DEBOUNCE_MS = 35;
static constexpr uint32_t MODE_HOLD_MS = 700;
static constexpr uint32_t END_HOLD_MS = 2500;
static constexpr uint32_t IDLE_SLEEP_MS = 60000;
static constexpr uint32_t TARGET_FRESH_MS = 12000;
static constexpr uint16_t LOCATE_DURATION_S = 120;
static constexpr uint8_t LOCATE_PERIOD_DS = 200; // 20 s
static constexpr uint16_t WAVE_LAYER_GAP_MS = 140;
static constexpr uint8_t COMMAND_SEND_COUNT = 6;
static constexpr uint16_t COMMAND_SEND_GAP_MS = 8;
static constexpr uint8_t FRAME_SEND_COUNT = 3;
static constexpr uint16_t FRAME_SEND_GAP_MS = 12;
static constexpr uint8_t RELIABLE_SEND_COUNT = 4;
static constexpr uint16_t RELIABLE_SEND_GAP_MS = 5;
static constexpr size_t RX_QUEUE_DEPTH = 32;
static constexpr size_t RX_DATA_MAX = 250;

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF,
                                         0xFF, 0xFF, 0xFF};
static const uint8_t ALL_TARGETS[3] = {0, 0, 0};

enum EffectMode : uint8_t {
  EFFECT_CHIME = 0,
  EFFECT_RED = 1,
  EFFECT_GREEN = 2,
  EFFECT_BLUE = 3,
  EFFECT_WHITE = 4,
  EFFECT_COUNT = 5,
};

struct RxFrame {
  uint16_t len;
  int8_t rssi;
  uint32_t receivedMs;
  uint8_t data[RX_DATA_MAX];
};

Adafruit_NeoPixel pixels(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
AtomSession session;
AtomFleet fleet;
AtomWaveTarget waveTargets[ATOM_FLEET_MAX_PEERS];
TimeConsensus timeConsensus;
QueueHandle_t rxQueue = nullptr;

uint8_t myId[3] = {};
uint32_t txSeq = 0;
bool radioReady = false;
EffectMode nightColorMode = EFFECT_RED;
uint32_t idleSinceMs = 0;
uint32_t displayNextMs = 0;
uint32_t transientUntilMs = 0;
uint32_t transientColor = 0;

bool buttonRawReleased = true;
bool buttonStableReleased = true;
bool suppressNextRelease = false;
uint32_t buttonChangedMs = 0;
uint32_t buttonPressedMs = 0;

volatile uint32_t sendOk = 0;
volatile uint32_t sendFail = 0;
volatile uint32_t rxDropped = 0;

static uint32_t rgb(uint8_t red, uint8_t green, uint8_t blue) {
  return pixels.Color(red, green, blue);
}

static const char *effectName(EffectMode mode) {
  switch (mode) {
  case EFFECT_CHIME: return "chime";
  case EFFECT_RED: return "red";
  case EFFECT_GREEN: return "green";
  case EFFECT_BLUE: return "blue";
  case EFFECT_WHITE: return "white";
  default: return "unknown";
  }
}

static uint32_t effectFaceColor(EffectMode mode) {
  switch (mode) {
  case EFFECT_CHIME: return rgb(24, 10, 0);
  case EFFECT_RED: return rgb(24, 0, 0);
  case EFFECT_GREEN: return rgb(0, 24, 0);
  case EFFECT_BLUE: return rgb(0, 0, 24);
  case EFFECT_WHITE: return rgb(18, 18, 18);
  default: return 0;
  }
}

static void showCenter(uint32_t color) {
  pixels.clear();
  pixels.setPixelColor(CENTER_PIXEL, color);
  pixels.show();
}

static void showSolid(uint32_t color) {
  pixels.fill(color);
  pixels.show();
}

static void showChime() {
  pixels.clear();
  const uint8_t bell[] = {2, 6, 7, 8, 10, 11, 12, 13, 14,
                          16, 17, 18, 21, 22, 23};
  for (uint8_t index : bell) pixels.setPixelColor(index, rgb(24, 10, 0));
  pixels.show();
}

static void showTimeUnknown() {
  pixels.clear();
  const uint8_t question[] = {1, 2, 3, 8, 12, 17, 22};
  for (uint8_t index : question) pixels.setPixelColor(index, rgb(18, 0, 18));
  pixels.show();
}

static void flashStatus(uint32_t color, uint32_t durationMs) {
  transientColor = color;
  transientUntilMs = millis() + durationMs;
  showSolid(color);
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
  if (!rxQueue) {
    Serial.println("rx queue allocation FAILED");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(false, false);
  if (esp_wifi_set_channel(NB_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("esp_wifi_set_channel FAILED");
    return false;
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init FAILED");
    return false;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
  peer.channel = NB_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("esp_now_add_peer FAILED: %d\n", (int)result);
    return false;
  }
  return true;
}

static void sendRepeated(const void *packet, size_t len, uint8_t count,
                         uint16_t gapMs) {
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
  Serial.printf("locate %s seq=%lu\n", durationS ? "start/refresh" : "stop",
                (unsigned long)packet.h.seq);
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

static void handleSessionActions(uint16_t actions) {
  if (!actions) return;
  if (actions & ATOM_ACTION_STOP_LOCATE) sendLocate(0);
  if (actions & ATOM_ACTION_SEND_WAKE) {
    // The six-minute gather uses the T-Deck's quiet 0.5 packet/s campaign.
    // A five-minute READY renewal is a short reliable burst.
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
    Serial.printf("READY peers=%u fresh_canopies=%u directed_edges=%u "
                  "rx_dropped=%lu\n",
                  (unsigned)fleet.count, (unsigned)fresh,
                  (unsigned)atomFleetDirectedEdgeCount(fleet),
                  (unsigned long)rxDropped);
    flashStatus(rgb(0, 24, 12), 1000);
  }
  if (actions & ATOM_ACTION_SLEEP) {
    Serial.printf("sleep tx_ok=%lu tx_fail=%lu rx_drop=%lu\n",
                  (unsigned long)sendOk, (unsigned long)sendFail,
                  (unsigned long)rxDropped);
    pixels.clear();
    pixels.show();
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
    delay(30);
    esp_deep_sleep_start();
  }
}

static void startSession() {
  if (!radioReady) {
    flashStatus(rgb(24, 0, 0), 1000);
    Serial.println("session refused: radio not ready");
    return;
  }
  atomFleetInit(fleet);
  atomSessionStart(session, millis());
  Serial.println("session start: six-minute Wake + neighbor gather");
}

static void sendStrike(const uint8_t target[3]) {
  NbTargetU16 packet = {};
  fillHeader(&packet.h, NB_TARGET_SOLENOID);
  memcpy(packet.target_id, target, sizeof(packet.target_id));
  packet.value = RES_ATOM_PULSE_MS;
  sendRepeated(&packet, sizeof(packet), COMMAND_SEND_COUNT,
               COMMAND_SEND_GAP_MS);
}

static void sendColorLayer(const AtomWaveTarget *targets, size_t count,
                           uint8_t red, uint8_t green, uint8_t blue,
                           uint8_t white) {
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
      packet.entries[i].r = red;
      packet.entries[i].g = green;
      packet.entries[i].b = blue;
      packet.entries[i].w = white;
    }
    size_t wireLen = offsetof(NbDirectFrame, entries) +
                     (size_t)chunk * sizeof(NbDirectEntry);
    sendRepeated(&packet, wireLen, FRAME_SEND_COUNT, FRAME_SEND_GAP_MS);
    offset += chunk;
  }
}

static void showWaveProgress(EffectMode mode, uint8_t layer) {
  pixels.clear();
  uint8_t lit = (uint8_t)(1 + (layer % PIXEL_COUNT));
  uint32_t color = effectFaceColor(mode);
  for (uint8_t i = 0; i < lit; ++i) pixels.setPixelColor(i, color);
  pixels.show();
}

static AtomActionFamily currentActionFamily(uint32_t nowMs,
                                            TimeEstimate *estimateOut) {
  TimeEstimate estimate = timeConsensusEstimate(timeConsensus, nowMs);
  if (estimateOut) *estimateOut = estimate;
  bool civilNight = estimate.valid && showScheduleAt(estimate.utcS).civilNight;
  return atomActionFamily(estimate.valid, civilNight);
}

static void playEffect() {
  uint32_t now = millis();
  atomSessionNoteInput(session, now);
  processRxQueue();
  TimeEstimate wall = {};
  AtomActionFamily family = currentActionFamily(now, &wall);
  if (family == ATOM_ACTION_UNKNOWN) {
    Serial.println("effect refused: no trustworthy fleet UTC");
    flashStatus(rgb(24, 0, 0), 800);
    return;
  }
  EffectMode playMode =
      family == ATOM_ACTION_CHIME ? EFFECT_CHIME : nightColorMode;
  size_t count = atomFleetPlanWave(fleet, now, TARGET_FRESH_MS,
                                   FIXTURE_DOWNLIGHT, waveTargets,
                                   ATOM_FLEET_MAX_PEERS);
  if (!count) {
    Serial.println("effect refused: no fresh classified canopies");
    flashStatus(rgb(24, 0, 0), 800);
    return;
  }

  uint8_t red = 0, green = 0, blue = 0, white = 0;
  if (playMode == EFFECT_RED) red = RES_ATOM_COLOR_VALUE;
  if (playMode == EFFECT_GREEN) green = RES_ATOM_COLOR_VALUE;
  if (playMode == EFFECT_BLUE) blue = RES_ATOM_COLOR_VALUE;
  if (playMode == EFFECT_WHITE) white = RES_ATOM_COLOR_VALUE;

  Serial.printf("effect %s attempted utc=%lu source=%u votes=%u "
                "targets=%u first=%02X%02X%02X rssi=%d edges=%u\n",
                effectName(playMode), (unsigned long)wall.utcS,
                (unsigned)wall.source, (unsigned)wall.votes, (unsigned)count,
                waveTargets[0].id[0], waveTargets[0].id[1],
                waveTargets[0].id[2], (int)waveTargets[0].atomRssi,
                (unsigned)atomFleetDirectedEdgeCount(fleet));

  size_t begin = 0;
  while (begin < count) {
    uint8_t layer = waveTargets[begin].layer;
    size_t end = begin + 1;
    while (end < count && waveTargets[end].layer == layer) ++end;
    showWaveProgress(playMode, layer);
    if (playMode == EFFECT_CHIME) {
      for (size_t i = begin; i < end; ++i) sendStrike(waveTargets[i].id);
    } else {
      sendColorLayer(&waveTargets[begin], end - begin, red, green, blue, white);
    }
    processRxQueue();
    if (end < count) delay(WAVE_LAYER_GAP_MS);
    begin = end;
  }
  transientColor = effectFaceColor(playMode);
  transientUntilMs = millis() + 700;
  if (family == ATOM_ACTION_COLOR)
    nightColorMode = nightColorMode == EFFECT_WHITE
                         ? EFFECT_RED
                         : (EffectMode)((uint8_t)nightColorMode + 1);
}

static void handleGesture(uint32_t heldMs) {
  uint32_t now = millis();
  if (heldMs >= END_HOLD_MS) {
    if (session.phase != ATOM_SESSION_IDLE)
      handleSessionActions(atomSessionRequestEnd(session, now));
    else
      handleSessionActions(ATOM_ACTION_SLEEP);
    return;
  }

  if (heldMs >= MODE_HOLD_MS) {
    if (session.phase == ATOM_SESSION_IDLE) {
      startSession();
      return;
    }
    if (session.phase == ATOM_SESSION_GATHER ||
        session.phase == ATOM_SESSION_READY) {
      atomSessionNoteInput(session, now);
      nightColorMode = nightColorMode == EFFECT_WHITE
                           ? EFFECT_RED
                           : (EffectMode)((uint8_t)nightColorMode + 1);
      Serial.printf("next night color: %s\n", effectName(nightColorMode));
      flashStatus(effectFaceColor(nightColorMode), 600);
    }
    return;
  }

  if (session.phase == ATOM_SESSION_IDLE) {
    startSession();
  } else if (session.phase == ATOM_SESSION_GATHER) {
    atomSessionNoteInput(session, now);
    Serial.printf("still gathering: %lus remaining\n",
                  (unsigned long)((session.gatherUntilMs - now + 999) / 1000));
    flashStatus(rgb(18, 10, 0), 500);
  } else if (session.phase == ATOM_SESSION_READY) {
    playEffect();
  }
}

static void buttonTick() {
  bool released = digitalRead(BUTTON_PIN) == HIGH;
  uint32_t now = millis();
  if (released != buttonRawReleased) {
    buttonRawReleased = released;
    buttonChangedMs = now;
  }
  if (released == buttonStableReleased ||
      (uint32_t)(now - buttonChangedMs) < DEBOUNCE_MS)
    return;

  buttonStableReleased = released;
  if (!released) {
    buttonPressedMs = now;
    return;
  }
  if (suppressNextRelease) {
    suppressNextRelease = false;
    return;
  }
  handleGesture(now - buttonPressedMs);
}

static void displayTick() {
  uint32_t now = millis();
  if ((int32_t)(now - displayNextMs) < 0) return;
  displayNextMs = now + 100;
  if (!buttonStableReleased &&
      (uint32_t)(now - buttonPressedMs) >= END_HOLD_MS) {
    showSolid(rgb(16, 0, 16));
    return;
  }
  if ((int32_t)(transientUntilMs - now) > 0) {
    showSolid(transientColor);
    return;
  }
  switch (session.phase) {
  case ATOM_SESSION_IDLE:
    showCenter(radioReady ? rgb(0, 0, 18) : rgb(24, 0, 0));
    break;
  case ATOM_SESSION_GATHER: {
    uint32_t elapsedMs = now - session.startedMs;
    uint8_t lit = (uint8_t)((elapsedMs * PIXEL_COUNT) / ATOM_GATHER_MS);
    if (lit >= PIXEL_COUNT) lit = PIXEL_COUNT - 1;
    pixels.clear();
    for (uint8_t i = 0; i <= lit; ++i)
      pixels.setPixelColor(i, rgb(0, 6, 20));
    pixels.show();
    break;
  }
  case ATOM_SESSION_READY:
    switch (currentActionFamily(now, nullptr)) {
    case ATOM_ACTION_CHIME:
      showChime();
      break;
    case ATOM_ACTION_COLOR:
      showSolid(effectFaceColor(nightColorMode));
      break;
    default:
      showTimeUnknown();
      break;
    }
    break;
  case ATOM_SESSION_CLEANUP:
    showSolid(rgb(12, 0, 12));
    break;
  case ATOM_SESSION_SLEEP_DUE:
    pixels.clear();
    pixels.show();
    break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pixels.begin();
  pixels.setBrightness(32);
  pixels.clear();
  pixels.show();
  pinMode(BUTTON_PIN, INPUT); // the Atom board supplies the pull-up
  delay(2);

  buttonRawReleased = digitalRead(BUTTON_PIN) == HIGH;
  buttonStableReleased = buttonRawReleased;
  buttonChangedMs = millis();
  buttonPressedMs = millis();

  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  memcpy(myId, mac + 3, sizeof(myId));

  atomSessionInit(session);
  atomFleetInit(fleet);
  timeConsensusInit(timeConsensus);
  Serial.println();
  Serial.println("=== Resonance " ATOM_CONDUCTOR_VERSION " ===");
  Serial.printf("node=%02X%02X%02X channel=%d pulse=%u color=%u\n",
                myId[0], myId[1], myId[2], NB_CHANNEL,
                (unsigned)RES_ATOM_PULSE_MS,
                (unsigned)RES_ATOM_COLOR_VALUE);
  Serial.println("short=auto day-chime/night-color; 0.7s=next night color; "
                 "2.5s=Auto+sleep");

  radioReady = setupEspNow();
  idleSinceMs = millis();
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  if (radioReady && wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    suppressNextRelease = !buttonStableReleased;
    startSession();
  }
  Serial.printf("radio=%s wake_cause=%d\n",
                radioReady ? "ready" : "FAILED", (int)wakeCause);
  displayTick();
}

void loop() {
  processRxQueue();
  buttonTick();
  if (session.phase != ATOM_SESSION_IDLE)
    handleSessionActions(atomSessionTick(session, millis()));
  else if ((uint32_t)(millis() - idleSinceMs) >= IDLE_SLEEP_MS)
    handleSessionActions(ATOM_ACTION_SLEEP);
  displayTick();
  delay(2);
}
