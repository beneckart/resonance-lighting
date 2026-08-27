// Resonance Bridge OS — LilyGO T-Deck Plus handheld (ADR 0037).
// M0: board bring-up, channel guard, mesh frame counter, raw status page.
// Wiring only — logic lives in src/ (core = native-testable, hal/net/ui/store).

#include "src/hal/hal_board.h"
#include "src/hal/hal_display.h"
#include "src/hal/hal_input.h"
#include "src/hal/pins_tdeck.h"
#include "src/net/census_svc.h"
#include "src/net/contagion_fanout.h"
#include "src/net/claude_client.h"
#include "src/net/espnow_link.h"
#include "src/net/mesh_tx.h"
#include "src/net/net_mgr.h"
#include "src/net/stream_svc.h"
#include "src/net/time_svc.h"
#include "src/core/version.h"
#include "src/store/serial_cli.h"
#include "src/store/store.h"
#include "src/ui/status_page.h"
#include "src/ui/ui_theme.h"
#include "src/ui/ui_task.h"

static bool gSunTest = false;

void setup() {
  halBoardPowerOn();  // peripheral rail FIRST; nothing answers without it

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println("=== Resonance tdeck-bridge " TDECK_FW_VERSION " ===");

  storeBegin();

  if (!halDisplayInit()) Serial.println("display init FAILED (headless)");
  halInputInit();

  ProbeReport probe;
  halProbeRun(&probe);  // GPS autodetect can take ~5 s worst case
  Serial.printf("probe: psram=%d(%lu) kbd=%d touch=%d es7210=0x%02X gps=%d@%lu%s\n",
                probe.psram, (unsigned long)probe.psramBytes, probe.keyboard,
                probe.touch, probe.es7210Addr, probe.gpsNmea,
                (unsigned long)probe.gpsBaud, probe.gpsSwapped ? " (swapped)" : "");

  censusSvcBegin(millis());
  netMgrBegin();
  meshTxBegin();
  timeSvcBegin();
  claudeBegin();

  // LVGL shell (M2); the raw status page remains the plan-B fallback.
  if (uiStart()) {
    Serial.println("LVGL shell up");
  } else {
    Serial.println("LVGL init FAILED -> raw status page fallback");
    statusPageTick();
  }
  halDisplaySetBacklight(uiDisplayBacklight());
  Serial.println("Bridge OS up; type `help` for the CLI");
}

void loop() {
  serialCliTick();
  netMgrTick();
  espnowEnsureUp();
  censusSvcTick(millis());
  contagionFanoutTick(millis());
  streamSvcTick(millis());
  halGpsTick();
  timeSvcTick();
  meshTxTick();

  if (!uiActive()) {
    // Plan-B raw status page: keyboard polled here (LVGL owns it otherwise).
    char key = halKeyboardRead();
    if (key) {
      statusPageNoteKey(key);
      Serial.printf("key: '%c' (0x%02X)\n", key, (uint8_t)key);
      if (key == 's') {
        gSunTest = !gSunTest;
        statusPageSunTest(gSunTest);
        if (!gSunTest) halDisplaySetBacklight(uiDisplayBacklight());
      }
    }
    statusPageTick();
  }

  static uint32_t lastMemMs = 0;
  if (millis() - lastMemMs >= 10000) {
    lastMemMs = millis();
    Serial.printf("nb-mem heap_free=%u heap_min=%u psram_free=%u psram_min=%u bat_mv=%u\n",
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)ESP.getFreePsram(), (unsigned)ESP.getMinFreePsram(),
                  halBatteryMv());
  }
  delay(5);
}
