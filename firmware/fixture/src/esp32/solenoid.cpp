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
static bool gButtonWakePending = false;
static bool gButtonRawReleased = true;
static bool gButtonStableReleased = true;
static uint32_t gButtonChangedMs = 0;

static void solenoidPulseEnd(void *) { // esp_timer task context, not an ISR
  digitalWrite(RES_SOLENOID_PIN, LOW);
  gGateOn = false;
  gLastEndMs = millis();
}

void solenoidInit() {
  pinMode(RES_SOLENOID_PIN, OUTPUT);
  digitalWrite(RES_SOLENOID_PIN, LOW);
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
  digitalWrite(RES_SOLENOID_PIN, LOW);
  if (gGateOn) {
    gGateOn = false;
    gLastEndMs = millis();
    Serial.printf("solenoid forced LOW (%s)\n", why);
  }
}

bool solenoidStrike(uint16_t pulseMs, const char *why) {
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

  gFailsafeMs = now + pulseMs + 50;
  gGateOn = true;
  digitalWrite(RES_SOLENOID_PIN, HIGH);
  esp_err_t err = esp_timer_start_once(gPulseTimer, (uint64_t)pulseMs * 1000ULL);
  if (err != ESP_OK) {
    digitalWrite(RES_SOLENOID_PIN, LOW);
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
    digitalWrite(RES_SOLENOID_PIN, LOW);
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
