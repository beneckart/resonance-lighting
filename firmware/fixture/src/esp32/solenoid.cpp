#include "solenoid.h"

#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "boot_park.h"
#include "maintenance.h"
#include "nvs_store.h"

static esp_timer_handle_t gPulseTimer = nullptr;
static volatile bool gGateOn = false;
static uint32_t gLastEndMs = 0;
static uint32_t gFailsafeMs = 0;
static uint32_t gStrikes = 0, gBlocked = 0, gFailsafeCount = 0;
static uint16_t gLastPulseMs = 0;
static bool gExternalReady = false;
static bool gButtonWakePending = false;
static bool gButtonRawReleased = true;
static bool gButtonStableReleased = true;
static uint32_t gButtonChangedMs = 0;

// Rev-2 solarnoid has its own 10k D7 pulldown plus hardware-bounded SW1 and
// RX480E one-shot sources on the same net. An armed fixture must therefore
// release D7 between MCU pulses; OUTPUT LOW would clamp both manual sources.
// A disarmed fixture deliberately keeps the old active-LOW clamp.
static void solenoidIdle() {
  if (gCfg.solEn) {
    pinMode(RES_SOLENOID_PIN, INPUT);
    digitalWrite(RES_SOLENOID_PIN, LOW); // preload the output latch for safety
  } else {
    pinMode(RES_SOLENOID_PIN, OUTPUT);
    digitalWrite(RES_SOLENOID_PIN, LOW);
  }
}

static void solenoidPulseEnd(void *) { // esp_timer task context, not an ISR
  solenoidIdle();
  gGateOn = false;
  gLastEndMs = millis();
}

void solenoidInit() {
  solenoidIdle();
  gLastEndMs = millis() - RES_SOLENOID_REST_MS;
  // EXT0 owns the RTC pad through deep sleep. Return it to normal GPIO before
  // sampling the active-LOW USER button and enabling its internal pull-up.
  rtc_gpio_deinit((gpio_num_t)RES_SOLENOID_BUTTON_PIN);
  pinMode(RES_SOLENOID_BUTTON_PIN, INPUT_PULLUP);
  delay(1);
  bool released = digitalRead(RES_SOLENOID_BUTTON_PIN) == HIGH;
  gButtonWakePending = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
  // EXT0 itself proves a deliberate LOW. Start latched as pressed even if the
  // user released before setup sampled GPIO0, so contact bounce cannot create a
  // second active-mode falling edge during the same press.
  gButtonRawReleased = gButtonWakePending ? false : released;
  gButtonStableReleased = gButtonWakePending ? false : released;
  gButtonChangedMs = millis();

  const esp_timer_create_args_t timerArgs = {
      .callback = &solenoidPulseEnd,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "fx_solenoid",
      .skip_unhandled_events = true,
  };
  esp_err_t err = esp_timer_create(&timerArgs, &gPulseTimer);
  if (err != ESP_OK) {
    gPulseTimer = nullptr;
    Serial.printf("solenoid timer init failed: %s\n", esp_err_to_name(err));
  } else if (gCfg.solEn) {
    Serial.printf("solenoid armed: D7/GPIO%d, VDC tap, pulse %u..%u ms\n",
                  RES_SOLENOID_PIN, RES_SOLENOID_MIN_MS, RES_SOLENOID_MAX_MS);
  }
}

void solenoidStop(const char *why) {
  if (gPulseTimer) esp_timer_stop(gPulseTimer);
  solenoidIdle();
  if (gGateOn) {
    gGateOn = false;
    gLastEndMs = millis();
    Serial.printf("solenoid forced LOW (%s)\n", why);
  }
}

static bool solenoidStrikeImpl(uint16_t pulseMs, const char *why,
                              bool takeExternalHigh) {
  if (!gCfg.solEn) {
    gBlocked++;
    return false;
  }
  pulseMs = constrain(pulseMs, (uint16_t)RES_SOLENOID_MIN_MS,
                      (uint16_t)RES_SOLENOID_MAX_MS);
  uint32_t now = millis();
  if (!gPulseTimer || gGateOn || now - gLastEndMs < RES_SOLENOID_REST_MS) {
    gBlocked++;
    return false;
  }
  // Normal MCU requests must not fight a physical one-shot already driving D7.
  // The external-edge path is the deliberate exception: it takes over that
  // same HIGH and extends it to the firmware-bounded production pulse width.
  if (!takeExternalHigh && digitalRead(RES_SOLENOID_PIN) == HIGH) {
    gBlocked++;
    return false;
  }

  gFailsafeMs = now + pulseMs + 50;
  gGateOn = true;
  // Preload HIGH before enabling output so the MCU pulse has no LOW glitch.
  digitalWrite(RES_SOLENOID_PIN, HIGH);
  pinMode(RES_SOLENOID_PIN, OUTPUT);
  esp_err_t err = esp_timer_start_once(gPulseTimer, (uint64_t)pulseMs * 1000ULL);
  if (err != ESP_OK) {
    solenoidIdle();
    gGateOn = false;
    gLastEndMs = millis();
    gFailsafeCount++;
    Serial.printf("solenoid timer start failed: %s\n", esp_err_to_name(err));
    return false;
  }

  gStrikes++;
  gLastPulseMs = pulseMs;
  Serial.printf("solenoid strike #%lu: %ums (%s)\n", (unsigned long)gStrikes,
                (unsigned)pulseMs, why);
  return true;
}

bool solenoidStrike(uint16_t pulseMs, const char *why) {
  return solenoidStrikeImpl(pulseMs, why, false);
}

void solenoidExternalTick() {
  if (!gCfg.solEn || gGateOn) return;

  bool high = digitalRead(RES_SOLENOID_PIN) == HIGH;
  if (!high) {
    // Require a released LOW after every trigger and after boot. This prevents
    // a stuck-high input from firing at boot or repeatedly retriggering.
    gExternalReady = true;
    return;
  }
  if (!gExternalReady) return;

  // Consume this edge even if maintenance/rest-time policy rejects it. A new
  // strike then requires a physical release followed by another rising edge.
  gExternalReady = false;
  if (maintMode() != MODE_COMMS) {
    Serial.println("solenoid SW1 input ignored during maintenance");
    return;
  }
  solenoidStrikeImpl(RES_SOLENOID_DEFAULT_MS, "capboard SW1", true);
}

void solenoidButtonHandleWake() {
  if (!gButtonWakePending) return;
  gButtonWakePending = false;
  solenoidStrike(RES_SOLENOID_DEFAULT_MS, "user button wake");
}

void solenoidButtonTick() {
  bool released = digitalRead(RES_SOLENOID_BUTTON_PIN) == HIGH;
  uint32_t now = millis();
  if (released != gButtonRawReleased) {
    gButtonRawReleased = released;
    gButtonChangedMs = now;
  }
  if (released == gButtonStableReleased ||
      now - gButtonChangedMs < RES_SOLENOID_BUTTON_DEBOUNCE_MS)
    return;

  gButtonStableReleased = released;
  if (!released) {
    if (maintMode() == MODE_COMMS)
      solenoidStrike(RES_SOLENOID_DEFAULT_MS, "user button");
    else
      Serial.println("solenoid USER button ignored during maintenance");
  }
}

void solenoidButtonPrepareSleep() {
  bool released = digitalRead(RES_SOLENOID_BUTTON_PIN) == HIGH;
  // Reset any wake configuration inherited within this boot, then re-enable it
  // only from a physically released level. A held LOW therefore gets the timer
  // wake only and cannot form an immediate EXT0 reboot/strike loop.
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
  if (!released) {
    Serial.println("solenoid USER button held: button wake disabled for this sleep");
    return;
  }

  gpio_num_t pin = (gpio_num_t)RES_SOLENOID_BUTTON_PIN;
  esp_err_t err = rtc_gpio_init(pin);
  if (err == ESP_OK) err = rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
  if (err == ESP_OK) err = rtc_gpio_pullup_en(pin);
  if (err == ESP_OK) err = rtc_gpio_pulldown_dis(pin);
  if (err == ESP_OK) err = esp_sleep_enable_ext0_wakeup(pin, 0);
  if (err != ESP_OK)
    Serial.printf("solenoid USER button wake setup failed: %s\n", esp_err_to_name(err));
}

void solenoidFailsafeTick() {
  if (gGateOn && (int32_t)(millis() - gFailsafeMs) >= 0) {
    solenoidIdle();
    gGateOn = false;
    gLastEndMs = millis();
    gFailsafeCount++;
    Serial.println("solenoid FAILSAFE: gate forced LOW");
  }
}

bool solenoidGateOn() { return gGateOn; }
uint32_t solenoidStrikes() { return gStrikes; }
uint32_t solenoidBlocked() { return gBlocked; }
uint32_t solenoidFailsafes() { return gFailsafeCount; }
uint16_t solenoidLastPulseMs() { return gLastPulseMs; }
