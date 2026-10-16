// Decompression button: one local signature request per named lantern.
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../fixture/src/core/packet.h"
#include "roster.h"
#include "src/core/button.h"

#ifndef ATOM_DECOMPRESSION_VERSION
#define ATOM_DECOMPRESSION_VERSION "atom-decompression-dev"
#endif
#ifndef NB_CHANNEL
#define NB_CHANNEL 11
#endif

static constexpr uint8_t BUTTON_PIN = 39, PIXEL_PIN = 27;
static constexpr uint8_t BCAST[6] = {255,255,255,255,255,255};
static constexpr uint8_t FACE[SQUAD_COUNT] = {6,7,8,11,12,13,16,17,18};
struct RxFrame { uint16_t len; uint8_t data[250]; };
struct Peer {
  bool heard = false, ready = false;
  uint32_t heardMs = 0, readyMs = 0;
  uint16_t leaseLeft = 0;
  uint8_t playStatus = 0, hits = 0;
  uint32_t lateUs = 0;
  char fw[25] = {};
};
static Peer peers[SQUAD_COUNT];
static QueueHandle_t rxQueue;
static Adafruit_NeoPixel pixels(25, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
static DecompressionButton button;
static uint8_t queuedPresses=0;
// Sample the button and queue any accepted press. Called from loop() and while
// sendSquad() is busy (~90 ms), so the release between the two presses of a
// quick double click is never missed.
static void sampleButton() {
  if (button.tick(digitalRead(BUTTON_PIN)==HIGH,millis()) && queuedPresses<4) ++queuedPresses;
}
static uint8_t myId[3];
static uint32_t bootNonce, txSeq = 0, leaseSeq = 0, playSeq = 0;
static uint32_t nextLeaseMs = 0, lastPlayMs = 0, nextStatusMs = 0;
static uint32_t presses = 0;
static bool radioReady = false, enabled = true, havePlayed = false;
static volatile uint32_t rxDropped = 0;

static int member(const uint8_t id[3]) {
  for (unsigned i=0;i<SQUAD_COUNT;++i)
    if (memcmp(id,SQUAD[i].id,3)==0) return (int)i;
  return -1;
}
static void receive(const esp_now_recv_info_t *, const uint8_t *data, int len) {
  if (!rxQueue || len<(int)sizeof(NbHeader) || len>250) return;
  const auto *h = (const NbHeader *)data;
  if (h->ver!=NB_PROTO_VER || member(h->src_id)<0 ||
      (h->type!=NB_HEARTBEAT && h->type!=NB_DECOMPRESSION_RECEIPT)) return;
  RxFrame frame = {};
  frame.len=len;
  memcpy(frame.data,data,len);
  if (xQueueSend(rxQueue,&frame,0)!=pdTRUE) ++rxDropped;
}
static bool startRadio() {
  rxQueue=xQueueCreate(40,sizeof(RxFrame));
  if (!rxQueue) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(false,false);
  if (esp_wifi_set_channel(NB_CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK ||
      esp_now_init()!=ESP_OK) return false;
  esp_now_register_recv_cb(receive);
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr,BCAST,6);
  p.channel=NB_CHANNEL; p.ifidx=WIFI_IF_STA;
  return esp_now_add_peer(&p)==ESP_OK;
}
static void drain() {
  RxFrame frame;
  while (xQueueReceive(rxQueue,&frame,0)==pdTRUE) {
    const auto *h=(const NbHeader *)frame.data;
    int i=member(h->src_id);
    if (i<0) continue;
    Peer &p=peers[i];
    p.heard=true; p.heardMs=millis();
    if (h->type==NB_HEARTBEAT) {
      if (NB_HAS_HB_FIELD(frame.len,fw_rev)) {
        const auto *hb=(const NbHeartbeat *)frame.data;
        memcpy(p.fw,hb->fw_rev,24); p.fw[24]=0;
      }
      continue;
    }
    if (frame.len!=sizeof(NbDecompressionReceipt)) continue;
    const auto &r=*(const NbDecompressionReceipt *)frame.data;
    if (memcmp(r.request_source,myId,3)!=0 || r.controller_boot!=bootNonce) continue;
    if (r.action==NB_DECOMP_SESSION && r.request_seq==leaseSeq) {
      p.ready=enabled && r.status==NB_DECOMP_READY && r.lease_left_s>10;
      p.readyMs=millis(); p.leaseLeft=r.lease_left_s;
    }
    if (r.action==NB_DECOMP_PLAY && havePlayed && r.request_seq==playSeq) {
      // A delayed STARTED copy cannot overwrite a terminal result.
      if (p.playStatus!=NB_DECOMP_COMPLETE && p.playStatus!=NB_DECOMP_ABORTED) {
        p.playStatus=r.status; p.hits=r.signature_hits;
        p.lateUs=r.signature_max_late_us;
      }
    }
    Serial.printf("receipt %s seq=%lu action=%u status=%u hits=%u late_us=%lu\n",
      SQUAD[i].name,(unsigned long)r.request_seq,r.action,r.status,r.signature_hits,
      (unsigned long)r.signature_max_late_us);
  }
}
static uint32_t sendSquad(uint8_t action,uint8_t minutes) {
  const uint32_t seq=++txSeq, started=millis();
  // Bounded retries reuse the exact identity and never become later plays.
  for (uint8_t copy=0;copy<3;++copy) {
    for (unsigned i=0;i<SQUAD_COUNT;++i) {
      NbDecompressionControl c = {};
      c.h.ver=NB_PROTO_VER; c.h.type=NB_DECOMPRESSION_CONTROL;
      memcpy(c.h.src_id,myId,3); c.h.seq=seq; c.h.uptime_ms=started;
      memcpy(c.target_id,SQUAD[i].id,3);
      c.controller_boot=bootNonce; c.action=action; c.minutes=minutes;
      if (esp_now_send(BCAST,(const uint8_t *)&c,sizeof(c))!=ESP_OK)
        Serial.printf("send refused %s seq=%lu\n",SQUAD[i].name,(unsigned long)seq);
      for (const uint32_t t=millis(); millis()-t<3;) { sampleButton(); delayMicroseconds(250); }
    }
  }
  return seq;
}
// Button presses were already spaced by DecompressionButton at press time (a
// queued one may run right after the previous send), so only USB "play" is
// throttled here.
static void play(bool fromButton) {
  const uint32_t now=millis();
  if (!radioReady || !enabled || (!fromButton && havePlayed && now-lastPlayMs<DecompressionButton::PRESS_GAP_MS)) {
    Serial.println("play refused: paused, radio unavailable or press gap");
    return;
  }
  lastPlayMs=now; havePlayed=true; ++presses;
  for (auto &p:peers) { p.playStatus=0; p.hits=0; p.lateUs=0; }
  playSeq=sendSquad(NB_DECOMP_PLAY,0);
  Serial.printf("play #%lu seq=%lu requested for all nine; waiting for receipts\n",
    (unsigned long)presses,(unsigned long)playSeq);
}
static void status() {
  unsigned ready=0;
  const uint32_t now=millis();
  for (const auto &p:peers) if (p.ready && now-p.readyMs<15000) ++ready;
  Serial.printf("{\"fw\":\"%s\",\"id\":\"%02X%02X%02X\",\"uptime_ms\":%lu,"
    "\"enabled\":%s,\"radio\":%s,\"ready\":%u,\"total\":%u,\"presses\":%lu,"
    "\"rx_dropped\":%lu,\"peers\":[",ATOM_DECOMPRESSION_VERSION,myId[0],myId[1],myId[2],
    (unsigned long)now,enabled?"true":"false",radioReady?"true":"false",ready,SQUAD_COUNT,
    (unsigned long)presses,(unsigned long)rxDropped);
  for (unsigned i=0;i<SQUAD_COUNT;++i) {
    const Peer &p=peers[i];
    Serial.printf("%s{\"name\":\"%s\",\"id\":\"%02X%02X%02X\",\"ready\":%s,"
      "\"heard\":%s,\"fw\":\"%s\",\"play_status\":%u,\"hits\":%u,\"late_us\":%lu}",
      i?",":"",SQUAD[i].name,SQUAD[i].id[0],SQUAD[i].id[1],SQUAD[i].id[2],
      p.ready && now-p.readyMs<15000?"true":"false",p.heard && now-p.heardMs<15000?"true":"false",
      p.fw,p.playStatus,p.hits,(unsigned long)p.lateUs);
  }
  Serial.println("]}");
}
static void serialTick() {
  static char line[24]; static unsigned used=0; static bool overflow=false;
  while (Serial.available()) {
    char c=Serial.read();
    if (c=='\r') continue;
    if (c!='\n') {
      if (used<sizeof(line)-1) line[used++]=c; else overflow=true;
      continue;
    }
    line[used]=0;
    if (!overflow) {
      if (!strcmp(line,"t")) status();
      else if (!strcmp(line,"play")) play(false);
      else if (!strcmp(line,"pause")) {
        enabled=false;
        if (radioReady) leaseSeq=sendSquad(NB_DECOMP_SESSION,0);
        for (auto &p:peers) p.ready=false;
      } else if (!strcmp(line,"resume")) { enabled=true; nextLeaseMs=millis(); }
    }
    used=0; overflow=false;
  }
}
static void face() {
  pixels.clear();
  const uint32_t now=millis();
  for (unsigned i=0;i<SQUAD_COUNT;++i) {
    const Peer &p=peers[i];
    uint32_t color=pixels.Color(0,0,12); // waiting for receiver
    if (p.heard && now-p.heardMs<15000) color=pixels.Color(16,6,0);
    if (p.ready && now-p.readyMs<15000) color=pixels.Color(0,16,0);
    if (havePlayed && now-lastPlayMs<4500) {
      if (p.playStatus==NB_DECOMP_STARTED) color=pixels.Color(14,14,14);
      else if (p.playStatus==NB_DECOMP_COMPLETE) color=pixels.Color(16,10,0);
      // Relay mode: the lantern took the press but leaves it to the host (Tone Lab).
      else if (p.playStatus==NB_DECOMP_READY) color=pixels.Color(0,12,16);
      else if (p.playStatus) color=pixels.Color(20,0,0);
    }
    if (!enabled) color=pixels.Color(3,0,4);
    if (!radioReady) color=pixels.Color(20,0,0);
    pixels.setPixelColor(FACE[i],color);
  }
  pixels.show();
}
void setup() {
  Serial.begin(115200); delay(300);
  pixels.begin(); pixels.setBrightness(40);
  pinMode(BUTTON_PIN,INPUT); button.begin(digitalRead(BUTTON_PIN)==HIGH,millis());
  uint8_t mac[6]; esp_read_mac(mac,ESP_MAC_WIFI_STA); memcpy(myId,mac+3,3);
  do { bootNonce=esp_random(); } while (!bootNonce);
  radioReady=startRadio();
  Serial.println("\n" ATOM_DECOMPRESSION_VERSION);
  Serial.println("Face press: all-nine local signatures, any hour. USB: t/play/pause/resume.");
  Serial.println("Nine pixels in roster order: blue waiting, amber heard, green ready, white started, gold complete, cyan handed to host (relay), red refused.");
  lastPlayMs=millis()-5000; nextLeaseMs=millis();
}
void loop() {
  if (radioReady) drain();
  serialTick();
  sampleButton();
  while (queuedPresses) { --queuedPresses; play(true); }
  if (radioReady && enabled && (int32_t)(millis()-nextLeaseMs)>=0 &&
      (!havePlayed || millis()-lastPlayMs>4500)) {
    leaseSeq=sendSquad(NB_DECOMP_SESSION,5);
    nextLeaseMs=millis()+5000;
  }
  if ((int32_t)(millis()-nextStatusMs)>=0) {
    status(); face(); nextStatusMs=millis()+1000;
  }
  delay(2);
}
