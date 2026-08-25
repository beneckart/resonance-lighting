// Resonance production fixture firmware -- one image for all four fixture
// classes (downlight / perimeter / uplight / chandelier), extracted from the
// proven bench sketches (net_bench lineage; see firmware/fixture/README.md).
//
// This file is deliberately thin: setup()/loop() ORDERING only. All logic
// lives in src/core/ (platform-independent, natively unit-tested) and
// src/esp32/ (glue/drivers).
//
// Build/flash: ./build.sh --port /dev/ttyACMx   (see build.sh for flags)

#include "src/core/class_probe.h"
#include "src/core/version.h"
#include "src/esp32/behavior_glue.h"
#include "src/esp32/board_power.h"
#include "src/esp32/boot_guard_io.h"
#include "src/esp32/boot_park.h"
#include "src/esp32/espnow_link.h"
#include "src/esp32/identity.h"
#include "src/esp32/led_driver.h"
#include "src/esp32/maintenance.h"
#include "src/esp32/net_peer.h"
#include "src/esp32/nvs_store.h"
#include "src/esp32/ota_verify.h"
#include "src/esp32/power_glue.h"
#include "src/esp32/sensors/sensor_bus.h"
#include "src/esp32/sensors/sensors.h"
#include "src/esp32/serial_cli.h"
#include "src/esp32/solenoid.h"
#include "src/esp32/status_led.h"
#include "src/esp32/telemetry.h"
#include "src/esp32/time_anchor.h"
#include "src/esp32/watchdog.h"

#include "esp_system.h"
#include "esp_task_wdt.h"

void setup() {
  bool haveRtcAnchor = false;
  // 1. Fail-safe parks before ANY other work: solenoid gate (VDC is always
  //    live), pixel data, EN_3V3 rail (RTC-held low until deliberately raised).
  bootParkAll();

  // 2. Boot-guard stage decision, still before Serial: a consumed retry must
  //    be durable in NVS before anything can energize a rail.
  bootGuardPreInit();

  // 3. Serial + banner. Skip the USB-CDC settle on deep-sleep wakes: it would
  //    dominate the wake's active time and no host is attached in the field.
  Serial.begin(115200);
  if (esp_reset_reason() != ESP_RST_DEEPSLEEP) delay(1500);
  Serial.println();
  Serial.println("=== Resonance fixture " RES_FIXTURE_VERSION " ===");
  bootGuardReport();

  // 4-5. Solenoid pulse machinery + MAC-derived identity.
  nvsLoadConfig();
  solenoidInit();
  identityInit();

  // 6. Power stack: Board.init (rail re-parked after every attempt), charger
  //    policy,
  //    charging deferred until the gauge shows a plausible cell.
  boardPowerInit();

  // 7. Rails: force a verified sensor-domain cold start before the class probe.
  //    PowerFeather deliberately preserves VSQT across warm/OTA resets, which
  //    otherwise lets a stale TMF firmware/ranging state survive the MCU reset.
  //    The LED rail stays parked until a deliberate, ramped turn-on.
  if (deepRecoveryBuild()) {
    // Test-class recovery images never power sensors or LEDs. The charger and
    // radio are the only active subsystems needed for the supervised canary.
    railEnableVSQT(false);
  } else if (!bootGuardParked()) {
    if (!railCycleVSQT())
      Serial.println("VSQT cold-start verification FAILED; sensors may degrade");
  } else if (!railEnableVSQT(false)) {
    Serial.println("VSQT park FAILED");
  }

  // 8. Class probe -> LED profile (one image, hardware decides; ADR 0009).
  if (!deepRecoveryBuild() && !bootGuardParked()) {
    ProbeBits bits = sensorBusProbe();
    haveRtcAnchor = bits.ds3231;
    gTelemetrySensorBits = probeBitsMask(bits);
    ClassDecision cd = classDecide(bits, gCfg.classOvr, gCfg.classLast);
    if (cd.persistLast) nvsPersistClassLast(cd.persistLast);
    gTelemetryFixtureClass = cd.cls;
    gTelemetryClassMismatch = cd.mismatch;
    ledProfileForClass(cd.cls);
    Serial.printf("class: %s%s%s\n", fixtureClassName(cd.cls),
                  gCfg.classOvr ? " (override)" : "",
                  cd.mismatch ? " MISMATCH" : "");
  }
  // 9. Class sensors (cooperative machines; perimeter's VL53L5CX firmware
  //    upload takes several seconds, once).
  if (!deepRecoveryBuild() && !bootGuardParked()) sensorsInit(gTelemetryFixtureClass);

  // 11. Watchdog before network bring-up.
  watchdogInit();
  statusLedInit();

  // 12. Radio: STA-unassociated on the configured channel, ESP-NOW up.
  enterComms();

  // 13. Boot announce: smoke line + jittered heartbeats.
  solenoidButtonHandleWake();
  readBatteryNow();
  Serial.printf("boot: id=%s class=%s profile=%s reset=%s bv=%.3fV soc=%d ch=%u\n",
                gShortId.c_str(),
                fixtureClassName(gCfg.classOvr ? gCfg.classOvr : gCfg.classLast),
                gCfg.profile == PROFILE_DEV ? "commission" : "field",
                resetReasonName(esp_reset_reason()), batteryVolts(),
                batterySocPct(), gCfg.channel);
  netPeerInit();
  for (int i = 0; i < 3; i++) {
    netPeerSendHeartbeat(i == 0); // first announce is a full heartbeat
    delay(30 + (esp_random() % 40));
  }

  // 14. OTA pending-verify resolves in-loop via otaVerifyTick(); deep sleep is
  //     blocked until it does.
  if (otaVerifyPending())
    Serial.println("ota-verify: image PENDING_VERIFY; self-test at t+20s");

  // 15. Power policy seeded from the boot-guard stage; behavior layer
  //     (lifecycle + choreo runtime) seeded from the class decision.
  powerGlueInit();
  behaviorInit(gTelemetryFixtureClass, ledPixelCount(),
               ((uint32_t)gMyId[0] << 16) | ((uint32_t)gMyId[1] << 8) | gMyId[2]);
  timeAnchorInit(haveRtcAnchor);
}

void loop() {
  esp_task_wdt_reset();
  solenoidFailsafeTick();
  solenoidExternalTick();
  solenoidButtonTick();
  handleSerial();
  boardPowerTick();
  statusLedTick();
  otaVerifyTick();
  sensorsTick();
  commsRecoveryTick();

  if (maintMode() == MODE_MAINT) {
    maintenanceTick();
    return;
  }

  // COMMS mode.
  netPeerTick();
  timeAnchorTick();
  powerGlueTick();
  behaviorTick();
  renderTick();
}

// Render arbitration: color-identify (rig ordering) > bench smoke toggle >
// the program runtime's night show. The power veto caps everything; a
// hard-parked boot never lights.
void renderTick() {
  if (deepRecoveryBuild()) {
    if (ledRailIsOn()) ledRailOff();
    return;
  }
  if (bootGuardParked()) return;
  if (gBenchRailForcedOff) {
    if (ledRailIsOn()) ledRailOff();
    return;
  }
  uint8_t cap = powerBudget().brightness_cap; // the power veto: no path around it
  uint32_t now = millis();
  bool ident = netPeerIdentifyActive() && netPeerIdentifyColor() > 0;
  FrameBuffer f;
  bool have = false;
  if (ident) {
    ledIdentifyFrame(f, netPeerIdentifyColor(), netPeerIdentifyBlink(),
                     netPeerIdentifyValue(), now);
    have = true;
  } else if (gSmokeRender) {
    ledSmokeFrame(f, now);
    have = true;
  } else {
    have = behaviorFrame(f); // NIGHT_SHOW program output
  }
  if (cap == 0 || !have) {
    if (ledRailIsOn()) ledRailOff();
    return;
  }
  if (!ledRailIsOn()) {
    if (!ledRailOnWithRamp(f, cap)) { // parks itself on a sagging cell
      gSmokeRender = false;
      return;
    }
  }
  static uint32_t lastFrame = 0;
  if (now - lastFrame >= 100) {
    lastFrame = now;
    ledRender(f, cap);
  }
}
