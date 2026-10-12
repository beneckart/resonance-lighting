#include "solenoid.h"

#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "boot_guard_io.h"
#include "boot_park.h"
#include "maintenance.h"
#include "nvs_store.h"
#include "decompression.h"

static esp_timer_handle_t gPulseTimer = nullptr;
static esp_timer_handle_t gSignatureTimer = nullptr;
static SignatureSequence gSignature;
static portMUX_TYPE gSolenoidMux = portMUX_INITIALIZER_UNLOCKED;
struct SolenoidLock {
  SolenoidLock() { portENTER_CRITICAL(&gSolenoidMux); }
  ~SolenoidLock() { portEXIT_CRITICAL(&gSolenoidMux); }
};
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
#if RES_DEEP_RECOVERY_TARGET != 0UL
  pinMode(RES_SOLENOID_PIN, OUTPUT);
  digitalWrite(RES_SOLENOID_PIN, LOW);
  return;
#endif
  if (gCfg.solEn) {
    gpio_set_direction((gpio_num_t)RES_SOLENOID_PIN, GPIO_MODE_INPUT);
    gpio_set_level((gpio_num_t)RES_SOLENOID_PIN, 0);
  } else {
    gpio_set_level((gpio_num_t)RES_SOLENOID_PIN, 0);
    gpio_set_direction((gpio_num_t)RES_SOLENOID_PIN, GPIO_MODE_OUTPUT);
  }
}

static void solenoidPulseEnd(void *) { // esp_timer task context, not an ISR
  SolenoidLock lock;
  solenoidIdle();
  gGateOn = false;
  gLastEndMs = millis();
}

// Called with gSolenoidMux held. No NVS, I2C, allocation or logging here.
static bool pulsePrepared(uint16_t pulseMs, bool externalHigh = false,
                          uint64_t *onsetUs = nullptr) {
  uint32_t now = millis();
  if (!gCfg.solEn || !gPulseTimer || gGateOn ||
      now - gLastEndMs < RES_SOLENOID_REST_MS ||
      (!externalHigh && gpio_get_level((gpio_num_t)RES_SOLENOID_PIN)))
    return false;
  gFailsafeMs = now + pulseMs + 50;
  gpio_set_level((gpio_num_t)RES_SOLENOID_PIN, 1);
  gpio_set_direction((gpio_num_t)RES_SOLENOID_PIN, GPIO_MODE_INPUT_OUTPUT);
  if (onsetUs) *onsetUs = esp_timer_get_time();
  gGateOn = true;
  if (esp_timer_start_once(gPulseTimer, (uint64_t)pulseMs * 1000ULL) != ESP_OK) {
    solenoidIdle();
    gGateOn = false;
    gLastEndMs = now;
    gFailsafeCount++;
    return false;
  }
  gStrikes++;
  gLastPulseMs = pulseMs;
  return true;
}

static void signatureTimerTick(void *) {
  SolenoidLock lock;
  uint64_t now = esp_timer_get_time();
  SignatureSequence::Action action = gSignature.poll(now, gGateOn);
  if (action == SignatureSequence::FIRE) {
    uint64_t onsetUs = 0;
    if (pulsePrepared(SignatureSequence::PULSE_US / 1000, false, &onsetUs)) {
      gSignature.fired(onsetUs);
    } else {
      gSignature.cancel(now, SignatureSequence::MECHANISM);
      gBlocked++;
      action = SignatureSequence::ABORT;
    }
  }
  if (action == SignatureSequence::ABORT) {
    esp_timer_stop(gPulseTimer);
    solenoidIdle();
    gGateOn = false;
    gLastEndMs = millis();
  }
  if (!gSignature.active()) esp_timer_stop(gSignatureTimer);
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
  if (decompressionBuild()) {
    esp_timer_create_args_t args = timerArgs;
    args.callback = &signatureTimerTick;
    args.name = "fx_signature";
    if (esp_timer_create(&args, &gSignatureTimer) != ESP_OK)
      gSignatureTimer = nullptr;
  }
}

void solenoidStop(const char *why) {
  SolenoidLock lock;
  gSignature.cancel(esp_timer_get_time());
  if (gSignatureTimer) esp_timer_stop(gSignatureTimer);
  if (gPulseTimer) esp_timer_stop(gPulseTimer);
  solenoidIdle();
  if (gGateOn) {
    gGateOn = false;
    gLastEndMs = millis();
  }
  (void)why;
}

static bool solenoidStrikeImpl(uint16_t pulseMs, const char *why,
                              bool takeExternalHigh) {
#if RES_DEEP_RECOVERY_TARGET != 0UL
  (void)pulseMs;
  (void)why;
  (void)takeExternalHigh;
  { SolenoidLock lock; gBlocked++; }
  return false;
#endif
  pulseMs = constrain(pulseMs, (uint16_t)RES_SOLENOID_MIN_MS,
                      (uint16_t)RES_SOLENOID_MAX_MS);
  // Normal MCU requests must not fight a physical one-shot already driving D7.
  // The external-edge path is the deliberate exception: it takes over that
  // same HIGH and extends it to the firmware-bounded production pulse width.
  {
    SolenoidLock lock;
    if (!gCfg.solEn || !gPulseTimer || gGateOn || gSignature.active() ||
        millis() - gLastEndMs < RES_SOLENOID_REST_MS ||
        (!takeExternalHigh && gpio_get_level((gpio_num_t)RES_SOLENOID_PIN))) {
      gBlocked++;
      return false;
    }
  }

  // ADR 0051: persist the load-armed marker BEFORE the gate drives; an
  // unpersistable marker refuses the strike.
  if (!bootGuardLoadArm()) {
    SolenoidLock lock;
    gBlocked++;
    return false;
  }
  {
    SolenoidLock lock;
    if (!pulsePrepared(pulseMs, takeExternalHigh)) {
      gBlocked++;
      return false;
    }
  }
  Serial.printf("solenoid strike #%lu: %ums (%s)\n", (unsigned long)solenoidStrikes(),
                (unsigned)pulseMs, why);
  return true;
}

bool solenoidStrike(uint16_t pulseMs, const char *why) {
  return solenoidStrikeImpl(pulseMs, why, false);
}

bool solenoidSignatureStart() {
  if (!decompressionBuild() || !gSignatureTimer || !gPulseTimer || !gCfg.solEn ||
      !solenoidQuietFor(RES_SOLENOID_REST_MS)) return false;
  // Flash write finishes before the phrase clock starts, once per phrase.
  if (!bootGuardLoadArm()) return false;
  SolenoidLock lock;
  if (!gSignature.start(esp_timer_get_time())) return false;
  if (esp_timer_start_periodic(gSignatureTimer, 1000) != ESP_OK) {
    gSignature.cancel(esp_timer_get_time(), SignatureSequence::MECHANISM);
    return false;
  }
  return true;
}

SignatureSequence solenoidSignatureSnapshot() {
  SolenoidLock lock;
  return gSignature;
}

bool solenoidBusy() {
  SolenoidLock lock;
  return gGateOn || gSignature.active();
}

bool solenoidQuietFor(uint32_t ms) {
  SolenoidLock lock;
  if (gGateOn || gSignature.active()) return false;
  return millis() - gLastEndMs >= ms; // gLastEndMs==0: quiet since boot
}

void solenoidExternalTick() {
  if (!gCfg.solEn || solenoidBusy()) return;

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
  if (decompressionBuild()) {
    decompressionButtonWake();
    return;
  }
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
    if (decompressionBuild()) {
      decompressionPlaySignature();
      return;
    }
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
  SolenoidLock lock;
  if (gGateOn && (int32_t)(millis() - gFailsafeMs) >= 0) {
    gSignature.cancel(esp_timer_get_time(), SignatureSequence::MECHANISM);
    if (gSignatureTimer) esp_timer_stop(gSignatureTimer);
    if (gPulseTimer) esp_timer_stop(gPulseTimer);
    solenoidIdle();
    gGateOn = false;
    gLastEndMs = millis();
    gFailsafeCount++;
  }
}

bool solenoidGateOn() { SolenoidLock lock; return gGateOn; }
uint32_t solenoidStrikes() { SolenoidLock lock; return gStrikes; }
uint32_t solenoidBlocked() { SolenoidLock lock; return gBlocked; }
uint32_t solenoidFailsafes() { SolenoidLock lock; return gFailsafeCount; }
uint16_t solenoidLastPulseMs() { SolenoidLock lock; return gLastPulseMs; }
