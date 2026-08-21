#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <Arduino.h>
#include <time.h>

#include "../core/battery_model.h"
#include "../hal/hal_board.h"
#include "../hal/hal_display.h"
#include "../hal/hal_input.h"
#include "../net/census_svc.h"
#include "../net/espnow_link.h"
#include "../net/net_mgr.h"
#include "../store/store.h"
#include "status_page.h"

static char gLastKey = 0;
static bool gSunTest = false;

void statusPageNoteKey(char c) { gLastKey = c; }
void statusPageSunTest(bool on) { gSunTest = on; }

static void drawSunTest(lgfx::LGFX_Sprite *c) {
  // ADR 0037 M0 readability check: max-contrast bars, chat-density text, RGB
  // patches. Judged outdoors through sunglasses; verdict recorded in README.
  c->fillSprite(TFT_BLACK);
  for (int i = 0; i < 8; ++i)
    c->fillRect(i * 40, 0, 20, 60, TFT_WHITE);
  c->setTextColor(TFT_WHITE, TFT_BLACK);
  c->setTextSize(2);
  c->setCursor(4, 70);
  c->println("SUN TEST 0123456789");
  // Verdict 2026-08-19 (Ben): direct sun = barely legible even at max
  // contrast; shade = usable but size-1 text needs squinting. Field UI
  // minimum is therefore size 2 — both densities kept here for comparison.
  c->setTextSize(1);
  c->setCursor(4, 92);
  c->println("size1: the quick brown fox jumps over 13 lazy dogs");
  c->setTextSize(2);
  c->setCursor(4, 104);
  c->println("size2: quick brown fox");
  c->fillRect(0, 124, 106, 40, TFT_RED);
  c->fillRect(107, 124, 106, 40, TFT_GREEN);
  c->fillRect(214, 124, 106, 40, TFT_BLUE);
  c->setTextSize(2);
  c->setCursor(4, 170);
  c->setTextColor(TFT_YELLOW, TFT_BLACK);
  c->println("press s to exit");
  c->setTextSize(3);
  c->setCursor(4, 200);
  c->setTextColor(TFT_WHITE, TFT_BLACK);
  c->println("BRIGHT 255");
}

void statusPageTick() {
  static uint32_t lastDrawMs = 0;
  static uint32_t lastFrames = 0, lastFpsMs = 0;
  static float fps = 0;
  uint32_t now = millis();
  if (now - lastDrawMs < 200) return;  // ~5 Hz
  lastDrawMs = now;

  lgfx::LGFX_Sprite *c = halCanvas();
  if (!c) return;  // headless fallback: serial still carries everything

  if (gSunTest) {
    halDisplaySetBacklight(255);
    drawSunTest(c);
    halCanvasPush();
    return;
  }

  MeshStats ms = espnowStats();
  if (now - lastFpsMs >= 2000) {
    fps = (ms.frames - lastFrames) * 1000.0f / (now - lastFpsMs);
    lastFrames = ms.frames;
    lastFpsMs = now;
  }

  c->fillSprite(TFT_BLACK);
  c->setTextSize(2);
  c->setTextColor(TFT_CYAN, TFT_BLACK);
  c->setCursor(2, 2);
  c->printf("Resonance Bridge OS M0\n");
  c->setTextSize(1);
  c->setTextColor(TFT_WHITE, TFT_BLACK);

  // --- Wi-Fi / guard ---
  c->setCursor(2, 24);
  bool guard = netState() == NetState::GUARD_BLOCKED;
  c->setTextColor(guard ? TFT_RED : TFT_WHITE, TFT_BLACK);
  c->printf("wifi: %s", netStateName());
  if (netState() == NetState::ONLINE)
    c->printf("  %s  %ddBm  %s", settings().ssid, netRssi(), netIp());
  if (guard)
    c->printf("  AP ch=%u != mesh ch=%u", netApChannel(), settings().channel);
  c->setTextColor(TFT_WHITE, TFT_BLACK);

  c->setCursor(2, 36);
  if (netSntpSynced()) {
    time_t t = time(nullptr);
    struct tm tmv;
    gmtime_r(&t, &tmv);
    c->printf("sntp: %04d-%02d-%02d %02d:%02d:%02dZ", tmv.tm_year + 1900,
              tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  } else {
    c->print("sntp: unsynced");
  }

  // --- Mesh ---
  c->setCursor(2, 52);
  c->setTextColor(espnowUp() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  c->printf("mesh: %s ch=%u", espnowUp() ? "UP" : "DOWN", settings().channel);
  c->setTextColor(TFT_WHITE, TFT_BLACK);
  c->printf("  frames=%lu (%.1f/s) drop=%lu", (unsigned long)ms.frames, fps,
            (unsigned long)ms.dropped);
  c->setCursor(2, 64);
  if (ms.frames > 0) {
    c->printf("last: %02X%02X%02X type=%u rssi=%d age=%lus", ms.lastSrcId[0],
              ms.lastSrcId[1], ms.lastSrcId[2], ms.lastType, ms.lastRssi,
              (unsigned long)((now - ms.lastFrameMs) / 1000));
    if (now - ms.lastFrameMs > 15000) {
      c->setTextColor(TFT_ORANGE, TFT_BLACK);
      c->print("  MESH SILENT");
      c->setTextColor(TFT_WHITE, TFT_BLACK);
    }
  } else {
    c->print("last: (no frames yet)");
  }

  // --- Census ---
  c->setCursor(2, 76);
  uint32_t nowC = millis();
  c->printf("peers: %d live / %d seen  ringdrop=%lu", census().liveCount(nowC),
            census().seenCount(), (unsigned long)censusRingDrops());
  CensusView rows[3];
  size_t nRows = census().snapshot(rows, 3, nowC);
  for (size_t i = 0; i < nRows; ++i) {
    c->setCursor(2, 88 + 10 * (int)i);
    c->printf(" %02X%02X%02X %4ddBm pdr=%u%% soc=%u%% cls=%u", rows[i].id[0],
              rows[i].id[1], rows[i].id[2], rows[i].rssiEwma,
              rows[i].pdrX1000 / 10, rows[i].soc, rows[i].fixtureClass);
  }

  // --- Battery ---
  uint16_t mv = halBatteryMv();
  c->setCursor(2, 120);
  c->printf("batt: %umV  ~%u%%", mv, lipoPercentFromMv(mv));

  // --- Inputs ---
  TrackballCounts tb = halTrackballRead();
  c->setCursor(2, 134);
  c->printf("key: '%c'  tb a=%lu b=%lu c=%lu d=%lu click=%d",
            gLastKey ? gLastKey : ' ', (unsigned long)tb.a, (unsigned long)tb.b,
            (unsigned long)tb.c, (unsigned long)tb.d, tb.click ? 1 : 0);
  int16_t tx, ty;
  c->setCursor(2, 146);
  if (halTouchRead(&tx, &ty)) c->printf("touch: %d,%d", tx, ty);
  else c->print("touch: -");

  // --- Probes ---
  const ProbeReport &p = halProbeLast();
  c->setCursor(2, 162);
  c->printf("psram=%s(%luK) kbd=%s touch=%s mic=%s", p.psram ? "ok" : "NO",
            (unsigned long)(p.psramBytes / 1024), p.keyboard ? "ok" : "NO",
            p.touch ? "ok" : "NO",
            p.es7210Addr ? "ok" : "NO");
  c->setCursor(2, 174);
  c->print(halGpsSummary());

  // --- Memory ---
  c->setCursor(2, 190);
  c->printf("heap %u/%u  psram %u/%u  (free/min)", (unsigned)ESP.getFreeHeap(),
            (unsigned)ESP.getMinFreeHeap(), (unsigned)ESP.getFreePsram(),
            (unsigned)ESP.getMinFreePsram());

  c->setTextColor(TFT_DARKGREY, TFT_BLACK);
  c->setCursor(2, 228);
  c->print("s=sun test   serial 115200: help");
  halCanvasPush();
}
